#include <gtest/gtest.h>
#include "../includes/logger.hpp"

TEST(LoggerTestSuit, BasicLogging) {
    server_logger::init_logger("./test/output/basic_logging_test.log");

    LOG_TRACE           << "This is a TRACE log message.";
    LOG_DEBUG           << "This is a DEBUG log message.";
    LOG_INFO            << "This is an INFO log message.";
    LOG_WARNING         << "This is a WARNING log message.";
    LOG_ERROR           << "This is an ERROR log message.";
    LOG_FATAL           << "This is a FATAL log message.";
}

TEST(LoggerTestSuit, LocationLogging) {
    server_logger::init_logger("./test/output/location_logging_test.log");

    LOG_TRACE_LOC       << "This is a TRACE log message with location.";
    LOG_DEBUG_LOC       << "This is a DEBUG log message with location.";
    LOG_INFO_LOC        << "This is an INFO log message with location.";
    LOG_WARNING_LOC     << "This is a WARNING log message with location.";
    LOG_ERROR_LOC       << "This is an ERROR log message with location.";
    LOG_FATAL_LOC       << "This is a FATAL log message with location.";
}
