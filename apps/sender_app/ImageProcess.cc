#include "ImageProcess.hh"
#include <algorithm>
#include <cmath>
#include <map>
#include <ranges>
#include <vector>

namespace Processing {

const std::map<EPDColor, std::array<uint8_t, 3>> PALETTE = {
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

auto nearest_color(const std::array<uint8_t, 3> &c) -> std::array<uint8_t, 3> {
  int best_d = 1 << 30;
  std::array<uint8_t, 3> best = {};
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

void Atkinson(std::vector<uint8_t> &img, int w, int h) {
  auto idx = [&](int x, int y) { return 3 * (y * w + x); };
  for (int y = 0; y < h - 2; ++y) {
    for (int x = 0; x < w - 2; ++x) {
      int i = idx(x, y);
      std::array<uint8_t, 3> old = {img[i], img[i + 1], img[i + 2]};
      std::array<uint8_t, 3> newc = nearest_color(old);
      std::array<int, 3> err = {(int(old[0]) - newc[0]) / 8,
                                (int(old[1]) - newc[1]) / 8,
                                (int(old[2]) - newc[2]) / 8};
      for (int c = 0; c < 3; ++c) {
        img[i + c] = newc[c];
      }

      const std::array<std::pair<int, int>, 6> offset = {
          {{1, 0}, {2, 0}, {-1, 1}, {0, 1}, {1, 1}, {0, 2}}};
      for (auto [dx, dy] : offset) {
        int nx = x + dx, ny = y + dy;
        if (nx < 0 || nx >= w || ny < 0 || ny >= h) {
          continue;
        }
        int j = idx(nx, ny);
        for (int c = 0; c < 3; ++c) {
          int val = int(img[j + c]) + err[c];
          img[j + c] = std::clamp(val, 0, 255);
        }
      }
    }
  }
}

}; // namespace Processing
