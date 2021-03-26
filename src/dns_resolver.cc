#include "boots/dns_resolver.h"
#include "boots/event_loop.h"
#include "net.h"
#include "util.h"
#include <fstream>
#include <memory>
#include <random>
#include <regex>
#include <spdlog/spdlog.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/socket.h>

namespace boots {

static const char *IP_DOT_DELIM = ".";

#pragma pack(push, 1)
struct HeaderSection {
  uint16_t request_id{};
  struct Flags {
    unsigned char qr : 1 {};
    unsigned char op_code : 4 {};
    unsigned char aa : 1 {};
    unsigned char tc : 1 {};
    unsigned char rd : 1 {};
    unsigned char ra : 1 {};
    unsigned char z : 1 {};
    unsigned char ad : 1 {};
    unsigned char cd : 1 {};
    unsigned char rcode : 4 {};
  } flags{};
  uint16_t questions{};
  uint16_t answer_rr{};
  uint16_t authority_rr{};
  uint16_t additonal_rr{};
  HeaderSection() : request_id{static_cast<decltype(request_id)>(rand())} {}
  std::vector<uint8_t> ToPlain();
};

struct QuestionSection {
  std::string qname{};
  uint16_t qtype{};
  uint16_t qclass{};
  std::vector<uint8_t> ToPlain();
};
#pragma pack(pop)

enum class QuestionType : uint16_t {
  kA = 1,
};

enum class QuestionClass : uint16_t {
  kIn = 1,
};

void InplaceHostToNetwork(uint16_t *x) { *x = htons(*x); }

std::vector<uint8_t> HeaderSection::ToPlain() {
  std::vector<uint8_t> res{};
  res.resize(sizeof(HeaderSection));
  str::SwapEndian(&request_id);
  str::SwapEndian(&flags);
  str::SwapEndian(&questions);
  str::SwapEndian(&answer_rr);
  str::SwapEndian(&authority_rr);
  str::SwapEndian(&additonal_rr);
  memcpy(res.data(), this, sizeof(HeaderSection));
  return res;
}

std::vector<uint8_t> QuestionSection::ToPlain() {
  std::vector<uint8_t> res{};
  res.reserve(qname.size() + 2 + sizeof(qtype) + sizeof(qclass));
  auto segments = str::Split(str::Trim(qname), IP_DOT_DELIM);
  for (auto &segment : segments) {
    res.push_back(static_cast<uint8_t>(segment.size()));
    res.insert(res.end(), segment.begin(), segment.end());
  }
  res.push_back(0);

  str::SwapEndian(&qtype);
  str::SwapEndian(&qclass);
  auto pos = res.size();
  res.resize(pos + sizeof(qtype) + sizeof(qclass));
  memcpy(res.data() + pos, &qtype, sizeof(qtype));
  pos += sizeof(qtype);
  memcpy(res.data() + pos, &qclass, sizeof(qclass));
  return res;
}

DnsResolver::DnsResolver(EventLoop *loop, std::vector<std::string> servers)
    : loop_{loop}, servers_{std::move(servers)} {}

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

  if (net::IsIp(hostname)) {
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
  HeaderSection header{};
  header.questions = 1;
  header.flags.ra = 1;
  auto plain = header.ToPlain();

  QuestionSection question{};
  question.qname = hostname;
  question.qtype = static_cast<uint16_t>(QuestionType::kA);
  question.qclass = static_cast<uint16_t>(QuestionClass::kIn);
  auto question_plain = question.ToPlain();
  plain.insert(plain.end(), question_plain.begin(), question_plain.end());

  for (const auto &server : servers_) {
    struct sockaddr_in sa {};
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = 53;
    str::SwapEndian(&sa.sin_port);
    inet_pton(AF_INET, server.data(), &sa.sin_addr);
    int n = sendto(fd_, plain.data(), plain.size(), 0, (struct sockaddr *)&sa,
           sizeof(sa));
  }
}

void DnsResolver::Callback(int fd, uint32_t events) {
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
    spdlog::error("[DnsResolver] recv length is not equal to peeked length, "
                  "n={}, errno={}",
                  n, errno);
  }

  spdlog::info("length: {}", n);
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
    if (net::IsIpv4(server)) {
      servers_.emplace_back(server);
    }
  }

  if (servers_.empty()) {
    servers_.assign({"8.8.8.8", "8.8.4.4"});
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
    if (!net::IsIp(ip)) {
      continue;
    }

    for (size_t i = 1; i < parts.size(); ++i) {
      std::string hostname{parts[i]};
      hostnames_.insert({hostname, ip});
    }
  }
}

} // namespace boots
