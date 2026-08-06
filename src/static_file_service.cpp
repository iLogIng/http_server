#include "../includes/static_file_service.hpp"

#include "../includes/logger.hpp"
#include "../includes/utils.hpp"

#include <string>
#include <fstream>

server_service::static_file_service::
static_file_service(const server_config::configuration &config)
    : config_(config)
    , lru_cache_(config)
{}

server_service::Handler
server_service::static_file_service::
as_handler() const {
    return
    [this](const http::request<http::string_body>& req) {
        return this->handle_request(req);
    };
}

server_service::http::message_generator
server_service::static_file_service::
handle_GET_request(
    const http::request<http::string_body>& req,
    const std::string& full_path
) const
{
    // 检查缓存（传引用，免构造临时 key）
    auto cached = lru_cache_.get(full_path);
    // 缓存命中
    if(cached) {
        http::response<shared_string_body>
        res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, server_utils::mime_type(full_path));
        res.content_length(cached.value()->size());
        res.keep_alive(req.keep_alive());
        res.body() = cached.value();
        return res;
    }

    // 未命中 从磁盘读入内存
    std::ifstream file(full_path.data(), std::ios::binary | std::ios::ate);
    if (!file) {
        LOG_WARNING << "No Such File or Directory";
        return server_utils::make_not_found(req, full_path);
    }

    auto size = static_cast<std::size_t>(file.tellg());
    if (size > this->config_.max_body_size()) {
        LOG_WARNING << "Payload Too Large: " << size << " > " << this->config_.max_body_size();
        return server_utils::make_payload_too_large(req, this->config_.max_body_size(), size);
    }

    auto content = std::make_shared<std::string>(size, '\0');
    file.seekg(0);
    file.read(content->data(), size);
    // 存入缓存
    lru_cache_.put(std::string(full_path), content);

    // 返回 string_body 响应
    http::response<shared_string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, server_utils::mime_type(full_path));
    res.content_length(content->size());
    res.keep_alive(req.keep_alive());
    res.body() = content;

    return res;
}

server_service::http::message_generator
server_service::static_file_service::
handle_HEAD_request(
    const http::request<http::string_body>& req,
    const std::string& full_path
) const
{
    // 检查缓存
    auto cached = lru_cache_.get(full_path);
    if (cached) {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, server_utils::mime_type(full_path));
        res.content_length(cached.value()->size());
        res.keep_alive(req.keep_alive());
        return res;
    }

    http::file_body::value_type body;
    beast::error_code ec;

    body.open(full_path.data(), beast::file_mode::scan, ec);
    if (ec) {
        if (ec == beast::errc::no_such_file_or_directory) {
            LOG_WARNING << "No Such File or Directory";
            return server_utils::make_not_found(req, full_path);
        }
        else {
            std::string err_msg = ec.message();
            return server_utils::make_server_error(req, err_msg);
        }
    }

    const std::size_t size = body.size();

    if(size > this->config_.max_body_size()) {
        LOG_WARNING << "Payload Too Large: " << size << " > " << this->config_.max_body_size();
        return server_utils::make_payload_too_large(req, this->config_.max_body_size(), size);
    }

    http::response<http::empty_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, server_utils::mime_type(full_path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());

    return res;
}

server_service::http::message_generator
server_service::static_file_service::handle_request(
    const http::request<http::string_body>& req
) const
{
    if(req.method() != http::verb::get &&
        req.method() != http::verb::head) {
        return server_utils::make_bad_request(req, "Unknown HTTP-method");
    }

    // 将根路径 / 映射为 /index.html，避免 boost::filesystem 将 / 视为绝对根路径
    auto target = req.target();
    if (target == "/") {
        target = "/index.html";
    }

    if(!server_utils::is_safe_path(target)) {
        return server_utils::make_bad_request(req, "Illegal request-target");
    }

    std::string full_path;
    {
        // 优先查路径解析缓存：命中即跳过 weakly_canonical / is_directory 的系统调用
        std::shared_lock lock(path_mutex_);
        auto it = path_cache_.find(target);
        if (it != path_cache_.end()) {
            full_path = it->second;
        }
    }
    if (full_path.empty()) {
        // 缓存未命中：规范化 + 目录解析，仅 首次或缓存淘汰后 执行
        full_path = server_utils::secure_file_cat(this->config_.doc_root(), target);
        if (full_path.empty()) {
            return server_utils::make_bad_request(req, req.target());
        }
        // 目录请求解析为目录下的 index.html（覆盖 /dir/ 与 /dir 两种写法）
        boost::system::error_code ec;
        if (fs::is_directory(full_path, ec)) {
            full_path = (fs::path(full_path) / "index.html").string();
            if (!fs::exists(full_path, ec)) {
                return server_utils::make_not_found(req, target);
            }
        }
        std::unique_lock lock(path_mutex_);
        if (path_cache_.size() >= max_path_cache_entries) {
            path_cache_.clear();
        }
        path_cache_[std::string(target)] = full_path;
    }


    if(req.method() == http::verb::get) {
        return handle_GET_request(req, full_path);
    }

    if(req.method() == http::verb::head) {
        return handle_HEAD_request(req, full_path);
    }

    return server_utils::make_method_not_allowed(req, full_path);
}
