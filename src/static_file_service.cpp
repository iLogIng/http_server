#include "../includes/static_file_service.hpp"

#include "../includes/logger.hpp"
#include "../includes/utils.hpp"

#include <string>
#include <fstream>
#include <ctime>

namespace fs = boost::filesystem;

namespace server_service {
namespace {

// Range 求解结果
struct range_result
{
    // 状态码
    http::status status = http::status::ok;
    // 区间
    std::size_t start = 0;
    std::size_t length = 0;
    // Content-Range 头
    std::string content_range;
};

// 解析 Range 头并求解
// 无头/非法/多区间 -> 200；不可满足 -> 416
range_result
apply_range(const http::request<http::string_body>& req, std::size_t total)
{
    range_result res;
    res.length = total;
    if (req[http::field::range].empty()) {
        return res;
    }
    std::vector<server_utils::byte_range> ranges;
    if (server_utils::parse_range_header(req[http::field::range], ranges) && ranges.size() == 1) {
        if (server_utils::resolve_range(ranges[0], total, res.start, res.length)) {
            res.status = http::status::partial_content;
            res.content_range = "bytes " + std::to_string(res.start) + "-"
                + std::to_string(res.start + res.length - 1) + "/" + std::to_string(total);
        } else {
            res.status = http::status::range_not_satisfiable;
            res.length = 0;
            res.content_range = "bytes */" + std::to_string(total);
        }
    }
    // 非法/多区间 保持 200，length = total，无 Content-Range
    return res;
}

} // namespace
} // namespace server_service

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

// 构建 响应体：304 优先 -> Range（206/416）-> gzip 协商 -> 200
server_service::http::message_generator
server_service::static_file_service::
build_content_response(
    const http::request<http::string_body>& req,
    const std::string& full_path,
    const std::shared_ptr<const std::string>& content,
    const std::shared_ptr<const std::string>& compressed,
    const std::string& etag,
    const std::string& last_modified,
    std::time_t last_modified_time) const
{
    // 304 优先，回 Vary 保持缓存一致性
    if(server_utils::is_not_modified(req, etag, last_modified_time)) {
        auto res = server_utils::make_not_modified(req, etag, last_modified);
        res.set(http::field::vary, "Accept-Encoding");
        return res;
    }

    auto range = apply_range(req, content->size());

    // 206 切片发送（Range 请求不压缩）
    if(range.status == http::status::partial_content) {
        http::response<shared_slice_body> res{http::status::partial_content, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, server_utils::mime_type(full_path));
        res.set(http::field::etag, etag);
        res.set(http::field::last_modified, last_modified);
        res.set(http::field::vary, "Accept-Encoding");
        res.set(http::field::content_range, range.content_range);
        res.content_length(range.length);
        res.keep_alive(req.keep_alive());
        res.body() = shared_slice_body::value_type{content, range.start, range.length};
        return res;
    }

    // 416 Range 不可满足
    if(range.status == http::status::range_not_satisfiable) {
        http::response<http::string_body> res{http::status::range_not_satisfiable, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_range, range.content_range);
        res.keep_alive(req.keep_alive());
        res.content_length(0);
        return res;
    }

    // gzip 协商 -> 压缩 200
    if (compressed &&
        server_utils::should_compress(server_utils::mime_type(full_path))) {
        http::token_list encodings{req[http::field::accept_encoding]};
        if (encodings.exists("gzip")) {
            http::response<shared_string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, server_utils::mime_type(full_path));
            res.set(http::field::etag, etag);
            res.set(http::field::last_modified, last_modified);
            res.set(http::field::content_encoding, "gzip");
            res.set(http::field::vary, "Accept-Encoding");
            res.content_length(compressed->size());
            res.keep_alive(req.keep_alive());
            res.body() = compressed;
            return res;
        }
    }

    // 200 无 Range / 忽略
    http::response<shared_string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, server_utils::mime_type(full_path));
    res.set(http::field::etag, etag);
    res.set(http::field::last_modified, last_modified);
    res.set(http::field::vary, "Accept-Encoding");
    res.content_length(content->size());
    res.keep_alive(req.keep_alive());
    res.body() = content;
    return res;
}

// 构建 响应头
server_service::http::message_generator
server_service::static_file_service::
build_head_response(
    const http::request<http::string_body>& req,
    const std::string& full_path,
    std::size_t total,
    const std::string& etag,
    const std::string& last_modified,
    std::time_t last_modified_time,
    std::size_t compressed_size) const
{
    // 条件请求 -> 304（优先于 Range），回 Vary 保持缓存一致性
    if(server_utils::is_not_modified(req, etag, last_modified_time)) {
        auto res = server_utils::make_not_modified(req, etag, last_modified);
        res.set(http::field::vary, "Accept-Encoding");
        return res;
    }

    auto range = apply_range(req, total);

    http::response<http::empty_body> res{range.status, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, server_utils::mime_type(full_path));
    res.set(http::field::etag, etag);
    res.set(http::field::last_modified, last_modified);
    res.set(http::field::vary, "Accept-Encoding");
    if(!range.content_range.empty()) {
        res.set(http::field::content_range, range.content_range);
    }
    // gzip 协商 -> 长度与 GET 一致
    if (range.status == http::status::ok && compressed_size > 0 &&
        server_utils::should_compress(server_utils::mime_type(full_path))) {
        http::token_list encodings{req[http::field::accept_encoding]};
        if (encodings.exists("gzip")) {
            res.set(http::field::content_encoding, "gzip");
            res.content_length(compressed_size);
            res.keep_alive(req.keep_alive());
            return res;
        }
    }
    res.content_length(range.length);
    res.keep_alive(req.keep_alive());
    return res;
}

