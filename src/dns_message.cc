#include "dns_message.h"

#include <arpa/inet.h>

#include <cstring>

#include "util.h"

namespace boots {

static const char *IP_DOT_DELIM = ".";

size_t ParseName(std::string_view s, size_t offset, std::string *name);
std::string ParseIp(RecordType record_type, std::string_view data,
                    size_t offset, size_t length);

HeaderSection HeaderSection::BuildRequest() {
  HeaderSection header{true};
  header.questions = 1;
  header.flags.ra = 1;
  return header;
}

size_t HeaderSection::Deserialize(std::string_view s, bool *ok) {
  if (s.size() < sizeof(HeaderSection)) {
    *ok = false;
    return 0;
  }

  memcpy(this, s.data(), sizeof(HeaderSection));
  SwapSelf();
  *ok = true;
  return sizeof(HeaderSection);
}

void HeaderSection::Serialize(std::vector<uint8_t> *s) {
  auto len = s->size();
  s->resize(s->size() + sizeof(HeaderSection));
  SwapSelf();
  memcpy(s->data() + len, this, sizeof(HeaderSection));
}

void HeaderSection::SwapSelf() {
  str::SwapOrder(&request_id);
  str::SwapOrder(&flags);
  str::SwapOrder(&questions);
  str::SwapOrder(&answer_rr);
  str::SwapOrder(&authority_rr);
  str::SwapOrder(&additional_rr);
}

QuestionSection QuestionSection::BuildRequest(const std::string &hostname) {
  QuestionSection question{};
  question.qname = hostname;
  question.bin.qtype = RecordType::A;
  question.bin.qclass = RecordClass::kIn;
  return question;
}

size_t QuestionSection::Deserialize(std::string_view data, size_t offset) {
  size_t name_len = ParseName(data, offset, &qname);
  std::string_view s = data.substr(offset + name_len);
  memcpy(&bin, s.data(), sizeof(bin));
  str::SwapOrder(&bin.qtype);
  str::SwapOrder(&bin.qclass);
  return name_len + sizeof(bin);
}

void QuestionSection::Serialize(std::vector<uint8_t> *v) {
  v->reserve(v->size() + qname.size() + 2 + sizeof(bin));
  auto segments = str::Split(str::Trim(qname), IP_DOT_DELIM);
  for (auto &segment : segments) {
    v->push_back(static_cast<uint8_t>(segment.size()));
    v->insert(v->end(), segment.begin(), segment.end());
  }
  v->push_back(0);

  str::SwapOrder(&bin.qtype);
  str::SwapOrder(&bin.qclass);
  auto len = v->size();
  v->resize(v->size() + sizeof(bin));
  memcpy(v->data() + len, &bin.qtype, sizeof(bin.qtype));
  len += sizeof(bin.qtype);
  memcpy(v->data() + len, &bin.qclass, sizeof(bin.qclass));
}

size_t RecordSection::Deserialize(std::string_view data, size_t offset) {
  size_t name_len = ParseName(data, offset, &name);
  std::string_view s = data.substr(offset + name_len);
  memcpy(&bin, s.data(), sizeof(bin));
  str::SwapOrder(&bin.type);
  str::SwapOrder(&bin.clazz);
  str::SwapOrder(&bin.ttl);
  str::SwapOrder(&bin.rdlength);
  ip = ParseIp(bin.type, data, offset + name_len + sizeof(bin), bin.rdlength);
  return name_len + sizeof(bin) + bin.rdlength;
}

std::vector<uint8_t> SerializeDnsRequest(const std::string &hostname) {
  std::vector<uint8_t> res{};
  HeaderSection header{HeaderSection::BuildRequest()};
  header.Serialize(&res);

  QuestionSection question{QuestionSection::BuildRequest(hostname)};
  question.Serialize(&res);
  return res;
}

size_t ParseName(std::string_view s, size_t offset, std::string *name) {
  std::vector<std::string> labels{};
  size_t cur = offset;
  for (auto i = static_cast<uint8_t>(s[cur]); i > 0;
       i = static_cast<uint8_t>(s[cur])) {
    if ((i & 0b1100'0000) == 0b1100'0000) {
      uint16_t pointer{};
      memcpy(&pointer, s.data() + cur, sizeof(pointer));
      str::SwapOrder(&pointer);
      pointer &= 0x3FFF;
      std::string pointer_name{};
      ParseName(s, pointer, &pointer_name);
      labels.push_back(std::move(pointer_name));
      cur += 2;
      *name = str::Join(".", labels);
      return cur - offset;
    } else {
      ++cur;
      labels.emplace_back(s.substr(cur, i));
      cur += i;
    }
  }
  *name = str::Join(".", labels);
  return cur - offset + 1;
}

std::string ParseIp(RecordType record_type, std::string_view data,
                    size_t offset, size_t length) {
  std::string ip{};
  switch (record_type) {
    case RecordType::A: {
      ip.resize(INET_ADDRSTRLEN);
      inet_ntop(AF_INET, data.data() + offset, ip.data(), INET_ADDRSTRLEN);
      ip.resize(ip.find_first_of('\000'));
      break;
    }
    case RecordType::AAAA: {
      ip.resize(INET6_ADDRSTRLEN);
      inet_ntop(AF_INET6, data.data() + offset, ip.data(), INET6_ADDRSTRLEN);
      ip.resize(ip.find_first_of('\000'));
      break;
    }
    case RecordType::CNAME:
    case RecordType::NS: {
      ParseName(data, offset, &ip);
      break;
    }
    default: {
      ip.assign(data.substr(offset, length));
      break;
    }
  }
  return ip;
}

bool DeserializeDnsResponse(std::string_view s, DnsResponse *response) {
  HeaderSection header{};
  bool ok{};
  size_t offset{header.Deserialize(s, &ok)};
  if (!ok) {
    return false;
  }

  for (uint16_t i = 0; i < header.questions; ++i) {
    QuestionSection question{};
    offset += question.Deserialize(s, offset);
    response->questions.push_back(std::move(question));
  }

  for (uint16_t i = 0; i < header.answer_rr; ++i) {
    RecordSection record{};
    offset += record.Deserialize(s, offset);
    response->records.push_back(std::move(record));
  }
  return true;
}

}  // namespace boots
