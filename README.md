# HTTP 服务器

> **START**
> **2026.4.1**
>
> **version 0.0.1**
> **2026.5.X**
>

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

## 功能特性 Features

> 实现 HTTP/1.1 异步并发 超时控制
>

## 项目结构

### [目录 contents](./docs/CONTENTS.md)

> 目录汇总，包含各个模块的引用
>

**模块简述**

- **logger**
  - 日志模块
- **config**
  - 配置模块
- **utils**
  - 工具模块
- **static_file_service**
  - 静态文件服务模块
- **request_handler**
  - 请求处理模块
- **server**
  - 连接监听与管理模块

- ***test/***
  - 测试单元

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

```mermaid
classDiagram
    direction TD

    class configuration {
        +address()
        +port()
        +doc_root()
        +threads()
        +timeout_seconds()
        +max_body_size()
    }

    class static_file_service {
        -const configuration& config_
        +static_file_service(config)
        +handle_request(req) message_generator
        +as_handler() Handler
        -handle_GET_request(req, full_path)
        -handle_HEAD_request(req, full_path)
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
        -http::request req_
        -const configuration& config_
        -request_handler_ptr handler_
        +session(socket, config, handler)
        +run()
        -do_read()
        -on_read(ec, bytes)
        -send_response(msg)
        -on_write(keep_alive, ec, bytes)
        -do_close()
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
    session --> configuration : 持有引用（超时）
    listener --> configuration : 持有引用

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

**三层结构**

- **static_file_service**
  - 静态文件处理，生成相应的http响应报文
  - 为request_handler提供响应报文
- **request_handler**
  - 请求处理，管理各个功能路由，包含静态文件处理路由
  - 将传来的请求，发送至相应的文件处理模块，获取响应报文
- **session**
  - 会话处理，处理服务器与客户的连接

## TODO

1. 优雅关闭 + 请求大小限制 + 路径规范化
2. [x] 结构化日志 + 请求日志 **logger** 模块
    - 后期添加配置文件对日志进行灵活配置
    - 异步记录，避免日志I/O阻塞网路主进程
    - 日志级别
    - 多线程安全支持
    - 结构化的日志输出
    - 多目标输出（Sink）
      - 控制台
      - 日志文件
    - 日志切分保证日志文件大小，避免过大的单个文件
3. 动态路由（无中间件）
4. [x] 配置文件支持 **config** 模块，实现基于JSON文件的功能配置
5. Range 请求 + 缓存控制
6. 统计接口（/metrics）
7. 单元测试（使用Google Test 关键模块）
8. 添加完整的 ***HTTP/1.1 + 并发 + 超时控制*** 功能
9. HTTPS 支持
10. 增加C++20协程，HTTPS，高级网络特性

- 目的是产出一个可配置、可处理并发连接的、代码结构清晰的静态文件服务器

## 快速开始 Getting Start

> **>= C17**
>

**构建可执行文件**

```bash
$ make http_server
# 或
$ make
```


## 文件结构

```text
.
├── CMakeLists.txt
├── docs
│   └── logger.md
├── index.html
├── logs
│   └── http_server.log
├── makefile
├── README.md
├── src
│   ├── logger.hpp
│   ├── main.cpp
│   ├── request_handler.hpp
│   ├── router.hpp
│   ├── server.hpp
│   └── utils.hpp
└── test
     └── test.cpp
```

## END
