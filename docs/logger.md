# 日志模块

> ***/includes/logger.hpp***
> ***日志类***
>

> 基于 ***Boost::Log*** 库
>

为该 http_server 项目提供日志处理能力。

将日志信息存放到通过 `log_file` 参数指定的路径（默认 `./logs/http_server.log`），日志目录由该路径自动派生。

## 结构

所有定义都包含在 ***namespace server_logger*** 命名空间中

提供 ***init_logger*** 作为最外层接口，初始化日志系统

### 日志信息属性

使用 `BOOST_LOG_ATTRIBUTE_KEYWORD` 宏自定义了三个日志属性：

- `TimeStamp`：记录日志事件的时间戳
- `ThreadID`：记录日志事件发生的线程ID
- `Severity`：记录日志事件的严重级别

日志格式：

```text
[TimeStamp] [ThreadID] [Severity] log_message
```

### 便捷宏

#### 日志级别宏

> 命名格式：
> 普通信息 `LOG_[RANK]`
> 扩展位置信息 `LOG_[RANK]_LOC` 用于追踪代码位置

- 使用示例：
  - `LOG_[RANK] << "[message]";`
    ```text
    [2026-04-11 17:22:30.120915] [0x00007f89e1c7bc00] [info] Starting HTTP server...
    ```
  - `LOG_[RANK]_LOC << "[message]";`
    ```text
    [2026-04-11 18:19:55.278796] [0x00007fcf5568eec0] [fatal] src/main.cpp:33 - Invalid command line arguments
    ```

日志格式，例子：

提供 ***六个等级*** 的日志类型
|宏名称|级别|用途|
|:---:|:---:|:---:|
|**`LOG_TRACE[_LOC]`**|***TRACE***|最详细调试信息|
|**`LOG_DEBUG[_LOC]`**|***DEBUG***|调试信息|
|**`LOG_INFO[_LOC]`**|***INFO***|正常运行时调试信息|
|**`LOG_WARNING[_LOC]`**|***WARNING***|潜在问题|
|**`LOG_ERROR[_LOC]`**|***ERROR***|可恢复错误|
|**`LOG_FATAL[_LOC]`**|***FATAL***|致命错误，将导致程序终止|

