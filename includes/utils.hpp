#pragma once

#include <boost/beast.hpp>

namespace server_utils
{

namespace beast = boost::beast;
namespace http = beast::http;

beast::string_view
mime_type(beast::string_view path);

std::string
path_cat(beast::string_view base, beast::string_view path);

// 生成 400 Bad Request 响应
http::response<http::string_body> make_bad_request(
    const http::request<http::string_body>& req,
    beast::string_view why);

// 生成 404 Not Found 响应
http::response<http::string_body> make_not_found(
    const http::request<http::string_body>& req,
    beast::string_view target);

// 生成 500 Internal Server Error 响应
http::response<http::string_body> make_server_error(
    const http::request<http::string_body>& req,
    beast::string_view what);

// 生成任意状态码的简单响应
http::response<http::string_body> make_error_response(
    http::status status,
    unsigned int version,
    bool keep_alive,
    beast::string_view body);

}
