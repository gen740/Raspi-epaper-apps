# set(CMAKE_C_STANDARD 99) set(CMAKE_C_STANDARD_REQUIRED ON)

# アプリケーションディレクトリのスキャン
file(
  GLOB children
  RELATIVE ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/*)

set(CMAKE_CXX_STANDARD 23)

foreach(child ${children})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${child}/main.c
       OR EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${child}/main.cc)
      message(STATUS "Found application: ${child}")
      file(GLOB APP_C_SOURCES CONFIGURE_DEPENDS ${child}/*.c)
      file(GLOB APP_CXX_SOURCES CONFIGURE_DEPENDS ${child}/*.cc)

      add_executable(${child} ${APP_C_SOURCES} ${APP_CXX_SOURCES})

      target_link_libraries(${child} PRIVATE epaper)

      install(TARGETS ${child} RUNTIME DESTINATION bin)
    endif()
  endif()
endforeach()
