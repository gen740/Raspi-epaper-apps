set(CMAKE_CXX_STANDARD 23)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

file(GLOB RASPI_APPS_DIRS CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*)

foreach(child ${RASPI_APPS_DIRS})
  if(IS_DIRECTORY ${child})
    message(STATUS "Found application: ${child}")
    file(GLOB APP_C_SOURCES CONFIGURE_DEPENDS ${child}/*.c)
    file(GLOB APP_CXX_SOURCES CONFIGURE_DEPENDS ${child}/*.cc)
    get_filename_component(child_name ${child} NAME)
    add_executable(${child_name} ${APP_C_SOURCES} ${APP_CXX_SOURCES})
    target_link_libraries(${child_name} PRIVATE gRPC::grpc++
                                                protobuf::libprotobuf)
    target_link_libraries(${child_name} PRIVATE common epaper
                                                image_server_proto)
    target_compile_features(${child_name} PRIVATE cxx_std_23)
    install(TARGETS ${child_name} RUNTIME DESTINATION bin)
  endif()
endforeach()
