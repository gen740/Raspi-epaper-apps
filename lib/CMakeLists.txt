set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

file(GLOB LIB_SRC "${SRC_DIR}/*.c")

add_library(epaper STATIC ${LIB_SRC})

target_include_directories(epaper PUBLIC ${INCLUDE_DIR})
target_link_libraries(epaper PRIVATE gpiod)

target_compile_definitions(epaper PRIVATE RPI USE_DEV_LIB)
