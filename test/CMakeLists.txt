include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.17.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 添加测试文件
set(TEST_SOURCES
    logger.test.cpp
)

add_executable(server_tests ${TEST_SOURCES})

target_include_directories(server_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_directories(server_tests
PRIVATE
    GTest::gtest
    GTest::gtest_main
    Boost::headers
    Boost::filesystem
    Boost::system
    Boost::thread
    Boost::log
    Boost::log_setup
    Boost::json
)

include(GoogleTest)
gtest_discover_tests(server_tests)
