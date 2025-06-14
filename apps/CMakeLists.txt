# set(CMAKE_C_STANDARD 99)
# set(CMAKE_C_STANDARD_REQUIRED ON)

# アプリケーションディレクトリのスキャン
file(GLOB children RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)

foreach(child ${children})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${child}/main.c)
            message(STATUS "Found application: ${child}")
            file(GLOB APP_SOURCES CONFIGURE_DEPENDS
                ${child}/*.c
            )
            add_executable(${child} ${APP_SOURCES})
            target_link_libraries(${child} PRIVATE epaper)

            install(TARGETS ${child} RUNTIME DESTINATION bin)
        endif()
    endif()
endforeach()
