#include "boots/dns_resolver.h"

#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <memory>
#include <regex>
#include <sstream>

#include "boots/event_loop.h"
#include "dns_message.h"
#include "net.h"
#include "util.h"

namespace boots {
DnsResolver::DnsResolver(EventLoop *loop, std::vector<std::string> servers)
    : loop_{loop} {
  for (const auto &s : servers) {
    sockaddr_in sa{};
    if (net::ToIpv4(s, reinterpret_cast<sockaddr_storage *>(&sa))) {
      sa.sin_family = AF_INET;
      sa.sin_port = 53;
      str::SwapOrder(&sa.sin_port);
      servers_.emplace_back(sa);
    }
  }
}

void DnsResolver::Init() {
  if (servers_.empty()) {
    ParseResolv();
  }
  ParseHosts();
  if (loop_ != nullptr) {
    AddToLoop();
  }
}

void DnsResolver::Resolve(const std::string &hostname, CallbackFunc callback) {
  if (hostname.empty()) {
    callback(hostname, {}, "empty hostname");
    return;
  }

  if (net::ToIp(hostname)) {
    callback(hostname, hostname, {});
    return;
  }

  auto it = hostnames_.find(hostname);
  if (it != hostnames_.end()) {
    callback(hostname, it->second, {});
    return;
  }

  std::string ip{};
  if (cache_.Get(hostname, &ip)) {
    callback(hostname, ip, {});
    return;
  }

  auto cb_it = hostname_callbacks_.find(hostname);
  if (cb_it == hostname_callbacks_.end()) {
    hostname_callbacks_.insert({hostname, {std::move(callback)}});
  } else {
    cb_it->second.emplace_back(std::move(callback));
  }
  SendReq(hostname);
}

void DnsResolver::SendReq(const std::string &hostname) {
  auto plain = SerializeDnsRequest(hostname);

  for (const auto &server : servers_) {
    sendto(fd_, plain.data(), plain.size(), 0, (struct sockaddr *)&server,
           sizeof(server));
  }
}

void DnsResolver::Callback(int fd, uint32_t events) const {
  if (fd != fd_) {
    return;
  }

  if (events & EventLoop::kPollErr) {
    spdlog::error("[DnsResolver] callback socket error, errno={}", errno);
    return;
  }

  int size{};
  if (ioctl(fd_, FIONREAD, &size) != 0) {
    spdlog::error("[DnsResolver] ioctl read failed, errno={}", errno);
    return;
  }

  std::unique_ptr<char[]> buf{new char[size]};
  struct sockaddr_in sa {};
  socklen_t sa_len{};
  int n = recvfrom(fd_, buf.get(), size, 0, (struct sockaddr *)&sa, &sa_len);
  if (n != size) {
    spdlog::error(
        "[DnsResolver] recv length is not equal to peeked length, "
        "n={}, errno={}",
        n, errno);
  }

  spdlog::info("length: {}", n);
  std::string_view s(buf.get(), size);
  DnsResponse response{};
  DeserializeDnsResponse(s, &response);
  int i = 0;
  ++i;
}

void DnsResolver::AddToLoop() {
  fd_ = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  loop_->Add(fd_, EventLoop::kPollIn,
             [r = shared_from_this()](int fd, uint32_t events) {
               r->Callback(fd, events);
             });
}

void DnsResolver::ParseResolv() {
  file::Lines lines{"/etc/resolv.conf"};
  std::regex pattern{R"(^\s*nameserver\s+(\S+))"};
  std::smatch match{};
  for (auto it = lines.begin(); it != lines.end(); ++it) {
    if (!std::regex_search(*it, match, pattern)) {
      continue;
    }

    std::string server = match[1].str();
    sockaddr_in sa{};
    if (net::ToIpv4(server, reinterpret_cast<sockaddr_storage *>(&sa))) {
      sa.sin_family = AF_INET;
      sa.sin_port = 53;
      str::SwapOrder(&sa.sin_port);
      servers_.emplace_back(sa);
    }
  }

  if (servers_.empty()) {
    constexpr std::array<std::string_view, 2> kGoogleDnsServers{"8.8.8.8",
                                                                "4.4.4.4"};
    for (const auto &s : kGoogleDnsServers) {
      sockaddr_in sa{};
      net::ToIpv4(s, reinterpret_cast<sockaddr_storage *>(&sa));
      servers_.emplace_back(sa);
    }
  }
}

void DnsResolver::ParseHosts() {
  file::Lines lines{"/etc/hosts"};
  auto it = lines.begin();
  if (it == lines.end()) {
    hostnames_.insert({"localhost", "127.0.0.1"});
    return;
  }

  for (; it != lines.end(); ++it) {
    std::string_view s = str::Trim(*it);
    std::vector<std::string_view> parts = str::Split(s);
    if (parts.size() < 2) {
      continue;
    }
    std::string ip{parts[0]};
    if (!net::ToIp(ip)) {
      continue;
    }

    for (size_t i = 1; i < parts.size(); ++i) {
      std::string hostname{parts[i]};
      hostnames_.insert({hostname, ip});
    }
  }
}

}  // namespace boots
