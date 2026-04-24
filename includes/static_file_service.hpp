#pragma once

#include "config.hpp"
#include "router.hpp"

#include <boost/beast.hpp>
#include <boost/filesystem.hpp>

namespace server_service
{
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = boost::filesystem;

// 基于一个网站根目录提供服务
class static_file_service
{
    using file_body_type = http::file_body::value_type;

private:
    const server_config::configuration &config_;

public:
    explicit static_file_service(const server_config::configuration &config);

    const server_config::configuration& config() const { return config_; }

    http::message_generator handle_request(
        const http::request<http::string_body>& req
    ) const;

    Handler as_handler() const;

private:

    http::message_generator handle_GET_request(
        const http::request<http::string_body>& req,
        beast::string_view full_path
    ) const;

    http::message_generator handle_HEAD_request(
        const http::request<http::string_body>& req,
        beast::string_view full_path
    ) const;

};

} // namespace server_service
