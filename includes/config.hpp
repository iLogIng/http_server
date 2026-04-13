#pragma once

#include <boost/program_options.hpp>
#include <boost/json.hpp>

#include <fstream>
#include <stdlib.h>
#include <string>

// 服务器配置参数
namespace server_config
{

namespace po = boost::program_options;
namespace json = boost::json;
namespace prog_opts = boost::program_options;

// 配置参数结构体
struct config_values
{
    std::string         address_ = "0.0.0.0";
    unsigned short      port_ = 8080;
    std::string         doc_root_ = ".";
    unsigned int        threads_ = 1;
    unsigned int        timeout_seconds_ = 30;
    size_t              max_body_size_ = 1 << 20;

    // 验证配置参数是否合法
    bool validate() const;

};

// 配置类
// 命令行参数 > 配置文件 > 默认值 逐层覆盖
class configuration
{
public:
    // 从命令行参数解析配置
    // 1. load_defaults()
    // 2. parse_json_config()
    // 3. parse_command_line()
    // 在 merge_configs() 中进行汇总合并
    // 由 load_config() 统一覆盖到 config_vals_ 中
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
    const unsigned short &port() const;
    const std::string& doc_root() const;
    const unsigned int &threads() const;
    const unsigned int &timeout_seconds() const;
    const size_t &max_body_size() const;

private:

    // 加载 默认配置
    static config_values load_defaults();
    // 从 json配置文件 解析配置
    static config_values parse_json_config(const std::string& path);
    // 从 命令行参数 解析配置
    static config_values parse_command_line(int argc, char *argv[]);

    // 覆盖配置值
    static void merge_configs(config_values& base, const config_values& overrides);

public:
    // 加载配置
    static config_values load_config(int argc, char *argv[]);

};

} // namespace config

