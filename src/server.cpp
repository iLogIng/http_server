#include "../includes/server.hpp"

#include "../includes/logger.hpp"

std::atomic<std::size_t> server_host::session::active_sessions_{0};

void
server_host::
fail(boost::beast::error_code ec, const char* what)
{
    LOG_ERROR << what << ": " << ec.message() << "\n";
}

server_host::session::
session(
    server_host::tcp::socket&& socket,
    const server_config::configuration& config,
    request_handler_ptr handler
)
    : stream_(std::move(socket))
    , config_(config)
    , handler_(std::move(handler))
{
    ++active_sessions_;
}

server_host::session::
~session() {
    --active_sessions_;
}

void
server_host::session::
run()
{
    net::dispatch(stream_.get_executor(),
        [self = shared_from_this()] {
            self->do_read();
        }
    );
    // strand 串行执行链，用于保证回调函数不会并发的执行
}

void server_host::session::
do_read()
{
    // emplace 新的分析器（parser 为单次使用设计，官方推荐 stored in optional）
    parser_.emplace();
    parser_->body_limit(config_.max_body_size());

    // 读请求（parser_ 内置 body_limit，超限时 async_read 返回 413）
    http::async_read(stream_, buffer_, *parser_,
        beast::bind_front_handler(
            &session::on_read,
            shared_from_this()
        )
    );
}

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

    auto& req = parser_->get();
    req_info_ = std::string(req.method_string()) + " " + std::string(req.target());
    req_start_time_ = std::chrono::steady_clock::now();

    // 发送响应
    send_response(
        handler_->handle_request(parser_->get())
    );
}

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

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - req_start_time_).count();
    LOG_INFO << req_info_ << " " << elapsed << "ms";

    // 若不为保持连接
    if(! keep_alive)
    {
        // 这意味着我们应该关闭连接，通常因为响应指明了 "Connection: close" 语义
        return do_close();
    }

    // 读另一个请求
    do_read();
}

void
server_host::session::
do_close()
{
    // 发送一个 TCP 关闭消息
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}


// ===============================


server_host::listener::
listener(
    server_host::net::io_context& ioc,
    server_host::tcp::endpoint endpoint,
    const server_config::configuration& config,
    request_handler_ptr handler
)
    : ioc_(ioc)
    , acceptor_(server_host::net::make_strand(ioc))
    , config_(config)
    , handler_(std::move(handler))
    , is_stopped_(false)
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

void
server_host::listener::
run()
{
    do_accept();
}

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
    if (is_stopped_) {
        LOG_INFO << "Listener Stopped, Ignoring accept event.";
        return;
    }
    if(ec)
    {
        if (ec == net::error::operation_aborted) {
            LOG_INFO << "Accept operation aborted due to shutdown.";
            return;
        }
        fail(ec, "accept");
        return;
    }

    // 创建并开始一个新会话
    std::make_shared<session>(
        std::move(socket),
        config_,
        handler_
    )->run();

    // 监听另一个连接
    do_accept();
}

void
server_host::listener::
stop()
{
    is_stopped_ = true;
    beast::error_code ec;
    acceptor_.close(ec);
    if (ec) {
        LOG_ERROR << "Error closing acceptor: " << ec.message();
    }
    else {
        LOG_INFO << "Acceptor closed, no new connections will be accepted.";
    }
}

