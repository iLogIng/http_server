#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../includes/static_file_service.hpp"
#include "../includes/config.hpp"
#include "../includes/server.hpp"
#include "../includes/router.hpp"
#include "../includes/graceful_shutdown.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
    server_config::configuration config(argc, argv);
    server_logger::init_logger(config.log_file());

    LOG_INFO << "Starting HTTP server...";

    net::io_context io{static_cast<int>(config.threads())};

    auto static_service = std::make_shared<server_service::static_file_service>(config);
    auto static_handler = static_service->as_handler();
    auto router = std::make_shared<server_service::router>();
    router->add_prefix_route(http::verb::get, "/", static_handler);
    router->add_prefix_route(http::verb::head, "/", static_handler);
    
    // 注册动态API
    router->add_exact_route(http::verb::get, "/api/hello",
        [](const auto& req) -> http::message_generator {
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.body() = R"({"message":"Hello"})";
            return res;
        }
    );

    // 构建默认处理器
    auto default_handler = [](const auto& req) {
        return server_utils::make_not_found(req, req.target());
    };

    auto handler = std::make_shared<server_service::request_handler>(router, default_handler);
    auto endpoint = tcp::endpoint(net::ip::make_address(config.address()), config.port());
    auto listener = std::make_shared<server_host::listener>(io, endpoint, config, handler);

    server_host::graceful_shutdown shutdown_listener(
        io, listener,
        []() {
            return server_host::session::active_sessions();
        },
        std::chrono::seconds(1),
        std::chrono::seconds(30)
    );
    shutdown_listener.start_shutdown_listener([&io] { io.stop(); });

    listener->run();
    
    std::vector<std::thread> thrds;
    thrds.reserve(config.threads() - 1);
    for (size_t i = 1; i < config.threads(); ++i) {
        thrds.emplace_back([&io] {
            io.run();
        });
    }
    io.run();

    for (auto & t : thrds) {
        t.join();
    }

    return EXIT_SUCCESS;
}