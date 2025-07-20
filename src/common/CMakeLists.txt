file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_library(${COMMON_LIB} OBJECT ${COMMON_SOURCES})

target_include_directories(${COMMON_LIB} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

target_link_libraries(${COMMON_LIB}
    PUBLIC nlohmann_json::nlohmann_json
)