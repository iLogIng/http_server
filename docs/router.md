# router

> ***/includes/router.hpp***
> ***连接监听与管理模块***
>

> 依赖:
> ***Boost::Beast***
> ***functional***
> ***unordered_map***
>

## 结构

所有定义都包含在 ***namespace server_service*** 命名空间中

## 类实现

仅包含一个 **router** 类提供路由功能

报文处理包装器

- ***using*** `std::function<http::message_generator(const http::request<http::string_body>&)>` **Handler**

### router 类

- ***using*** `std::shared_ptr<server_service::request_handler>` **request_handler_ptr**

#### 成员变量

##### private

```cpp
struct exact_route {
    http::verb method;
    std::string path;
    bool operator ==(const exact_route& other) const {
        return this->method == other.method
            && this->path == other.path;
    }
};

struct exact_route_hash {
    std::size_t operator ()(const exact_route& key) const {
        auto h1 = std::hash<int>{}(static_cast<int>(key.method));
        auto h2 = std::hash<std::string>{}(key.path);
        return h1 ^ (h2 << 1);
    }
};
```

- `std::unordered_map<exact_route, Handler, exact_route_hash>` **exact_routes_**
  - 精确的路由
- `std::vector<prefix_route>` **prefix_routes_**
  - 前缀路由

#### 成员函数

##### public

- **add_exact_route**
  > 注册精确路由
  - **args**
    - `http::verb`
    - `const std::string&`
    - `Handler`

- **add_prefix_route**
  > 注册前缀路由
  - **args**
    - `http::verb method`
    - `const std::string&`
    - `handler`

- **match**
  > 路由匹配，返回对应的处理器
  - **args**
    - `const http::request<http::string_body>&`


