#include "image_server.grpc.pb.h"
#include <grpcpp/grpcpp.h>

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
