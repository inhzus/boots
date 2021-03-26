#pragma once
#include <chrono>
#include <list>
#include <unordered_map>
#include <utility>

namespace boots {

template <typename K, typename V> class LRUCache {
public:
  using Clock = std::chrono::high_resolution_clock;
  using TimePoint = std::chrono::time_point<Clock>;
  struct Pair;
  using ListIt = typename std::list<Pair>::iterator;

  struct Pair {
    K key;
    V value;
    TimePoint expire;
    Pair(K key_, V value_, TimePoint expire_)
        : key{std::move(key_)}, value{std::move(value_)}, expire{expire_} {}
  };

  LRUCache(size_t seconds) : timeout_{seconds} {}

  void Put(const K &key, const V &value) {
    auto it = map_.find(key);
    auto now = Clock::now();
    list_.emplace_front(key, value, now + timeout_);
    if (it != map_.end()) {
      list_.erase(it->second);
      map_.erase(it);
    }
    map_.insert({key, list_.begin()});
    expire(now);
  }

  bool Get(const K &key, V *value) {
    auto it = map_.find(key);
    if (it == map_.end()) {
      return false;
    }

    auto now = Clock::now();
    it->second->expire = now + timeout_;
    list_.splice(list_.begin(), list_, it->second);
    expire(now);
    return true;
  }

  bool Exists(const K &key) const { return map_.find(key) != map_.end(); }

  bool Size() const { return map_.size(); }

private:
  void expire(TimePoint now) {
    while (!list_.empty()) {
      auto &back = *list_.rbegin();
      if (back.expire < now) {
        break;
      }
      map_.erase(back.key);
      list_.pop_back();
    }
  }

  std::chrono::seconds timeout_;
  std::list<Pair> list_{};
  std::unordered_map<K, ListIt> map_{};
};

} // namespace boots
