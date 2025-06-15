#include <grpcpp/grpcpp.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "epd_7in3e.hh"
#include "image_server.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using image_server::DataRequest;
using image_server::DataResponse;
using image_server::DataService;

class DataServiceImpl final : public DataService::Service {
 public:
  ::grpc::Status SendData(ServerContext* context, const DataRequest* request,
                          DataResponse* response) override {
    const std::string& data = request->payload();

    if (data.size() != 800 * 480 / 2) {
      response->set_status(image_server::Status::IMAGE_SIZE_MISMATCH);
      return ::grpc::Status::OK;
    }

    // data to std::array<uint8_t, 800 * 480 / 2>
    std::array<uint8_t, 800 * 480 / 2> buffer;
    for (size_t i = 0; i < data.size(); ++i) {
      buffer[i] = static_cast<uint8_t>(data[i]);
    }
    epd7in3e_.display(buffer.data());
    response->set_status(image_server::Status::OK);
    return ::grpc::Status::OK;
  }

 private:
  Epaper::EPD7IN3E epd7in3e_;
};

int main() {
  const std::string server_address("0.0.0.0:50051");
  DataServiceImpl service;

  ServerBuilder builder;
  builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
  return 0;
}
