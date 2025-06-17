#include <algorithm>
#include <array>
#include <iostream>
#include <ranges>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE2_IMPLEMENTATION

#include "color_palette.hh"
#include "stb/stb_image.h"
#include "stb/stb_image_resize2.h"
#include "stb/stb_image_write.h"

constexpr int WIDTH = 800;
constexpr int HEIGHT = 480;

using Apps::Common::Color;
using Apps::Common::PALETTE;

auto nearest_color(const Color &c) -> Color {
  int best_d = 1 << 30;
  Color best = {};
  for (const auto &p : PALETTE | std::views::values) {
    int dr = int(c[0]) - p[0], dg = int(c[1]) - p[1], db = int(c[2]) - p[2];
    int d = dr * dr + dg * dg + db * db;
    if (d < best_d) {
      best_d = d, best = p;
    }
  }
  return best;
}

void Floyd(std::vector<uint8_t> &img, int w, int h) {
  auto idx = [&](int x, int y) { return 3 * (y * w + x); };
  for (int y = 0; y < h - 1; ++y) {
    for (int x = 1; x < w - 1; ++x) {
      int i = idx(x, y);
      Color old = {img[i], img[i + 1], img[i + 2]};
      Color newc = nearest_color(old);
      std::array<int, 3> err = {int(old[0]) - newc[0], int(old[1]) - newc[1],
                                int(old[2]) - newc[2]};
      for (int c = 0; c < 3; ++c) {
        img[i + c] = newc[c];
      }

      for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = 0; dy <= 1; ++dy) {
          int nx = x + dx, ny = y + dy;
          if (nx < 0 || nx >= w || ny < 0 || ny >= h) {
            continue;
          }
          double coef = 0.f;
          if (dx == 1 && dy == 0) {
            coef = 7. / 16;
          }
          if (dx == -1 && dy == 1) {
            coef = 3. / 16;
          }
          if (dx == 0 && dy == 1) {
            coef = 5. / 16;
          }
          if (dx == 1 && dy == 1) {
            coef = 1. / 16;
          }
          int j = idx(nx, ny);
          for (int c = 0; c < 3; ++c) {
            int val = int(img[j + c]) + int(err[c] * coef);
            img[j + c] = std::clamp(val, 0, 255);
          }
        }
      }
    }
  }
}

void Atkinson(std::vector<uint8_t> &img, int w, int h) {
  auto idx = [&](int x, int y) { return 3 * (y * w + x); };
  for (int y = 0; y < h - 2; ++y) {
    for (int x = 0; x < w - 2; ++x) {
      int i = idx(x, y);
      Color old = {img[i], img[i + 1], img[i + 2]};
      Color newc = nearest_color(old);
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

auto main(int argc, char **argv) -> int {
  if (argc != 4) {
    std::cerr << "Usage: ./main <0:Floyd|1:Atkinson> <input> <output>\n";
    return 1;
  }

  int method = std::stoi(argv[1]);
  int iw, ih, ic;
  uint8_t *input = stbi_load(argv[2], &iw, &ih, &ic, 3);
  if (!input) {
    std::cerr << "Failed to load image\n";
    return 2;
  }

  double in_ar = double(iw) / ih;
  double out_ar = double(WIDTH) / HEIGHT;

  int rw, rh;
  if (in_ar > out_ar) {
    rh = HEIGHT;
    rw = int(HEIGHT * in_ar); // wider than canvas, crop x
  } else {
    rw = WIDTH;
    rh = int(WIDTH / in_ar); // taller than canvas, crop y
  }

  std::vector<uint8_t> resized(static_cast<size_t>(rw * rh * 3));
  stbir_resize_uint8_linear(input, iw, ih, 0, resized.data(), rw, rh, 0,
                            STBIR_RGB);
  stbi_image_free(input);

  std::vector<uint8_t> canvas(static_cast<size_t>(WIDTH * HEIGHT * 3));
  int crop_x = (rw - WIDTH) / 2;
  int crop_y = (rh - HEIGHT) / 2;
  for (int y = 0; y < HEIGHT; ++y) {
    for (int x = 0; x < WIDTH; ++x) {
      for (int c = 0; c < 3; ++c) {
        canvas[3 * (y * WIDTH + x) + c] =
            resized[3 * ((y + crop_y) * rw + (x + crop_x)) + c];
      }
    }
  }

  if (method == 0) {
    Floyd(canvas, WIDTH, HEIGHT);
  } else {
    Atkinson(canvas, WIDTH, HEIGHT);
  }

  if (!stbi_write_png(argv[3], WIDTH, HEIGHT, 3, canvas.data(), WIDTH * 3)) {
    std::cerr << "Failed to write image\n";
    return 3;
  }

  return 0;
}
