#include <grpcpp/grpcpp.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include "Utility.hh"
#include "epd_7in3e.hh"
#include "image_server.grpc.pb.h"

namespace fs = std::filesystem;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using image_server::DataRequest;
using image_server::DataResponse;
using image_server::DataService;

class DataServiceImpl final : public DataService::Service {
public:
  auto SendData([[maybe_unused]] ServerContext *context,
                const DataRequest *request, DataResponse *response)
      -> ::grpc::Status override {
    const std::string &data = request->payload();

    if (data.size() != 800 * 480 / 2) {
      response->set_status(image_server::Status::IMAGE_SIZE_MISMATCH);
      return ::grpc::Status::OK;
    }

    // data to std::array<uint8_t, 800 * 480 / 2>
    std::array<uint8_t, 800 * 480 / 2> buffer;
    for (size_t i = 0; i < data.size(); ++i) {
      buffer[i] = static_cast<uint8_t>(data[i]);
    }
    epd7in3e_->display(buffer.data());
    response->set_status(image_server::Status::OK);
    return ::grpc::Status::OK;
  }

  DataServiceImpl(std::shared_ptr<Epaper::EPD7IN3E> epd7in3e,
                  std::shared_ptr<std::mutex> mutex)
      : epd7in3e_(std::move(epd7in3e)), mutex_(std::move(mutex)) {}

private:
  std::shared_ptr<Epaper::EPD7IN3E> epd7in3e_;
  std::shared_ptr<std::mutex> mutex_;
};

auto main() -> int {
  std::cout << "Starting..." << std::endl;
  // auto epd7in3e_ = std::make_shared<Epaper::EPD7IN3E>();
  std::shared_ptr<Epaper::EPD7IN3E> epd7in3e_ = nullptr;
  auto mutex_ = std::make_shared<std::mutex>();

  const std::string server_address("0.0.0.0:50051");
  DataServiceImpl service(epd7in3e_, mutex_);

  ServerBuilder builder;
  builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  auto server = builder.BuildAndStart();

  auto t = std::thread([&server, &server_address]() {
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
  });

  while (true) {
    const char *home = std::getenv("HOME");
    if (home == nullptr) {
      std::cerr << "HOME environment variable not set\n";
      return 1;
    }
    const fs::path images_root = fs::path(home) / "images";

    std::random_device rd;
    std::mt19937_64 rng(rd());

    for (auto const &dir_entry : fs::directory_iterator(images_root)) {
      if (!dir_entry.is_directory()) {
        continue;
      }
      std::vector<fs::directory_entry> files;
      for (auto const &file_entry : fs::directory_iterator(dir_entry.path())) {
        if (file_entry.is_regular_file()) {
          files.push_back(file_entry);
        }
      }
      if (files.empty()) {
        continue;
      }
      std::uniform_int_distribution<std::size_t> dist(0, files.size() - 1);
      const auto &chosen = files[dist(rng)];
      try {
        auto data = Apps::Common::load_bmp(chosen.path().string());
        epd7in3e_->display(Apps::Common::convert_to_buffer(data).data());
      } catch (const std::exception &e) {
        std::cerr << "Error processing file " << chosen.path() << ": "
                  << e.what() << '\n';
      }
    }
    // Sleep for 300 seconds before the next iteration
    std::this_thread::sleep_for(std::chrono::seconds(300));
  }

  t.join();
  return 0;
}
