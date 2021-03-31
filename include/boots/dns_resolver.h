#pragma once
#include <arpa/inet.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "boots/lru_cache.h"

namespace boots {
class EventLoop;
class DnsResolver : public std::enable_shared_from_this<DnsResolver> {
 public:
  using CallbackFunc =
      std::function<void(const std::string &hostname, const std::string &ip,
                         const std::string &error)>;
  explicit DnsResolver(EventLoop *loop, std::vector<std::string> servers = {});
  void Init();
  void Resolve(const std::string &hostname, CallbackFunc callback);

 private:
  void SendReq(const std::string &hostname);

  void Callback(int fd, uint32_t events) const;
  void AddToLoop();
  void ParseResolv();
  void ParseHosts();

  EventLoop *loop_;
  int fd_{};
  std::unordered_map<std::string, std::string> hostnames_{};
  std::unordered_map<std::string, std::vector<CallbackFunc>>
      hostname_callbacks_{};
  std::vector<sockaddr_in> servers_{};
  LRUCache<std::string, std::string> cache_{300};
};
}  // namespace boots
