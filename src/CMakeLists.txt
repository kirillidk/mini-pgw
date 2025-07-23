set(COMMON_LIB "${PROJECT_NAME}_common_lib")

find_package(nlohmann_json 3.2.0)
if (NOT nlohmann_json_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(magic_enum 0.97.0)
if (NOT magic_enum_FOUND)
  include(FetchContent)
  FetchContent_Declare(
      magic_enum
      URL https://github.com/Neargye/magic_enum/releases/download/v0.9.7/magic_enum-v0.9.7.tar.gz
    )
  FetchContent_MakeAvailable(magic_enum)
endif()

include(FetchContent)
FetchContent_Declare(
  boost_di
  URL https://github.com/boost-ext/di/archive/refs/tags/v1.3.2.tar.gz
)
FetchContent_MakeAvailable(boost_di)

add_subdirectory(common)
add_subdirectory(server)
