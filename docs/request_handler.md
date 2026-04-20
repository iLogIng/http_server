# request_handler

> ***/includes/request_handler.hpp***
> ***日志类***
>

> 依赖:
> **memory**
> **router.hpp**
> **config.hpp**
> **utils.hpp**
>

## 结构

所有定义都包含在 ***namespace server_service*** 命名空间中

## 类实现

包含一个 **request_handler** 类提供静态文件响应功能

### request_handler 类

- ***using*** `std::shared_ptr<router>` **router_ptr**

#### 成员变量

##### private

- `router_ptr` **routers_**
  - 共享的路由表

- `Handler` **default_handler_**
  - 默认的处理器

#### 成员函数

##### public

- **handle_request()**
  > 处理请求，生成响应报文
  - **args**
    - `http::request<http::string_body>&`
  - **return**
    - `http::message_generator`

- **add_exact_route**
  > 注册精确路由
  - **args**
    - `http::method`
    - `const std::string&`
    - `Handler`

- **add_exact_route**
  > 注册前缀路由
  - **args**
    - `http::method`
    - `const std::string&`
    - `Handler`

#### 构造函数

目前经包含唯一的构造函数

- **(const server_config::configuration& config)**
  > 传入一个配置类的引用常量，初始化组合的静态文件服务对象
