#include "../includes/request_handler.hpp"

server_service::request_handler::
request_handler(router_ptr ptr, Handler default_handler)
    : routers_(std::move(ptr))
    , default_handler_(std::move(default_handler))
{}

server_service::http::message_generator
server_service::request_handler::
handle_request(const http::request<http::string_body>& req)
{
    if (auto matched_handler = routers_->match(req))
    {
        return matched_handler(req);
    }
    return default_handler_(req);
}
