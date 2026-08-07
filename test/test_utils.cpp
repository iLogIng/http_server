#include <gtest/gtest.h>
#include <string>
#include <zlib.h>
#include "../includes/utils.hpp"

using namespace server_utils;

// gzip 解压辅助（测试用）：验证 gzip_compress 的往返还原
static std::string
inflate_gzip(const std::string& data){
    z_stream strm{};
    if (inflateInit2(&strm, 15 + 16) != Z_OK) {
        return "";
    }
    std::string out(data.size() * 4, '\0');
    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    strm.avail_in = static_cast<uInt>(data.size());
    strm.next_out = reinterpret_cast<Bytef*>(out.data());
    strm.avail_out = static_cast<uInt>(out.size());
    int ret = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);
    if (ret != Z_STREAM_END) {
        return "";
    }
    out.resize(out.size() - strm.avail_out);
    return out;
}

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
// target_path — query/fragment 剥离
// ============================================================

TEST(UtilsTargetPathTest, NoQueryReturnsAsIs)
{
    EXPECT_EQ(target_path("/index.html"), "/index.html");
    EXPECT_EQ(target_path("/"), "/");
    EXPECT_EQ(target_path("/api/hello"), "/api/hello");
}

TEST(UtilsTargetPathTest, QueryStripped)
{
    EXPECT_EQ(target_path("/index.html?x=1"), "/index.html");
    EXPECT_EQ(target_path("/api/hello?a=1&b=2"), "/api/hello");
    EXPECT_EQ(target_path("/?page=2"), "/");
}

TEST(UtilsTargetPathTest, FragmentStripped)
{
    EXPECT_EQ(target_path("/index.html#section"), "/index.html");
    EXPECT_EQ(target_path("/index.html?x=1#frag"), "/index.html");
}

// ============================================================
// url_decode / parse_urlencoded
// ============================================================

TEST(UtilsUrlDecodeTest, BasicDecoding)
{
    std::string out;
    EXPECT_TRUE(url_decode("hello%20world", out));
    EXPECT_EQ(out, "hello world");
    EXPECT_TRUE(url_decode("c%2B%2B", out));
    EXPECT_EQ(out, "c++");
    EXPECT_TRUE(url_decode("a+b+c", out));
    EXPECT_EQ(out, "a b c");
    EXPECT_TRUE(url_decode("%E4%B8%AD%E6%96%87", out));
    EXPECT_EQ(out, "中文");
    EXPECT_TRUE(url_decode("plain", out));
    EXPECT_EQ(out, "plain");
}

TEST(UtilsUrlDecodeTest, InvalidEncodingReturnsFalse)
{
    std::string out;
    EXPECT_FALSE(url_decode("abc%", out));      // 截断 %
    EXPECT_FALSE(url_decode("abc%2", out));     // 截断
    EXPECT_FALSE(url_decode("%zz", out));       // 非法十六进制
    EXPECT_FALSE(url_decode("%0", out));
}

TEST(UtilsParseUrlencodedTest, MultipleFields)
{
    std::unordered_map<std::string, std::string> form;
    parse_urlencoded("name=alice&age=20&city=beijing", form);
    EXPECT_EQ(form.size(), 3u);
    EXPECT_EQ(form["name"], "alice");
    EXPECT_EQ(form["age"], "20");
    EXPECT_EQ(form["city"], "beijing");
}

TEST(UtilsParseUrlencodedTest, DecodingApplied)
{
    std::unordered_map<std::string, std::string> form;
    parse_urlencoded("lang=c%2B%2B&note=hello+world", form);
    EXPECT_EQ(form["lang"], "c++");
    EXPECT_EQ(form["note"], "hello world");
}

TEST(UtilsParseUrlencodedTest, SegmentWithoutEqualsHasEmptyValue)
{
    std::unordered_map<std::string, std::string> form;
    parse_urlencoded("flag&name=alice", form);
    EXPECT_EQ(form["flag"], "");
    EXPECT_EQ(form["name"], "alice");
}

TEST(UtilsParseUrlencodedTest, EmptyAndTrailingAmp)
{
    std::unordered_map<std::string, std::string> form;
    parse_urlencoded("", form);
    EXPECT_TRUE(form.empty());
    parse_urlencoded("name=alice&", form);
    EXPECT_EQ(form.size(), 1u);
    EXPECT_EQ(form["name"], "alice");
}

// ============================================================
// should_compress / gzip_compress
// ============================================================

TEST(UtilsShouldCompressTest, TextTypesCompressible)
{
    EXPECT_TRUE(should_compress("text/html"));
    EXPECT_TRUE(should_compress("text/css"));
    EXPECT_TRUE(should_compress("text/plain"));
    EXPECT_TRUE(should_compress("application/json"));
    EXPECT_TRUE(should_compress("application/javascript"));
    EXPECT_TRUE(should_compress("application/xml"));
    EXPECT_TRUE(should_compress("image/svg+xml"));
}

