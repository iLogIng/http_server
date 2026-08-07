#include <gtest/gtest.h>

#include <string>
#include <map>
#include <fstream>
#include <iterator>
#include <thread>
#include <chrono>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>

#include <boost/beast.hpp>
#include <boost/asio.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

static constexpr int TEST_PORT = 19876;

class IntegrationTest : public ::testing::Test
{
protected:
    static pid_t server_pid_;

    static std::string server_path()
    {
#ifdef SERVER_BINARY
        return SERVER_BINARY;
#else
        return "../http_server";
#endif
    }

    static std::string doc_root()
    {
#ifdef DOC_ROOT
        return DOC_ROOT;
#else
        return "../app";
#endif
    }

    static void SetUpTestSuite()
    {
        auto bin_path = server_path();
        ASSERT_TRUE(access(bin_path.c_str(), X_OK) == 0)
            << "Server binary not found: " << bin_path;

        server_pid_ = fork();
        if (server_pid_ == 0) {
            std::string port_str = std::to_string(TEST_PORT);
            std::string doc_root_str = doc_root();
            execl(bin_path.c_str(), bin_path.c_str(),
                  "--port", port_str.c_str(),
                  "--doc_root", doc_root_str.c_str(),
                  "--threads", "2",
                  "--timeout_seconds", "10",
                  "--max_cache_entries", "64",
                  "--cache_ttl_seconds", "1",
                  "--log_file", "/tmp/integration_test.log",
                  static_cast<char*>(nullptr));
            _exit(127);
        }
        ASSERT_GT(server_pid_, 0) << "fork() failed";

        bool ready = false;
        for (int i = 0; i < 50; ++i) {
            if (try_connect()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                ready = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        ASSERT_TRUE(ready) << "Server failed to start on port " << TEST_PORT;
    }

    static void TearDownTestSuite()
    {
        if (server_pid_ > 0) {
            kill(server_pid_, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            kill(server_pid_, SIGKILL);
            waitpid(server_pid_, nullptr, 0);
        }
    }

    static bool try_connect()
    {
        try {
            net::io_context io;
            tcp::socket sock(io);
            sock.connect(tcp::endpoint(
                net::ip::make_address("127.0.0.1"), TEST_PORT));
            sock.close();
            return true;
        } catch (...) {
            return false;
        }
    }

    http::response<http::string_body> send_request(
        http::verb method, const std::string& target,
        const std::string& body = "",
        const std::string& content_type = "") const
    {
        net::io_context io;
        tcp::socket sock(io);
        sock.connect(tcp::endpoint(
            net::ip::make_address("127.0.0.1"), TEST_PORT));

        http::request<http::string_body> req{method, target, 10};
        req.set(http::field::host, "localhost");
        if (!body.empty()) {
            req.body() = body;
            req.prepare_payload();
            if (!content_type.empty())
                req.set(http::field::content_type, content_type);
        }

        beast::flat_buffer buffer;
        http::write(sock, req);

        http::response<http::string_body> res;
        if (method == http::verb::head) {
            http::response_parser<http::string_body> parser;
            parser.skip(true);
            http::read(sock, buffer, parser);
            res = parser.release();
        } else {
            http::read(sock, buffer, res);
        }

        beast::error_code ec;
        sock.shutdown(tcp::socket::shutdown_both, ec);
        sock.close();

        return res;
    }

    // 带自定义请求头的请求（用于条件请求测试）
    http::response<http::string_body> send_request_with_headers(
        http::verb method, const std::string& target,
        const std::map<std::string, std::string>& headers) const
    {
        net::io_context io;
        tcp::socket sock(io);
        sock.connect(tcp::endpoint(
            net::ip::make_address("127.0.0.1"), TEST_PORT));

        http::request<http::string_body> req{method, target, 11};
        req.set(http::field::host, "localhost");
        for (const auto& [name, value] : headers) {
            req.set(name, value);
        }
        req.prepare_payload();

        beast::flat_buffer buffer;
        http::write(sock, req);

        http::response<http::string_body> res;
        http::read(sock, buffer, res);

        beast::error_code ec;
        sock.shutdown(tcp::socket::shutdown_both, ec);
        sock.close();

        return res;
    }

    // 读取 doc_root 下文件内容，用于 Range 切片比对
    std::string read_file_content(const std::string& rel) const
    {
        std::ifstream f(doc_root() + "/" + rel, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }
};

pid_t IntegrationTest::server_pid_ = -1;

// 测试

TEST_F(IntegrationTest, GetExistingFileReturns200)
{
    auto res = send_request(http::verb::get, "/index.html");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::content_type], "text/html");
    EXPECT_FALSE(res.body().empty());
}

TEST_F(IntegrationTest, GetNonexistentFileReturns404)
{
    auto res = send_request(http::verb::get, "/nonexistent.html");
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(IntegrationTest, GetDirectoryPathResolvesToIndexHtml)
{
    // 带尾斜杠目录：/docs/ 应解析为 /docs/index.html
    auto res = send_request(http::verb::get, "/docs/");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::content_type], "text/html");
    EXPECT_FALSE(res.body().empty());

    // 无尾斜杠目录：/docs 应同样解析为 index.html，而非读取目录内容
    auto res2 = send_request(http::verb::get, "/docs");
    EXPECT_EQ(res2.result(), http::status::ok);
    EXPECT_EQ(res2.body(), res.body());
}

TEST_F(IntegrationTest, GetDirectoryWithoutIndexHtmlReturns404)
{
    auto res = send_request(http::verb::get, "/nonexistent-dir/");
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(IntegrationTest, GetFileIncludesEtagAndLastModified)
{
    auto res = send_request(http::verb::get, "/index.html");
    ASSERT_EQ(res.result(), http::status::ok);
    EXPECT_FALSE(res[http::field::etag].empty());
    EXPECT_FALSE(res[http::field::last_modified].empty());
}

TEST_F(IntegrationTest, GetWithMatchingEtagReturns304)
{
    // 先取真实 ETag
    auto first = send_request(http::verb::get, "/index.html");
    ASSERT_EQ(first.result(), http::status::ok);
    auto etag = first[http::field::etag];
    ASSERT_FALSE(etag.empty());

    // 条件 GET If-None-Match 命中 -> 304 空 body
    auto res = send_request_with_headers(http::verb::get, "/index.html",
        {{"If-None-Match", std::string(etag)}});
    EXPECT_EQ(res.result(), http::status::not_modified);
    EXPECT_TRUE(res.body().empty());
}

TEST_F(IntegrationTest, GetWithMismatchedEtagReturns200)
{
    // 先 GET 一次填充缓存
    auto first = send_request(http::verb::get, "/index.html");
    ASSERT_EQ(first.result(), http::status::ok);

    auto res = send_request_with_headers(http::verb::get, "/index.html",
        {{"If-None-Match", "\"wrong-etag\""}});
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_FALSE(res.body().empty());
}

TEST_F(IntegrationTest, GetWithWeakEtagReturns304)
{
    // 客户端以 W/ 弱前缀发送 If-None-Match -> 304
    auto first = send_request(http::verb::get, "/index.html");
    ASSERT_EQ(first.result(), http::status::ok);
    auto etag = first[http::field::etag];
    ASSERT_FALSE(etag.empty());

    auto res = send_request_with_headers(http::verb::get, "/index.html",
        {{"If-None-Match", "W/" + std::string(etag)}});
    EXPECT_EQ(res.result(), http::status::not_modified);
    EXPECT_TRUE(res.body().empty());
}

TEST_F(IntegrationTest, GetWithEtagListReturns304)
{
    // 逗号分隔列表，包含匹配项 -> 304
    auto first = send_request(http::verb::get, "/index.html");
    ASSERT_EQ(first.result(), http::status::ok);
    auto etag = first[http::field::etag];
    ASSERT_FALSE(etag.empty());

    auto res = send_request_with_headers(http::verb::get, "/index.html",
        {{"If-None-Match", "\"wrong-etag\", " + std::string(etag)}});
    EXPECT_EQ(res.result(), http::status::not_modified);
}

TEST_F(IntegrationTest, GetWithNewerIfModifiedSinceReturns304)
{
    // 先 GET 一次填充缓存
    auto first = send_request(http::verb::get, "/index.html");
    ASSERT_EQ(first.result(), http::status::ok);

    // 未来时间戳 -> 文件未修改 -> 304
    auto res = send_request_with_headers(http::verb::get, "/index.html",
        {{"If-Modified-Since", "Sun, 06 Nov 2099 08:49:37 GMT"}});
    EXPECT_EQ(res.result(), http::status::not_modified);
}

TEST_F(IntegrationTest, GetWithRangeReturns206)
{
    auto content = read_file_content("index.html");
    auto res = send_request_with_headers(http::verb::get, "/index.html",
        {{"Range", "bytes=0-99"}});
    EXPECT_EQ(res.result(), http::status::partial_content);
    EXPECT_EQ(res[http::field::content_range],
        "bytes 0-99/" + std::to_string(content.size()));
    EXPECT_EQ(res.body().size(), 100u);
    EXPECT_EQ(res.body(), content.substr(0, 100));
}

TEST_F(IntegrationTest, GetWithOpenEndedRangeReturns206)
{
    auto content = read_file_content("index.html");
    auto res = send_request_with_headers(http::verb::get, "/index.html",
        {{"Range", "bytes=9000-"}});
    EXPECT_EQ(res.result(), http::status::partial_content);
    EXPECT_EQ(res.body(), content.substr(9000));
    EXPECT_EQ(res.body().size(), content.size() - 9000);
}

TEST_F(IntegrationTest, GetWithSuffixRangeReturns206)
{
    auto content = read_file_content("index.html");
    auto res = send_request_with_headers(http::verb::get, "/index.html",
        {{"Range", "bytes=-100"}});
    EXPECT_EQ(res.result(), http::status::partial_content);
    EXPECT_EQ(res.body(), content.substr(content.size() - 100));
    EXPECT_EQ(res.body().size(), 100u);
}

TEST_F(IntegrationTest, GetWithUnsatisfiableRangeReturns416)
{
    auto content = read_file_content("index.html");
    auto res = send_request_with_headers(http::verb::get, "/index.html",
        {{"Range", "bytes=99999-"}});
    EXPECT_EQ(res.result(), http::status::range_not_satisfiable);
    EXPECT_EQ(res[http::field::content_range],
        "bytes */" + std::to_string(content.size()));
    EXPECT_TRUE(res.body().empty());
}

TEST_F(IntegrationTest, GetWithInvalidRangeReturns200)
{
    auto res = send_request_with_headers(http::verb::get, "/index.html",
        {{"Range", "bytes=abc"}});
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_FALSE(res.body().empty());
}

TEST_F(IntegrationTest, GetWithMatchingEtagAndRangeReturns304)
{
    // 条件请求优先于 Range: If-None-Match 命中 -> 304，不应 206
    auto first = send_request(http::verb::get, "/index.html");
    ASSERT_EQ(first.result(), http::status::ok);
    auto etag = first[http::field::etag];
    ASSERT_FALSE(etag.empty());

    auto res = send_request_with_headers(http::verb::get, "/index.html",
        {{"If-None-Match", std::string(etag)}, {"Range", "bytes=0-99"}});
    EXPECT_EQ(res.result(), http::status::not_modified);
}

TEST_F(IntegrationTest, GetApiHelloEndpoint)
{
    auto res = send_request(http::verb::get, "/api/hello");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::content_type], "application/json");
    EXPECT_EQ(res.body(), R"({"message":"Hello"})");
}

