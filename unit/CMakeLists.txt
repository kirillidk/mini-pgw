file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(unit_tests ${TEST_SOURCES})

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/server
    ${GTest_INCLUDE_DIRS}
)

target_link_libraries(unit_tests PRIVATE
    ${COMMON_LIB}
    GTest::gtest
    GTest::gtest_main
    # GTest::gmock
    # GTest::gmock_main
)

enable_testing()

include(GoogleTest)
gtest_discover_tests(unit_tests)
