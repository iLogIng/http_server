#pragma once

#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "logger.hpp"
#include "request_handler.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// 错误处理
void
fail(beast::error_code ec, char const* what);


// 会话
// 处理HTTP服务器的连接
class session
    :   public std::enable_shared_from_this<session>
{
    beast::tcp_stream stream_;                          // TCP 连接流
    beast::flat_buffer buffer_;                         // 平坦的缓存区域
    std::shared_ptr<std::string const> doc_root_;       // 网站根目录
    http::request<http::string_body> req_;              // 客户端请求报文

public:
    // 构造函数
    // 获取该套接字接口上，流的所有权
    session(
        tcp::socket&& socket,
        std::shared_ptr<std::string const> const& doc_root
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

//------------------------------------------------------------------------------

// 允许接入的连接请求并开始会话
class listener
    :   public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;                              // 异步IO上下文
    tcp::acceptor acceptor_;                            // 接收监听器
    std::shared_ptr<std::string const> doc_root_;       // 文件根目录

public:
    // 初始化对象，设置监听
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        std::shared_ptr<std::string const> const& doc_root
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