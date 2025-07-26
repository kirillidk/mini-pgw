include(FetchContent)

FetchContent_Declare(
  boost_di
  URL https://github.com/boost-ext/di/archive/refs/tags/v1.3.2.tar.gz
)
FetchContent_MakeAvailable(boost_di)

FetchContent_Declare(
  cpp_httplib
  URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.23.1.tar.gz
)
FetchContent_MakeAvailable(cpp_httplib)

find_package(Boost COMPONENTS log log_setup)
if (NOT Boost_FOUND)
  FetchContent_Declare(
    boost_log
      URL https://github.com/boostorg/log/archive/refs/tags/boost-1.88.0.tar.gz
    )
  FetchContent_MakeAvailable(boost_log)
endif ()

find_package(nlohmann_json 3.2.0)
if (NOT nlohmann_json_FOUND)
  FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(magic_enum 0.97.0)
if (NOT magic_enum_FOUND)
  FetchContent_Declare(
      magic_enum
      URL https://github.com/Neargye/magic_enum/releases/download/v0.9.7/magic_enum-v0.9.7.tar.gz
    )
  FetchContent_MakeAvailable(magic_enum)
endif()

set(COMMON_LIB "${PROJECT_NAME}_common_lib")

add_subdirectory(common)
add_subdirectory(server)
