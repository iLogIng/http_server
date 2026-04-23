#include <gtest/gtest.h>
#include <string>
#include <cstring>
#include "../includes/config.hpp"

using namespace server_config;

// ============================================================
// valid_* 边界值验证函数
// ============================================================

TEST(ConfigValidTest, Address)
{
    std::string a1 = "0.0.0.0";
    std::string a2 = "192.168.1.1";
    std::string a3 = "::1";
    EXPECT_TRUE(valid_address(a1));
    EXPECT_TRUE(valid_address(a2));
    EXPECT_TRUE(valid_address(a3));
}

TEST(ConfigValidTest, EmptyAddressInvalid)
{
    std::string empty;
    EXPECT_FALSE(valid_address(empty));
}

TEST(ConfigValidTest, Port)
{
    EXPECT_TRUE(valid_port(80));
    EXPECT_TRUE(valid_port(8080));
    EXPECT_TRUE(valid_port(1));
    EXPECT_TRUE(valid_port(65535));
}

TEST(ConfigValidTest, PortBoundary)
{
    EXPECT_FALSE(valid_port(0));
    EXPECT_FALSE(valid_port(65536));
}

TEST(ConfigValidTest, DocRoot)
{
    std::string p1 = "/var/www";
    std::string p2 = ".";
    EXPECT_TRUE(valid_doc_root(p1));
    EXPECT_TRUE(valid_doc_root(p2));
}

TEST(ConfigValidTest, EmptyDocRootInvalid)
{
    std::string empty;
    EXPECT_FALSE(valid_doc_root(empty));
}

TEST(ConfigValidTest, Threads)
{
    EXPECT_TRUE(valid_threads(1));
    EXPECT_TRUE(valid_threads(4));
    EXPECT_TRUE(valid_threads(8));
}

TEST(ConfigValidTest, ThreadsBoundary)
{
    EXPECT_FALSE(valid_threads(0));
    // 上限是 hardware_concurrency * 2，不在此处断言具体值，仅验证正数通过
}

TEST(ConfigValidTest, Timeout)
{
    EXPECT_TRUE(valid_timeout_seconds(1));
    EXPECT_TRUE(valid_timeout_seconds(30));
    EXPECT_TRUE(valid_timeout_seconds(3600));
}

TEST(ConfigValidTest, TimeoutBoundary)
{
    EXPECT_FALSE(valid_timeout_seconds(0));
}

TEST(ConfigValidTest, MaxBodySize)
{
    EXPECT_TRUE(valid_max_body_size(1));
    EXPECT_TRUE(valid_max_body_size(10485760));
}

TEST(ConfigValidTest, MaxBodySizeBoundary)
{
    EXPECT_FALSE(valid_max_body_size(0));
}

// ============================================================
// configuration 构造函数
// 注意：构造函数内部会尝试读取 config.json，若不存在则 catch 异常并继续
// ============================================================

class ConfigConstructorTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        // 清除 get_normalized_doc_root 缓存的副作用
        // 下次构造 config 时会重新加载 config.json
    }
};

// 当项目根目录存在 config.json 时，构造函数会先加载它，再叠加命令行参数
// 以下测试验证 JSON 中的值被正确读取（而非纯默认值）
TEST_F(ConfigConstructorTest, ConfigFileLoadedWithDefaults)
{
    const char* argv[] = {"http_server"};
    int argc = 1;

    configuration cfg(argc, const_cast<char**>(argv));

    // 以下值来源于项目根目录的 config.json
    EXPECT_EQ(cfg.port(), 8080);
    EXPECT_EQ(cfg.address(), "0.0.0.0");
    // doc_root 和 max_body_size 会被 config.json 覆盖：
    EXPECT_EQ(cfg.doc_root(), "./app/");
    EXPECT_EQ(cfg.max_body_size(), 10485760u);
}

TEST_F(ConfigConstructorTest, CommandLineOverridesDefault)
{
    const char* argv[] = {
        "http_server",
        "--port", "9090",
        "--threads", "4",
        "--address", "127.0.0.1",
        "--timeout_seconds", "60",
        "--max_body_size", "2097152",
        "--doc_root", "/tmp"
    };
    int argc = 13;

    configuration cfg(argc, const_cast<char**>(argv));

    EXPECT_EQ(cfg.address(), "127.0.0.1");
    EXPECT_EQ(cfg.port(), 9090);
    EXPECT_EQ(cfg.threads(), 4);
    EXPECT_EQ(cfg.timeout_seconds(), 60);
    EXPECT_EQ(cfg.max_body_size(), 2097152u);
    EXPECT_EQ(cfg.doc_root(), "/tmp");
}

TEST_F(ConfigConstructorTest, PartialCommandLineOverride)
{
    const char* argv[] = {
        "http_server",
        "--port", "9090"
    };
    int argc = 3;

    configuration cfg(argc, const_cast<char**>(argv));

    // port 被覆盖，其余保持默认
    EXPECT_EQ(cfg.port(), 9090);
    EXPECT_EQ(cfg.address(), "0.0.0.0");
    EXPECT_EQ(cfg.threads(), 1);
}

// 注：JSON 配置文件测试需要在 CWD 存在 config.json 时才能验证覆盖逻辑。
// 由于测试环境不确定，此处仅测试命令行层和默认值层。
