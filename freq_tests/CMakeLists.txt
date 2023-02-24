cmake_minimum_required(VERSION 3.23)

include(cmake/GoogleTest.cmake)

add_executable(FreqTests
        src/freq.h
        src/freq.cpp
        src/dummy/freq_dummy.h
        src/dummy/freq_dummy.cpp
        freq_tests/FreqTests.cpp)

add_custom_target(tests
        COMMAND FreqTests
        DEPENDS FreqTests
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/freq_tests
        )

target_link_libraries(FreqTests GTest::gtest_main)

# Link Shlwapi to the project
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
    target_link_libraries(FreqTests Shlwapi)
endif ()
