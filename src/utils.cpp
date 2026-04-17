#include "../includes/utils.hpp"

#include <unordered_map>

using namespace server_utils;

static std::unordered_map<std::string, std::string>
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
        return mime_types[std::string(ext)];
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
    beast::string_view target)
{
    http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "Resource: '" + std::string(target) + "' was not allowed.";
    LOG_WARNING << "Resource is Not Allowed: '" << target << "'";
    res.prepare_payload();
    return res;
}

// 生成 413 Payload Too Large 响应
http::response<http::string_body>
make_payload_too_large(
    const http::request<http::string_body>& req,
    beast::string_view target)
{
    http::response<http::string_body> res{http::status::payload_too_large, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "Payload Too Large: '" + std::to_string(req.payload_size().value());
    LOG_WARNING << "Payload Too Large: " << req.payload_size();
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
