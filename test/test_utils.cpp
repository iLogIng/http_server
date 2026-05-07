#include <gtest/gtest.h>
#include <string>
#include "../includes/utils.hpp"

using namespace server_utils;

// ============================================================
// mime_type
// ============================================================

TEST(UtilsMimeTypeTest, KnownExtensions)
{
    EXPECT_EQ(mime_type("index.html"), "text/html");
    EXPECT_EQ(mime_type("style.css"),  "text/css");
    EXPECT_EQ(mime_type("app.js"),     "application/javascript");
    EXPECT_EQ(mime_type("data.json"),  "application/json");
    EXPECT_EQ(mime_type("image.png"),  "image/png");
    EXPECT_EQ(mime_type("photo.jpg"),  "image/jpeg");
    EXPECT_EQ(mime_type("photo.jpeg"), "image/jpeg");
    EXPECT_EQ(mime_type("icon.svg"),   "image/svg+xml");
    EXPECT_EQ(mime_type("icon.ico"),   "image/vnd.microsoft.icon");
}

TEST(UtilsMimeTypeTest, UnknownExtension)
{
    EXPECT_EQ(mime_type("file.xyz"), "application/text");
}

TEST(UtilsMimeTypeTest, NoExtension)
{
    EXPECT_EQ(mime_type("Makefile"), "application/text");
    EXPECT_EQ(mime_type(""),         "application/text");
}

// ============================================================
// is_safe_path
// ============================================================

TEST(UtilsIsSafePathTest, NormalPaths)
{
    EXPECT_TRUE(is_safe_path("/index.html"));
    EXPECT_TRUE(is_safe_path("/"));
    EXPECT_TRUE(is_safe_path("/path/to/file.txt"));
    EXPECT_TRUE(is_safe_path("/a"));
}

TEST(UtilsIsSafePathTest, UnsafePaths)
{
    EXPECT_FALSE(is_safe_path(""));               // 空
    EXPECT_FALSE(is_safe_path("index.html"));      // 无前导 /
    EXPECT_FALSE(is_safe_path("/../index.html"));  // 目录遍历
    EXPECT_FALSE(is_safe_path("/a/../b"));
    EXPECT_FALSE(is_safe_path("/.."));
}

// ============================================================
// path_cat
// ============================================================

TEST(UtilsPathCatTest, Normal)
{
    std::string result = path_cat("/var/www", "/index.html");
    EXPECT_EQ(result, "/var/www/index.html");
}

TEST(UtilsPathCatTest, EmptyBase)
{
    EXPECT_EQ(path_cat("", "/index.html"), "/index.html");
}

TEST(UtilsPathCatTest, TrailingSlash)
{
    EXPECT_EQ(path_cat("/var/www/", "/index.html"), "/var/www/index.html");
}

// path_cat 仅拼接不补分隔符，所以 "index.html" 直接附在 base 末尾
TEST(UtilsPathCatTest, NoLeadingSlashInPath)
{
    EXPECT_EQ(path_cat("/var/www", "index.html"), "/var/wwwindex.html");
}

// ============================================================
// secure_file_cat  — 依赖文件系统，相对 CWD
// 默认从项目根目录运行，此时 app/ 存在，/nonexistent_test_dir 不存在
// ============================================================

TEST(UtilsSecureFileCatTest, UnsafeTargetReturnsEmpty)
{
    EXPECT_EQ(secure_file_cat(".", "/../etc/passwd"), "");
    EXPECT_EQ(secure_file_cat(".", "/.."), "");
    EXPECT_EQ(secure_file_cat(".", ""), "");
}

TEST(UtilsSecureFileCatTest, NonexistentDocRootReturnsEmpty)
{
    // doc_root 目录不存在时返回空
    EXPECT_EQ(secure_file_cat("/nonexistent_test_dir_12345", "/index.html"), "");
}

// ============================================================
// get_normalized_doc_root
// ============================================================

TEST(UtilsNormalizedDocRootTest, NonexistentRootReturnsEmpty)
{
    EXPECT_EQ(get_normalized_doc_root("/nonexistent_test_dir_12345"), "");
}

// ============================================================
// make_* 错误响应函数
// ============================================================

class UtilsMakeResponseTest : public ::testing::Test
{
protected:
    http::request<http::string_body> req;

    void SetUp() override
    {
        req = {http::verb::get, "/test", 11};
        req.keep_alive(false);
    }
};

TEST_F(UtilsMakeResponseTest, BadRequest)
{
    auto res = make_bad_request(req, "invalid input");
    EXPECT_EQ(res.result(), http::status::bad_request);
    EXPECT_EQ(res.result_int(), 400);
    EXPECT_FALSE(res.body().empty());
}

TEST_F(UtilsMakeResponseTest, NotFound)
{
    auto res = make_not_found(req, "/missing.html");
    EXPECT_EQ(res.result(), http::status::not_found);
    EXPECT_EQ(res.result_int(), 404);
    EXPECT_FALSE(res.body().empty());
}

TEST_F(UtilsMakeResponseTest, MethodNotAllowed)
{
    auto res = make_method_not_allowed(req, "POST");
    EXPECT_EQ(res.result(), http::status::method_not_allowed);
    EXPECT_EQ(res.result_int(), 405);
    EXPECT_FALSE(res.body().empty());
}

TEST_F(UtilsMakeResponseTest, PayloadTooLarge)
{
    // 需要设置 Content-Length，否则 payload_size().value() 会抛出异常
    http::request<http::string_body> post_req{http::verb::post, "/upload", 11};
    post_req.keep_alive(false);
    post_req.body() = std::string(100, 'x');
    post_req.prepare_payload();

    auto res = make_payload_too_large(post_req, 50);
    EXPECT_EQ(res.result(), http::status::payload_too_large);
    EXPECT_EQ(res.result_int(), 413);
    EXPECT_FALSE(res.body().empty());
}

TEST_F(UtilsMakeResponseTest, ServerError)
{
    auto res = make_server_error(req, "disk full");
    EXPECT_EQ(res.result(), http::status::internal_server_error);
    EXPECT_EQ(res.result_int(), 500);
    EXPECT_FALSE(res.body().empty());
}

TEST_F(UtilsMakeResponseTest, ServiceUnavalible)
{
    auto res = make_service_unavailable(11, false, "Too Many Connections");
    EXPECT_EQ(res.result(), http::status::service_unavailable);
    EXPECT_EQ(res.result_int(), 503);
    EXPECT_FALSE(res.body().empty());
}

// make_error_response 声明于 utils.hpp，但尚未在 utils.cpp 中实现
// 故暂不测试
