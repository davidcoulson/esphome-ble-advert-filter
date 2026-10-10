#pragma once
#include <cstring>
// One flash slot shared by every preference object, which is all the tests
// need: a value saved before a "reboot" is what the next load() returns.
namespace esphome {
struct TestFlash {
  bool has{false};
  unsigned char bytes[16]{};
};
inline TestFlash &test_flash() {
  static TestFlash flash;
  return flash;
}
class ESPPreferenceObject {
 public:
  template<typename T> bool save(const T *src) {
    static_assert(sizeof(T) <= sizeof(TestFlash::bytes), "test flash slot too small");
    std::memcpy(test_flash().bytes, src, sizeof(T));
    test_flash().has = true;
    return true;
  }
  template<typename T> bool load(T *dest) {
    if (!test_flash().has)
      return false;
    std::memcpy(dest, test_flash().bytes, sizeof(T));
    return true;
  }
};
}  // namespace esphome
