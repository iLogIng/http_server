#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>

#include <boost/config.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// 以文件扩展名为基础返回mime类型
beast::string_view
mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == beast::string_view::npos)
            return beast::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))      return     "text/html";
    if(iequals(ext, ".html"))     return     "text/html";
    if(iequals(ext, ".php"))      return     "text/html";
    if(iequals(ext, ".css"))      return     "text/css";
    if(iequals(ext, ".txt"))      return     "text/plain";
    if(iequals(ext, ".js"))       return     "application/javascript";
    if(iequals(ext, ".json"))     return     "application/json";
    if(iequals(ext, ".xml"))      return     "application/xml";
    if(iequals(ext, ".swf"))      return     "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))      return     "video/x-flv";
    if(iequals(ext, ".png"))      return     "image/png";
    if(iequals(ext, ".jpe"))      return     "image/jpeg";
    if(iequals(ext, ".jpeg"))     return     "image/jpeg";
    if(iequals(ext, ".jpg"))      return     "image/jpeg";
    if(iequals(ext, ".gif"))      return     "image/gif";
    if(iequals(ext, ".bmp"))      return     "image/bmp";
    if(iequals(ext, ".ico"))      return     "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff"))     return     "image/tiff";
    if(iequals(ext, ".tif"))      return     "image/tiff";
    if(iequals(ext, ".svg"))      return     "image/svg+xml";
    if(iequals(ext, ".svgz"))     return     "image/svg+xml";
    return "application/text";
}

// 安全的文件路径连接方法，返回该平台支持的路径字符串
std::string
path_cat(
    beast::string_view base,
    beast::string_view path)
{
    if(base.empty())
        return std::string(path);
    std::string result(base);
// MSVC 编译器
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// 针对不同的请求返回响应
//
// 正确的响应信息（依赖于请求信息的状况）
// 类型擦除的消息生成器
template <class Body, class Allocator>
http::message_generator
handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req)
{
    // 返回一个错误请求的响应
    auto const bad_request =
    [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // 返回一个未找到的响应
    auto const not_found =
    [&req](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // 返回一个服务器错误响应
    auto const server_error =
    [&req](beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // 确保我们处理了事务
    if( req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return bad_request("Unknown HTTP-method");

    // 请求路径必须是相对于文件根目录的绝对路径，并且不能包含 ".." 进行文件递归查找
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return bad_request("Illegal request-target");

    // 生成一个指向被请求文件（资源）的路径
    std::string path = path_cat(doc_root, req.target());
    if(req.target().back() == '/')
        path.append("index.html");

    // 尝试打开该文件（资源）
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // 处理文件不存在的情况
    if(ec == beast::errc::no_such_file_or_directory)
        return not_found(req.target());

    // 处理未知错误
    if(ec)
        return server_error(ec.message());

    // 缓存文件大小
    auto const size = body.size();

    // 响应 HEAD 请求
    if(req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

    // 响应 GEI 请求
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())
    };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}

//------------------------------------------------------------------------------

// 错误处理
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

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

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // 检查命令行参数
    if (argc != 5)
    {
        std::cerr <<
            "Usage: http-server-async <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    http-server-async 0.0.0.0 8080 . 1\n";
        return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const doc_root = std::make_shared<std::string>(argv[3]);
    auto const threads = std::max<int>(1, std::atoi(argv[4]));

    // 异步上下文是运行以下所有I/O程序的基础
    net::io_context ioc{threads};

    // 建立并打开一个监听端口
    std::make_shared<listener>(
        ioc,
        tcp::endpoint{address, port},
        doc_root
    )->run();

    // 在被请求的县城上运行I/O服务
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    return EXIT_SUCCESS;
}