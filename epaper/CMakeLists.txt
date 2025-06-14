add_library(epaper STATIC ${CMAKE_CURRENT_LIST_DIR}/src/Epaper.cc)

find_package(PkgConfig REQUIRED)
pkg_check_modules(gpiodcxx REQUIRED IMPORTED_TARGET libgpiodcxx)

target_compile_features(epaper PUBLIC cxx_std_23)

target_link_libraries(epaper PRIVATE PkgConfig::gpiodcxx)

target_include_directories(
  epaper PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
                $<INSTALL_INTERFACE:include>)
