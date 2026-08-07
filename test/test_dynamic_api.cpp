#include <gtest/gtest.h>
#include <string>

#include "../includes/dynamic_api_service.hpp"

using namespace server_service;
namespace beast = boost::beast;

// 辅助函数：构造请求
static http::request<http::string_body>
make_req(http::verb method, const std::string& target)
{
    return {method, target, 11};
}

// message_generator 为 BuffersGenerator 接口（无 result()），
// 序列化为完整 HTTP 报文文本后断言状态行与头字段
static std::string
serialize_gen(http::message_generator gen)
{
    std::string out;
    beast::error_code ec;
    while (!gen.is_done()) {
        auto buffers = gen.prepare(ec);
        if (ec) {
            break;
        }
        for (auto b : buffers) {
            out.append(static_cast<const char*>(b.data()), b.size());
        }
        gen.consume(beast::buffer_bytes(buffers));
    }
    return out;
}

// 返回固定 200 "endpoint" 的处理器
static Handler
ok_handler()
{
    return [](const http::request<http::string_body>&) -> http::message_generator {
        http::response<http::string_body> res{http::status::ok, 11};
        res.body() = "endpoint";
        res.prepare_payload();
        return res;
    };
}

// ============================================================
// 端点注册与匹配
// ============================================================

TEST(DynamicApiServiceTest, ChainedRegistrationReturnsSelf)
{
    dynamic_api_service api{"/api"};
    auto& ref = api.add_endpoint(http::verb::get, "/api/hello", ok_handler());
    EXPECT_EQ(&ref, &api);
}

TEST(DynamicApiServiceTest, MatchGetEndpoint)
{
    dynamic_api_service api{"/api"};
    api.add_endpoint(http::verb::get, "/api/hello", ok_handler());

    auto raw = serialize_gen(api.as_handler()(make_req(http::verb::get, "/api/hello")));
    EXPECT_NE(raw.find("200 OK"), std::string::npos);
}

TEST(DynamicApiServiceTest, MatchPostEndpoint)
{
    dynamic_api_service api{"/api"};
    api.add_endpoint(http::verb::post, "/api/echo", ok_handler());

    auto raw = serialize_gen(api.as_handler()(make_req(http::verb::post, "/api/echo")));
    EXPECT_NE(raw.find("200 OK"), std::string::npos);
}

TEST(DynamicApiServiceTest, UnknownPathReturns404)
{
    dynamic_api_service api{"/api"};
    api.add_endpoint(http::verb::get, "/api/hello", ok_handler());

    auto raw = serialize_gen(api.as_handler()(make_req(http::verb::get, "/api/nonexistent")));
    EXPECT_NE(raw.find("404 Not Found"), std::string::npos);
}

TEST(DynamicApiServiceTest, QueryStringStrippedBeforeMatch)
{
    dynamic_api_service api{"/api"};
    api.add_endpoint(http::verb::get, "/api/hello", ok_handler());

    auto raw = serialize_gen(api.as_handler()(make_req(http::verb::get, "/api/hello?x=1&y=2")));
    EXPECT_NE(raw.find("200 OK"), std::string::npos);
}

// ============================================================
// 405 Method Not Allowed + Allow 头
// ============================================================

TEST(DynamicApiServiceTest, WrongMethodReturns405WithAllow)
{
    dynamic_api_service api{"/api"};
    api.add_endpoint(http::verb::post, "/api/echo", ok_handler());

    auto raw = serialize_gen(api.as_handler()(make_req(http::verb::get, "/api/echo")));
    EXPECT_NE(raw.find("405 Method Not Allowed"), std::string::npos);
    EXPECT_NE(raw.find("Allow: POST, OPTIONS"), std::string::npos);
}

TEST(DynamicApiServiceTest, MultiMethod405ListsAll)
{
    dynamic_api_service api{"/api"};
    api.add_endpoint(http::verb::get, "/api/hello", ok_handler())
       .add_endpoint(http::verb::post, "/api/hello", ok_handler())
       .add_endpoint(http::verb::put, "/api/hello", ok_handler());

    auto raw = serialize_gen(api.as_handler()(make_req(http::verb::delete_, "/api/hello")));
    EXPECT_NE(raw.find("405 Method Not Allowed"), std::string::npos);
    EXPECT_NE(raw.find("Allow: GET, HEAD, POST, PUT, OPTIONS"), std::string::npos);
}

// ============================================================
// OPTIONS 全局支持：动态 Allow
// ============================================================

TEST(DynamicApiServiceTest, OptionsOnGetEndpoint)
{
    dynamic_api_service api{"/api"};
    api.add_endpoint(http::verb::get, "/api/hello", ok_handler());

    auto raw = serialize_gen(api.as_handler()(make_req(http::verb::options, "/api/hello")));
    EXPECT_NE(raw.find("200 OK"), std::string::npos);
    // GET 隐含 HEAD
    EXPECT_NE(raw.find("Allow: GET, HEAD, OPTIONS"), std::string::npos);
}

TEST(DynamicApiServiceTest, OptionsOnPostEndpoint)
{
    dynamic_api_service api{"/api"};
    api.add_endpoint(http::verb::post, "/api/echo", ok_handler());

    auto raw = serialize_gen(api.as_handler()(make_req(http::verb::options, "/api/echo")));
    EXPECT_NE(raw.find("200 OK"), std::string::npos);
    EXPECT_NE(raw.find("Allow: POST, OPTIONS"), std::string::npos);
}

TEST(DynamicApiServiceTest, OptionsOnUnknownPath)
{
    dynamic_api_service api{"/api"};
    api.add_endpoint(http::verb::get, "/api/hello", ok_handler());

    // 未注册路径：仅 OPTIONS，仍 200
    auto raw = serialize_gen(api.as_handler()(make_req(http::verb::options, "/index.html")));
    EXPECT_NE(raw.find("200 OK"), std::string::npos);
    EXPECT_NE(raw.find("Allow: OPTIONS"), std::string::npos);
}

// ============================================================
// base_path 泛化
// ============================================================

TEST(DynamicApiServiceTest, BasePathGeneralized)
{
    // base_path 为任意前缀均可挂载；端点仍以完整路径注册
    dynamic_api_service api{"/v1"};

    bool called = false;
    api.add_endpoint(http::verb::get, "/v1/status",
        [&](const http::request<http::string_body>&) -> http::message_generator {
            called = true;
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    auto raw = serialize_gen(api.as_handler()(make_req(http::verb::get, "/v1/status")));
    EXPECT_NE(raw.find("200 OK"), std::string::npos);
    EXPECT_TRUE(called);

    // base_path 外的路径不受影响
    auto raw2 = serialize_gen(api.as_handler()(make_req(http::verb::get, "/api/status")));
    EXPECT_NE(raw2.find("404 Not Found"), std::string::npos);
}

TEST(DynamicApiServiceTest, RegistrationOutsideBasePathStillWorks)
{
    // 注册路径不以 base_path 开头仅 WARNING，不拒绝注册
    dynamic_api_service api{"/api"};
    api.add_endpoint(http::verb::get, "/hello", ok_handler());

    auto raw = serialize_gen(api.as_handler()(make_req(http::verb::get, "/hello")));
    EXPECT_NE(raw.find("200 OK"), std::string::npos);
}
