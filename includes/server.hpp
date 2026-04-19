#pragma once

#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/chrono.hpp>

#include "logger.hpp"
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
private:
    beast::tcp_stream stream_;                          // TCP 连接流
    beast::flat_buffer buffer_;                         // 平坦的缓存区域
    http::request<http::string_body> req_;              // 客户端请求报文
    server_service::request_handler request_handler_;   // 请求处理器对象

public:
    // 构造函数
    // 获取该套接字接口上，流的所有权
    session(
        tcp::socket&& socket,
        const server_config::configuration& config
    );

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
};

class listener
    : public std::enable_shared_from_this<listener>
{
private:
    net::io_context& ioc_;                              // 异步IO上下文
    tcp::acceptor acceptor_;                            // 接收监听器
    const server_config::configuration& config_;        // 服务器配置对象

public:
    // 初始化对象，设置监听
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        const server_config::configuration& config
    );

    // 开始监听接入连接
    void
    run();

private:
    // 进行监听
    void
    do_accept();

    void
    on_accept(beast::error_code ec, tcp::socket &&socket);
};

}   // namespace server_host


