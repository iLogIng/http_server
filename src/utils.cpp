#include "../includes/utils.hpp"

#include <unordered_map>

namespace beast = boost::beast;

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
server_utils::mime_type(beast::string_view path)
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
    else
    {
        return "application/text";
    }
}

// 安全的文件路径连接方法，返回该平台支持的路径字符串
std::string
server_utils::path_cat(
    beast::string_view base,
    beast::string_view path)
{
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
