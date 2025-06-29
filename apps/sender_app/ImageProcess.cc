#include "ImageProcess.hh"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <random>
#include <ranges>
#include <vector>

namespace Processing {

const std::map<EPDColor, Pixel> PALETTE = {
    {EPDColor::BLACK, {0, 0, 0}},       //
    {EPDColor::WHITE, {255, 255, 255}}, //
    {EPDColor::YELLOW, {255, 255, 0}},  //
    {EPDColor::RED, {255, 0, 0}},       //
    {EPDColor::BLUE, {0, 0, 255}},      //
    {EPDColor::GREEN, {0, 255, 0}},     //
};

[[nodiscard]] auto closest_epd_color(const std::uint8_t *rgb) -> EPDColor {
  double min_dist = std::numeric_limits<double>::max();
  EPDColor closest = EPDColor::BLACK;
  for (const auto &[color, val] : PALETTE) {
    double dist = 0.0;
    for (int i = 0; i < 3; ++i) {
      dist += std::pow(rgb[i] - val[i], 2);
    }
    if (dist < min_dist) {
      min_dist = dist;
      closest = color;
    }
  }
  return closest;
}

auto nearest_color(const Pixel &c) -> Pixel {
  int best_d = 1 << 30;
  Pixel best = {};
  for (const auto &p : PALETTE | std::views::values) {
    int dr = int(c[0]) - p[0], dg = int(c[1]) - p[1], db = int(c[2]) - p[2];
    int d = dr * dr + dg * dg + db * db;
    if (d < best_d) {
      best_d = d, best = p;
    }
  }
  return best;
}

auto rotate90(const std::vector<uint8_t> &src, std::size_t width,
              std::size_t height) -> std::vector<uint8_t> {
  constexpr std::size_t BYTES_PER_PX = 3; // r g b
  if (src.size() != width * height * BYTES_PER_PX) {
    throw std::invalid_argument("buffer size mismatch");
  }

  const std::size_t dstW = height; // 480 ← 800, etc.
  const std::size_t dstH = width;
  std::vector<uint8_t> dst(dstW * dstH * BYTES_PER_PX);

  for (std::size_t y = 0; y < height; ++y) {
    for (std::size_t x = 0; x < width; ++x) {
      const std::size_t src_idx = (y * width + x) * BYTES_PER_PX;

      const std::size_t dstX = y;             // 0‥(height-1)
      const std::size_t dstY = width - 1 - x; // (width-1)‥0
      const std::size_t dst_idx = (dstY * dstW + dstX) * BYTES_PER_PX;

      dst[dst_idx + 0] = src[src_idx + 0]; // R
      dst[dst_idx + 1] = src[src_idx + 1]; // G
      dst[dst_idx + 2] = src[src_idx + 2]; // B
    }
  }
  return dst;
}

void rgbToHsl(float r, float g, float b, float &h, float &s, float &l) {
  float max_val = std::max({r, g, b});
  float min_val = std::min({r, g, b});
  l = (max_val + min_val) / 2.f;
  if (max_val == min_val) {
    h = s = 0;
  } else {
    float d = max_val - min_val;
    s = l > 0.5f ? d / (2.f - max_val - min_val) : d / (max_val + min_val);
    if (max_val == r) {
      h = (g - b) / d + (g < b ? 6.f : 0.f);
    } else if (max_val == g) {
      h = (b - r) / d + 2.f;
    } else {
      h = (r - g) / d + 4.f;
    }
    h /= 6.f;
  }
}

auto hueToRgb(float p, float q, float t) -> float {
  if (t < 0.f) {
    t += 1.f;
  }
  if (t > 1.f) {
    t -= 1.f;
  }
  if (t < 1.f / 6.f) {
    return p + (q - p) * 6.f * t;
  }
  if (t < 1.f / 2.f) {
    return q;
  }
  if (t < 2.f / 3.f) {
    return p + (q - p) * (2.f / 3.f - t) * 6.f;
  }
  return p;
}

void hslToRgb(float h, float s, float l, float &r, float &g, float &b) {
  if (s == 0) {
    r = g = b = l;
  } else {
    float q = l < 0.5f ? l * (1.f + s) : l + s - l * s;
    float p = 2.f * l - q;
    r = hueToRgb(p, q, h + 1.f / 3.f);
    g = hueToRgb(p, q, h);
    b = hueToRgb(p, q, h - 1.f / 3.f);
  }
}

void adjustImage(const std::vector<uint8_t> &src, std::vector<uint8_t> &dst,
                 int w, int h, const ImageAdjustParams &p) {

  dst.resize(src.size());
  const size_t num_pixels = static_cast<size_t>(w) * h;

  for (size_t i = 0; i < num_pixels; ++i) {
    float r = src[i * 3 + 0] / 255.f;
    float g = src[i * 3 + 1] / 255.f;
    float b = src[i * 3 + 2] / 255.f;

    // 1. 露出 (Exposure)
    float exposure_factor = std::pow(2.f, p.exposure);
    r *= exposure_factor;
    g *= exposure_factor;
    b *= exposure_factor;

    // ★★★ 修正点 ★★★
    // シャドウ・ハイライト処理の前に、値を[0, 1]の範囲にクランプする。
    // これにより、1を超えた値がシャドウ計算式に入るのを防ぐ。
    r = std::max(0.f, std::min(1.f, r));
    g = std::max(0.f, std::min(1.f, g));
    b = std::max(0.f, std::min(1.f, b));

    // 2. シャドウ (Shadow)
    if (p.shadow != 0.f) {
      float shadow_factor = std::pow(2.f, p.shadow);
      r = 1.f - std::pow(1.f - r, shadow_factor);
      g = 1.f - std::pow(1.f - g, shadow_factor);
      b = 1.f - std::pow(1.f - b, shadow_factor);
    }

    // 3. ハイライト (Highlight)
    if (p.highlight != 0.f) {
      float highlight_factor = std::pow(2.f, -p.highlight);
      r = std::pow(r, highlight_factor);
      g = std::pow(g, highlight_factor);
      b = std::pow(b, highlight_factor);
    }

    // 4. コントラスト (Contrast)
    r = (r - 0.5f) * p.contrast + 0.5f;
    g = (g - 0.5f) * p.contrast + 0.5f;
    b = (b - 0.5f) * p.contrast + 0.5f;

    // 5. 彩度 (Saturation)
    if (p.saturation != 1.f) {
      r = std::max(0.f, std::min(1.f, r));
      g = std::max(0.f, std::min(1.f, g));
      b = std::max(0.f, std::min(1.f, b));

      float h_val, s_val, l_val;
      rgbToHsl(r, g, b, h_val, s_val, l_val);
      s_val *= p.saturation;
      s_val = std::max(0.f, std::min(1.f, s_val));
      hslToRgb(h_val, s_val, l_val, r, g, b);
    }

    // 6. 色温度 (Temperature) と 色合い (Tint)
    r += p.temperature * 0.05f;
    b -= p.temperature * 0.05f;
    g -= p.tint * 0.05f;

    // 7. 最終的なクランプ
    r = std::max(0.f, std::min(1.f, r));
    g = std::max(0.f, std::min(1.f, g));
    b = std::max(0.f, std::min(1.f, b));

    // uint8_tに変換して格納
    dst[i * 3 + 0] = static_cast<uint8_t>(r * 255.f + 0.5f);
    dst[i * 3 + 1] = static_cast<uint8_t>(g * 255.f + 0.5f);
    dst[i * 3 + 2] = static_cast<uint8_t>(b * 255.f + 0.5f);
  }
}

struct Neighbor {
  int dx;       // x offset from current pixel
  int dy;       // y offset from current pixel
  float weight; // integer weight (normalisation is done separately)
};

enum class Algorithm {
  Atkinson,
  FloydSteinberg,
  JarvisJudiceNinke,
  Stucki,
  Burkes,
};

namespace detail {

// ----  diffusion kernels  -----------------------------------
constexpr std::array<Neighbor, 6> atkinson{{
    {.dx = 1, .dy = 0, .weight = 1.f / 8},
    {.dx = 2, .dy = 0, .weight = 1.f / 8},
    {.dx = -1, .dy = 1, .weight = 1.f / 8},
    {.dx = 0, .dy = 1, .weight = 1.f / 8},
    {.dx = 1, .dy = 1, .weight = 1.f / 8},
    {.dx = 0, .dy = 2, .weight = 1.f / 8},
}};

constexpr std::array<Neighbor, 4> floyd{{
    {.dx = 1, .dy = 0, .weight = 7.f / 16},
    {.dx = -1, .dy = 1, .weight = 3.f / 16},
    {.dx = 0, .dy = 1, .weight = 5.f / 16},
    {.dx = 1, .dy = 1, .weight = 1.f / 16},
}};

constexpr std::array<Neighbor, 12> jjn{{
    {.dx = 1, .dy = 0, .weight = 7.f / 48},
    {.dx = 2, .dy = 0, .weight = 5.f / 48},
    {.dx = -2, .dy = 1, .weight = 3.f / 48},
    {.dx = -1, .dy = 1, .weight = 5.f / 48},
    {.dx = 0, .dy = 1, .weight = 7.f / 48},
    {.dx = 1, .dy = 1, .weight = 5.f / 48},
    {.dx = 2, .dy = 1, .weight = 3.f / 48},
    {.dx = -2, .dy = 2, .weight = 1.f / 48},
    {.dx = -1, .dy = 2, .weight = 3.f / 48},
    {.dx = 0, .dy = 2, .weight = 5.f / 48},
    {.dx = 1, .dy = 2, .weight = 3.f / 48},
    {.dx = 2, .dy = 2, .weight = 1.f / 48},
}};

constexpr std::array<Neighbor, 12> stucki{{
    {.dx = 1, .dy = 0, .weight = 8.f / 42},
    {.dx = 2, .dy = 0, .weight = 4.f / 42},
    {.dx = -2, .dy = 1, .weight = 2.f / 42},
    {.dx = -1, .dy = 1, .weight = 4.f / 42},
    {.dx = 0, .dy = 1, .weight = 8.f / 42},
    {.dx = 1, .dy = 1, .weight = 4.f / 42},
    {.dx = 2, .dy = 1, .weight = 2.f / 42},
    {.dx = -2, .dy = 2, .weight = 1.f / 42},
    {.dx = -1, .dy = 2, .weight = 2.f / 42},
    {.dx = 0, .dy = 2, .weight = 4.f / 42},
    {.dx = 1, .dy = 2, .weight = 2.f / 42},
    {.dx = 2, .dy = 2, .weight = 1.f / 42},
}};

constexpr std::array<Neighbor, 7> burkes{{
    {.dx = 1, .dy = 0, .weight = 8.f / 32},
    {.dx = 2, .dy = 0, .weight = 4.f / 32},
    {.dx = -2, .dy = 1, .weight = 2.f / 32},
    {.dx = -1, .dy = 1, .weight = 4.f / 32},
    {.dx = 0, .dy = 1, .weight = 8.f / 32},
    {.dx = 1, .dy = 1, .weight = 4.f / 32},
    {.dx = 2, .dy = 1, .weight = 2.f / 32},
}};

constexpr std::array<Neighbor, 10> sierra{{
    {.dx = 1, .dy = 0, .weight = 5.f / 32},
    {.dx = 2, .dy = 0, .weight = 3.f / 32},
    {.dx = -2, .dy = 1, .weight = 2.f / 32},
    {.dx = -1, .dy = 1, .weight = 4.f / 32},
    {.dx = 0, .dy = 1, .weight = 5.f / 32},
    {.dx = 1, .dy = 1, .weight = 4.f / 32},
    {.dx = 2, .dy = 1, .weight = 2.f / 32},
    {.dx = -1, .dy = 2, .weight = 2.f / 32},
    {.dx = 0, .dy = 2, .weight = 3.f / 32},
    {.dx = 1, .dy = 2, .weight = 2.f / 32},
}};

constexpr std::array<Neighbor, 7> sierra2{{
    {.dx = 1, .dy = 0, .weight = 4.f / 16},
    {.dx = 2, .dy = 0, .weight = 3.f / 16},
    {.dx = -2, .dy = 1, .weight = 1.f / 16},
    {.dx = -1, .dy = 1, .weight = 2.f / 16},
    {.dx = 0, .dy = 1, .weight = 3.f / 16},
    {.dx = 1, .dy = 1, .weight = 2.f / 16},
    {.dx = 2, .dy = 1, .weight = 1.f / 16},
}};

constexpr std::array<Neighbor, 3> sierraLite{{
    {.dx = 1, .dy = 0, .weight = 2.f / 4},
    {.dx = -1, .dy = 1, .weight = 1.f / 4},
    {.dx = 0, .dy = 1, .weight = 1.f / 4},
}};

// ----  generic diffusion engine  ----------------------------

template <typename Pattern>
void diffuse(std::vector<std::uint8_t> &img, int w, int h, const Pattern &pat) {
  auto idx = [&](int x, int y) { return 3 * (y * w + x); };

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int i = idx(x, y);
      Pixel old{img[i], img[i + 1], img[i + 2]};
      Pixel newc = nearest_color(old);
      std::array<int, 3> err{int(old[0]) - newc[0], int(old[1]) - newc[1],
                             int(old[2]) - newc[2]};

      // write quantised pixel back
      for (int c = 0; c < 3; ++c) {
        img[i + c] = newc[c];
      }

      // distribute the error
      for (auto [dx, dy, wgt] : pat) {
        const int nx = x + dx;
        const int ny = y + dy;
        if (nx < 0 || nx >= w || ny < 0 || ny >= h) {
          continue;
        }
        const int j = idx(nx, ny);
        for (int c = 0; c < 3; ++c) {
          const int v = int(img[j + c]) + err[c] * wgt;
          img[j + c] = std::clamp(v, 0, 255);
        }
      }
    }
  }
}

} // namespace detail

