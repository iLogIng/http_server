# 架构设计 Architecture

## 结构简图

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

## 核心结构简图

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

## 目录结构

```text
.
├── .github/
│   └── workflows/
│       ├── http-server-ci.yml   # 持续集成
│       └── http-server-cd.yml   # 持续部署
├── CMakeLists.txt          # 顶层 CMake 构建文件
├── Dockerfile              # 容器化构建
├── LICENSE                 # 开源许可
├── Makefile                # 顶层 Makefile
├── README.md               # 本文件
├── TODO.md                 # 功能扩展清单
├── app/
│   ├── index.html          # 测试用静态页面
│   ├── style.css           # index.html 样式表
│   └── test.html           # 响应测试页面
├── config.json             # 服务器配置文件
├── docs/
│   ├── CONTENTS.md         # 文档目录
│   ├── architecture.md     # 架构设计：结构简图 / 类图 / 三层设计 / 目录结构
│   ├── cache.md
│   ├── config.md
│   ├── graceful_shutdown.md
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
├── stress-test/
│   └── stress-test.md      # 压力测试报告
└── test/
    ├── CMakeLists.txt
    ├── makefile
    ├── config.json         # 测试用服务器配置
    ├── test_config.cpp
    ├── test_integration.cpp
    ├── test_logger.cpp
    ├── test_router.cpp
    └── test_utils.cpp
```
