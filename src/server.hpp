#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "utils.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

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
    )
        :   stream_(std::move(socket)),
            doc_root_(doc_root)
    { }

    // 开始异步操作
    void
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
    void
    do_read()
    {
        // 在读操作之前，先清空请求
        // 否则以下操作就是未定义的
        req_ = {};

        // 设定超时时长
        stream_.expires_after(std::chrono::seconds(30));

        // 读请求
        http::async_read(stream_, buffer_, req_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()
            )
        );
    }

    // 读操作
    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // 意味着他们关闭了连接
        if(ec == http::error::end_of_stream)
            return do_close();

        if(ec)
            return fail(ec, "read");

        // 发送响应
        send_response(
            handle_request(*doc_root_, std::move(req_))
        );
    }

    // 发送响应
    void
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
    do_close()
    {
        // 发送一个 TCP 关闭消息
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

        // 在此处，该连接已经优雅的关闭了
    }
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
    )
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
        , doc_root_(doc_root)
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
    run()
    {
        do_accept();
    }

private:
    // 进行监听
    void
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
                doc_root_
            )->run();
        }

        // 监听另一个连接
        do_accept();
    }
};