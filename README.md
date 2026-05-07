# HTTP 服务器

> **START**
> **2026.4.1**
>
> **version 0.0.1**
> **2026.5.5**

## 项目简介 Description

**http-server** 基于**Boost.Asio Boost.Beast** 进行编写，提供异步的http服务器实现

- 设计背景：学习Asio异步模型，异步服务器的工作原理

-----

## 包依赖

- **Boost**
  - **Asio**
  - **Beast**
  - **Filesystem**
  - **JSON**
  - **Log**
  - **Program Options**
  - **Thread**

-----

## 功能特性 Features

> 实现 HTTP/1.1 异步并发 超时控制 路由分发 静态文件服务
> 配置系统（JSON + 命令行） 结构化日志（控制台 + 文件轮转）
> 请求日志（方法 + 路径 + 耗时） 优雅关闭（信号捕获 + 会话排空 + 强制超时）
> 路径穿越防护（双重校验） 请求体大小限制（413）
> 连接限流（503 + Retry-After 头） LRU 文件内容缓存（线程安全，可配置容量）
> Google Test 单元测试 + 端到端集成测试（57 用例全通过）

## 项目结构

### [目录 contents](./docs/CONTENTS.md)

> 目录汇总，包含各个模块的引用
>

### 模块简述

- **src/main.cpp**
  - 项目入口
- **logger**
  - 日志模块
- **config**
  - 配置模块
- **utils**
  - 工具模块
- **static_file_service**
  - 静态文件服务模块
- **cache**
  - LRU 缓存模块，泛型模板，多线程安全
- **router**
  - 路由模块，管理精确路由与前缀路由匹配
- **request_handler**
  - 请求处理模块
- **server**
  - 连接监听与管理模块，包含 listener 和 session
- **graceful_shutdown**
  - 优雅关闭模块，处理 SIGINT/SIGTERM 信号

