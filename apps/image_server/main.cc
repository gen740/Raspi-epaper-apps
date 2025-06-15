#include <grpcpp/grpcpp.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "image_server.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using image_server::DataRequest;
using image_server::DataResponse;
using image_server::DataService;
// using ;  // avoid conflict with grpc::Status

class DataServiceImpl final : public DataService::Service {
 public:
  ::grpc::Status SendData(ServerContext* context, const DataRequest* request,
                          DataResponse* response) override {
    const std::string& data = request->payload();

    // 出力: バイト列を16進で標準出力に
    std::cout << "Received payload (" << data.size() << " bytes):" << std::endl;
    for (unsigned char c : data) {
      std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << ' ';
    }
    std::cout << std::dec << std::endl;

    // レスポンス: QUEUED を返す
    response->set_status(image_server::Status::QUEUED);
    return ::grpc::Status::OK;
  }
};

void RunServer() {
  const std::string server_address("0.0.0.0:50051");
  DataServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main() {
  RunServer();
  return 0;
}
