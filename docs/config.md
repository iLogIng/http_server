# 配置模块

> ***/includes/config.hpp***
> ***配置功能***
>

> 依赖 ***Boost::Program_options*** 库提供的命令行配置能力
> 依赖 ***Boost::JSON*** 库提供的JSON文件解析功能
>

为该 http_server 项目提供配置解析功能，支持 JSON 配置文件与命令行参数。

优先级遵循 **命令行参数 > JSON 配置文件 > 默认值**，逐层覆盖。

配置模块的核心任务是：接收不同来源的原始配置，经过解析、验证和优先级合并，最终输出一个已验证的 `configuration` 对象。

## 结构

所有定义都包含在 ***namespace server_config*** 命名空间

### config_values 结构体

> 该结构体保存各个相关配置值

```cpp
struct config_values
{
    std::string         config_path_ = "config.json";
    std::string         address_ = "0.0.0.0";
    unsigned short      port_ = 8080;
    std::string         doc_root_ = ".";
    std::string         log_file_ = "./logs/http_server.log";
    unsigned int        threads_ = 1;
    unsigned int        timeout_seconds_ = 30;
    size_t              max_body_size_ = 1 << 20;
    size_t              max_connections_ = 0;
};
```

### configuration 配置类

包装 **`config_values`** 结构体

仅提供 **`configuration(int argc, char *argv[])`** 构造函数，按以下顺序加载配置：
  1. 预解析 `--config` 确定 JSON 配置文件路径
  2. 调用 **`apply_json_config`** 加载 JSON 文件配置
  3. 调用 **`apply_command_line`** 以命令行参数覆盖

提供针对 **`config_values`** 结构体中各个数据成员的 getter 方法（`address()`、`port()`、`doc_root()`、`log_file()`、`threads()`、`timeout_seconds()`、`max_body_size()`、`max_connections()`）。

## JSON配置文件

默认查找 CWD 下的 `config.json`，可通过 `--config <路径>` 指定。

以下为该配置文件的格式:

```json
{
    "address":"0.0.0.0",
    "port":8080,
    "doc_root":".",
    "threads":1,
    "timeout_seconds":30,
    "max_body_size":10485760,
    "log_file":"./logs/http_server.log",
    "max_connections":10000
}
```

## 命令行参数

```
  -h [ --help ]                Show help message
  -c [ --config ] arg          Path to JSON config file
  -a [ --address ] arg         Server address
  -p [ --port ] arg            Server port
  -r [ --doc_root ] arg        Document root
  -l [ --log_file ] arg        Log file path
  -t [ --threads ] arg         Number of threads
  -s [ --timeout_seconds ] arg Timeout in seconds
  -b [ --max_body_size ] arg   Maximum body size
  -n [ --max_connections ] arg Maximum concurrent connections
```

## 优先级

**命令行参数 > JSON 配置文件 > 默认值**

配置构造时先预解析 `--config` 确定配置文件路径，加载 JSON，最后以命令行参数覆盖。
若用户显式指定 `--config` 但文件不存在则 FATAL 退出；未指定时默认尝试加载 `config.json`，文件不存在仅 WARNING 回退默认值。
