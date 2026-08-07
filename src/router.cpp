#include "../includes/router.hpp"

namespace {

// 路径前缀检查，确保 / 分割
bool
path_prefix_match(boost::beast::string_view target, boost::beast::string_view prefix)
{
    if (target.size() < prefix.size() ||
        target.substr(0, prefix.size()) != prefix) {
        return false;
    }
    // 完全相等，或前缀以路径分隔符结尾
    if (target.size() == prefix.size() || prefix.back() == '/') {
        return true;
    }
    // 前缀后必须是路径分隔符，防止 /api 误匹配到 /apiary
    return target[prefix.size()] == '/';
}

} // namespace

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
        if (pre.method == method && path_prefix_match(target, pre.prefix)) {
            if (pre.prefix.length() > best_len) {
                best_len = pre.prefix.length();
                best_match = pre.handler_;
            }
        }
    }

    return best_match;
}

