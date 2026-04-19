#include "../includes/server.hpp"

// 错误处理
void
fail(beast::error_code ec, char const* what)
{
    LOG_ERROR << what << ": " << ec.message() << "\n";
}

server_host::session::
session(
    server_host::tcp::socket&& socket,
    request_handler_ptr handler
)
    : stream_(std::move(socket))
    , handler_(std::move(handler))
{}

// 开始异步操作
void
server_host::session::
run()
{
    // 虽然单线程环境并非严格必要，
    // 但是本示例代码默认以线程安全的方式编写
    net::dispatch(stream_.get_executor(),
        [self = shared_from_this()] {
            self->do_read();
        }
    );
    // strand 串行执行链，用于保证回调函数不会并发的执行
}

// 读操作
void server_host::session::
do_read()
{
    // 在读操作之前，先清空请求
    // 否则以下操作就是未定义的
    req_ = {};

    // 设定超时时长
    stream_.expires_after(std::chrono::seconds(handler_->config().timeout_seconds()));

    // 读请求
    http::async_read(stream_, buffer_, req_,
        beast::bind_front_handler(
            &session::on_read,
            shared_from_this()
        )
    );
}

// 读操作
void server_host::session::
on_read(
    beast::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    // 意味着他们关闭了连接
    if(ec == http::error::end_of_stream)
        return do_close();

    if(ec)
    {
        return fail(ec, "read");
    }

    // 发送响应
    send_response(
        handler_->handle_request(req_)
    );
}

// 发送响应
void
server_host::session::
send_response(http::message_generator&& msg)
{
    // 获取持续连接字段值
    bool keep_alive = msg.keep_alive();

    // 写回响应
    beast::async_write(
        stream_,
        std::move(msg),
        beast::bind_front_handler(
            &session::on_write,
            shared_from_this(),
            keep_alive
        )
    );
}

// 写操作
void
server_host::session::
on_write(
    bool keep_alive,
    beast::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec)
        return fail(ec, "write");

    // 若不为保持连接
    if(! keep_alive)
    {
        // 这意味着我们应该关闭连接，通常因为响应指明了 "Connection: close" 语义
        return do_close();
    }

    // 读另一个请求
    do_read();
}

// 关闭操作
void
server_host::session::
do_close()
{
    // 发送一个 TCP 关闭消息
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

    // 在此处，该连接已经优雅的关闭了
}


// ===============================


server_host::listener::
listener(
    server_host::net::io_context& ioc,
    server_host::tcp::endpoint endpoint,
    request_handler_ptr handler
)
    : ioc_(ioc)
    , acceptor_(server_host::net::make_strand(ioc))
    , handler_(std::move(handler))
{
    beast::error_code ec;

    // 打开监听器
    acceptor_.open(endpoint.protocol(), ec);
    if(ec)
    {
        fail(ec, "open");
        return;
    }

    // 允许地址复用
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if(ec)
    {
        fail(ec, "set_option");
        return;
    }

    // 绑定到服务器地址
    acceptor_.bind(endpoint, ec);
    if(ec)
    {
        fail(ec, "bind");
        return;
    }

    // 开始监听该连接
    acceptor_.listen(
        net::socket_base::max_listen_connections, ec);
    if(ec)
    {
        fail(ec, "listen");
        return;
    }
}

// 开始监听接入连接
void
server_host::listener::
run()
{
    do_accept();
}

// 进行监听
void
server_host::listener::
do_accept()
{
    // 新连接获得其自己的strand串行执行链
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            &listener::on_accept,
            shared_from_this()
        )
    );
}

void
server_host::listener::
on_accept(beast::error_code ec, tcp::socket &&socket)
{
    if(ec)
    {
        fail(ec, "accept");
        return; // 避免无止尽的循环
    }
    else
    {
        // 创建并开始一个新会话
        std::make_shared<session>(
            std::move(socket),
            handler_
        )->run();
    }

    // 监听另一个连接
    do_accept();
}