TEST(UtilsShouldCompressTest, BinaryTypesNotCompressible)
{
    EXPECT_FALSE(should_compress("image/png"));
    EXPECT_FALSE(should_compress("image/jpeg"));
    EXPECT_FALSE(should_compress("application/octet-stream"));
    EXPECT_FALSE(should_compress("application/x-shockwave-flash"));
    EXPECT_FALSE(should_compress("application/text"));
    EXPECT_FALSE(should_compress(""));
}

TEST(UtilsGzipCompressTest, RoundTripPreservesContent)
{
    const std::string input = "hello gzip world hello gzip world hello gzip world";
    std::string compressed;
    ASSERT_TRUE(gzip_compress(input, compressed));
    EXPECT_LT(compressed.size(), input.size());       // 文本应有压缩收益
    EXPECT_EQ(inflate_gzip(compressed), input);       // 往返还原
}

TEST(UtilsGzipCompressTest, CompressedHasGzipMagic)
{
    std::string compressed;
    ASSERT_TRUE(gzip_compress("data", compressed));
    // gzip 魔数 0x1f 0x8b
    ASSERT_GE(compressed.size(), 2u);
    EXPECT_EQ(static_cast<unsigned char>(compressed[0]), 0x1f);
    EXPECT_EQ(static_cast<unsigned char>(compressed[1]), 0x8b);
}

TEST(UtilsGzipCompressTest, EmptyInputFails)
{
    std::string compressed = "dummy";
    EXPECT_FALSE(gzip_compress("", compressed));
    EXPECT_TRUE(compressed.empty());
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

TEST_F(UtilsMakeResponseTest, ServiceUnavailable)
{
    auto res = make_service_unavailable(11, false, "Too Many Connections");
    EXPECT_EQ(res.result(), http::status::service_unavailable);
    EXPECT_EQ(res.result_int(), 503);
    EXPECT_FALSE(res.body().empty());
}

// ============================================================
// is_not_modified — If-None-Match / If-Modified-Since（RFC 7232）
// ============================================================

TEST(UtilsIsNotModifiedTest, NoConditionalHeadersReturnsFalse)
{
    http::request<http::string_body> req{http::verb::get, "/index.html", 11};
    EXPECT_FALSE(is_not_modified(req, "\"abc-123\"", 1000));
}

TEST(UtilsIsNotModifiedTest, ExactEtagMatch)
{
    http::request<http::string_body> req{http::verb::get, "/index.html", 11};
    req.set(http::field::if_none_match, "\"abc-123\"");
    EXPECT_TRUE(is_not_modified(req, "\"abc-123\"", 1000));
    EXPECT_FALSE(is_not_modified(req, "\"other\"", 1000));
}

TEST(UtilsIsNotModifiedTest, StarMatchesAnything)
{
    http::request<http::string_body> req{http::verb::get, "/index.html", 11};
    req.set(http::field::if_none_match, "*");
    EXPECT_TRUE(is_not_modified(req, "\"abc-123\"", 1000));
}

TEST(UtilsIsNotModifiedTest, ListWithAnyMatch)
{
    http::request<http::string_body> req{http::verb::get, "/index.html", 11};
    req.set(http::field::if_none_match, "\"a\", \"b\", \"abc-123\"");
    EXPECT_TRUE(is_not_modified(req, "\"abc-123\"", 1000));
}

TEST(UtilsIsNotModifiedTest, ListWithoutMatchReturnsFalse)
{
    http::request<http::string_body> req{http::verb::get, "/index.html", 11};
    req.set(http::field::if_none_match, "\"a\", \"b\"");
    EXPECT_FALSE(is_not_modified(req, "\"abc-123\"", 1000));
}

TEST(UtilsIsNotModifiedTest, WeakComparison)
{
    http::request<http::string_body> req{http::verb::get, "/index.html", 11};
    // 客户端弱 ETag 匹配服务器强 ETag
    req.set(http::field::if_none_match, "W/\"abc-123\"");
    EXPECT_TRUE(is_not_modified(req, "\"abc-123\"", 1000));
    // 服务器弱 ETag 匹配客户端强 ETag
    req.set(http::field::if_none_match, "\"abc-123\"");
    EXPECT_TRUE(is_not_modified(req, "W/\"abc-123\"", 1000));
    // 弱前缀 + 列表
    req.set(http::field::if_none_match, "W/\"a\", W/\"abc-123\"");
    EXPECT_TRUE(is_not_modified(req, "\"abc-123\"", 1000));
}

TEST(UtilsIsNotModifiedTest, IfNoneMatchTakesPriorityOverIfModifiedSince)
{
    http::request<http::string_body> req{http::verb::get, "/index.html", 11};
    // IMS 命中（未来时间）但 INM 不匹配 -> 整体不命中
    req.set(http::field::if_none_match, "\"nope\"");
    req.set(http::field::if_modified_since, "Sun, 06 Nov 2099 08:49:37 GMT");
    EXPECT_FALSE(is_not_modified(req, "\"abc-123\"", 1000));
}
