#pragma once

#include <capnp/ez-rpc.h>
#include <capnp/message.h>
#include <capnp/serialize.h>
#include "image_service.capnp.h"
#include <stdexcept>
#include <vector>

namespace CAPNP {

class ImageClient {
public:
  explicit ImageClient(const std::string& address) : client_(address), cap_(client_.getMain<ImageService>()) {
  }

  void Send(const std::vector<std::uint8_t>& payload) {
    auto request = cap_.sendImageRequest();
    
    auto imageData = request.getImageData();
    imageData.setPayload(kj::arrayPtr(payload.data(), payload.size()));
    
    auto promise = request.send();
    auto response = promise.wait(client_.getWaitScope());
    
    if (response.getResponse().getStatus() != Status::OK) {
      throw std::runtime_error("Cap'n Proto failed: status != OK");
    }
  }

private:
  capnp::EzRpcClient client_;
  ImageService::Client cap_;
};

} // namespace CAPNP