#include "util.h"

#include <numeric>
#include <string>

namespace boots {
namespace str {

const char *kDefaultDelimiters = " \t";

std::string_view LeftTrim(std::string_view s, std::string_view delimiters) {
  const auto pos = s.find_first_not_of(delimiters);
  if (std::string_view::npos != pos) {
    s = s.substr(pos);
  }
  return s;
}

std::string_view RightTrim(std::string_view s, std::string_view delimiters) {
  const auto pos = s.find_last_not_of(delimiters);
  if (std::string_view::npos != pos) {
    s = s.substr(0, pos + 1);
  }
  return s;
}

std::string_view Trim(std::string_view s, std::string_view delimiters) {
  s = LeftTrim(s, delimiters);
  return RightTrim(s, delimiters);
}

std::vector<std::string_view> Split(std::string_view s,
                                    std::string_view delimiters) {
  std::vector<std::string_view> res{};
  size_t cur = 0;
  while (cur < s.size() && cur != std::string_view::npos) {
    const auto next = s.find_first_of(delimiters, cur);
    if (next == std::string_view::npos) {
      res.emplace_back(s.substr(cur, s.size() - cur));
      break;
    }
    res.emplace_back(s.substr(cur, next - cur));
    cur = s.find_first_not_of(delimiters, next);
  }
  return res;
}

std::string Join(std::string_view delimiter,
                 const std::vector<std::string> &v) {
  if (v.empty()) return std::string();
  return std::accumulate(
      v.begin() + 1, v.end(), v[0],
      [delimiter](const std::string &lhs, const std::string &rhs) {
        std::string s{lhs};
        s.reserve(lhs.size() + rhs.size() + delimiter.size());
        return s.append(delimiter).append(rhs);
      });
}

}  // namespace str

namespace file {
Lines::Iterator::Iterator() : file_{} {}
Lines::Iterator::Iterator(const std::string &filename)
    : file_{filename, std::ifstream::in} {
  eof_ = !file_.is_open();
  ++*this;
}
Lines::Iterator::~Iterator() {
  if (file_.is_open()) {
    file_.close();
  }
}

Lines::Iterator &Lines::Iterator::operator++() {
  if (eof_) {
    return *this;
  }

  if (!std::getline(file_, line_)) {
    eof_ = true;
  }
  return *this;
}

bool operator==(const Lines::Iterator &lhs, const Lines::Iterator &rhs) {
  return lhs.eof_ && rhs.eof_;
}
bool operator!=(const Lines::Iterator &lhs, const Lines::Iterator &rhs) {
  return !(lhs.eof_ && rhs.eof_);
}

}  // namespace file

}  // namespace boots
