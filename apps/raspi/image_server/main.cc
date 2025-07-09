#include <capnp/ez-rpc.h>
#include <capnp/message.h>
#include <capnp/serialize.h>
#include "image_service.capnp.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include "Utility.hh"
#include "epd_7in3e.hh"

namespace fs = std::filesystem;

std::atomic<bool> image_changed_by_user{false};

class ImageServiceImpl final : public ImageService::Server {
public:
  ImageServiceImpl(std::shared_ptr<Epaper::EPD7IN3E> epd7in3e,
                   std::shared_ptr<std::mutex> mutex)
      : epd7in3e_(std::move(epd7in3e)), mutex_(std::move(mutex)) {}

  kj::Promise<void> sendImage(SendImageContext context) override {
    auto params = context.getParams();
    auto imageData = params.getImageData();
    auto payload = imageData.getPayload();
    
    if (payload.size() != 800 * 480 / 2) {
      auto response = context.getResults();
      response.getResponse().setStatus(Status::IMAGE_SIZE_MISMATCH);
      return kj::READY_NOW;
    }

    // Convert payload to std::array<uint8_t, 800 * 480 / 2>
    std::array<uint8_t, 800 * 480 / 2> buffer;
    for (size_t i = 0; i < payload.size(); ++i) {
      buffer[i] = static_cast<uint8_t>(payload[i]);
    }
    
    std::lock_guard<std::mutex> lock(*mutex_);
    image_changed_by_user = true;
    epd7in3e_->display(buffer.data());
    
    auto response = context.getResults();
    response.getResponse().setStatus(Status::OK);
    return kj::READY_NOW;
  }

private:
  std::shared_ptr<Epaper::EPD7IN3E> epd7in3e_;
  std::shared_ptr<std::mutex> mutex_;
};

auto main() -> int {
  std::cout << "Starting Cap'n Proto server..." << std::endl;
  auto epd7in3e_ = std::make_shared<Epaper::EPD7IN3E>();
  auto mutex_ = std::make_shared<std::mutex>();

  capnp::EzRpcServer server(kj::heap<ImageServiceImpl>(epd7in3e_, mutex_), "*", 50051);
  
  auto& waitScope = server.getWaitScope();
  
  std::cout << "Server listening on port 50051" << std::endl;
  
  // Run slideshow loop in a separate thread
  auto slideshow_thread = std::thread([&epd7in3e_, &mutex_]() {
    while (true) {
      if (image_changed_by_user) {
        image_changed_by_user = false;
        std::this_thread::sleep_for(std::chrono::seconds(900));
      } else {
        const fs::path images_root = "/home/gen/images";
        std::random_device rd;
        std::mt19937_64 rng(rd());
        std::cout << "Loading images from: " << images_root << std::endl;
        std::vector<fs::directory_entry> files{};
        for (auto const &file_entry : fs::directory_iterator(images_root)) {
          if (!file_entry.is_regular_file()) {
            continue;
          }
          files.push_back(file_entry);
        }
        std::uniform_int_distribution<std::size_t> dist(0, files.size() - 1);
        const auto &chosen = files.at(dist(rng));
        std::cout << "Chosen file: " << chosen.path() << std::endl;
        try {
          auto data = Apps::Common::load_bmp(chosen.path().string());
          std::lock_guard<std::mutex> lock(*mutex_);
          epd7in3e_->display(Apps::Common::convert_to_buffer(data).data());
        } catch (const std::exception &e) {
          std::cerr << "Error processing file " << chosen.path() << ": "
                    << e.what() << '\n';
        }
      }
      std::this_thread::sleep_for(std::chrono::seconds(900));
    }
  });

  // Keep server running on main thread
  kj::NEVER_DONE.wait(waitScope);
  return 0;
}