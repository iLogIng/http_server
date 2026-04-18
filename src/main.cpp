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

    net::io_context io{static_cast<int>(config.threads())};
    std::make_shared<server_host::listener>(
        io,
        tcp::endpoint{net::ip::make_address(config.address()), config.port()},
        config
    )->run();
    
    std::vector<std::thread> thrds;
    thrds.reserve(config.threads() - 1);
    for (size_t i = 0; i < config.threads() - 1; ++i) {
        thrds.emplace_back([&io] {
            io.run();
        });
    }
    io.run();

    return EXIT_SUCCESS;
}