# 目录 Contents

## 基础支撑层 (Foundation)

|模块|说明|头文件|
|:---|:---|---:|
|[config](./config.md)|配置解析 — JSON 文件、命令行参数、默认值三层优先级|`config.hpp`|
|[logger](./logger.md)|日志系统 — 基于 Boost.Log，六级别日志|`logger.hpp`|
|[utils](./utils.md)|工具函数 — MIME 类型、路径安全、响应报文生成|`utils.hpp`|
|[cache](./cache.md)|LRU 缓存 — 泛型模板，多线程安全，文件内容缓存|`cache.hpp`|

## 请求处理层 (Request Handling)

|模块|说明|头文件|
|:---|:---|---:|
|[router](./router.md)|路由分发 — 精确匹配 + 前缀匹配|`router.hpp`|
|[static_file_service](./static_file_service.md)|静态文件服务 — GET/HEAD 请求的文件响应|`static_file_service.hpp`|
|[request_handler](./request_handler.md)|请求处理 — 整合路由与静态文件服务|`request_handler.hpp`|

## 服务器核心层 (Server Core)

|模块|说明|头文件|
|:---|:---|---:|
|[server](./server.md)|连接管理 — listener（监听）+ session（会话）|`server.hpp`|
|[graceful_shutdown](./graceful_shutdown.md)|优雅关闭 — 信号捕获、等待活跃会话、超时强制停止|`graceful_shutdown.hpp`|
