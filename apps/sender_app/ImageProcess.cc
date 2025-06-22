#include "ImageProcess.hh"
#include <algorithm>
#include <cmath>
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

void adjustImage(const std::vector<uint8_t> &src, std::vector<uint8_t> &dst,
                 int w, int h, const ImageAdjustParams &p) {
  const std::size_t expected = static_cast<std::size_t>(w) * h * 3;
  if (src.size() != expected) {
    return;
  }
  dst.resize(expected);

  /* 1. 前計算 ---------------------------------------------- */
  const float kExp = std::exp2(p.exposure);
  const float kC = p.contrast;
  const float kSat = p.saturation;
  const float hHL = std::clamp(p.highlight, 0.f, 1.f);
  const float hSH = std::clamp(p.shadow, 0.f, 1.f);

  const float rT = 1.f + std::max(0.f, p.temperature) * 1.0f + p.tint * 0.05f;
  const float gT = 1.f + p.tint * -0.10f;
  const float bT = 1.f + std::max(0.f, -p.temperature) * 1.0f + p.tint * 0.05f;

  auto luma = [](float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b; // Rec.709
  };
  auto clip = [](float v) { return std::clamp(v, 0.f, 1.f); };

  /* 2. ピクセル処理 ---------------------------------------- */
  const int n = w * h;
  for (int i = 0; i < n; ++i) {
    const std::size_t idx = static_cast<std::size_t>(i) * 3;

    /* 8-bit → 0–1 float */
    float r = src[idx] / 255.f;
    float g = src[idx + 1] / 255.f;
    float b = src[idx + 2] / 255.f;

    /* 露出 */
    r *= kExp;
    g *= kExp;
    b *= kExp;

    /* 色温度 + ティント */
    r *= rT;
    g *= gT;
    b *= bT;

    /* コントラスト (中心 0.5) */
    r = (r - 0.5f) * kC + 0.5f;
    g = (g - 0.5f) * kC + 0.5f;
    b = (b - 0.5f) * kC + 0.5f;

    /* ハイライト & シャドウ */
    float y = luma(r, g, b);
    float yAdj = (y < 0.5f) ? y + (0.5f - y) * hSH  // シャドウ持ち上げ
                            : y - (y - 0.5f) * hHL; // ハイライト抑制
    float f = (y == 0.f) ? 0.f : yAdj / y;
    r *= f;
    g *= f;
    b *= f;

    /* 彩度 */
    float gray = luma(r, g, b);
    r = gray + (r - gray) * kSat;
    g = gray + (g - gray) * kSat;
    b = gray + (b - gray) * kSat;

    /* 書き戻し (0–1 → 8-bit) */
    dst[idx] = static_cast<uint8_t>(std::round(clip(r) * 255.f));
    dst[idx + 1] = static_cast<uint8_t>(std::round(clip(g) * 255.f));
    dst[idx + 2] = static_cast<uint8_t>(std::round(clip(b) * 255.f));
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