TEST_F(IntegrationTest, PostFormUrlencodedEchoesJson)
{
    // form-urlencoded -> JSON 回显（键序不定，按内容断言）
    auto res = send_request(http::verb::post, "/api/echo",
        "name=alice&lang=c%2B%2B&note=hello+world",
        "application/x-www-form-urlencoded");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::content_type], "application/json");
    EXPECT_NE(res.body().find("\"name\":\"alice\""), std::string::npos);
    EXPECT_NE(res.body().find("\"lang\":\"c++\""), std::string::npos);
    EXPECT_NE(res.body().find("\"note\":\"hello world\""), std::string::npos);
}

TEST_F(IntegrationTest, PostJsonEchoesRaw)
{
    // JSON 原样回显
    auto payload = R"({"key":"value","n":42})";
    auto res = send_request(http::verb::post, "/api/echo", payload,
        "application/json");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::content_type], "application/json");
    EXPECT_EQ(res.body(), payload);
}

TEST_F(IntegrationTest, PostUnsupportedContentTypeReturns400)
{
    auto res = send_request(http::verb::post, "/api/echo", "hello", "text/plain");
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(IntegrationTest, OptionsRequestReturnsDynamicAllow)
{
    // OPTIONS 是全局方法：Allow 由端点注册表动态计算
    // /api/hello 注册 GET -> GET 隐含 HEAD
    auto res = send_request(http::verb::options, "/api/hello");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::allow], "GET, HEAD, OPTIONS");

    // /api/echo 仅注册 POST
    res = send_request(http::verb::options, "/api/echo");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::allow], "POST, OPTIONS");

    // 未注册路径：仅 OPTIONS
    res = send_request(http::verb::options, "/index.html");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::allow], "OPTIONS");
}

