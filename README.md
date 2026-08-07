# HTTP 服务器

> **START**
> **2026.4.1**
>
> **version 0.7.0**
> **2026.5.7**

## 项目简介 Description

**http-server** 基于**Boost.Asio Boost.Beast** 进行编写，提供异步的http服务器实现

- 设计背景：学习Asio异步模型，异步服务器的工作原理

> **功能特性 Features**
>
> 实现 HTTP/1.1 异步并发 超时控制 路由分发 静态文件服务
>
> 配置系统（JSON + 命令行） 结构化日志（控制台 + 文件轮转）
>
> 请求日志（方法 + 路径 + 耗时） 优雅关闭（信号捕获 + 会话排空 + 强制超时）
>
> 路径穿越防护（双重校验） 请求体大小限制（413）
>
> 连接限流（503 + Retry-After 头） LRU 文件内容缓存（线程安全，可配置容量）
>
> 动态 API 服务（端点注册表：method + path 自由注册） POST 请求解析
> （form-urlencoded / JSON 回显） OPTIONS 全局支持（动态 Allow 头） 405 + Allow
>
> gzip 内容协商（Accept-Encoding + Vary，文本预压缩缓存，Range 请求不压缩）
>
> Google Test 单元测试 + 端到端集成测试（140 用例全通过）

-----

## 快速开始 Getting Start

> **>= C17**
> **Boost >= 1.83**
> **Boost 组件** Asio / Beast / Filesystem / JSON / Log / Program Options / Thread
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
:$ ./http_server --port 8080 --doc_root ./app/ --threads 4 --log_file ./logs/app.log --log_level warning
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
    "log_level":"debug",
    "threads":1,
    "timeout_seconds":30,
    "max_body_size":10485760,
    "max_connections":10000,
    "max_cache_entries":64,
    "cache_ttl_seconds":30
}
```

### 容器运行

> Docker / Podman

```bash
# 构建
# （国内网络建议加 --build-arg 换国内 apt 源，绕开代理/加速）
## docker :
docker build -t http-server:latest .
## podman :
podman build -t http-server:latest .

# 运行
# （默认监听 0.0.0.0:8080，非 root 用户，threads=2）
docker run --rm -p 8080:8080 http-server:latest
## podman 类似

# 验证
curl http://localhost:8080/
```

- 国内网络构建：`docker build --build-arg APT_MIRROR=mirrors.tuna.tsinghua.edu.cn -t http-server:latest .`
- 镜像约 99MB，基于 ubuntu:24.04 + Boost 1.83（多阶段构建，仅含运行时库）。
- 自定义静态目录：`docker run --rm -p 8080:8080 -v $(pwd)/my_site:/srv/app http-server:latest`

**容器管理**

```bash
docker stop http-server-test       # 停止容器
docker start http-server-test      # stop 后再次启动
docker restart http-server-test    # 重启
docker logs -f http-server-test    # 日志
docker rm -f http-server-test      # 停止并删除容器
docker image rm http_server:test   # 连镜像一起删
```

### 公网部署测试

> 使用 cloudflared 搭建临时隧道
> 将本机容器暴露成公网 HTTPS URL ，用于公网部署验证

```bash
# 1. 确认容器已运行
curl http://127.0.0.1:8080/            # 期望 200

# 2. 启动临时隧道
cloudflared tunnel --url http://127.0.0.1:8080

## 中国大陆：IPv4 edge + HTTP/2，绕开运营商拦截；origin 用显式 127.0.0.1
cloudflared tunnel --url http://127.0.0.1:8080 --protocol http2 --edge-ip-version 4
```

- 等待日志出现 `Registered tunnel connection` 后，从输出取公网 URL（检查日志中类似 `https://xxxx.trycloudflare.com` URL），任意设备访问验证
- 实测（移动宽带 + IPv4 edge）：根路径 200、404 透传正常、连续 10 次全 200、100 并发全 200；边缘往返约 0.7~1.3s
- *注意：*quick tunnel 为临时 URL，重启会变，仅用于验证/展示；长期部署请改用命名隧道或镜像分发

**常见问题**

- **530**：边缘连不上隧道 -> 加 `--edge-ip-version 4 --protocol http2`，强制 IPv4 + TCP 443 ，绕过运营商的 IPv6/7844 拦截
- **502**：隧道已通但够不到服务 -> origin 写显式 `http://127.0.0.1:8080`（`localhost` 会被解析成 `::1`）

### 可复现基准

> 平台：i5-1235U（12 逻辑线程）/ 15GB / Fedora 44；目标文件 `app/index.html`（9KB）

```bash
# 服务端：6 线程 + 关访问日志 访问日志会拖慢 25~38%
./http_server --doc_root ./app/ --threads 6 -L warning

# 压测：wrk 与服务端配比 6/6 最接近真实能力，避免同机客户端抢占
wrk -t6 -c100 -d30s http://127.0.0.1:8080/index.html
wrk -t6 -c400 -d30s http://127.0.0.1:8080/index.html
```

| 并发 | http_server | nginx | 比率 |
|:---:|:---:|:---:|:---:|
| c100 | ~194k | ~309k | ~62% |
| c400 | ~157k | ~283k | ~56% |

> nginx 对照组：root 指向同一 `./app/`，access_log off

-----

## 项目结构

### [目录 contents](./docs/CONTENTS.md)

> 目录汇总，包含各个模块的引用

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
- **dynamic_api_service**
  - 动态 API 服务模块（端点注册表：method + path 自由注册，OPTIONS 全局支持）
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
  - Google Test 单元测试（71 用例覆盖 config、logger、router、utils、集成测试模块）

-----

## 文档导航

> [架构设计](./docs/architecture.md) — 结构简图、类图与三层设计
>
> [压力测试](./docs/stress-test/stress-test-task.md) — 压力测试，见文件参考一栏
>
> [性能优化](./docs/stress-test/perf.md) — 优化方法、实测数据
>
> [TODO](./TODO.md) — 功能扩展清单

-----

## END
