#include "util.h"

namespace boots {
namespace str {

const char *kDelims = " \t";

std::string_view LeftTrim(std::string_view s, std::string_view delims) {
  const auto pos = s.find_first_not_of(delims);
  if (std::string_view::npos != pos) {
    s = s.substr(pos);
  }
  return s;
}

std::string_view RightTrim(std::string_view s, std::string_view delims) {
  const auto pos = s.find_last_not_of(delims);
  if (std::string_view::npos != pos) {
    s = s.substr(0, pos + 1);
  }
  return s;
}

std::string_view Trim(std::string_view s, std::string_view delims) {
  s = LeftTrim(s, delims);
  return RightTrim(s, delims);
}

std::vector<std::string_view> Split(std::string_view s,
                                    std::string_view delims) {
  std::vector<std::string_view> res{};
  size_t cur = 0;
  while (cur < s.size() && cur != s.npos) {
    const auto next = s.find_first_of(delims, cur);
    if (next == s.npos) {
      res.emplace_back(s.substr(cur, s.size() - cur));
      break;
    }
    res.emplace_back(s.substr(cur, next - cur));
    cur = s.find_first_not_of(delims, next);
  }
  return res;
}

} // namespace str

namespace file {
Lines::Iterator::Iterator() : file_{} {}
Lines::Iterator::Iterator(const std::string &filename)
    : file_{filename, file_.in} {
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

} // namespace file

} // namespace boots