TEST_F(IntegrationTest, GetOnPostOnlyEndpointReturns405WithAllow)
{
    // 路径存在但方法未注册 -> 405 + Allow
    auto res = send_request(http::verb::get, "/api/echo");
    EXPECT_EQ(res.result(), http::status::method_not_allowed);
    EXPECT_EQ(res[http::field::allow], "POST, OPTIONS");
}

TEST_F(IntegrationTest, GetApiHelloWithQueryString)
{
    // query 剥离后应匹配 /api/hello 精确路由
    auto res = send_request(http::verb::get, "/api/hello?name=world&lang=zh");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res.body(), R"({"message":"Hello"})");
}

TEST_F(IntegrationTest, GetFileWithQueryStringReturns200)
{
    // 静态文件请求带 query 不再 404
    auto res = send_request(http::verb::get, "/index.html?v=1");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::content_type], "text/html");
    EXPECT_FALSE(res.body().empty());
}

TEST_F(IntegrationTest, GetDirectoryWithQueryStringResolvesToIndexHtml)
{
    auto res = send_request(http::verb::get, "/docs/?page=2");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::content_type], "text/html");
    EXPECT_FALSE(res.body().empty());
}

TEST_F(IntegrationTest, HeadRequestReturnsNoBody)
{
    auto res = send_request(http::verb::head, "/index.html");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_TRUE(res.body().empty());
}

