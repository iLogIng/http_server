#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

#include "../includes/static_file_service.hpp"
#include "../includes/dynamic_api_service.hpp"
#include "../includes/config.hpp"
#include "../includes/server.hpp"
#include "../includes/router.hpp"
#include "../includes/graceful_shutdown.hpp"
#include "../includes/utils.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// GET /api/hello：返回固定 JSON
http::message_generator
handle_hello(const http::request<http::string_body>& req)
{
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    res.body() = R"({"message":"Hello"})";
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return res;
}

// POST /api/echo：按 Content-Type 分流回显（详情见 docs/dynamic_api_service.md）
//   json -> 原样回显；form-urlencoded -> 转 JSON 回显；其他 -> 400
http::message_generator
handle_echo(const http::request<http::string_body>& req)
{
    auto json_escape = [](const std::string& s) -> std::string {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '"')  out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else out += c;
        }
        return out;
    };

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    auto ct = req[http::field::content_type];

    if (beast::iequals(ct, "application/json")) {
        // JSON 原样回显
        res.set(http::field::content_type, "application/json");
        res.body() = req.body().empty() ? "{}" : req.body();
    }
    else if (beast::iequals(ct, "application/x-www-form-urlencoded")) {
        // form-urlencoded -> JSON 回显
        std::unordered_map<std::string, std::string> form;
        server_utils::parse_urlencoded(req.body(), form);
        std::string json = "{";
        bool first = true;
        for (const auto& [k, v] : form) {
            if (!first) json += ",";
            first = false;
            json += "\"" + json_escape(k) + "\":\"" + json_escape(v) + "\"";
        }
        json += "}";
        res.set(http::field::content_type, "application/json");
        res.body() = json;
    }
    else {
        return server_utils::make_bad_request(req,
            "Unsupported Content-Type: expected application/json or application/x-www-form-urlencoded");
    }

    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return res;
}

int main(int argc, char* argv[])
{
    server_config::configuration config(argc, argv);
    server_logger::init_logger(config.log_file(), config.log_level());
    config.dump();

    LOG_INFO << "Starting HTTP server...";

    net::io_context io{static_cast<int>(config.threads())};

    auto static_service = std::make_shared<server_service::static_file_service>(config);
    auto static_handler = static_service->as_handler();
    auto router = std::make_shared<server_service::router>();
    router->add_prefix_route(http::verb::get, "/", static_handler);
    router->add_prefix_route(http::verb::head, "/", static_handler);

    // 动态 API 服务
    // 挂载在 /api 前缀下，端点完全自由注册
    auto dynamic_api = std::make_shared<server_service::dynamic_api_service>("/api");
    dynamic_api->add_endpoint(http::verb::get, "/api/hello", handle_hello)
               .add_endpoint(http::verb::post, "/api/echo", handle_echo);
    auto dynamic_handler = dynamic_api->as_handler();
    router->add_prefix_route(http::verb::get, "/api", dynamic_handler);
    router->add_prefix_route(http::verb::post, "/api", dynamic_handler);

    // OPTIONS 全局支持: Allow 由端点注册表动态计算
    router->add_prefix_route(http::verb::options, "/", dynamic_handler);

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
