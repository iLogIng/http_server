#pragma once

#include <optional>

#include <boost/beast.hpp>
#include <boost/filesystem.hpp>

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

// 规范化根目录路径（带缓存）
const std::string
get_normalized_doc_root(const std::string& raw_root);

// 安全拼接并规范化路径（boost::filesystem，双重路径穿越防护）
std::string
secure_file_cat(
    beast::string_view doc_root,
    beast::string_view target
);

http::response<http::string_body>
make_bad_request(
    const http::request<http::string_body>& req,
    beast::string_view why);

http::response<http::string_body>
make_not_found(
    const http::request<http::string_body>& req,
    beast::string_view target);

http::response<http::string_body>
make_method_not_allowed(
    const http::request<http::string_body>& req,
    beast::string_view method);

http::response<http::string_body>
make_payload_too_large(
    const http::request<http::string_body>& req,
    std::size_t max_size,
    std::optional<std::size_t> actual_size = std::nullopt);

http::response<http::string_body>
make_server_error(
    const http::request<http::string_body>& req,
    beast::string_view what);

http::response<http::string_body>
make_service_unavailable(
    unsigned int version,
    bool keep_alive,
    beast::string_view what);

http::response<http::string_body>
make_error_response(
    http::status status,
    unsigned version,
    bool keep_alive,
    beast::string_view body);

}