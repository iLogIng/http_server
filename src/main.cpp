#include <boost/beast.hpp>

#include <boost/asio.hpp>

#include <boost/config.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

#include "server.hpp"

#include "utils.hpp"

#include "request_handler.hpp"


//------------------------------------------------------------------------------

// 错误处理
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}


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

    // 在被请求的线程上运行I/O服务
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