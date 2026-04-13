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
    server_config::configuration config(argc, argv); // 加载配置

    LOG_INFO << "Starting HTTP server...";

    auto const address = net::ip::make_address(config.address());
    auto const port = config.port();
    auto const doc_root = std::make_shared<std::string>(config.doc_root());
    auto const threads = std::max<int>(1, static_cast<int>(config.threads()));

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
    {
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    }
    ioc.run();

    return EXIT_SUCCESS;
}