#pragma once
#include <bit>
#include <climits>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace boots {

namespace str {

extern const char *kDefaultDelimiters;

std::string_view LeftTrim(std::string_view s,
                          std::string_view delimiters = kDefaultDelimiters);
std::string_view RightTrim(std::string_view s,
                           std::string_view delimiters = kDefaultDelimiters);
std::string_view Trim(std::string_view s, std::string_view delimiters = kDefaultDelimiters);

std::vector<std::string_view> Split(std::string_view s,
                                    std::string_view delimiters = kDefaultDelimiters);
std::string Join(std::string_view delimiter, const std::vector<std::string> &v);

namespace {
template <size_t> struct GenInt {};
template <> struct GenInt<sizeof(uint8_t)> { using type = uint8_t; };
template <> struct GenInt<sizeof(uint16_t)> { using type = uint16_t; };
template <> struct GenInt<sizeof(uint32_t)> { using type = uint32_t; };
template <> struct GenInt<sizeof(uint64_t)> { using type = uint64_t; };

template <size_t kSize> using GenIntType = typename GenInt<kSize>::type;

template <typename T> T SwapOrder(T val) {
  if constexpr (std::endian::little != std::endian::native) {
    return;
  }

  T swapped{};
  constexpr auto totalBytes = sizeof(T);
  using IntType = GenIntType<totalBytes>;
  auto *src = reinterpret_cast<IntType *>(&val);
  auto *dest = reinterpret_cast<IntType *>(&swapped);
  for (int i = 0; i < totalBytes; ++i) {
    *dest |= (*src >> (8 * (totalBytes - i - 1)) & 0xFF) << (8 * i);
  }
  return swapped;
}

} // namespace

template <typename T> void SwapOrder(T *val) { *val = SwapOrder(*val); }

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
  [[nodiscard]] Iterator begin() const { return Iterator{filename_}; }
  [[nodiscard]] Iterator end() const { return Iterator{}; }

private:
  std::string filename_;
};

} // namespace file

} // namespace boots
