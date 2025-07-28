cmake_minimum_required(VERSION 3.15)
project("mini-pgw" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.0/CPM.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_BINARY_DIR}/.cpm-cache" CACHE PATH "CPM source cache")
set(CPM_USE_LOCAL_PACKAGES ON)

CPMAddPackage(
    NAME Boost
    VERSION 1.88.0
    FORCE YES
    URL https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.tar.xz
    OPTIONS
        "BOOST_ENABLE_CMAKE ON"
        "BOOST_INCLUDE_LIBRARIES log\\;log_setup"
        "BUILD_SHARED_LIBS OFF"
        "Boost_USE_STATIC_LIBS ON"
)

CPMAddPackage(
    NAME boost_di
    GITHUB_REPOSITORY boost-ext/di
    VERSION 1.3.2
)

add_library(boost_di_interface INTERFACE)
target_include_directories(boost_di_interface INTERFACE
    "${boost_di_SOURCE_DIR}/include"
)

CPMAddPackage("gh:yhirose/cpp-httplib@0.23.1")
CPMAddPackage("gh:nlohmann/json@3.12.0")
CPMAddPackage("gh:Neargye/magic_enum@0.9.7")

CPMAddPackage(
    NAME googletest
    GITHUB_REPOSITORY google/googletest
    VERSION 1.14.0
    OPTIONS
        "INSTALL_GTEST OFF"
        "gtest_force_shared_crt ON"
)

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
