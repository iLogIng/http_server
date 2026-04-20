#pragma once

#include <boost/beast.hpp>
#include <functional>
#include <unordered_map>

namespace server_service
{
namespace http = boost::beast::http;

// 处理程序的包装器
using Handler = std::function<http::message_generator(const http::request<http::string_body>&)>;

class router
{
public:

    // 精确匹配注册
    void add_exact_route(http::verb method, const std::string& path, Handler handler);

    // 前缀匹配注册
    void add_prefix_route(http::verb method, const std::string& prefix, Handler handler);

    // 路由匹配，返回对应的Handler
    Handler match(const http::request<http::string_body>& req) const;

private:
    // 精确路由
    struct exact_route {
        http::verb method;
        std::string path;
        bool operator ==(const exact_route& other) const {
            return this->method == other.method
                && this->path == other.path;
        }
    };
    struct exact_route_hash {
        std::size_t operator ()(const exact_route& key) const {
            auto h1 = std::hash<int>{}(static_cast<int>(key.method));
            auto h2 = std::hash<std::string>{}(key.path);
            return h1 ^ (h2 << 1); // 破坏对称性减少哈希冲突
        }
    };
    std::unordered_map<exact_route, Handler, exact_route_hash> exact_routes_;   // 精确路由

    // 前缀路由
    struct prefix_route {
        http::verb method;
        std::string prefix;
        Handler handler_;
    };
    std::vector<prefix_route> prefix_routes_;       // 路由前缀

};

} // namespace server_service
