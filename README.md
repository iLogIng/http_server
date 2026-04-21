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

## 压力测试

> 该压力测试基于 **wrk** 工具
> ***[wrk tool](https://github.com/wg/wrk)***
>

使用示例

```bash
$ wrk -t8 -c400 -d30s http://<静态文件>

$ wrk -t8 -c400 -d30s http://localhost:8080/index.html
```

### 测试平台：

```text
Operating System: Fedora Linux 43
KDE Plasma Version: 6.6.4
KDE Frameworks Version: 6.25.0
Qt Version: 6.10.3
Kernel Version: 6.19.11-200.fc43.x86_64 (64-bit)
Graphics Platform: Wayland

Processors: 8 × Intel® Core™ i5-8250U CPU @ 1.60GHz
Memory: 8 GiB of RAM (7.6 GiB usable)
Graphics Processor: Intel® UHD Graphics 620

Manufacturer: Acer
Product Name: Swift SF514-52T
System Version: V1.07
```
### 测试

以下测试以 `./http_server --threads 8` 为测试基础

以下压力测试参数基于：8线程，[100 200 400 600 800 1000]、30秒

```bash
$ ./wrk -t8 -c100 -d30s http://0.0.0.0:8080/index.html

Running 30s test @ http://0.0.0.0:8080/index.html
  8 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    41.71ms    1.91ms 132.20ms   96.21%
    Req/Sec   288.64     26.01   363.00     65.94%
  69084 requests in 30.09s, 608.87MB read
Requests/sec:   2295.91
Transfer/sec:     20.24MB
```

```bash
$ ./wrk -t8 -c200 -d30s http://0.0.0.0:8080/index.html

Running 30s test @ http://0.0.0.0:8080/index.html
  8 threads and 200 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    41.77ms    2.00ms  81.22ms   93.09%
    Req/Sec   600.19     49.03   757.00     66.54%
  143658 requests in 30.09s, 1.24GB read
Requests/sec:   4773.95
Transfer/sec:     42.08MB
```

```bash
$ ./wrk -t8 -c400 -d30s http://0.0.0.0:8080/index.html

Running 30s test @ http://0.0.0.0:8080/index.html
  8 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    41.64ms    2.05ms 104.60ms   97.13%
    Req/Sec     1.20k    71.01     1.40k    77.08%
  287979 requests in 30.05s, 2.48GB read
Requests/sec:   9581.91
Transfer/sec:     84.45MB
```

```bash
$ ./wrk -t8 -c600 -d30s http://0.0.0.0:8080/index.html
Running 30s test @ http://0.0.0.0:8080/index.html
  8 threads and 600 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    42.60ms    3.29ms 195.66ms   93.88%
    Req/Sec     1.77k   139.11     2.21k    72.42%
  422457 requests in 30.09s, 3.64GB read
Requests/sec:  14038.62
Transfer/sec:    123.73MB
```

```bash
$ ./wrk -t8 -c800 -d30s http://0.0.0.0:8080/index.html
Running 30s test @ http://0.0.0.0:8080/index.html
  8 threads and 800 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    58.37ms   29.32ms   1.08s    81.89%
    Req/Sec     1.41k   497.77     2.58k    60.45%
  336367 requests in 30.10s, 2.41GB read
  Non-2xx or 3xx responses: 56814
Requests/sec:  11176.80
Transfer/sec:     82.11MB
```

```bash
$ ./wrk -t8 -c1000 -d30s http://0.0.0.0:8080/index.html

Running 30s test @ http://0.0.0.0:8080/index.html
  8 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    51.38ms   22.48ms 714.20ms   71.71%
    Req/Sec     1.85k   622.61     3.40k    56.97%
  441681 requests in 30.10s, 1.25GB read
  Non-2xx or 3xx responses: 301848
Requests/sec:  14674.03
Transfer/sec:     42.38MB
```
```mermaid
%%{
  init: {
    'theme': 'base',
    'themeVariables': {
      'xyChart': {
        'plotColorPalette': '#E76F51, #2A9D8F',
        'xAxisLineColor': '#264653',
        'yAxisLineColor': '#264653',
        'xAxisTitleColor': '#264653',
        'yAxisTitleColor': '#264653',
        'xAxisLabelColor': '#264653',
        'yAxisLabelColor': '#264653',
        'backgroundColor': '#F4F1DE'
      }
    }
  }
}%%
xychart-beta
    title "性能曲线"
    x-axis "并发连接数" [100, 200, 400, 600, 800, 1000]
    y-axis "QPS" 0 --> 16000
    line "总 QPS" [2296, 4774, 9582, 14039, 11177, 14674]
    line "有效 QPS" [2296, 4774, 9582, 14039, 9289, 4646]
```

## END
