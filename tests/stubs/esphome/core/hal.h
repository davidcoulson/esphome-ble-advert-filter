#pragma once
#include <cstdint>
// A clock the tests set by hand.
namespace esphome {
inline uint32_t &test_millis() {
  static uint32_t now = 0;
  return now;
}
inline uint32_t millis() { return test_millis(); }
}  // namespace esphome
