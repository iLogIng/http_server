#include "../includes/utils.hpp"

namespace beast = boost::beast;

// 以文件扩展名为基础返回mime类型
// 目前是写死的状态，后续可以通过配置文件或环境变量进行调整，以适应不同的部署环境和需求
// 预计通过集成JSON配置文件来实现更灵活的mime类型映射，以便用户可以根据需要添加或修改mime类型，而无需修改代码
beast::string_view
http_utils::mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == beast::string_view::npos)
            return beast::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))      return     "text/html";
    if(iequals(ext, ".html"))     return     "text/html";
    if(iequals(ext, ".php"))      return     "text/html";
    if(iequals(ext, ".css"))      return     "text/css";
    if(iequals(ext, ".txt"))      return     "text/plain";
    if(iequals(ext, ".js"))       return     "application/javascript";
    if(iequals(ext, ".json"))     return     "application/json";
    if(iequals(ext, ".xml"))      return     "application/xml";
    if(iequals(ext, ".swf"))      return     "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))      return     "video/x-flv";
    if(iequals(ext, ".png"))      return     "image/png";
    if(iequals(ext, ".jpe"))      return     "image/jpeg";
    if(iequals(ext, ".jpeg"))     return     "image/jpeg";
    if(iequals(ext, ".jpg"))      return     "image/jpeg";
    if(iequals(ext, ".gif"))      return     "image/gif";
    if(iequals(ext, ".bmp"))      return     "image/bmp";
    if(iequals(ext, ".ico"))      return     "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff"))     return     "image/tiff";
    if(iequals(ext, ".tif"))      return     "image/tiff";
    if(iequals(ext, ".svg"))      return     "image/svg+xml";
    if(iequals(ext, ".svgz"))     return     "image/svg+xml";
    return "application/text";
}

// 安全的文件路径连接方法，返回该平台支持的路径字符串
std::string
http_utils::path_cat(
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
