# dynamic_api_service

> ***/includes/dynamic_api_service.hpp***
> ***动态 API 服务模块***
>

> 依赖:
> ***Boost::Beast***
> ***router.hpp***
> ***utils.hpp***
>

## 结构

所有定义都包含在 ***namespace server_service*** 命名空间中

## 概述

为服务器提供**任意动态请求解析**能力：端点以完整路径（method + path）自由注册，
无需内置处理器，由使用方（如 `main.cpp`）自定义并注册。

- 挂载方式：以 `as_handler()` 转换为路由 `Handler`，注册到 router 的前缀路由
- 拦截前缀：构造函数传入 `base_path`（如 `/api`），仅用于注册路径校验与文档用途
- 方法覆盖：GET / POST / OPTIONS 等任意 `http::verb` 均可注册

## 类实现

包含一个 **dynamic_api_service** 类提供端点注册与请求分发功能

### dynamic_api_service 类

#### 成员变量

##### private

- `std::string` **base_path_**
  - 拦截前缀（如 `/api`），注册路径不符前缀时 WARNING 提示但不拒绝
- `std::unordered_map<std::string, method_handlers>` **endpoints_**
  - 端点注册表：path -> (method -> handler)
  - `method_handlers = std::unordered_map<http::verb, Handler>`

#### 成员函数

##### public

- **add_endpoint**
  > 注册端点（完整路径，如 `/api/hello`），返回自身引用以支持链式注册
  - **args**
    - `http::verb method`
    - `std::string path`
    - `Handler handler`
  - **return**
    - `dynamic_api_service&`

- **as_handler()**
  > 包装 handle_request 返回处理器函数对象，供 router 挂载
  - **return**
    - `Handler`

##### private

- **handle_request**
  > 请求分发，按以下优先级处理：
  > 1. OPTIONS -> 动态 Allow（任意路径 200）
  > 2. 端点命中 -> 调用对应处理器
  > 3. 路径存在但方法未注册 -> 405 + Allow（RFC 7231 §6.5.5）
  > 4. 路径未注册 -> 404
  - **args**
    - `const http::request<http::string_body>&`
  - **return**
    - `http::message_generator`

- **handle_options**
  > 处理 OPTIONS 请求：任意路径返回 200 + Allow 头；
  > Allow 由端点注册表动态计算（GET 隐含 HEAD，OPTIONS 恒支持）
  - **args**
    - `const http::request<http::string_body>&`
  - **return**
    - `http::message_generator`

- **make_method_not_allowed**
  > 构造 405 Method Not Allowed 响应，Allow 头列出该路径全部已注册方法
  - **args**
    - `const http::request<http::string_body>&`
    - `const method_handlers&`
  - **return**
    - `http::message_generator`

- **build_allow_header**
  > 组装 Allow 头：固定方法输出序（GET, HEAD, POST, PUT, DELETE, PATCH），
  > 注册 GET 时隐含 HEAD，OPTIONS 恒在末尾
  - **args**
    - `const method_handlers&`
  - **return**
    - `std::string`

#### 构造函数

目前仅包含唯一的构造函数

- **(std::string base_path)**
  > 传入拦截前缀（如 `/api`），用于注册路径校验与文档用途；
  > 端点仍以完整路径注册，任意 method + path 组合

## 请求分发流程

```text
请求到达 (base_path 前缀已由 router 保证)
    │
    ├─ OPTIONS ──► 查端点表 → 动态计算 Allow（GET 隐含 HEAD）
    │                    └──► 200 + Allow 头（未注册路径仅 OPTIONS）
    │
    └─ 其他方法 ──► target_path 剥离 query
              ├─ 路径命中 + 方法命中 ──► 调用端点处理器
              ├─ 路径命中 + 方法不符 ──► 405 + Allow（列出全部方法）
              └─ 路径未命中 ──► 404 Not Found
```

## 使用示例

```cpp
// main.cpp：自定义处理器并注册
auto dynamic_api = std::make_shared<server_service::dynamic_api_service>("/api");
dynamic_api->add_endpoint(http::verb::get, "/api/hello", handle_hello)
           .add_endpoint(http::verb::post, "/api/echo", handle_echo);
auto dynamic_handler = dynamic_api->as_handler();

router->add_prefix_route(http::verb::get, "/api", dynamic_handler);
router->add_prefix_route(http::verb::post, "/api", dynamic_handler);
router->add_prefix_route(http::verb::options, "/", dynamic_handler);   // OPTIONS 全局
```
