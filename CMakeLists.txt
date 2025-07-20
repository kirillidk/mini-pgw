cmake_minimum_required(VERSION 3.15)
project("mini-pgw" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

configure_file(
    "${CMAKE_SOURCE_DIR}/config.json"
    "${CMAKE_BINARY_DIR}/bin/config.json"
    COPYONLY
)

add_subdirectory(src)