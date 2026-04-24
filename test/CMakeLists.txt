find_package(GTest CONFIG REQUIRED)

# ---- logger test ----
add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger
PRIVATE
    GTest::gtest
    GTest::gtest_main
    Boost::log
    Boost::log_setup
    Boost::thread
    Boost::filesystem
    Boost::system)
target_include_directories(test_logger PRIVATE ${PROJECT_SOURCE_DIR}/includes
    ${Boost_INCLUDE_DIRS})
include(GoogleTest)
gtest_discover_tests(test_logger)

# ---- utils test ----
add_executable(test_utils test_utils.cpp ${PROJECT_SOURCE_DIR}/src/utils.cpp)
target_link_libraries(test_utils
PRIVATE
    GTest::gtest
    GTest::gtest_main
    Boost::filesystem
    Boost::system
    Boost::thread
    Boost::log
    Boost::log_setup)
target_include_directories(test_utils PRIVATE ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/includes ${Boost_INCLUDE_DIRS})
gtest_discover_tests(test_utils)

# ---- router test ----
add_executable(test_router test_router.cpp ${PROJECT_SOURCE_DIR}/src/router.cpp)
target_link_libraries(test_router
PRIVATE
    GTest::gtest GTest::gtest_main
    Boost::filesystem
    Boost::system
    Boost::thread
    Boost::log
    Boost::log_setup)
target_include_directories(test_router PRIVATE ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/includes ${Boost_INCLUDE_DIRS})
gtest_discover_tests(test_router)

# ---- config test ----
add_executable(test_config test_config.cpp ${PROJECT_SOURCE_DIR}/src/config.cpp)
target_link_libraries(test_config
PRIVATE
    GTest::gtest GTest::gtest_main
    Boost::filesystem
    Boost::system
    Boost::thread
    Boost::log
    Boost::log_setup
    Boost::program_options
    Boost::json)
target_include_directories(test_config PRIVATE ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/includes ${Boost_INCLUDE_DIRS})
gtest_discover_tests(test_config)

# ---- integration test (end-to-end) ----
add_executable(test_integration test_integration.cpp)
target_compile_definitions(test_integration PRIVATE
    SERVER_BINARY="${CMAKE_BINARY_DIR}/src/http_server"
    DOC_ROOT="${PROJECT_SOURCE_DIR}/app")
target_link_libraries(test_integration
PRIVATE
    GTest::gtest GTest::gtest_main
    Boost::system
    Boost::thread)
target_include_directories(test_integration PRIVATE
    ${Boost_INCLUDE_DIRS})
gtest_discover_tests(test_integration)

# ---- custom target: build all tests ----
add_custom_target(all-test
    DEPENDS test_logger test_utils test_router test_config test_integration
)

# ---- custom target: build and run all tests ----
add_custom_target(all-test-run
    DEPENDS test_logger test_utils test_router test_config test_integration
    COMMAND $<TARGET_FILE:test_logger>
    COMMAND $<TARGET_FILE:test_utils>
    COMMAND $<TARGET_FILE:test_router>
    COMMAND $<TARGET_FILE:test_config>
    COMMAND $<TARGET_FILE:test_integration>
    COMMENT "Running all tests..."
)