// 构建 GET 响应
server_service::http::message_generator
server_service::static_file_service::
handle_GET_request(
    const http::request<http::string_body>& req,
    const std::string& full_path
) const
{
    const auto now = std::chrono::steady_clock::now();

    // 检查缓存
    auto cached = lru_cache_.get(full_path);
    // 缓存命中且未过期
    if(cached && cached->expires_at > now) {
        return build_content_response(req, full_path, cached->content,
            cached->compressed, cached->etag, cached->last_modified, cached->last_modified_time);
    }
    // 命中但已过期，驱逐后重新读盘
    if (cached) {
        lru_cache_.erase(full_path);
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

    // 生成 ETag / Last-Modified，随内容一并缓存
    boost::system::error_code ec;
    const std::time_t mtime_t = fs::last_write_time(full_path, ec);
    const std::string etag = server_utils::make_etag(mtime_t, size);
    const std::string last_modified = server_utils::to_http_date(mtime_t);

    // 文本预压缩，随缓存保存
    std::shared_ptr<const std::string> compressed;
    if (server_utils::should_compress(server_utils::mime_type(full_path))) {
        std::string gz;
        if (server_utils::gzip_compress(*content, gz)) {
            compressed = std::make_shared<const std::string>(std::move(gz));
        }
    }

    // 先响应后入缓存，避免 move 后空指针
    auto resp = build_content_response(req, full_path, content, compressed,
        etag, last_modified, mtime_t);

    lru_cache_.put(full_path,
        cached_file{content, etag, last_modified, mtime_t,
            now + std::chrono::seconds(config_.cache_ttl_seconds()),
            std::move(compressed)});

    return resp;
}

// 构建 HEAD 响应
server_service::http::message_generator
server_service::static_file_service::
handle_HEAD_request(
    const http::request<http::string_body>& req,
    const std::string& full_path
) const
{
    const auto now = std::chrono::steady_clock::now();

    // 检查缓存
    auto cached = lru_cache_.get(full_path);
    if (cached && cached->expires_at > now) {
        return build_head_response(req, full_path, cached->content->size(),
            cached->etag, cached->last_modified, cached->last_modified_time,
            cached->compressed ? cached->compressed->size() : 0);
    }
    // 命中但已过期，驱逐后重新读元数据
    if (cached) {
        lru_cache_.erase(full_path);
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

    // 未命中 读取元数据生成 ETag/Last-Modified
    boost::system::error_code ec2;
    const std::time_t mtime_t = fs::last_write_time(full_path, ec2);
    const std::string etag = server_utils::make_etag(mtime_t, size);
    const std::string last_modified = server_utils::to_http_date(mtime_t);

    // HEAD 未命中不读内容，无压缩版本（压缩长度与 GET 缓存命中后一致）
    return build_head_response(req, full_path, size, etag, last_modified, mtime_t, 0);
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

    // 将根路径 / 映射为 /index.html
    // 先剥离 query/fragment，仅以路径部分进行解析与缓存
    auto target = server_utils::target_path(req.target());
    if (target == "/") {
        target = "/index.html";
    }

    if(!server_utils::is_safe_path(target)) {
        return server_utils::make_bad_request(req, "Illegal request-target");
    }

    std::string full_path;
    {
        // 优先查路径解析缓存 命中且未过期即跳过
        // weakly_canonical / is_directory 的系统调用
        const auto now = std::chrono::steady_clock::now();
        std::shared_lock lock(path_mutex_);
        auto it = path_cache_.find(target);
        if (it != path_cache_.end() && it->second.expires_at > now) {
            full_path = it->second.full_path;
        }
    }
    if (full_path.empty()) {
        // 缓存未命中或已过期
        // 仅 首次、过期或缓存淘汰后 执行
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
        path_cache_[std::string(target)] = path_entry{full_path,
            std::chrono::steady_clock::now() + std::chrono::seconds(config_.cache_ttl_seconds())};
    }


    if(req.method() == http::verb::get) {
        return handle_GET_request(req, full_path);
    }

    if(req.method() == http::verb::head) {
        return handle_HEAD_request(req, full_path);
    }

    return server_utils::make_method_not_allowed(req, full_path);
}
