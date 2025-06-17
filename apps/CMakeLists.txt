# set(CMAKE_C_STANDARD 99) set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(app_linux "draw_box" "image_server" "simple")
set(app_common "dithering" "image_client")

set(children "") # 初期化

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(children ${app_linux})
endif()

list(APPEND children ${app_common})

foreach(child ${children})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${child}/main.c
       OR EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${child}/main.cc)
      message(STATUS "Found application: ${child}")
      file(GLOB APP_C_SOURCES CONFIGURE_DEPENDS ${child}/*.c)
      file(GLOB APP_CXX_SOURCES CONFIGURE_DEPENDS ${child}/*.cc)

      add_executable(${child} ${APP_C_SOURCES} ${APP_CXX_SOURCES})

      target_link_libraries(${child} PRIVATE gRPC::grpc++ protobuf::libprotobuf)
      if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        target_link_libraries(${child} PRIVATE epaper)
      endif()
      target_include_directories(${child} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common)

      install(TARGETS ${child} RUNTIME DESTINATION bin)
    endif()
  endif()
endforeach()
