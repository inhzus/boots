#pragma once

#include <cstdint>
#include <random>
#include <string_view>

namespace boots {

enum class RecordType : uint16_t {
  A = 1,
  AAAA = 28,
  CNAME = 5,
  NS = 2,
};

enum class RecordClass : uint16_t {
  kIn = 1,
};

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
  uint16_t additional_rr{};
  explicit HeaderSection(bool rand_request_id = false)
      : request_id{rand_request_id ? static_cast<decltype(request_id)>(rand())
                                   : uint16_t{}} {}

  static HeaderSection BuildRequest();
  size_t Deserialize(std::string_view s, bool *ok);

  void Serialize(std::vector<uint8_t> *s);

 private:
  void SwapSelf();
};

struct QuestionSection {
  std::string qname{};
  struct {
    RecordType qtype{};
    RecordClass qclass{};
  } bin;

  static QuestionSection BuildRequest(const std::string &hostname);
  size_t Deserialize(std::string_view data, size_t offset);

  void Serialize(std::vector<uint8_t> *v);
};

struct RecordSection {
  std::string name{};
  std::string ip{};
  struct {
    RecordType type{};
    RecordClass clazz{};
    int32_t ttl{};
    uint16_t rdlength{};
  } bin;

  size_t Deserialize(std::string_view data, size_t offset);
};
#pragma pack(pop)

struct DnsResponse {
  std::vector<QuestionSection> questions{};
  std::vector<RecordSection> records{};

  bool Deserialize(std::string_view s);
};

std::vector<uint8_t> SerializeDnsRequest(const std::string &hostname);

}  // namespace boots
