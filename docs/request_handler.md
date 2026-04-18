# request_handler

> ***/includes/request_handler.hpp***
> ***日志类***
>

> 依赖:
> ***static_file_service.hpp***
>

## 结构

所有定义都包含在 ***namespace server_service*** 命名空间中

## 类实现

包含一个 **request_handler** 类提供静态文件响应功能

### request_handler

#### 成员变量

##### private

- `server_service::static_file_service` **static_file_service_**
  - 组合静态文件服务类，用于处理静态文件请求

#### 成员函数

##### public

- **config()**
  > 提供对配置信息的常量引用
  - **return**
    - `const server_config::configuration&`

#### 构造函数

目前经包含唯一的构造函数

- **(const server_config::configuration& config)**
  > 传入一个配置类的引用常量，初始化组合的静态文件服务对象
