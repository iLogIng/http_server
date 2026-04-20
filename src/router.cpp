#include "../includes/router.hpp"

// 注册路由前缀
void
server_service::router::
add_prefix_route(http::verb method, const std::string& prefix, Handler handler)
{
    prefix_routes_.push_back({method, prefix, std::move(handler)});
}

// 添加精确路由
void
server_service::router::
add_exact_route(http::verb method, const std::string& path, Handler handler)
{
    exact_routes_[{method, path}] = std::move(handler);
}

// 匹配路由
server_service::Handler
server_service::router::
match(const http::request<http::string_body>& req) const
{
    auto method = req.method();
    auto target = req.target();

    // 精确匹配
    auto it = exact_routes_.find({method, std::string(target)});
    if (it != exact_routes_.end()) {
        // 返回匹配的路由
        return it->second;
    }

    // 前缀匹配
    Handler best_match = nullptr;
    std::size_t best_len = 0;
    for (const auto& pre : prefix_routes_) {
        if (pre.method == method && target.starts_with(pre.prefix)) {
            if (pre.prefix.length() > best_len) {
                best_len = pre.prefix.length();
                best_match = pre.handler_;
            }
        }
    }

    return best_match;
}

