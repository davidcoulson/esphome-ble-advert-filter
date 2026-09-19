#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
namespace esphome {
// Same contract as esphome::parse_hex for the way the filter calls it: parse
// `len` hex characters into `count` bytes, return the number of characters
// consumed, 0 on a non-hex character.
inline size_t parse_hex(const char *str, size_t len, uint8_t *data, size_t count) {
  auto nib = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  if (len > count * 2) len = count * 2;
  for (size_t i = 0; i + 1 < len; i += 2) {
    const int hi = nib(str[i]), lo = nib(str[i + 1]);
    if (hi < 0 || lo < 0) return 0;
    data[i / 2] = static_cast<uint8_t>((hi << 4) | lo);
  }
  return len;
}
}  // namespace esphome
