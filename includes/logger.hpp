#pragma once

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/filesystem.hpp>

// 将boost::filesystem的依赖替换为标准库的std::filesystem
// #include <filesystem>

namespace server_logger
{
    namespace logging = boost::log;
    namespace trivial = boost::log::trivial;
    namespace sinks = boost::log::sinks;
    namespace sources = boost::log::sources;
    namespace expr = boost::log::expressions;
    namespace attrs = boost::log::attributes;
    namespace keywords = boost::log::keywords;

    // TimeStamp: 记录日志事件的时间戳
    BOOST_LOG_ATTRIBUTE_KEYWORD(TimeStamp, "TimeStamp", boost::posix_time::ptime)
    // ThreadID: 记录日志事件发生的线程ID
    BOOST_LOG_ATTRIBUTE_KEYWORD(ThreadID, "ThreadID", attrs::current_thread_id::value_type)
    // Severity: 记录日志事件的严重级别（trace, debug, info, warning, error, fatal）
    BOOST_LOG_ATTRIBUTE_KEYWORD(Severity, "Severity", trivial::severity_level)

/**
 * 获取全局日志记录器实例，使用 Boost.Log 的 severity_logger 模板类，支持不同的日志级别。
 * 在首次调用时初始化
 */
inline logging::sources::severity_logger<trivial::severity_level>&
get_logger() {
    using namespace server_logger;
    static logging::sources::severity_logger<trivial::severity_level> logger;
    return logger;
}

/**
 * 初始化日志系统，设置控制台和文件输出，以及日志格式和轮转策略。
 * 应该在程序入口处调用一次，以确保日志系统正确配置。
 * @param log_file日志文件路径，默认为 "./logs/http_server.log"
 */
inline void
init_logger(const std::string& log_file = "./logs/http_server.log") {
    using namespace server_logger;
    // 确保日志目录存在
    boost::filesystem::path log_path(log_file);
    boost::filesystem::path log_dir = log_path.parent_path();
    if (!log_dir.empty() && !boost::filesystem::exists(log_dir))
    {
        boost::filesystem::create_directories(log_dir);
    }

    // 添加常用属性：时间戳、线程ID
    logging::add_common_attributes();

    // 1. 控制台 sink（彩色可选，这里简化）
    using text_sink = sinks::asynchronous_sink<sinks::text_ostream_backend>;
    auto console_sink = boost::make_shared<text_sink>();
    console_sink->locked_backend()->add_stream(
        boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter())
    );
    // 设置格式：时间戳、线程ID、日志级别、消息内容
    console_sink->set_formatter(
        expr::format("[%1%] [%2%] [%3%] %4%")
            % expr::format_date_time(TimeStamp, "%Y-%m-%d %H:%M:%S.%f")
            % ThreadID
            % Severity
            % expr::smessage
    );
    logging::core::get()->add_sink(console_sink);

    // 2. 文件 sink（自动轮转：单文件最大 100MB，保留 5 个）
    // 文件大小按轮转自动切分
    // 后期添加配置文件对该部分进行灵活配置
    // 以减小硬编码的影响，并允许用户根据需要调整日志文件大小和数量
    using file_sink = sinks::asynchronous_sink<sinks::text_file_backend>;
    auto file_asink_ptr = boost::make_shared<file_sink>(
        keywords::file_name = log_file,
        keywords::rotation_size = static_cast<unsigned long long>(100 * 1024 * 1024),        // 100 MB
        keywords::max_size = static_cast<unsigned long long>(500 * 1024 * 1024),             // 500 MB
        keywords::min_free_space = static_cast<unsigned long long>(50 * 1024 * 1024)         // 50  MB
    );
    file_asink_ptr->set_formatter(
        expr::format("[%1%] [%2%] [%3%] %4%")
            % expr::format_date_time(TimeStamp, "%Y-%m-%d %H:%M:%S.%f")
            % ThreadID
            % Severity
            % expr::smessage
    );
    logging::core::get()->add_sink(file_asink_ptr);

    // 3. 设置全局最低级别（例如 DEBUG，可改为 INFO 以忽略 DEBUG）
    // 当前是硬编码为 DEBUG，后续可以通过配置文件或环境变量进行调整，以适应不同的运行环境和需求
    logging::core::get()->set_filter(
        trivial::severity >= trivial::debug
    );
}

// TRACE 级别日志，适用于非常详细的调试信息，通常在开发阶段使用，记录函数入口/出口、变量值等细节
#define LOG_TRACE   BOOST_LOG_SEV(server_logger::get_logger(), boost::log::trivial::trace)
// DEBUG 级别日志，适用于开发调试阶段，记录详细的内部状态和流程信息
#define LOG_DEBUG   BOOST_LOG_SEV(server_logger::get_logger(), boost::log::trivial::debug)
// INFO 级别日志，适用于正常运行时的重要事件，如服务器启动、请求处理完成等
#define LOG_INFO    BOOST_LOG_SEV(server_logger::get_logger(), boost::log::trivial::info)
// WARNING 级别日志，适用于潜在问题或非预期事件，如请求参数异常、资源访问失败等
#define LOG_WARNING BOOST_LOG_SEV(server_logger::get_logger(), boost::log::trivial::warning)
// ERROR 级别日志，适用于错误事件，如请求处理失败、服务器异常等
#define LOG_ERROR   BOOST_LOG_SEV(server_logger::get_logger(), boost::log::trivial::error)
// FATAL 级别日志，适用于严重错误事件，通常会导致程序终止，如配置错误、资源耗尽等
#define LOG_FATAL   BOOST_LOG_SEV(server_logger::get_logger(), boost::log::trivial::fatal)

// 记录文件名/行号的 各个 级别日志，适用于需要追踪代码位置的日志输出
#define LOG_TRACE_LOC LOG_TRACE     << __FILE__ << ":" << __LINE__ << " - "
#define LOG_DEBUG_LOC LOG_DEBUG     << __FILE__ << ":" << __LINE__ << " - "
#define LOG_INFO_LOC LOG_INFO       << __FILE__ << ":" << __LINE__ << " - "
#define LOG_ERROR_LOC LOG_ERROR     << __FILE__ << ":" << __LINE__ << " - "
#define LOG_WARNING_LOC LOG_WARNING << __FILE__ << ":" << __LINE__ << " - "
#define LOG_FATAL_LOC LOG_FATAL     << __FILE__ << ":" << __LINE__ << " - "

} // namespace server_logger
