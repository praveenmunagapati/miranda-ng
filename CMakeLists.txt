cmake_minimum_required (VERSION 2.8)
 
project (Miranda)

find_package(OpenSSL)

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS "_UNICODE;UNICODE;_CRT_SECURE_NO_WARNINGS;_CRT_NONSTDC_NO_WARNINGS")
if(CMAKE_CL_64)
	link_directories(${CMAKE_SOURCE_DIR}/libs/win64)
else()
	link_directories(${CMAKE_SOURCE_DIR}/libs/win32)
endif()
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include/msapi ${CMAKE_SOURCE_DIR}/plugins/ExternalAPI)

add_subdirectory(libs)
add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(protocols)