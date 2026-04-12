#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/config.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// ===========================================

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../includes/config.hpp"
#include "../includes/server.hpp"

int main(int argc, char* argv[])
{
    server_logger::init_logger("./logs/http_server.log"); // 初始化日志系统

    LOG_INFO << "Starting HTTP server...";

    // 检查命令行参数
    if (argc != 5)
    {
        LOG_FATAL_LOC <<
            "Invalid Command Line Arguments\n" <<
            "Usage: http_server <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    http_server 0.0.0.0 8080 . 1\n";
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