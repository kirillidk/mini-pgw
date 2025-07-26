set(SERVER_LIB "${CMAKE_PROJECT_NAME}_server_lib")

file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)
list(REMOVE_ITEM SERVER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(${SERVER_LIB} STATIC ${SERVER_SOURCES})

target_include_directories(${SERVER_LIB} PUBLIC 
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        ${boost_di_SOURCE_DIR}/include
)
target_compile_options(${SERVER_LIB} PUBLIC "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-Wno-error=maybe-uninitialized")

target_link_libraries(${SERVER_LIB} PUBLIC
        ${COMMON_LIB}
        magic_enum::magic_enum
        httplib::httplib
)

add_executable(server ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(server ${SERVER_LIB})