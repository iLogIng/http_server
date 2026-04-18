# static_file_service

> ***/includes/static_file_service.hpp***
> ***日志类***
>

> 依赖:
> ***Boost::Beast***
> ***Boost::FileSystem***
> ***utils.hpp***
>

## 结构

所有定义都包含在 ***namespace server_service*** 命名空间中

### 三层结构

- static_file_service 为request_handler提供静态文件的处理功能
- request_handler 处理客户端发来的请求
- session 处理客户端的异步I/O

## 类实现

包含一个 **static_file_service** 类提供静态文件响应功能

### static_file_service 类

#### 成员变量

#### private

- `const server_config::configuration&` **config_**
  - 是对配置类的引用

#### 成员函数

##### private

- **handle_GET_request**
  > 处理 **GET** 请求
  - **args**
    - `const http::request<http::string_body>&`
    - `beast::string_view full_path`
  - **return**
    - `http::message_generator`

- **handle_HEAD_request**
  > 处理 **HEAD** 请求
  - **args**
    - `const http::request<http::string_body>&`
    - `beast::string_view full_path`
  - **return**
    - `http::message_generator`

##### public

#### 构造函数

目前仅包含唯一的构造函数

- **(const server_config::configuration&)**
  > 传入一个配置类的引用常量，将成员变量指向该配置类，提供配置信息
