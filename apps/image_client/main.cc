#include <grpcpp/grpcpp.h>

#define STB_IMAGE_IMPLEMENTATION
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "color_palette.hh"
#include "image_server.grpc.pb.h"
#include "stb/stb_image.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using image_server::DataRequest;
using image_server::DataResponse;
using image_server::DataService;

constexpr int WIDTH = 800;
constexpr int HEIGHT = 480;

using Apps::Common::EPDColor;
using Apps::Common::PALETTE;

auto closest_epd_color(const uint8_t *rgb) -> EPDColor {
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

auto encode_image(const std::string &path) -> std::vector<uint8_t> {
  int w, h, ch;
  uint8_t *data = stbi_load(path.c_str(), &w, &h, &ch, 3);
  if (!data) {
    throw std::runtime_error("Failed to load image: " + path);
  }

  if (w != WIDTH || h != HEIGHT) {
    stbi_image_free(data);
    throw std::runtime_error("Expected 800x480 image");
  }

  std::vector<uint8_t> result;
  result.reserve(WIDTH * HEIGHT / 2);

  for (int i = 0; i < WIDTH * HEIGHT; i += 2) {
    EPDColor c1 = closest_epd_color(&data[static_cast<ptrdiff_t>(i * 3)]);
    EPDColor c2 = closest_epd_color(&data[static_cast<ptrdiff_t>((i + 1) * 3)]);
    result.push_back((static_cast<uint8_t>(c2) & 0x0F) |
                     (static_cast<uint8_t>(c1) << 4));
  }

  stbi_image_free(data);
  return result;
}

class ImageClient {
public:
  explicit ImageClient(std::shared_ptr<Channel> channel)
      : stub_(DataService::NewStub(channel)) {}

  void Send(const std::vector<uint8_t> &payload) {
    DataRequest request;
    request.set_payload(payload.data(), payload.size());

    DataResponse response;
    ClientContext context;

    Status status = stub_->SendData(&context, request, &response);
    if (!status.ok()) {
      std::cerr << "RPC failed: " << status.error_message() << std::endl;
      return;
    }

    std::cout << "Received status: "
              << image_server::Status_Name(response.status()) << std::endl;
  }

private:
  std::unique_ptr<DataService::Stub> stub_;
};

auto main(int argc, char *argv[]) -> int {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " /path/to/image.bmp" << std::endl;
    return 1;
  }

  try {
    std::vector<uint8_t> payload = encode_image(argv[1]);

    ImageClient client(grpc::CreateChannel("192.168.1.101:50051",
                                           grpc::InsecureChannelCredentials()));

    std::cout << "Sending " << payload.size() << " bytes" << std::endl;
    client.Send(payload);

  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
