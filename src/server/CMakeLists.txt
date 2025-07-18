set(OBJ_LIB "${CMAKE_PROJECT_NAME}_lib")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(${OBJ_LIB} OBJECT ${SOURCES})

target_include_directories(${OBJ_LIB} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_compile_options(${OBJ_LIB} PUBLIC "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-Wno-error=maybe-uninitialized")

add_executable(server ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(server ${OBJ_LIB})