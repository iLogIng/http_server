# 配置模块

> ***/src/config.hpp***
> ***配置功能***
>

> 依赖 ***Boost::Program_options*** 库提供的命令行配置能力
> 依赖 ***Boost::JSON*** 库提供的JSON文件解析功能
>

为该 http_server 项目提供JSON配置解析功能

提供 config.json 解析，优先级遵循

- ***命令行参数 > 配置文件 > 默认值***

Config 模块的核心任务是：接收不同来源的原始配置，经过解析和优先级合并，最终输出一个验证过的 ServerConfig 对象。

就目前的设计，对于Config的读取，采取覆盖式的更新处理。

## 结构

所有定义都包含在 ***namespace server_config*** 命名空间

### config_values 结构体

> 该结构体保存各个相关配置值
> 并提供 **`validate`** 方法进行参数合法性的检测
> 

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
};
```

### configuration 配置类

包装 **`config_values`** 结构体

提供 **`load_config(int argc, int *argv[])`** 方法加载配置

仅提供 **`configuration(int argc, int *argv[])`** 构造函数，通过调用 **`load_config`** 加载配置
  - 依赖 **`apply_json_config`** 方法加载json文件配置
  - 依赖 **`apply_command_line`** 方法加载命令行配置

提供针对 **`config_values`** 结构体中各个数据成员的getter方法

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
    "log_file":"./logs/http_server.log"
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
```

## 优先级

**命令行参数 > JSON 配置文件 > 默认值**

配置构造时先预解析 `--config` 确定配置文件路径，加载 JSON，最后以命令行参数覆盖。
若用户显式指定 `--config` 但文件不存在则 FATAL 退出；未指定时默认尝试加载 `config.json`，文件不存在仅 WARNING 回退默认值。
