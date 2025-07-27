set(CLIENT_LIB "${CMAKE_PROJECT_NAME}_client_lib")

file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)
list(REMOVE_ITEM CLIENT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(${CLIENT_LIB} STATIC ${CLIENT_SOURCES})

target_include_directories(${CLIENT_LIB} PUBLIC 
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        ${boost_di_SOURCE_DIR}/include
)
target_compile_options(${CLIENT_LIB} PUBLIC "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-Wno-error=maybe-uninitialized")

target_link_libraries(${CLIENT_LIB} PUBLIC
        ${COMMON_LIB}
        magic_enum::magic_enum
)

add_executable(client ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(client ${CLIENT_LIB})