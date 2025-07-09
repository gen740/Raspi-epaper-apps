file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS "*.c" "*.cc")
add_library(common STATIC ${COMMON_SOURCES})
target_include_directories(common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
