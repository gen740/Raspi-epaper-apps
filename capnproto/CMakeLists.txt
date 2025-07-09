find_package(PkgConfig REQUIRED)
pkg_check_modules(CAPNP REQUIRED capnp)
pkg_check_modules(CAPNP_RPC REQUIRED capnp-rpc)

# Function to generate Cap'n Proto C++ files
function(capnp_generate_cpp SRCS HDRS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: CAPNP_GENERATE_CPP() called without any proto files")
    return()
  endif()

  set(${SRCS})
  set(${HDRS})

  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    
    list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.capnp.c++")
    list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.capnp.h")
    
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.capnp.c++"
             "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.capnp.h"
      COMMAND capnp compile -oc++:${CMAKE_CURRENT_BINARY_DIR} --src-prefix=${CMAKE_CURRENT_SOURCE_DIR} ${ABS_FIL}
      DEPENDS ${ABS_FIL}
      COMMENT "Running Cap'n Proto compiler on ${FIL}"
      VERBATIM
    )
  endforeach()
  
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()

# Generate C++ files from Cap'n Proto schema
capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS image_service.capnp)

# Create a library for the generated Cap'n Proto files
add_library(capnp_image_service ${CAPNP_SRCS} ${CAPNP_HDRS})

target_include_directories(capnp_image_service PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CAPNP_INCLUDE_DIRS}
)

target_link_libraries(capnp_image_service
  ${CAPNP_LIBRARIES}
  ${CAPNP_RPC_LIBRARIES}
)

target_compile_options(capnp_image_service PRIVATE ${CAPNP_CFLAGS_OTHER})