#include "../includes/dynamic_api_service.hpp"

#include "../includes/logger.hpp"

#include <boost/beast/http/verb.hpp>

namespace server_service {

namespace {

// Allow 头的固定方法输出序
const http::verb k_allow_order[] = {
    http::verb::get, http::verb::head, http::verb::post,
    http::verb::put, http::verb::delete_, http::verb::patch,
};

} // namespace

dynamic_api_service::
dynamic_api_service(std::string base_path)
    : base_path_(std::move(base_path))
{}

dynamic_api_service&
dynamic_api_service::
add_endpoint(http::verb method, std::string path, Handler handler)
{
    // 注册路径应以 base_path 开头
    if (!base_path_.empty() &&
        path.compare(0, base_path_.size(), base_path_) != 0) {
        LOG_WARNING << "Endpoint path '" << path
                    << "' does not start with base_path '" << base_path_ << "'";
    }
    endpoints_[std::move(path)][method] = std::move(handler);
    return *this;
}

Handler
dynamic_api_service::
as_handler() const
{
    return [this](const http::request<http::string_body>& req) {
        return this->handle_request(req);
    };
}

http::message_generator
dynamic_api_service::
handle_request(
    const http::request<http::string_body>& req) const
{
    // 剥离 query/fragment，端点按纯路径匹配
    auto path = server_utils::target_path(req.target());

    // OPTIONS 全局支持
    if (req.method() == http::verb::options) {
        return handle_options(req);
    }

    auto it = endpoints_.find(std::string(path));
    if (it == endpoints_.end()) {
        return server_utils::make_not_found(req, path);
    }

    auto handler_it = it->second.find(req.method());
    if (handler_it == it->second.end()) {
        // RFC 7231 路径存在但方法未注册 -> 405 + Allow
        return make_method_not_allowed(req, it->second);
    }

    return handler_it->second(req);
}

http::message_generator
dynamic_api_service::
handle_options(
    const http::request<http::string_body>& req) const
{
    static const method_handlers empty_methods;

    auto path = server_utils::target_path(req.target());
    auto it = endpoints_.find(std::string(path));
    const auto& methods = (it != endpoints_.end()) ? it->second : empty_methods;

    http::response<http::empty_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::allow, build_allow_header(methods));
    res.keep_alive(req.keep_alive());
    res.content_length(0);
    return res;
}

http::message_generator
dynamic_api_service::
make_method_not_allowed(
    const http::request<http::string_body>& req,
    const method_handlers& methods) const
{
    http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.set(http::field::allow, build_allow_header(methods));
    res.keep_alive(req.keep_alive());
    res.body() = "Method Not Allowed: '" +
        std::string(http::to_string(req.method())) + "'";
    res.prepare_payload();
    return res;
}

std::string
dynamic_api_service::
build_allow_header(const method_handlers& methods)
{
    std::string allow;
    bool first = true;
    auto append = [&](http::verb v) {
        if (!first) {
            allow += ", ";
        }
        first = false;
        allow += std::string(http::to_string(v));
    };

    // GET 隐含 HEAD
    if (methods.count(http::verb::get)) {
        append(http::verb::get);
        append(http::verb::head);
    }
    for (http::verb v : k_allow_order) {
        if (v == http::verb::get) {
            continue;
        }
        if (methods.count(v)) {
            append(v);
        }
    }
    append(http::verb::options);
    return allow;
}

} // namespace server_service
