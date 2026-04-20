# server

> ***/includes/server.hpp***
> ***连接监听与管理模块***
>

> 依赖:
> ***Boost::Beast***
> ***Boost::Asio***
> ***logger.hpp***
> ***request_handler.hpp***
>

## 结构

所有定义都包含在 ***namespace server_host*** 命名空间中

### 处理请求的三层结构

- **[static_file_service](./static_file_service.md)**
  - 为request_handler提供静态文件的处理功能
- **[request_handler](./request_handler.md)**
  - 处理客户端发来的请求
- session 处理客户端的异步I/O

```mermaid
flowchart TD
  A[static_file_service]
  B[request_handler]
  C[session]
  D[Listener]

  A -- 提供静态文件请求处理 --> B -- 提供请求处理功能 --> C
```

## 类实现

包含两个类:

- **listener**
  > 监听请求

- **session**
  > 管理连接

### listener 类

- ***using*** `std::shared_ptr<server_service::request_handler>` **request_handler_ptr**

#### 成员变量

##### private

- `beast::tcp_stream` **stream_**
  - TCP连接流
- `beast::flat_buffer` **buffer_**
  - 平坦的缓存区域
- `http::request<http::string_body>` **req_**
  - 客户端的请求报文
- `const server_config::configuration&` **config_**
  - 服务器配置引用
- `request_handler_ptr` **handler_**
  - 请求处理对象的共享指针

#### 成员函数

##### public

- **run**
  > 开始异步处理

- **do_read**
  > 处理流读取

- **on_read**
  > 流读取
  - **args**
    - `beast::error_code` **ec**
    - `std::size_t` **bytes_transferred**

- **send_response**
  > 发回响应
  - **args**
    - `http::message_generator&&` **msg**

- **on_write**
  > 写操作
  - **args**
    - `bool` **keep_alive**
    - `beast::error_code` **ec**
    - `std::size_t` **bytes_transferred**

- **do_close**
  > 关闭连接

#### 构造函数

目前仅包含唯一的构造函数

- **(tcp::socket&& , request_handler_ptr)**

-----

### session 类

- ***using*** `std::shared_ptr<server_service::request_handler>` **request_handler_ptr**

#### 成员变量

##### private

- `net::io_context` **ioc_**
  - Asio异步上下文
- `tcp::acceptor` **acceptor_**
  - 监听，接收器
- `request_handler_ptr` **handler_**
  - 请求处理对象的共享指针
- `const server_config::configuration&` **config_**
  - 服务器配置引用
- `request_handler_ptr` **handler_**
  - 请求处理对象的共享指针

#### 成员函数

##### private

- **do_accept**
  > 监听网络端口

- **on_accept**
  > 从端口接收报文
  - **args**
    - `beast::error_code` **ec**
    - `tcp::socket` **socket**

##### public

- **run**
  > 开始进行监听网络端口

#### 构造函数

目前仅包含唯一的构造函数

- **(net::io_context& , tcp::endpoint , request_handler_ptr)**
