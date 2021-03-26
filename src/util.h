#pragma once
#include <climits>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace boots {

namespace str {

extern const char *kDelims;

std::string_view LeftTrim(std::string_view s,
                          std::string_view delims = kDelims);
std::string_view RightTrim(std::string_view s,
                           std::string_view delims = kDelims);
std::string_view Trim(std::string_view s, std::string_view delims = kDelims);

std::vector<std::string_view> Split(std::string_view s,
                                    std::string_view delims = kDelims);

template <typename T> void SwapEndian(T *u) {
  static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");
  union Union {
    T v{};
    unsigned char u8[sizeof(T)];
    Union() {}
  } source, dest;
  source.v = *u;
  for (size_t k = 0; k < sizeof(T); k++)
    dest.u8[k] = source.u8[sizeof(T) - k - 1];
  *u = dest.v;
}
} // namespace str

namespace file {

class Lines {
public:
  class Iterator {
  public:
    using iterator_tag = std::forward_iterator_tag;

    Iterator();
    explicit Iterator(const std::string &filename);
    ~Iterator();

    const std::string &operator*() const { return line_; }
    Iterator &operator++();
    friend bool operator==(const Iterator &lhs, const Iterator &rhs);
    friend bool operator!=(const Iterator &lhs, const Iterator &rhs);

  private:
    bool eof_{true};
    std::string line_{};
    std::ifstream file_;
  };

  explicit Lines(std::string_view filename) : filename_{filename} {}
  Iterator begin() const { return Iterator{filename_}; }
  Iterator end() const { return Iterator{}; }

private:
  std::string filename_;
};

} // namespace file

} // namespace boots
