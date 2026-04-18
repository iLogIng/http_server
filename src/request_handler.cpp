#include "../includes/request_handler.hpp"

const server_config::configuration&
server_service::request_handler::
config() const
{
    return static_file_service_.config();
}

server_service::http::message_generator
server_service::request_handler::
handle_request(http::request<http::string_body>& req)
{
    return static_file_service_.handle_request(req);
}

server_service::request_handler::
request_handler(const server_config::configuration& config)
    : static_file_service_(config)
{}
