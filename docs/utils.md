# 工具模块

> ***/src/utils.hpp***
> ***工具方法***
>

> 依赖
> ***logger.hpp***
> ***<unordered_map>***
>

为该 http_server 项目提供日志处理能力。

将日志信息存放在 `/logs` 目录下

## 结构

所有定义都包含在 ***namespace server_utils*** 命名空间中

### 静态变量

- ***static std::unordered_map<std::string, std::string> mime_types;
  - 该变量提供从文件后缀到mime类型的映射
  - 提供了用户自配置的基础

### 目标路径处理

- **mime_type(beast::string_view path)** 方法
  - 提供将文件扩展名与相应mime类型的映射，使用 ***mime_types*** 变量进行处理

- **is_safe_path(beast::string_view path)**
  - 防止路径遍历攻击，确保请求路径的安全性

- **std::string path_cat(beast::string_view base, beast::string_veiw path)**
  - 安全的将网站根目录与文件路径相连接，返回所处平台支持的路径字符串

### 响应码集成

- ***400 Bad Request***
  - *make_bad_request*

- ***404 Not Found***
  - *make_not_found*
  
- ***500 Internal Server Error***
  - *make_server_error*

