cmake_minimum_required(VERSION 3.15)
project("mini-pgw" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_LIB "${PROJECT_NAME}_common_lib")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

configure_file(
    "${CMAKE_SOURCE_DIR}/server_config.json"
    "${CMAKE_BINARY_DIR}/bin/server_config.json"
    COPYONLY
)

configure_file(
    "${CMAKE_SOURCE_DIR}/client_config.json"
    "${CMAKE_BINARY_DIR}/bin/client_config.json"
    COPYONLY
)

add_subdirectory(src)
add_subdirectory(unit)