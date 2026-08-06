#pragma once

#include <optional>
#include <string>
#include <ctime>

#include <boost/beast.hpp>
#include <boost/filesystem.hpp>

#include "logger.hpp"

namespace server_utils
{

namespace beast = boost::beast;
namespace http = beast::http;

// 字节区间
struct byte_range
{
    std::optional<std::size_t> start;   // 空 => 后缀区间
    std::optional<std::size_t> end;     // 空 => 到文件末尾
};

// 解析字节区间，返回实际的起始位置和长度
bool
resolve_range(
    const byte_range& range, std::size_t total_size,
    std::size_t& start, std::size_t& length);

// 解析单个字节区间，返回起始位置和结束位置
bool
parse_single_range(beast::string_view single_range, byte_range& out_range);

// 解析 Range 头，返回字节区间列表
bool
parse_range_header(beast::string_view value, std::vector<byte_range>& out_range);

beast::string_view
mime_type(beast::string_view path);

bool
is_safe_path(beast::string_view path);

// 拼接路径，返回平台支持的路径字符串
std::string
path_cat(beast::string_view base, beast::string_view path);

// 规范化根目录路径
const std::string
get_normalized_doc_root(const std::string& raw_root);

// 安全拼接并规范化路径
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

// 304 Not Modified：条件请求（If-None-Match / If-Modified-Since）命中时返回，无 body
http::response<http::string_body>
make_not_modified(
    const http::request<http::string_body>& req,
    beast::string_view etag,
    beast::string_view last_modified);

// 由修改时间与文件大小生成 ETag（格式 "mtime-size"）
std::string
make_etag(std::time_t mtime, std::size_t size);

// time_t -> HTTP-date 格式
std::string
to_http_date(std::time_t t);

// HTTP-date -> time_t，解析失败返回 false
bool
parse_http_date(beast::string_view s, std::time_t& out);

// 条件请求判定 RFC 7232 : If-None-Match 优先于 If-Modified-Since
bool
is_not_modified(
    const http::request<http::string_body>& req,
    beast::string_view etag,
    std::time_t last_modified_time);

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