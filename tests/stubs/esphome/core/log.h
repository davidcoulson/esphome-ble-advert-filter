#pragma once
// The filter's logging is diagnostics only; nothing asserts on it. The sink
// still consumes its arguments so the real source builds warning-free.
namespace esphome { inline void test_log_sink(const char *, const char *, ...) {} }
#define ESP_LOGE(tag, ...) ::esphome::test_log_sink(tag, __VA_ARGS__)
#define ESP_LOGW(tag, ...) ::esphome::test_log_sink(tag, __VA_ARGS__)
#define ESP_LOGI(tag, ...) ::esphome::test_log_sink(tag, __VA_ARGS__)
#define ESP_LOGD(tag, ...) ::esphome::test_log_sink(tag, __VA_ARGS__)
#define ESP_LOGVV(tag, ...) ::esphome::test_log_sink(tag, __VA_ARGS__)
#define ESP_LOGCONFIG(tag, ...) ::esphome::test_log_sink(tag, __VA_ARGS__)
#define YESNO(b) ((b) ? "YES" : "NO")
