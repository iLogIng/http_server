#pragma once

#include <boost/beast.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include "logger.hpp"

namespace server_utils
{

namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = boost::filesystem;

beast::string_view
mime_type(beast::string_view path);

bool
is_safe_path(beast::string_view path);

// 拼接路径，返回平台支持的路径字符串
std::string
path_cat(beast::string_view base, beast::string_view path);

// 提供根目录缓存功能，减少检查次数
// 返回拷贝，暂时不进行多线程优化
const std::string
get_normalized_doc_root(const std::string& raw_root);

// 安全的拼接路径，依赖boost::filesystem库，返回平台支持的路径字符串
// 但是，使用boost::filesystem库进行路径拼接和规范化需要访问主机的文件系统，带来一定的开销，
// 因此，在此之前定义的is_safe_path与path_cat方法仍然必要，以提供一种更轻量级的路径处理方式，
// 适用于大多数常见的请求场景，而secure_file_cat方法则作为一种更安全但可能更慢的选项，适用于需要更严格安全保障的场景。
std::string
secure_file_cath(
    beast::string_view doc_root,
    beast::string_view target
);

// 生成 400 Bad Request 响应
http::response<http::string_body>
make_bad_request(
    const http::request<http::string_body>& req,
    beast::string_view why);

// 生成 404 Not Found 响应
http::response<http::string_body>
make_not_found(
    const http::request<http::string_body>& req,
    beast::string_view target);

// 生成 405 Method Not Allowed 响应
http::response<http::string_body>
make_method_not_allowed(
    const http::request<http::string_body>& req,
    beast::string_view method);

// 生成 413 Payload Too Large 响应
http::response<http::string_body>
make_payload_too_large(
    const http::request<http::string_body>& req,
    std::size_t max_size);

// 生成 500 Internal Server Error 响应
http::response<http::string_body>
make_server_error(
    const http::request<http::string_body>& req,
    beast::string_view what);

// 生成任意状态码的简单响应
http::response<http::string_body>
make_error_response(
    http::status status,
    unsigned version,
    bool keep_alive,
    beast::string_view body);

}