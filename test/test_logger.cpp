#include <gtest/gtest.h>
#include <sstream>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "../includes/logger.hpp"

class LoggerTest : public ::testing::Test
{
protected:
    using text_sink = boost::log::sinks::synchronous_sink<
        boost::log::sinks::text_ostream_backend>;
    boost::shared_ptr<text_sink> sink_;
    boost::shared_ptr<std::ostringstream> stream_;

    void SetUp() override
    {
        stream_ = boost::make_shared<std::ostringstream>();
        sink_ = boost::make_shared<text_sink>();
        sink_->locked_backend()->add_stream(stream_);
        sink_->set_formatter(
            boost::log::expressions::format("[%1%] %2%")
                % boost::log::trivial::severity
                % boost::log::expressions::smessage
        );
        boost::log::core::get()->add_sink(sink_);
        boost::log::add_common_attributes();
    }

    void TearDown() override
    {
        boost::log::core::get()->remove_sink(sink_);
    }
};

TEST_F(LoggerTest, TraceLogging)
{
    LOG_TRACE << "trace message";
    auto output = stream_->str();
    EXPECT_NE(output.find("[trace]"), std::string::npos);
    EXPECT_NE(output.find("trace message"), std::string::npos);
}

TEST_F(LoggerTest, DebugLogging)
{
    LOG_DEBUG << "debug message";
    auto output = stream_->str();
    EXPECT_NE(output.find("[debug]"), std::string::npos);
    EXPECT_NE(output.find("debug message"), std::string::npos);
}

TEST_F(LoggerTest, InfoLogging)
{
    LOG_INFO << "info message";
    auto output = stream_->str();
    EXPECT_NE(output.find("[info]"), std::string::npos);
    EXPECT_NE(output.find("info message"), std::string::npos);
}

TEST_F(LoggerTest, WarningLogging)
{
    LOG_WARNING << "warning message";
    auto output = stream_->str();
    EXPECT_NE(output.find("[warning]"), std::string::npos);
    EXPECT_NE(output.find("warning message"), std::string::npos);
}

TEST_F(LoggerTest, ErrorLogging)
{
    LOG_ERROR << "error message";
    auto output = stream_->str();
    EXPECT_NE(output.find("[error]"), std::string::npos);
    EXPECT_NE(output.find("error message"), std::string::npos);
}

TEST_F(LoggerTest, FatalLogging)
{
    LOG_FATAL << "fatal message";
    auto output = stream_->str();
    EXPECT_NE(output.find("[fatal]"), std::string::npos);
    EXPECT_NE(output.find("fatal message"), std::string::npos);
}

TEST_F(LoggerTest, LocationMacrosIncludeFileInfo)
{
    LOG_INFO_LOC << "location test";
    auto output = stream_->str();
    EXPECT_NE(output.find("location test"), std::string::npos);
    EXPECT_NE(output.find("test_logger"), std::string::npos);
}
