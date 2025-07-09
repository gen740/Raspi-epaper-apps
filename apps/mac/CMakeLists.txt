find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAPNP REQUIRED capnp)
pkg_check_modules(CAPNP_RPC REQUIRED capnp-rpc)

file(GLOB MAC_APPS_DIRS CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*)

cmake_policy(SET CMP0100 NEW)
set(CMAKE_AUTOMOC ON)

foreach(child ${MAC_APPS_DIRS})
  if(IS_DIRECTORY ${child})
    message(STATUS "Found application: ${child}")
    file(GLOB APP_C_SOURCES CONFIGURE_DEPENDS ${child}/*.c)
    file(GLOB APP_CXX_SOURCES CONFIGURE_DEPENDS ${child}/*.cc ${child}/*.hh)
    get_filename_component(child_name ${child} NAME)
    add_executable(${child_name} ${APP_C_SOURCES} ${APP_CXX_SOURCES})
    
    # Common libraries for all Mac apps
    target_link_libraries(${child_name} PRIVATE common Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent)
    
    # Cap'n Proto dependencies for all apps
    target_link_libraries(${child_name} PRIVATE ${CAPNP_LIBRARIES} ${CAPNP_RPC_LIBRARIES} capnp_image_service)
    target_include_directories(${child_name} PRIVATE ${CAPNP_INCLUDE_DIRS})
    target_compile_options(${child_name} PRIVATE ${CAPNP_CFLAGS_OTHER})
    
    target_compile_features(${child_name} PRIVATE cxx_std_23)
    install(TARGETS ${child_name} RUNTIME DESTINATION bin)
  endif()
endforeach()
