
// gui_client.hh — common utilities: palette, dithering, gRPC client
// ------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "image_server.grpc.pb.h"
#define STB_IMAGE_IMPLEMENTATION // Define this before including stb_image.h
#include "stb/stb_image.h" // stb_image.h must be available; IMPLEMENTATION macro should be defined once in a .cc
#include <grpcpp/grpcpp.h>

namespace Apps::Common {

// ------------------------------------------------------------------------------------------------
//  Color palette for EPD (6‑color) displays
// ------------------------------------------------------------------------------------------------

enum class EPDColor : std::uint8_t {
  BLACK = 0x00,
  WHITE = 0x01,
  YELLOW = 0x02,
  RED = 0x03,
  BLUE = 0x05,
  GREEN = 0x06
};

using Color = std::array<std::uint8_t, 3>; // RGB triplet

static const std::map<EPDColor, Color> PALETTE = {
    {EPDColor::BLACK, {0, 0, 0}},      {EPDColor::WHITE, {255, 255, 255}},
    {EPDColor::YELLOW, {255, 255, 0}}, {EPDColor::RED, {255, 0, 0}},
    {EPDColor::BLUE, {0, 0, 255}},     {EPDColor::GREEN, {0, 255, 0}},
};

//---------------------------------------------------------------------
//  Utility: find nearest palette color in Euclidean RGB space
//---------------------------------------------------------------------
[[nodiscard]] inline auto nearest_color(const Color &in) -> Color {
  double min_dist = std::numeric_limits<double>::max();
  Color best = {0, 0, 0};
  for (const auto &[_, c] : PALETTE) {
    double dist = 0.0;
    for (int i = 0; i < 3; ++i) {
      dist += std::pow(static_cast<int>(in[i]) - static_cast<int>(c[i]), 2);
    }
    if (dist < min_dist) {
      min_dist = dist;
      best = c;
    }
  }
  return best;
}

//---------------------------------------------------------------------
//  Atkinson dithering (in‑place, RGB888 flat vector)
//---------------------------------------------------------------------
inline void Atkinson(std::vector<std::uint8_t> &img, int w, int h) {
  auto idx = [&](int x, int y) { return 3 * (y * w + x); };
  for (int y = 0; y < h - 2; ++y) {
    for (int x = 0; x < w - 2; ++x) {
      int i = idx(x, y);
      Color old = {img[i], img[i + 1], img[i + 2]};
      Color newc = nearest_color(old);
      std::array<int, 3> err = {(static_cast<int>(old[0]) - newc[0]) / 8,
                                (static_cast<int>(old[1]) - newc[1]) / 8,
                                (static_cast<int>(old[2]) - newc[2]) / 8};
      for (int c = 0; c < 3; ++c) {
        img[i + c] = newc[c];
      }

      static constexpr std::array<std::pair<int, int>, 6> OFF = {
          {{1, 0}, {2, 0}, {-1, 1}, {0, 1}, {1, 1}, {0, 2}}};
      for (auto [dx, dy] : OFF) {
        int nx = x + dx, ny = y + dy;
        if (nx < 0 || nx >= w || ny < 0 || ny >= h)
          continue;
        int j = idx(nx, ny);
        for (int c = 0; c < 3; ++c) {
          int val = static_cast<int>(img[j + c]) + err[c];
          img[j + c] = static_cast<std::uint8_t>(std::clamp(val, 0, 255));
        }
      }
    }
  }
}

//---------------------------------------------------------------------
//  Image → 4‑bit packed payload encoder (800×480)
//---------------------------------------------------------------------
constexpr int WIDTH = 800;
constexpr int HEIGHT = 480;

[[nodiscard]] inline EPDColor closest_epd_color(const std::uint8_t *rgb) {
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

[[nodiscard]] inline auto encode_image(const std::string &path)
    -> std::vector<std::uint8_t> {
  int w, h, ch;
  std::uint8_t *data = stbi_load(path.c_str(), &w, &h, &ch, 3);
  if (!data) {
    throw std::runtime_error("Failed to load image: " + path);
  }

  if ((w != WIDTH || h != HEIGHT) && !(w == HEIGHT && h == WIDTH)) {
    stbi_image_free(data);
    throw std::runtime_error("Expected 800x480 or rotated 480x800 image");
  }

  std::vector<std::uint8_t> result;
  result.reserve(WIDTH * HEIGHT / 2);

  if (w == WIDTH && h == HEIGHT) {
    // 正常な向き
    for (int i = 0; i < WIDTH * HEIGHT; i += 2) {
      EPDColor c1 = closest_epd_color(&data[static_cast<ptrdiff_t>(i * 3)]);
      EPDColor c2 =
          closest_epd_color(&data[(static_cast<ptrdiff_t>(i + 1) * 3)]);
      result.push_back((static_cast<std::uint8_t>(c2) & 0x0F) |
                       (static_cast<std::uint8_t>(c1) << 4));
    }
  } else {
    // 反時計回りに90度回転 (w = HEIGHT, h = WIDTH)
    for (int y = 0; y < HEIGHT; ++y) {
      for (int x = 0; x < WIDTH; x += 2) {
        int src_idx1 = (x * w + (h - 1 - y)) * 3;
        int src_idx2 = ((x + 1) * w + (h - 1 - y)) * 3;
        EPDColor c1 = closest_epd_color(&data[src_idx1]);
        EPDColor c2 = closest_epd_color(&data[src_idx2]);
        result.push_back((static_cast<std::uint8_t>(c2) & 0x0F) |
                         (static_cast<std::uint8_t>(c1) << 4));
      }
    }
  }
  stbi_image_free(data);
  return result;
}
//---------------------------------------------------------------------
//  gRPC image sender (blocking)
//---------------------------------------------------------------------
class ImageClient {
public:
  explicit ImageClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(image_server::DataService::NewStub(std::move(channel))) {}

  void Send(const std::vector<std::uint8_t> &payload) {
    image_server::DataRequest req;
    req.set_payload(payload.data(), payload.size());

    image_server::DataResponse resp;
    grpc::ClientContext ctx;

    grpc::Status status = stub_->SendData(&ctx, req, &resp);
    if (!status.ok()) {
      throw std::runtime_error("gRPC failed: " + status.error_message());
    }
  }

private:
  std::unique_ptr<image_server::DataService::Stub> stub_;
};

} // namespace Apps::Common