TEST_F(IntegrationTest, HeadRequestOnCachedFileReturnsNoBody)
{
    auto get_res = send_request(http::verb::get, "/index.html");
    ASSERT_EQ(get_res.result(), http::status::ok);
    ASSERT_FALSE(get_res.body().empty());

    auto head_res = send_request(http::verb::head, "/index.html");
    EXPECT_EQ(head_res.result(), http::status::ok);
    EXPECT_EQ(head_res[http::field::content_length],
              get_res[http::field::content_length]);
    EXPECT_TRUE(head_res.body().empty());
}

TEST_F(IntegrationTest, PostToUnregisteredRouteReturns404)
{
    auto res = send_request(http::verb::post, "/index.html");
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(IntegrationTest, SequentialRequestsAllSucceed)
{
    for (int i = 0; i < 3; ++i) {
        auto res = send_request(http::verb::get, "/index.html");
        EXPECT_EQ(res.result(), http::status::ok);
    }
}

TEST_F(IntegrationTest, CacheInvalidatedAfterTtl)
{
    // 临时文件 避免污染 app/
    const std::string rel = "__cache_ttl_test__.html";
    const std::string path = doc_root() + "/" + rel;
    const std::string target = "/" + rel;
    struct FileGuard { std::string p; ~FileGuard() { std::remove(p.c_str()); } } guard{path};
    std::remove(path.c_str());

    // 写入 v1 -> 命中缓存返回 v1
    {
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        f << "version-one-content-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    }
    auto first = send_request(http::verb::get, target);
    ASSERT_EQ(first.result(), http::status::ok);
    ASSERT_EQ(first.body(),
        "version-one-content-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

    // 等待 TTL 过期 服务器 cache_ttl_seconds=1 重写为 v2
    // 缓冲替换
    std::this_thread::sleep_for(std::chrono::seconds(2));
    {
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        f << "version-two-content-BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
    }
    auto second = send_request(http::verb::get, target);
    EXPECT_EQ(second.result(), http::status::ok);
    EXPECT_EQ(second.body(),
        "version-two-content-BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");

    // 删除临时文件 等 TTL 过期后路径缓存失效 -> 404
    std::remove(path.c_str());
    std::this_thread::sleep_for(std::chrono::seconds(2));
    auto third = send_request(http::verb::get, target);
    EXPECT_EQ(third.result(), http::status::not_found);
}
