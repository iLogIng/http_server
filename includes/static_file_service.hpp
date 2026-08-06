#pragma once

#include "config.hpp"
#include "router.hpp"
#include "cache.hpp"

#include <boost/beast.hpp>
#include <boost/filesystem.hpp>

#include <shared_mutex>
#include <unordered_map>

namespace server_service
{
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = boost::filesystem;

// 基于一个网站根目录提供服务
class static_file_service
{
    using file_body_type = http::file_body::value_type;
    using lru_cache_type = server_cache::lru_cache<std::string, std::shared_ptr<const std::string>>;

private:
    const server_config::configuration &config_;
    mutable lru_cache_type lru_cache_;
    // 读多写少，使用 shared_mutex允许并发读取
    mutable std::shared_mutex path_mutex_;
    mutable std::unordered_map<std::string, std::string> path_cache_;
    static constexpr std::size_t max_path_cache_entries = 4096;

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

private:
    // 共享字符串体，避免拷贝
    struct shared_string_body
    {
        using value_type = lru_cache_type::value_type;
        class writer
        {
            private:
            value_type const& body_;

            public:
                using const_buffers_type = boost::asio::const_buffer;

                template<bool isRequest, class Fields>
                explicit writer(
                    http::header<isRequest, Fields> const&,
                    value_type const& body
                )   : body_(body) {}

                void init(boost::beast::error_code& ec) const { ec = {}; }

                boost::optional<std::pair<const_buffers_type, bool>>
                get(boost::beast::error_code& ec) const {
                    ec = {};
                    return {
                        {const_buffers_type{body_->data(), body_->size()}, false}
                    };
                }
        };
    };
};

} // namespace server_service
