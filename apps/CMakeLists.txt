add_subdirectory("proto")
add_subdirectory("common")
add_subdirectory("${CMAKE_SOURCE_DIR}/capnproto" "${CMAKE_CURRENT_BINARY_DIR}/capnproto")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  add_subdirectory("raspi")
else()
  add_subdirectory("mac")
endif()
