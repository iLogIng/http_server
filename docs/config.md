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

## 结构

所有定义都包含在 ***namespace server_config*** 命名空间

### config_values 结构体

> 该结构体保存各个相关配置值
> 并提供 **`validate`** 方法进行参数合法性的检测
> 

```cpp
struct config_values
{
    std::string         address_ = "0.0.0.0";
    unsigned short      port_ = 8080;
    std::string         doc_root_ = ".";
    unsigned int        threads_ = 1;
    unsigned int        timeout_seconds_ = 30;
    size_t              max_body_size_ = 1 << 20;

    bool validate() const;

};
```

### configuration 配置类

包装 **`config_values`** 结构体

提供 **`load_config(int argc, int *argv[])`** 方法加载配置

仅提供 **`configuration(int argc, int *argv[])`** 构造函数，通过调用 **`load_config`** 加载配置

提供针对 **`config_values`** 结构体中各个数据成员的getter方法

## JSON配置文件

该配置文件必须放在项目根目录下，命名为 ***config.json***

以下为该配置文件的格式:

```json
{
    "address":"0.0.0.0",
    "port":8080,
    "doc_root":".",
    "threads":1,
    "timeout_seconds":30,
    "max_body_size":10485760
}
```
