# static_file_service

> ***/includes/static_file_service.hpp***
> ***静态文件处理类***
>

> 依赖:
> ***Boost::Beast***
> ***Boost::FileSystem***
> ***router.hpp***
> ***utils.hpp***
> ***cache.hpp***
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

##### private

- `const server_config::configuration&` **config_**
  - 是对配置类的引用
- `mutable server_cache::lru_cache<std::string, std::string>` **lru_cache_**
  - LRU 缓存实例，将文件路径映射到文件内容
  - 多线程安全，容量由 `config_.max_cache_entries()` 控制

#### 成员函数

##### private

- **handle_GET_request**
  > 处理 **GET** 请求。优先从 LRU 缓存查询文件内容；
  > 命中则直接返回 `string_body` 响应（零磁盘 I/O）；
  > 未命中则从磁盘读入文件，可压缩文本预压缩一份 gzip 版本随缓存保存，写入缓存后返回响应。
  - **args**
    - `const http::request<http::string_body>&`
    - `beast::string_view full_path`
  - **return**
    - `http::message_generator`

- **build_content_response**
  > 构建 GET 内容响应，处理顺序：304 优先 → Range（206/416）→ gzip 协商 → 200。
  > gzip 协商：客户端 `Accept-Encoding` 含 gzip 且内容可压缩且有压缩缓存时，
  > 返回 `Content-Encoding: gzip` + `Vary: Accept-Encoding`；带 Range 的请求不压缩。
  > 304/206 响应同样回 `Vary` 保持缓存一致性。

- **handle_HEAD_request**
  > 处理 **HEAD** 请求。优先从 LRU 缓存查询文件大小；
  > 命中则直接设置 Content-Length（无需打开文件）；
  > 未命中则回退到 `file_body` 获取文件大小。
  - **args**
    - `const http::request<http::string_body>&`
    - `beast::string_view full_path`
  - **return**
    - `http::message_generator`

##### public

- **config()**
  > 提供对配置信息的常量引用
  - **return**
    - `const server_config::configuration&`

- **handle_request()**
  > 处理请求，生成响应
  - **args**
    - `const http::request<http::string_body>&`
  - **return**
    - `http::message_generator`

- **as_handler()**
  > 包装 handle_request 返回处理器函数对象
  - **return**
    - `Handler`

#### 构造函数

目前仅包含唯一的构造函数

- **(const server_config::configuration&)**
  > 传入一个配置类的引用常量，将成员变量指向该配置类，提供配置信息；
  > 同时以该配置引用初始化 LRU 缓存 `lru_cache_`，通过 `config_.max_cache_entries()` 获取缓存容量。