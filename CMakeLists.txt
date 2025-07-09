cmake_minimum_required(VERSION 3.16)
project(RaspiEpaper C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(IS_LINUX TRUE)
  add_definitions(-DIS_LINUX=1)
  add_subdirectory("epaper")
else()
  set(IS_LINUX FALSE)
  add_definitions(-DIS_LINUX=0)
endif()

add_subdirectory("apps")
