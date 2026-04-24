#pragma once

#include <memory>

#include "router.hpp"
#include "config.hpp"
#include "utils.hpp"

namespace server_service
{
using namespace server_utils;

namespace beast = boost::beast;
namespace http = beast::http;


class request_handler
{
    using router_ptr = std::shared_ptr<router>;
private:
    router_ptr routers_;            // 共享路由表
    Handler default_handler_;       // 默认处理器

public:
    explicit request_handler(router_ptr ptr, Handler default_handler);

    void add_exact_route(http::verb method, const std::string& path, Handler handler) {
        routers_->add_exact_route(method, path, std::move(handler));
    }

    void add_prefix_route(http::verb method, const std::string& prefix, Handler handler) {
        routers_->add_prefix_route(method, prefix, std::move(handler));
    }

    http::message_generator handle_request(const http::request<http::string_body>& req);

};

} // namespace server_service
