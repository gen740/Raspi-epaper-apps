set(image_server_proto_file "${CMAKE_CURRENT_SOURCE_DIR}/image_server.proto")
set(image_server_proto_path ${CMAKE_CURRENT_SOURCE_DIR})
set(image_server_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/image_server.pb.cc")
set(image_server_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/image_server.pb.h")
set(image_server_grpc_srcs
    "${CMAKE_CURRENT_BINARY_DIR}/image_server.grpc.pb.cc")
set(image_server_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/image_server.grpc.pb.h")

add_library(
  image_server_proto ${image_server_grpc_srcs} ${image_server_grpc_hdrs}
                     ${image_server_proto_srcs} ${image_server_proto_hdrs})

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

add_custom_command(
  OUTPUT "${image_server_proto_srcs}" "${image_server_proto_hdrs}"
         "${image_server_grpc_srcs}" "${image_server_grpc_hdrs}"
  COMMAND
    ${_PROTOBUF_PROTOC} ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}" --cpp_out
    "${CMAKE_CURRENT_BINARY_DIR}" -I "${image_server_proto_path}"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${image_server_proto_file}"
  DEPENDS "${image_server_proto_file}")

target_include_directories(image_server_proto SYSTEM
                           PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_sources(image_server_proto PRIVATE ${image_server_proto_srcs}
                                          ${image_server_grpc_srcs})
target_link_libraries(image_server_proto PUBLIC gRPC::grpc++
                                                protobuf::libprotobuf)
