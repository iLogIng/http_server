#pragma once

#include <boost/asio.hpp>

#include "config.hpp"
#include "utils.hpp"
#include "static_file_service.hpp"

namespace beast = boost::beast;
namespace http = beast::http;

using namespace server_utils;

namespace server_service
{

class request_handler
{
private:
    // 组合了一个静态文件服务对象，用于处理静态文件请求
    server_service::static_file_service static_file_service_;

public:
    explicit request_handler(const server_config::configuration& config);

    const server_config::configuration& config() const;

    // 处理请求的结构，返回一个消息生成器
    http::message_generator handle_request(http::request<http::string_body>& req);


};

} // namespace server_service
