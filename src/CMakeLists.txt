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

add_subdirectory(common)
add_subdirectory(server)
