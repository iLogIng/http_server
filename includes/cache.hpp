#pragma once

#include <optional>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <list>
#include <string>

#include "config.hpp"

namespace server_cache
{

template<typename Key, typename Value>
class lru_cache
{
    using key_type = Key;
    using value_type = Value;
    using list_type = std::list<std::pair<key_type, value_type>>;
    using list_iterator = typename list_type::iterator;
    using map_type = std::unordered_map<key_type, list_iterator>;
    using map_iterator = typename map_type::iterator;
private:
    const server_config::configuration& config_;

private:
    mutable std::mutex mutex_;

    list_type items_;
    map_type lookup_;

public:

    explicit lru_cache(const server_config::configuration& config);

    lru_cache(const lru_cache&) = delete;
    lru_cache& operator =(const lru_cache&) = delete;
    lru_cache(lru_cache&&) = delete;
    lru_cache& operator =(lru_cache&&) = delete;

public:
    
    // 查询
    std::optional<value_type> get(const key_type& key);
    // 存入
    void put(key_type key, value_type value);
    // 删除
    bool erase(const key_type& key);
    // 清空
    void clear();

    // 判空
    bool empty() const;
    // 大小
    std::size_t size() const;
};

template<typename K, typename V>
server_cache::lru_cache<K, V>::
lru_cache(const server_config::configuration& config)
    : config_(config)
{}

template<typename K, typename V>
std::optional<V>
server_cache::lru_cache<K, V>::
get(const key_type& key)
{
    std::lock_guard lock(mutex_);
    map_iterator map_itr = lookup_.find(key);
    // 资源存在性
    if (map_itr == lookup_.end()) {
        return std::nullopt;
    }
    // 将资源再次作为list.front
    items_.splice(items_.begin(), items_, map_itr->second);
    // lookup::pair<key, itr[list_iterator]>::second
    return map_itr->second->second;
}

template<typename K, typename V>
void
server_cache::lru_cache<K, V>::
put(key_type key, value_type value)
{
    // 不进行缓存
    if(config_.max_cache_entries() == 0) {
        return;
    }
    std::lock_guard lock(mutex_);
    map_iterator map_itr = lookup_.find(key);
    // 存在缓存内容
    if(map_itr != lookup_.end()) {
        map_itr->second->second = std::move(value);
        items_.splice(items_.begin(), items_, map_itr->second);
        return;
    }
    // 缓存超出配置
    if(items_.size() >= config_.max_cache_entries()) {
        auto evict = std::prev(items_.end());
        lookup_.erase(evict->first);
        items_.erase(evict);
    }
    // 将新资源作为list.front
    items_.emplace_front(std::move(key), std::move(value));
    lookup_[items_.front().first] = items_.begin();
}

template<typename K, typename V>
bool
server_cache::lru_cache<K, V>::
erase(const key_type& key)
{
    std::lock_guard lock(mutex_);
    map_iterator itr = lookup_.find(key);
    if(itr == lookup_.end()) {
        return false;
    }
    items_.erase(itr->second);
    lookup_.erase(itr);
    return true;
}

template<typename K, typename V>
void
server_cache::lru_cache<K, V>::
clear()
{
    std::lock_guard lock(mutex_);
    items_.clear();
    lookup_.clear();
}

template<typename K, typename V>
bool
server_cache::lru_cache<K, V>::
empty() const
{
    std::lock_guard lock(mutex_);
    return items_.empty();
}

template<typename K, typename V>
std::size_t
server_cache::lru_cache<K, V>::
size() const
{
    std::lock_guard lock(mutex_);
    return items_.size();
}

} // namespace server_cache