void Atkinson(std::vector<uint8_t> &img, int w, int h) {
  detail::diffuse(img, w, h, detail::atkinson);
}

void FloydSteinberg(std::vector<uint8_t> &img, int w, int h) {
  detail::diffuse(img, w, h, detail::floyd);
}

void JarvisJudiceNinke(std::vector<uint8_t> &img, int w, int h) {
  detail::diffuse(img, w, h, detail::jjn);
}

void Stucki(std::vector<uint8_t> &img, int w, int h) {
  detail::diffuse(img, w, h, detail::stucki);
}

void Burkes(std::vector<uint8_t> &img, int w, int h) {
  detail::diffuse(img, w, h, detail::burkes);
}

void Sierra(std::vector<uint8_t> &img, int w, int h) {
  detail::diffuse(img, w, h, detail::sierra);
}

void Sierra2(std::vector<uint8_t> &img, int w, int h) {
  detail::diffuse(img, w, h, detail::sierra2);
}

void SierraLite(std::vector<uint8_t> &img, int w, int h) {
  detail::diffuse(img, w, h, detail::sierraLite);
}

void DBSDither(std::vector<std::uint8_t> &img, int w, int h) {
  constexpr int maxPass = 6;
  constexpr float threshold = 128.0f;
  constexpr int R = 2;
  constexpr float g5[5][5] = {{1, 4, 7, 4, 1},
                              {4, 16, 26, 16, 4},
                              {7, 26, 41, 26, 7},
                              {4, 16, 26, 16, 4},
                              {1, 4, 7, 4, 1}};
  constexpr float gSum = 273.0f;
  const int N = w * h;

  std::array<std::vector<float>, 3> src;
  std::array<std::vector<std::uint8_t>, 3> ht;
  std::array<std::vector<float>, 3> resid;
  for (int c = 0; c < 3; ++c) {
    src[c].resize(N);
    ht[c].resize(N);
    resid[c].resize(N);
  }

  for (int i = 0; i < N; ++i) {
    int p = 3 * i;
    src[0][i] = static_cast<float>(img[p]);
    src[1][i] = static_cast<float>(img[p + 1]);
    src[2][i] = static_cast<float>(img[p + 2]);
  }

  std::array<float, 25> kernel{};
  for (int dy = -R; dy <= R; ++dy) {
    for (int dx = -R; dx <= R; ++dx) {
      kernel[(dy + R) * 5 + (dx + R)] = g5[dy + R][dx + R] / gSum;
    }
  }

  auto idx = [&](int x, int y) { return y * w + x; };

  for (int c = 0; c < 3; ++c) {
    for (int i = 0; i < N; ++i) {
      ht[c][i] = src[c][i] >= threshold ? 255 : 0;
    }
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        float s = 0.0f;
        for (int dy = -R; dy <= R; ++dy) {
          int yy = y + dy;
          if (yy < 0 || yy >= h) {
            continue;
          }
          for (int dx = -R; dx <= R; ++dx) {
            int xx = x + dx;
            if (xx < 0 || xx >= w) {
              continue;
            }
            s += ht[c][idx(xx, yy)] * kernel[(dy + R) * 5 + (dx + R)];
          }
        }
        resid[c][idx(x, y)] = s - src[c][idx(x, y)];
      }
    }

    for (int pass = 0; pass < maxPass; ++pass) {
      bool improved = false;
      for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
          int q = idx(x, y);
          int pix = ht[c][q];
          int deltaPix = (pix == 0) ? 255 : -255;
          float deltaE = 0.0f;
          for (int dy = -R; dy <= R; ++dy) {
            int yy = y + dy;
            if (yy < 0 || yy >= h) {
              continue;
            }
            for (int dx = -R; dx <= R; ++dx) {
              int xx = x + dx;
              if (xx < 0 || xx >= w) {
                continue;
              }
              float kw = kernel[(dy + R) * 5 + (dx + R)];
              if (kw == 0.0f) {
                continue;
              }
              int p = idx(xx, yy);
              float diff = deltaPix * kw;
              deltaE += 2.0f * resid[c][p] * diff + diff * diff;
            }
          }
          if (deltaE < -1e-3f) {
            ht[c][q] = (pix == 0) ? 255 : 0;
            for (int dy = -R; dy <= R; ++dy) {
              int yy = y + dy;
              if (yy < 0 || yy >= h) {
                continue;
              }
              for (int dx = -R; dx <= R; ++dx) {
                int xx = x + dx;
                if (xx < 0 || xx >= w) {
                  continue;
                }
                float kw = kernel[(dy + R) * 5 + (dx + R)];
                if (kw == 0.0f) {
                  continue;
                }
                int p = idx(xx, yy);
                float diff = deltaPix * kw;
                resid[c][p] += diff;
              }
            }
            improved = true;
          }
        }
      }
      if (!improved) {
        break;
      }
    }
  }

  for (int i = 0; i < N; ++i) {
    img[3 * i] = ht[0][i];
    img[3 * i + 1] = ht[1][i];
    img[3 * i + 2] = ht[2][i];
  }
}
void Ordered(std::vector<uint8_t> &img, int w, int h) {
  static const std::array<std::array<int, 4>, 4> M = {
      {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}}};
  auto idx = [&](int x, int y) { return 3 * (y * w + x); };
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int threshold = ((M[y & 3][x & 3] * 16) + 8); // 0..255
      int i = idx(x, y);
      for (int c = 0; c < 3; ++c) {
        img[i + c] = (img[i + c] > threshold) ? 255 : 0;
      }
    }
  }
}

void Random(std::vector<uint8_t> &img, int w, int h) {
  static thread_local std::mt19937 rng(std::random_device{}());
  static thread_local std::uniform_int_distribution<int> dist(0, 255);
  auto idx = [&](int x, int y) { return 3 * (y * w + x); };
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int thr = dist(rng);
      int i = idx(x, y);
      for (int c = 0; c < 3; ++c) {
        img[i + c] = (img[i + c] > thr) ? 255 : 0;
      }
    }
  }
}

void Threshold(std::vector<uint8_t> &img, int w, int h) {
  constexpr int T = 128;
  auto idx = [&](int x, int y) { return 3 * (y * w + x); };
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int i = idx(x, y);
      for (int c = 0; c < 3; ++c) {
        img[i + c] = (img[i + c] > T) ? 255 : 0;
      }
    }
  }
}

}; // namespace Processing
