#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

struct ImageAdjustParams {
  float exposure = 0.f;    // EV (±N → 2^N 倍)
  float contrast = 1.f;    // 1 = 無変更
  float highlight = 0.f;   // 0–1
  float shadow = 0.f;      // 0–1
  float saturation = 1.f;  // 1 = 無変更
  float temperature = 0.f; // −1 … +1
  float tint = 0.f;        // −1 … +1
};

/*-------------------------------------------------------------
 * adjustImage(src, dst, w, h, params)
 *   - src  : 24-bit RGB バッファ（サイズ = 3*w*h）
 *   - dst  : 出力用バッファ（リサイズして上書き）
 *   - w,h  : 画像サイズ
 *   - p    : 各種調整値
 *-----------------------------------------------------------*/
inline void adjustImage(const std::vector<uint8_t> &src,
                        std::vector<uint8_t> &dst, int w, int h,
                        const ImageAdjustParams &p) {
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
