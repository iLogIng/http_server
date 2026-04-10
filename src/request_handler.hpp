#pragma once

#include <boost/asio.hpp>

#include "utils.hpp"

namespace beast = boost::beast;
namespace http = beast::http;

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
    // 返回一个错误请求的响应
    auto const bad_request =
    [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // 返回一个未找到的响应
    auto const not_found =
    [&req](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // 返回一个服务器错误响应
    auto const server_error =
    [&req](beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // 确保我们处理了事务
    if( req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return bad_request("Unknown HTTP-method");

    // 请求路径必须是相对于文件根目录的绝对路径，并且不能包含 ".." 进行文件递归查找
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return bad_request("Illegal request-target");

    // 生成一个指向被请求文件（资源）的路径
    std::string path = path_cat(doc_root, req.target());
    if(req.target().back() == '/')
        path.append("index.html");

    // 尝试打开该文件（资源）
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // 处理文件不存在的情况
    if(ec == beast::errc::no_such_file_or_directory)
        return not_found(req.target());

    // 处理未知错误
    if(ec)
        return server_error(ec.message());

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

    // 响应 GEI 请求
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