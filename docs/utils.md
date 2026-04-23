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

- ***static const std::unordered_map<std::string, std::string> mime_types;
  - 该变量提供从文件后缀到mime类型的映射（const 不可修改）
  - 提供了用户自配置的基础

### 目标路径处理

- **mime_type(beast::string_view path)** 方法
  - 提供将文件扩展名与相应mime类型的映射，使用 ***mime_types*** 变量进行处理

- **is_safe_path(beast::string_view path)**
  - 防止路径遍历攻击，确保请求路径的安全性

- **get_normalizaed_doc_root(const std::string& raw_root)**
  - 将网站根目录路径进行规范化，并添加根目录缓存功能，能缓存上一个处理的根目录路径，返回拷贝，暂时不考虑多线程优化

- **secure_file_cath(beast::string_view doc_root, beast::string_view target)**
  - 提供依赖 *boost::filesystem* 库的安全路径拼接功能，但使用boost::filesystem时，由于库接口设计的基础依赖，需要访问主机的文件系统，产生一定的开销，所以，在此之前的 *is_safe_path* 与 *path_cat* 方法所提供的轻量级路径处理方法仍有必要

- **std::string path_cat(beast::string_view base, beast::string_veiw path)**
  - 安全的将网站根目录与文件路径相连接，返回所处平台支持的路径字符串

### 响应码集成

- ***400 Bad Request***
  - *make_bad_request*

- ***404 Not Found***
  - *make_not_found*

- ***405 Method Not Allowed***
  - *make_method_not_allowed*

- ***413 Payload Too Large***
  - *make payload_too_large*
  
- ***500 Internal Server Error***
  - *make_server_error*

