#pragma once

#include <boost/asio.hpp>

#include "utils.hpp"

namespace beast = boost::beast;
namespace http = beast::http;

using namespace server_utils;

// 针对不同的请求返回响应
//
// 正确的响应信息（依赖于请求信息的状况）
// 类型擦除的消息生成器
template <class Body, class Allocator>
http::message_generator
handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req)
{
    // 请求方法必须是 GET 或 HEAD
    // 否则，返回 400 Bad Request 响应
    if( req.method() != http::verb::get &&
        req.method() != http::verb::head)
    {
        return make_bad_request(req, "Unknown HTTP-method");
    }

    // 保证请求路径合法，防止路径遍历攻击
    if(!is_safe_path(req.target()))
    {
        return make_bad_request(req, "Illegal request-target");
    }

    // 生成一个指向被请求文件（资源）的路径
    std::string path = path_cat(doc_root, req.target());
    if(req.target().back() == '/')
    {
        path.append("index.html");
    }

    // 尝试打开该文件（资源）
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // 404 Not Found
    if(ec == beast::errc::no_such_file_or_directory)
        return make_not_found(req, req.target());

    // 500 Internal Server Error
    if(ec)
        return make_server_error(req, ec.message());

    // 缓存文件大小
    auto const size = body.size();

    // 响应 HEAD 请求
    if(req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

    // 响应 GET 请求
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())
    };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}