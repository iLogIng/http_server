# 缓存模块

> ***/includes/cache.hpp***
> ***LRU 缓存实现***
>

> 依赖:
> ***C++17 <optional> <mutex> <unordered_map> <list>***
> ***config.hpp***
>

## 结构

所有定义都包含在 ***namespace server_cache*** 命名空间中

## 类实现

包含一个泛型 **lru_cache** 类

### lru_cache 模板类

```cpp
template<typename Key, typename Value>
class lru_cache { ... };
```

#### 数据结构

采用经典 LRU 双结构组合实现 O(1) 访问与淘汰：

- `std::list<std::pair<Key, Value>>` — 双向链表，前端为最近使用，后端为最久未使用
- `std::unordered_map<Key, list_iterator>` — 哈希表提供 O(1) 键值查找

#### 成员变量

##### private

- `const server_config::configuration&` **config_**
  - 配置引用，从中读取 `max_cache_entries()`
- `mutable std::mutex` **mutex_**
  - 互斥锁，保证多线程安全
- `list_type` **items_**
  - 链表存储键值对，顺序反映访问热度
- `map_type` **lookup_**
  - 哈希表映射键到链表节点迭代器

#### 成员函数

##### public

- **get(const key_type& key)**
  > 查询缓存，命中时将节点提升至链表前端（LRU 更新）
  - **return** `std::optional<value_type>` — 存在返回值，否则 `std::nullopt`

- **put(key_type key, value_type value)**
  > 存入/更新缓存条目。若 `max_cache_entries == 0` 则直接返回（缓存禁用）。
  > 已存在时更新值并提升至前端；满时淘汰最久未使用的条目。
  - **args**
    - `key_type key` — 按值传入，支持移动语义
    - `value_type value` — 按值传入，支持移动语义

- **erase(const key_type& key)**
  > 从缓存中删除指定条目
  - **return** `bool` — 条目存在并删除返回 `true`，不存在返回 `false`

- **clear()**
  > 清空所有缓存条目

- **empty()**
  > 判空
  - **return** `bool`

- **size()**
  > 返回当前缓存条目数
  - **return** `std::size_t`

#### 线程安全

所有公有方法均通过 `std::mutex` 保证线程安全。在服务器多线程环境下，多个 session 可安全并发访问同一缓存实例。

#### LRU 淘汰策略

当 `size() >= max_cache_entries()` 时，淘汰链表尾部节点（最久未访问），并在哈希表中同步删除对应键。新条目或命中的条目被移动到链表前端。

#### 配置关联

缓存容量通过 `config_.max_cache_entries()` 获取（默认 `0` = 禁用缓存）。当容量为 `0` 时，`put()` 直接返回，`get()` 永远返回 `std::nullopt`，零开销。

## 与 static_file_service 的集成

`static_file_service` 持有 `lru_cache<std::string, std::string>` 实例，将文件路径映射到文件内容。在 `handle_GET_request` 中：

1. 首先从缓存查询请求的文件路径
2. 命中 → 直接以 `string_body` 返回缓存的文件内容（零磁盘 I/O）
3. 未命中 → 从磁盘读入文件 → 写入缓存 → 以 `string_body` 返回

`handle_HEAD_request` 同样可查询缓存获得文件大小，避免打开文件。
