# graceful_shutdown

> ***includes/graceful_shutdown.hpp***
> ***优雅关闭模块***
>

> 依赖：
> **Boost::asio**
> **chrono**
> **functional**
> **memory**
> **server.hpp**
> 

## 结构

所有的定义都包含在 ***namespace server_host*** 命名空间中

## 设计说明

graceful_shutdown 模块提供 HTTP 服务器的优雅关闭能力：

1. 捕获 `SIGINT` / `SIGTERM` 信号
2. 停止 listener 接受新连接
3. 定期检查活跃会话数，等待现有请求处理完毕
4. 超时后强制停止 Asio io_context（默认 30 秒）
5. 全部完成后回调通知主程序

## 类实现

包含一个 **graceful_shutdown** 类，不可拷贝、不可移动。

### 成员变量

- `net::io_context&` **ioc_** — Asio IO 上下文引用
- `listener_ptr` **listener_** — 监听器对象指针
- `ActiveSessionGetter` **get_active_sessions_** — 获取当前活跃会话数的回调
- `chrono::seconds` **check_interval_** — 检查活跃会话的时间间隔（默认 1 秒）
- `chrono::seconds` **force_timeout_** — 强制关闭超时（默认 30 秒）
- `net::signal_set` **signals_** — 信号集（SIGINT, SIGTERM）
- `net::steady_timer` **check_timer_** — 轮询活跃会话计时器
- `net::steady_timer` **force_timer_** — 强制关闭计时器
- `CompletionHandler` **on_complete_** — 关闭完成后回调
- `bool` **shutting_down_** — 是否正在关闭中

### 成员函数

- **start_shutdown_listener(CompletionHandler on_complete)**
  > 注册信号处理，启动优雅关闭监听

- **force_stop()**
  > 立即强制停止：取消所有计时器和信号，调用完成回调

- **on_signal(ec, signal_number)** (private)
  > 信号处理：收到 SIGINT/SIGTERM 时调用 do_shutdown()

- **do_shutdown()** (private)
  > 停止 listener → 检查活跃会话 → 启动强制超时计时器

- **check_active_sessions()** (private)
  > 检查活跃会话数，为零则完成关闭；否则等待 check_interval 后重试

### 生命周期

```
信号到达 → do_shutdown()
           ├─ listener_->stop()           // 关闭 acceptor，拒绝新连接
           ├─ check_active_sessions()      // 轮询等待现有会话结束
           │   └─ 会话数为 0 → 回调完成
           └─ force_timer_                // 超时后直接 ioc_.stop()
```
