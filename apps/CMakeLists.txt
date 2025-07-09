add_subdirectory("proto")
add_subdirectory("common")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  add_subdirectory("raspi")
else()
  add_subdirectory("mac")
endif()
