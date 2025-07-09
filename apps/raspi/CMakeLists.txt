set(CMAKE_CXX_STANDARD 23)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAPNP REQUIRED capnp)
pkg_check_modules(CAPNP_RPC REQUIRED capnp-rpc)

file(GLOB RASPI_APPS_DIRS CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*)

foreach(child ${RASPI_APPS_DIRS})
  if(IS_DIRECTORY ${child})
    message(STATUS "Found application: ${child}")
    file(GLOB APP_C_SOURCES CONFIGURE_DEPENDS ${child}/*.c)
    file(GLOB APP_CXX_SOURCES CONFIGURE_DEPENDS ${child}/*.cc)
    get_filename_component(child_name ${child} NAME)
    add_executable(${child_name} ${APP_C_SOURCES} ${APP_CXX_SOURCES})
    
    # Common libraries
    target_link_libraries(${child_name} PRIVATE common epaper)
    
    # Different linking for image_server2 (Cap'n Proto) vs image_server (gRPC)
    if(${child_name} STREQUAL "image_server2")
      target_link_libraries(${child_name} PRIVATE ${CAPNP_LIBRARIES} ${CAPNP_RPC_LIBRARIES})
      target_link_libraries(${child_name} PRIVATE capnp_image_service)
      target_include_directories(${child_name} PRIVATE ${CAPNP_INCLUDE_DIRS})
      target_compile_options(${child_name} PRIVATE ${CAPNP_CFLAGS_OTHER})
    else()
      target_link_libraries(${child_name} PRIVATE gRPC::grpc++ protobuf::libprotobuf)
      target_link_libraries(${child_name} PRIVATE image_server_proto)
    endif()
    
    target_compile_features(${child_name} PRIVATE cxx_std_23)
    install(TARGETS ${child_name} RUNTIME DESTINATION bin)
  endif()
endforeach()
