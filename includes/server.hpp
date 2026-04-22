#pragma once

#include <memory>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "logger.hpp"
#include "config.hpp"
#include "request_handler.hpp"

// 错误处理
void
fail(beast::error_code ec, char const* what);

// 服务器主机，负责监听和处理连接请求
namespace server_host
{
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

/**
 * 目前session包含了一个request_handler对象，后续可以扩展为一个路由器对象，以支持更多的请求类型
 */
class session
    : public std::enable_shared_from_this<session>
{
    using request_handler_ptr = std::shared_ptr<server_service::request_handler>;
private:
    static std::atomic<std::size_t> active_sessions_;       // 活跃的会话数量
private:
    beast::tcp_stream stream_;                              // TCP 连接流
    beast::flat_buffer buffer_;                             // 平坦的缓存区域
    http::request<http::string_body> req_;                  // 客户端请求报文
    const server_config::configuration& config_;            // 配置文件
    request_handler_ptr handler_;                           // 请求处理器对象的共享指针

public:
    // 构造函数
    // 获取该套接字接口上，流的所有权
    session(
        tcp::socket&& socket,
        const server_config::configuration& config,
        request_handler_ptr handler
    );

    // 析构函数
    ~session();

    // 开始异步操作
    void
    run();

    // 读操作
    void
    do_read();

    // 读操作
    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred);


    void
    send_response(http::message_generator&& msg);

    // 写操作
    void
    on_write(
        bool keep_alive,
        beast::error_code ec,
        std::size_t bytes_transferred);

    // 关闭操作
    void
    do_close();

    // 获取当前活跃的http会话数
    static std::size_t active_sessions() { return active_sessions_; }
};

class listener
    : public std::enable_shared_from_this<listener>
{
    using request_handler_ptr = std::shared_ptr<server_service::request_handler>;
private:
    net::io_context& ioc_;                              // 异步IO上下文
    tcp::acceptor acceptor_;                            // 接收监听器
    const server_config::configuration& config_;        // 配置文件
    request_handler_ptr handler_;                       // 请求处理器对象的指针
    bool is_stopped_;

public:
    // 初始化对象，设置监听
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        const server_config::configuration& config,
        request_handler_ptr handler
    );

    // 开始监听接入连接
    void
    run();

    // 停止监听
    void stop();

    // 检查是否停止监听
    bool is_stopped();

private:
    // 进行监听
    void
    do_accept();

    void
    on_accept(beast::error_code ec, tcp::socket &&socket);
};

}   // namespace server_host


