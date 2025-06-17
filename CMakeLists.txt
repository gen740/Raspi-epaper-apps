cmake_minimum_required(VERSION 3.16)
project(RaspiEpaper C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  add_subdirectory("epaper")
endif()

add_subdirectory("apps")
