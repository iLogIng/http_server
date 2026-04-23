#include "../includes/utils.hpp"

#include <string>
#include <thread>
#include <unordered_map>

using namespace server_utils;

static const std::unordered_map<std::string, std::string>
mime_types = {
    {".htm", "text/html"},
    {".html", "text/html"},
    {".php", "text/html"},
    {".css", "text/css"},
    {".txt", "text/plain"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".swf", "application/x-shockwave-flash"},
    {".flv", "video/x-flv"},
    {".png", "image/png"},
    {".jpe", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"},
    {".bmp", "image/bmp"},
    {".ico", "image/vnd.microsoft.icon"},
    {".tiff", "image/tiff"},
    {".tif", "image/tiff"},
    {".svg", "image/svg+xml"},
    {".svgz", "image/svg+xml"}
};

// 以文件扩展名为基础返回mime类型
// 目前是写死的状态，后续可以通过配置文件或环境变量进行调整，以适应不同的部署环境和需求
// 预计通过集成JSON配置文件来实现更灵活的mime类型映射，以便用户可以根据需要添加或修改mime类型，而无需修改代码
beast::string_view
server_utils::
mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == beast::string_view::npos)
        {
            return beast::string_view{};
        }
        return path.substr(pos);
    }();
    if(mime_types.find(std::string(ext)) != mime_types.end())
    {
        return mime_types.find(std::string(ext))->second;
    }
    return "application/text";
}

// 防止路径遍历攻击，确保请求路径安全
bool
server_utils::
is_safe_path(beast::string_view path)
{
#ifdef BOOST_MSVC
    if(path.empty() ||
       path[0] != '/' ||
       path.find("..") != beast::string_view::npos ||
       path.find('\\') != beast::string_view::npos)
    {
        return false;
    }
#else
    if(path.empty() ||
       path[0] != '/' ||
       path.find("..") != beast::string_view::npos)
    {
        return false;
    }
#endif
    return true;
}

// 安全的文件路径连接方法，返回该平台支持的路径字符串
// 发往网站的资源请求，仅可见网站根目录，但是在服务器主机上，网站根目录是存在于某一个路径之下的
// 所以需要进行拼接
// base 一般为网站根目录
// path 一般为资源目标位置，（相对于网站根目录）
std::string
server_utils::
path_cat(
    beast::string_view base,
    beast::string_view path)
{
    // 如果 base(doc_root) 为空，则直接返回 path
    if(base.empty())
    {
        return std::string(path);
    }
    std::string result(base);
// MSVC 编译器
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
    {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());
    for(auto& c : result)
    {
        if(c == '/')
        {
            c = path_separator;
        }
    }
#else
    char constexpr path_separator = '/';
    if(result.back() == path_separator)
    {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());
#endif
    return result;
}

// 提供根目录缓存功能，减少检查次数
const std::string
server_utils::
get_normalized_doc_root(const std::string& raw_root)
{
    static std::mutex cache_mutex;
    static std::string cached_root;
    static std::string cached_raw;

    std::lock_guard<std::mutex> lock(cache_mutex);
    if(cached_raw != raw_root) {
        try {
            fs::path root_path(raw_root);
            if (!fs::exists(root_path)) {
                LOG_ERROR << "Document root does not exist: " << raw_root;
                cached_root.clear();
            }
            else {
                cached_root = fs::canonical(root_path).string();
                if (!cached_root.empty() && cached_root.back() != fs::path::preferred_separator) {
                    cached_root += fs::path::preferred_separator;
                }
            }
        }
        catch (std::exception &e) {
            LOG_ERROR << "Failed to canonicalize document root: " << e.what();
            cached_root.clear();
        }
        cached_raw = raw_root;
    }
    return cached_root;
}

// 安全的拼接路径，依赖boost::filesystem库，返回平台支持的路径字符串
// 但是，使用boost::filesystem库进行路径拼接和规范化需要访问主机的文件系统，带来一定的开销，
// 因此，在此之前定义的is_safe_path与path_cat方法仍然必要，以提供一种更轻量级的路径处理方式，
// 适用于大多数常见的请求场景，而secure_file_cat方法则作为一种更安全但可能更慢的选项，适用于需要更严格安全保障的场景。
std::string
server_utils::
secure_file_cath(
    beast::string_view doc_root,
    beast::string_view target
)
{
    // 轻量化的路径安全性检查
    if (!is_safe_path(target)) {
        LOG_WARNING << "Unsafe target path detected: " << target;
        return "";
    }

    // 获取规范路径缓存
    const std::string normal_root = get_normalized_doc_root(std::string(doc_root));
    if (normal_root.empty()) {
        LOG_ERROR << "Invalid document root: " << doc_root;
        return "";
    }

    // 路径拼接与规范化
    fs::path full_path = fs::path(normal_root) / fs::path(target.begin(), target.end());
    fs::path normalized_full_path;
    try {
        normalized_full_path = fs::weakly_canonical(full_path);
    }
    catch (const fs::filesystem_error& e) {
        LOG_ERROR << "Failed to canonicalize path: " << e.what();
        return "";
    }

    std::string norm_path = normalized_full_path.string();
    if (norm_path.find(normal_root) != 0) {
        LOG_WARNING << "Path traversal attempt: " << target << " -> " << norm_path;
        return "";
    }
    return norm_path;
}


// 生成 400 Bad Request 响应
http::response<http::string_body>
server_utils::
make_bad_request(
    const http::request<http::string_body>& req,
    beast::string_view why)
{
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "Bad Request: '" + std::string(why) + "'";
    LOG_ERROR << "Bad Request: '" << why << "'";
    res.prepare_payload();
    return res;
}

// 生成 404 Not Found 响应
http::response<http::string_body>
server_utils::
make_not_found(
    const http::request<http::string_body>& req,
    beast::string_view target)
{
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "Resource: '" + std::string(target) + "' was not found.";
    LOG_WARNING << "Resource Not Found: '" << target << "'";
    res.prepare_payload();
    return res;
}

// 生成 405 Method Not Allowed
http::response<http::string_body>
server_utils::
make_method_not_allowed(
    const http::request<http::string_body>& req,
    beast::string_view method)
{
    http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "Method: '" + std::string(method) + "' was not allowed.";
    LOG_WARNING << "Method Not Allowed: '" << method << "'";
    res.prepare_payload();
    return res;
}

// 生成 413 Payload Too Large 响应
http::response<http::string_body>
server_utils::
make_payload_too_large(
    const http::request<http::string_body>& req,
    std::size_t max_size)
{
    http::response<http::string_body> res{http::status::payload_too_large, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "Payload Too Large: '" + std::to_string(req.payload_size().value()) + "' exceeds the limit of '" + std::to_string(max_size) + "'";
    LOG_WARNING << "Payload Too Large: " << req.payload_size().value() << " > " << max_size;
    res.prepare_payload();
    return res;
}

// 生成 500 Internal Server Error 响应
http::response<http::string_body>
server_utils::
make_server_error(
    const http::request<http::string_body>& req,
    beast::string_view what)
{
    http::response<http::string_body> res{http::status::internal_server_error, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An Error Occurred: '" + std::string(what) + "'";
    LOG_ERROR << "Internal Server Error: '" << what << "'";
    res.prepare_payload();
    return res;
}
