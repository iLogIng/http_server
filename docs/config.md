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

