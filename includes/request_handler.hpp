#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "router.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "static_file_service.hpp"
using namespace server_utils;

namespace server_service
{
namespace beast = boost::beast;
namespace http = beast::http;


class request_handler
{
    using router_ptr = std::shared_ptr<router>;
private:
    // 组合了一个静态文件服务对象，用于处理静态文件请求
    // server_service::static_file_service static_file_service_;
    router_ptr routers_;    // 共享的路由表
    Handler default_handler_;       // 默认的处理器

public:
    explicit request_handler(router_ptr ptr, Handler default_handler);

    // 添加精确路由
    void add_exact_route(http::verb method, const std::string& path, Handler handler) {
        routers_->add_exact_route(method, path, std::move(handler));
    }

    // 添加前缀路由
    void add_prefix_route(http::verb method, const std::string& prefix, Handler handler) {
        routers_->add_prefix_route(method, prefix, std::move(handler));
    }

    // 处理请求的结构，返回一个消息生成器
    http::message_generator handle_request(const http::request<http::string_body>& req);

};

} // namespace server_service
