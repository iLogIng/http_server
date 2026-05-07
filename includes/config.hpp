#pragma once

#include <boost/program_options.hpp>
#include <boost/json.hpp>

#include <string>
#include <vector>

// 服务器配置参数
namespace server_config
{

namespace json = boost::json;
namespace prog_opts = boost::program_options;

// 配置参数结构体
struct config_values
{
    std::string         config_path_ = "config.json";
    std::string         address_ = "0.0.0.0";
    unsigned short      port_ = 8080;
    std::string         doc_root_ = ".";
    std::string         log_file_ = "./logs/http_server.log";
    unsigned int        threads_ = 1;
    unsigned int        timeout_seconds_ = 30;
    std::size_t         max_body_size_ = 1 << 20;
    std::size_t         max_connections_ = 1;
    std::size_t         max_cache_entries_ = 0;
};

bool
valid_address(const std::string &addr);

bool
valid_port(uint64_t port);

bool
valid_doc_root(const std::string &path);

bool
valid_threads(uint64_t count);

bool
valid_timeout_seconds(uint64_t timeout_second);

bool
valid_max_body_size(uint64_t max_body_size);

bool
valid_max_connections(uint64_t max_connections);

bool
valid_max_cache_entries(uint64_t max_cache_entries);

// 配置类
// 命令行参数 > 配置文件 > 默认值 逐层覆盖
class configuration
{
public:
    // 采用优先级覆盖的方法处理传入/配置的参数
    //      我认为关于json的解析可以承接该部分的设计
    // p1. command line
    // p2. json config
    // p3. default value
    // 所以最终的配置可能是以上三种杂合的结果，需要编写配置回显功能
    explicit configuration(int argc, char *argv[]);

    // 删除拷贝和移动构造，仅使用 arc argv 构造函数
    configuration(const configuration&) = delete;
    configuration& operator=(const configuration&) = delete;
    configuration(configuration&&) = delete;
    configuration& operator=(configuration&&) = delete;

private:
    config_values config_vals_;

public:

    const std::string& address() const;
    unsigned short port() const;
    const std::string& doc_root() const;
    const std::string& log_file() const;
    unsigned int threads() const;
    unsigned int timeout_seconds() const;
    std::size_t max_body_size() const;
    std::size_t max_connections() const;
    std::size_t max_cache_entries() const;

    void dump() const;

private:

    // 应用 命令行参数
    void apply_command_line(int argc, char *argv[]);
    // 应用 json配置文件
    void apply_json_config(std::string path);

};


} // namespace config

