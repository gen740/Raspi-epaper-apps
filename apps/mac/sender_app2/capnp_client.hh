#pragma once

#include <capnp/ez-rpc.h>
#include <capnp/message.h>
#include <capnp/serialize.h>
#include "image_service.capnp.h"
#include <stdexcept>
#include <vector>
#include <memory>

namespace CAPNP {

class ImageClient {
public:
  explicit ImageClient(const std::string& address) : address_(address) {
  }

  void Send(const std::vector<std::uint8_t>& payload) {
    // Create client and cap on the same thread where we'll use them
    capnp::EzRpcClient client(address_);
    ImageService::Client cap = client.getMain<ImageService>();
    
    auto request = cap.sendImageRequest();
    
    auto imageData = request.getImageData();
    imageData.setPayload(kj::arrayPtr(payload.data(), payload.size()));
    
    auto promise = request.send();
    auto response = promise.wait(client.getWaitScope());
    
    if (response.getResponse().getStatus() != Status::OK) {
      throw std::runtime_error("Cap'n Proto failed: status != OK");
    }
  }

private:
  std::string address_;
};

} // namespace CAPNP