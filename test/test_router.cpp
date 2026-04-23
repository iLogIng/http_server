#include <gtest/gtest.h>
#include <string>
#include "../includes/router.hpp"

using namespace server_service;

// 辅助函数：构造请求
static http::request<http::string_body>
make_req(http::verb method, const std::string& target)
{
    return {method, target, 11};
}

// ============================================================
// 精确路由
// ============================================================

TEST(RouterExactRouteTest, MatchExact)
{
    router r;
    bool called = false;

    r.add_exact_route(http::verb::get, "/api/hello",
        [&](const http::request<http::string_body>&) -> http::message_generator {
            called = true;
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    auto h = r.match(make_req(http::verb::get, "/api/hello"));
    ASSERT_TRUE(h);                     // 匹配成功，handler 非空
    h(make_req(http::verb::get, "/api/hello"));
    EXPECT_TRUE(called);                // 确认 handler 被调用
}

TEST(RouterExactRouteTest, NoMatchWrongMethod)
{
    router r;
    r.add_exact_route(http::verb::get, "/api/hello",
        [](const auto&) -> http::message_generator {
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    // POST 请求不应匹配 GET 路由
    auto h = r.match(make_req(http::verb::post, "/api/hello"));
    EXPECT_FALSE(h);
}

TEST(RouterExactRouteTest, NoMatchWrongPath)
{
    router r;
    r.add_exact_route(http::verb::get, "/api/hello",
        [](const auto&) -> http::message_generator {
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    auto h = r.match(make_req(http::verb::get, "/api/world"));
    EXPECT_FALSE(h);
}

TEST(RouterExactRouteTest, HeadMethodNotSharedWithGet)
{
    router r;
    r.add_exact_route(http::verb::get, "/api/data",
        [](const auto&) -> http::message_generator {
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    // HEAD 请求不应匹配 GET 路由
    auto h = r.match(make_req(http::verb::head, "/api/data"));
    EXPECT_FALSE(h);
}

// ============================================================
// 前缀路由
// ============================================================

TEST(RouterPrefixRouteTest, MatchPrefix)
{
    router r;
    bool called = false;

    r.add_prefix_route(http::verb::get, "/static/",
        [&](const auto&) -> http::message_generator {
            called = true;
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    auto h = r.match(make_req(http::verb::get, "/static/css/style.css"));
    ASSERT_TRUE(h);
    h(make_req(http::verb::get, "/static/css/style.css"));
    EXPECT_TRUE(called);
}

TEST(RouterPrefixRouteTest, NoPrefixMatch)
{
    router r;
    r.add_prefix_route(http::verb::get, "/static/",
        [](const auto&) -> http::message_generator {
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    auto h = r.match(make_req(http::verb::get, "/api/data"));
    EXPECT_FALSE(h);
}

// ============================================================
// 优先级：精确路由优先于前缀路由
// ============================================================

TEST(RouterPriorityTest, ExactOverPrefix)
{
    router r;
    std::string which;

    r.add_prefix_route(http::verb::get, "/",
        [&](const auto&) -> http::message_generator {
            which = "prefix";
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    r.add_exact_route(http::verb::get, "/api/hello",
        [&](const auto&) -> http::message_generator {
            which = "exact";
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    auto h = r.match(make_req(http::verb::get, "/api/hello"));
    ASSERT_TRUE(h);
    h(make_req(http::verb::get, "/api/hello"));
    EXPECT_EQ(which, "exact");
}

// ============================================================
// 最长前缀匹配
// ============================================================

TEST(RouterPrefixRouteTest, LongestPrefixWins)
{
    router r;
    std::string which;

    r.add_prefix_route(http::verb::get, "/",
        [&](const auto&) -> http::message_generator {
            which = "root";
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    r.add_prefix_route(http::verb::get, "/api",
        [&](const auto&) -> http::message_generator {
            which = "api";
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    r.add_prefix_route(http::verb::get, "/api/v2",
        [&](const auto&) -> http::message_generator {
            which = "api-v2";
            http::response<http::string_body> res{http::status::ok, 11};
            return res;
        });

    auto h = r.match(make_req(http::verb::get, "/api/v2/users"));
    ASSERT_TRUE(h);
    h(make_req(http::verb::get, "/api/v2/users"));
    EXPECT_EQ(which, "api-v2");
}

// ============================================================
// 空路由表
// ============================================================

TEST(RouterEmptyTest, NoRoutesReturnsNull)
{
    router r;
    auto h = r.match(make_req(http::verb::get, "/anything"));
    EXPECT_FALSE(h);
}
