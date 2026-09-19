#pragma once
// The filter's logging is diagnostics only; nothing asserts on it.
#define ESP_LOGE(tag, ...) ((void) 0)
#define ESP_LOGW(tag, ...) ((void) 0)
#define ESP_LOGI(tag, ...) ((void) 0)
#define ESP_LOGD(tag, ...) ((void) 0)
#define ESP_LOGVV(tag, ...) ((void) 0)
#define ESP_LOGCONFIG(tag, ...) ((void) 0)
#define YESNO(b) ((b) ? "YES" : "NO")
