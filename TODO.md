# HTTP 服务器 TODO

> **功能扩展 + 性能优化清单**
>
> **项目方向与阶段流程：**
> 
> **阶段一 POST ->**
>
> **阶段二 协议完整性 ->**
>
> **阶段三 HTTPS ->**
>
> **阶段四 WebSocket ->**
> 
> **v1.0 收尾**
>
> **核心功能已完成（配置 / 日志 / 路由 / 解耦 / LRU 缓存 / 限流 / 测试），当前聚焦阶段一**

---

## 一、功能扩展 Roadmap

### 阶段一 · POST + OPTIONS（计划已定，待实现）

> 范围：POST 解析 + 回显；方法覆盖 POST + OPTIONS。
> 顺带修复三处问题：query string 剥离、前缀路由路径边界、`max_body_size` 默认值对齐。

- [ ] **请求体解析工具**（`server_utils`）：`url_decode`（%XX、+ -> 空格）、`parse_urlencoded`（form-urlencoded -> map）
- [ ] **POST `/api/echo` 路由**（`main.cpp`）：按 `Content-Type` 分流，form -> 回显 JSON；JSON -> 原样回显；其余 -> 400
- [ ] **OPTIONS / Allow 头**：静态服务返回 `Allow: GET, HEAD, OPTIONS`
- [ ] **query string 剥离**：`router::match` 与 `static_file_service::handle_request` 用 `http::target_view::path()`，修复 `?x=1` 404
- [ ] **前缀路由边界修复**：`/api` 不误匹配 `/apiary`（`router.cpp`）
- [ ] **`max_body_size` 默认对齐**：`config.hpp` 1MB -> 10MB（与 README 一致）
- [ ] **测试**：`test_utils`（解码/解析）、`test_router`（query/边界/方法）、`test_integration`（POST 回显、OPTIONS、query 回归）
- [ ] **文档**：README 功能特性、TODO 完成清单同步

### 阶段二 · 协议完整性

- [x] **Range 请求**：解析 `Range` 头，用 `http::response<http::file_body>` 的 `seek` 实现断点续传；为 sendfile 打底
- [ ] **gzip 压缩传输**：按 `Accept-Encoding` 压缩文本响应，加 `Content-Encoding: gzip` + `Vary`
- [ ] **DELETE/PUT 方法**：路由扩展动态 API（DELETE 需权限校验）

### 阶段三 · HTTPS（TLS）

- [ ] **HTTPS 支持**：集成 `boost::asio::ssl`，新增 `--cert`/`--key` 参数；参考 Beast advanced_server 示例

### 阶段四 · WebSocket

- [ ] **WebSocket 回显**：识别 `Upgrade` 头，独立 `websocket_session`，消息回显

### 收尾 · v1.0

- [ ] **异步日志**：日志线程 + 队列 + 条件变量，避免日志阻塞 I/O 线程
- [ ] **/metrics 统计接口**：暴露 QPS、活跃连接数、请求/错误计数；承接压测报告的可观测性

---

## 二、性能优化

> 来源：`docs/stress-test/stress-test-i5-1235u.md` 压测结论 + 代码检视
> 结论：warm 缓存下 10 万 QPS，服务端本体健康；c600+ 下滑混有同机 wrk 客户端竞争，需分离测量确认
> 最新实测与已完成优化见 [perf.md](./docs/stress-test/perf.md)：隔离测试达 nginx 的 ~60%，关访问日志后 c100 ~194k

### 阶段 0 — 测量校正
- [ ] wrk 放到独立机器/容器重测 c600-c1000，确定服务端真实上限（避免在测量假象上投入优化）

### 阶段 1 — 高性价比改动
- [ ] **socket 收发缓冲调优**：设置 `send_buffer_size` / `receive_buffer_size`

### 阶段 2 — 协议级优化
- [ ] **超时与限流细化**：区分读超时与排队超时；限流按连接粒度（当前整机 503）

### 阶段 3 — 性能深水区（视阶段 0 的压测结果决定）
- [ ] sendfile / io_uring 零拷贝（大文件）
- [ ] 大文件 mmap + `MADV_SEQUENTIAL`
- [ ] SO_REUSEPORT 多 accept 队列（当前单 acceptor + strand，accept 串行）
- [ ] CPU 亲和性 / P 核绑定（Alder Lake 混合架构）

---

## 三、已完成

| 功能 | 说明 |
|:--|:--|
| 配置文件 | JSON + 命令行，优先级覆盖；`--log_level/-L` 六级日志可按需关闭 |
| 请求日志 | 方法 + 路径 + 耗时 |
| 业务解耦 | `request_handler` 独立类 |
| 简单路由 | 精确 + 前缀匹配，含 `/api/hello` |
| 静态服务 | 目录请求解析为 `index.html`；warm 路径零系统调用（路径缓存免 statx） |
| LRU 缓存 | 泛型模板，`shared_ptr` 零拷贝，key 免临时分配 |
| 条件请求 | ETag/Last-Modified，If-None-Match / If-Modified-Since -> 304 |
| 连接限流 | 503 + Retry-After |
| 单元测试 | 71 用例全通过 |
| 压测报告 | `docs/stress-test/` 多线程多并发曲线 |

> 设计决策记录：LRU 选型 / 线程安全 / 路由前缀匹配而非正则 / 路径解析缓存 —— 面试展示时突出这些思考。

---

## END
