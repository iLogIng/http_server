#include <gtest/gtest.h>
#include <string>
#include <cstring>
#include <fstream>
#include <cstdio>
#include <unistd.h>

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
// 命令行参数测试，无 JSON 依赖
// ============================================================

TEST(ConfigCommandLineTest, FullCommandLine)
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

TEST(ConfigCommandLineTest, PartialCommandLine)
{
    const char* argv[] = {
        "http_server",
        "--port", "9090"
    };
    int argc = 3;

    configuration cfg(argc, const_cast<char**>(argv));

    EXPECT_EQ(cfg.port(), 9090);
    EXPECT_EQ(cfg.address(), "0.0.0.0");
    EXPECT_EQ(cfg.threads(), 1);
}

// ============================================================
// TempConfigTest — 通过 --config 加载 JSON 配置文件
// ============================================================

class TempConfigTest : public ::testing::Test
{
protected:
    std::string temp_config_;

    void SetUp() override
    {
        char tmp[] = "/tmp/test_config_XXXXXX";
        int fd = mkstemp(tmp);
        if (fd != -1) {
            close(fd);
            temp_config_ = tmp;
        }
    }

    void TearDown() override
    {
        if (!temp_config_.empty()) {
            std::remove(temp_config_.c_str());
        }
    }

    std::string write_config(const std::string& content)
    {
        std::ofstream ofs(temp_config_);
        ofs << content;
        return temp_config_;
    }
};

TEST_F(TempConfigTest, JsonConfigLoaded)
{
    auto cfg_path = write_config(R"({
        "address": "192.168.1.1",
        "port": 3000,
        "doc_root": "/tmp",
        "threads": 2,
        "timeout_seconds": 60,
        "max_body_size": 2097152
    })");

    const char* argv[] = {"http_server", "--config", cfg_path.c_str()};
    int argc = 3;
    configuration cfg(argc, const_cast<char**>(argv));

    EXPECT_EQ(cfg.address(), "192.168.1.1");
    EXPECT_EQ(cfg.port(), 3000);
    EXPECT_EQ(cfg.doc_root(), "/tmp");
    EXPECT_EQ(cfg.threads(), 2);
    EXPECT_EQ(cfg.timeout_seconds(), 60);
    EXPECT_EQ(cfg.max_body_size(), 2097152u);
}

TEST_F(TempConfigTest, CommandLineOverridesJson)
{
    auto cfg_path = write_config(R"({
        "port": 3000,
        "doc_root": "/tmp"
    })");

    const char* argv[] = {
        "http_server", "--config", cfg_path.c_str(),
        "--port", "9090"
    };
    int argc = 5;
    configuration cfg(argc, const_cast<char**>(argv));

    EXPECT_EQ(cfg.port(), 9090);
    EXPECT_EQ(cfg.doc_root(), "/tmp");
    EXPECT_EQ(cfg.address(), "0.0.0.0");
}

TEST_F(TempConfigTest, PartialJsonKeepsDefaults)
{
    auto cfg_path = write_config(R"({
        "port": 8080
    })");

    const char* argv[] = {"http_server", "--config", cfg_path.c_str()};
    int argc = 3;
    configuration cfg(argc, const_cast<char**>(argv));

    EXPECT_EQ(cfg.port(), 8080);
    EXPECT_EQ(cfg.address(), "0.0.0.0");
    EXPECT_EQ(cfg.threads(), 1);
}

TEST_F(TempConfigTest, InvalidValuesFallbackToDefaults)
{
    auto cfg_path = write_config(R"({
        "port": 0,
        "threads": 0
    })");

    const char* argv[] = {"http_server", "--config", cfg_path.c_str()};
    int argc = 3;
    configuration cfg(argc, const_cast<char**>(argv));

    EXPECT_EQ(cfg.port(), 8080);
    EXPECT_EQ(cfg.threads(), 1);
}
