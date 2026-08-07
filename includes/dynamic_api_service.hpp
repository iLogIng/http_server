#pragma once

#include <boost/beast.hpp>
#include <functional>
#include <string>
#include <unordered_map>

#include "router.hpp"
#include "utils.hpp"

namespace server_service
{
namespace http = boost::beast::http;

// 动态 API 服务
// 端点注册表（method + path 自由注册），OPTIONS 全局动态 Allow
class dynamic_api_service
{
    // path -> (method -> handler)
    using method_handlers = std::unordered_map<http::verb, Handler>;

public:
    // base_path 为拦截前缀（如 "/api"），仅用于注册路径校验与文档用途
    explicit dynamic_api_service(std::string base_path);

    // 注册端点（完整路径）支持链式注册
    dynamic_api_service& add_endpoint(http::verb method, std::string path, Handler handler);

    // 转为路由 Handler
    // 供 router request_handler 挂载
    Handler as_handler() const;

private:
    std::string base_path_;
    std::unordered_map<std::string, method_handlers> endpoints_;

    // 请求分发 OPTIONS -> 动态 Allow
    http::message_generator handle_request(
        const http::request<http::string_body>& req) const;

    // OPTIONS 任意路径返回 200 + Allow（未注册路径仅 OPTIONS）
    http::message_generator handle_options(
        const http::request<http::string_body>& req) const;

    // 405 Method Not Allowed：Allow 列出该路径全部已注册方法
    http::message_generator make_method_not_allowed(
        const http::request<http::string_body>& req,
        const method_handlers& methods) const;

    // 组装 Allow 头部
    static std::string build_allow_header(const method_handlers& methods);
};

} // namespace server_service