- ***test/***
  - Google Test 单元测试（57 用例覆盖 config、logger、router、utils、集成测试模块）

-----

### 结构简图

```mermaid
graph TD
    subgraph 主入口
        main[main.cpp]
    end

    subgraph 配置与日志
        config[server_config::configuration]
        logger[server_logger]
    end

    subgraph 工具层
        utils[server_utils]
    end

    subgraph 服务层
        static_service[static_file_service]
        cache[cache<br/>lru_cache]
        router[router]
        request_handler[request_handler]
    end

    subgraph 网络层
        listener[listener]
        session[session]
    end

    main --> config
    main --> logger
    main --> static_service
    main --> router
    main --> request_handler
    main --> listener

    config --> logger
    utils --> logger
    static_service --> config
    static_service --> utils
    static_service --> logger
    static_service -.-> cache
    cache --> config

    router -.-> Handler[Handler 类型]
    request_handler --> router
    request_handler --> Handler

    listener --> request_handler
    session --> request_handler
    listener --> config
    session --> config

    static_service -- as_handler() --> Handler
    main -- 注册路由 --> router
    main -- 默认处理器 --> request_handler
```

#### 核心结构简图

```mermaid
classDiagram
    direction TD

    class configuration {
        +address()
        +port()
        +doc_root()
        +log_file()
        +threads()
        +timeout_seconds()
        +max_body_size()
        +max_connections()
        +max_cache_entries()
    }

    class static_file_service {
        -const configuration& config_
        -lru_cache cache_
        +static_file_service(config)
        +handle_request(req) message_generator
        +as_handler() Handler
        -handle_GET_request(req, full_path)
        -handle_HEAD_request(req, full_path)
    }

    class lru_cache {
        -mutex mutex_
        -list items_
        -map lookup_
        +get(key) optional~Value~
        +put(key, value)
        +erase(key) bool
        +clear()
    }

    class router {
        -unordered_map~exact_route,Handler~ exact_routes_
        -vector~prefix_route~ prefix_routes_
        +add_exact_route(method, path, handler)
        +add_prefix_route(method, prefix, handler)
        +match(req) Handler
    }

    class request_handler {
        -router_ptr routers_
        -Handler default_handler_
        +request_handler(router_ptr, default_handler)
        +add_exact_route(method, path, handler)
        +add_prefix_route(method, prefix, handler)
        +handle_request(req) message_generator
    }

    class session {
        -beast::tcp_stream stream_
        -flat_buffer buffer_
        -boost::optional~request_parser~ parser_
        -const configuration& config_
        -request_handler_ptr handler_
        +session(socket, config, handler)
        +run()
        -do_read()
        -on_read(ec, bytes)
        -send_response(msg)
        -on_write(keep_alive, ec, bytes)
        -do_close()
        +active_sessions() size_t
    }

    class listener {
        -io_context& ioc_
        -tcp::acceptor acceptor_
        -const configuration& config_
        -request_handler_ptr handler_
        +listener(ioc, endpoint, config, handler)
        +run()
        -do_accept()
        -on_accept(ec, socket)
    }

    %% 依赖关系
    static_file_service --> configuration : 持有引用
    static_file_service --> lru_cache : 持有缓存实例
    session --> configuration : 持有引用（超时）
    listener --> configuration : 持有引用

    lru_cache --> configuration : 读取容量配置

    request_handler --> router : 组合 (router_ptr)
    request_handler --> Handler : 持有 default_handler_

    session --> request_handler : 组合 (request_handler_ptr)
    listener --> request_handler : 组合 (request_handler_ptr)
    listener --> session : 创建并调用

    static_file_service ..> Handler : as_handler() 返回
    router ..> Handler : match 返回
    request_handler ..> Handler : handle_request 调用

    %% 路由注册时的关系（虚线）
    request_handler --> static_file_service : 可通过 add_route 注册其 as_handler()
```

## 服务器设计结构

### 三层结构

- **static_file_service**
  - 静态文件处理，生成相应的http响应报文
  - 为request_handler提供响应报文
- **request_handler**
  - 请求处理，管理各个功能路由，包含静态文件处理路由
  - 将传来的请求，发送至相应的文件处理模块，获取响应报文
- **session**
  - 会话处理，处理服务器与客户的连接

-----

## TODO

| 特性 | 优先级 | 说明 |
|:----:|:------:|:----:|
| **POST/DELETE 方法支持** | P3 | 扩展动态 API |
| **HTTPS 支持** | P3 | 集成 boost::asio::ssl |
| **Range 请求** | P3 | 支持断点续传 |
| **统计接口（/metrics）** | P3 | QPS、活跃连接数等指标 |
| **C++20 协程** | P3 | 迁移到 Asio 的 C++20 协程模型 |

## 快速开始 Getting Start

> **>= C17**
> **Boost >= 1.83**
>

### 构建可执行文件

```bash
:$ make http_server
# 或
:$ make
```

### 启动参数

可通过命令行参数或 JSON 配置文件配置服务器，优先级：**命令行 > JSON > 默认值**

```bash
:$ ./http_server --port 8080 --doc_root ./app/ --threads 4 --log_file ./logs/app.log
```

### 配置文件

> 默认查找 CWD 下的 `config.json`，可通过 `--config` 指定路径
>

```json
{
    "address":"0.0.0.0",
    "port":8080,
    "doc_root":"./app/",
    "log_file":"./logs/http_server.log",
    "threads":1,
    "timeout_seconds":30,
    "max_body_size":10485760,
    "max_connections":10000,
    "max_cache_entries":64
}
```

-----

## 文件结构

```text
.
├── CMakeLists.txt          # 顶层 CMake 构建文件
├── config.json             # 服务器配置文件
├── makefile                # 顶层 Makefile
├── README.md               # 本文件
├── TODO.md                 # 功能扩展清单
├── app/
│   ├── test.html           # 响应测试页面
│   ├── style.css           # test.html 样式表
│   └── index.html          # 测试用静态页面
├── docs/
│   ├── CONTENTS.md         # 文档目录
│   ├── cache.md
│   ├── logger.md
│   ├── request_handler.md
│   ├── router.md
│   ├── server.md
│   ├── static_file_service.md
│   └── utils.md
├── includes/
│   ├── cache.hpp
│   ├── config.hpp
│   ├── graceful_shutdown.hpp
│   ├── logger.hpp
│   ├── request_handler.hpp
│   ├── router.hpp
│   ├── server.hpp
│   ├── static_file_service.hpp
│   └── utils.hpp
├── src/
│   ├── CMakeLists.txt
│   ├── config.cpp
│   ├── graceful_shutdown.cpp
│   ├── main.cpp
│   ├── request_handler.cpp
│   ├── router.cpp
│   ├── server.cpp
│   ├── static_file_service.cpp
│   └── utils.cpp
└── test/
    ├── CMakeLists.txt
    ├── makefile
    ├── test_config.cpp
    ├── test_integration.cpp
    ├── test_logger.cpp
    ├── test_router.cpp
    └── test_utils.cpp
```

-----

## [压力测试](./stress_test.md)

-----

## END
