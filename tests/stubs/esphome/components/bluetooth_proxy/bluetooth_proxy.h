#pragma once
// The slice of upstream bluetooth_proxy (ESPHome 2026.10) the filter touches:
// the RawAdvertisement contract and the AdvertisementFilter slot.
#include <cstdint>
namespace esphome {
namespace ble_device_base {
struct RawAdvertisement {
  uint64_t address;
  const uint8_t *data;
  uint16_t data_len;
  int8_t rssi;
  uint8_t addr_type;
};
}  // namespace ble_device_base
namespace bluetooth_proxy {
struct AdvertisementFilter {
  void *instance{nullptr};
  bool (*fn)(void *instance, const ble_device_base::RawAdvertisement &adv){nullptr};
  bool is_set() const { return this->fn != nullptr; }
  bool should_forward(const ble_device_base::RawAdvertisement &adv) const { return this->fn(this->instance, adv); }
};
class BluetoothProxy {
 public:
  void set_advertisement_filter(AdvertisementFilter filter) { this->advertisement_filter_ = filter; }
  AdvertisementFilter advertisement_filter_{};
};
}  // namespace bluetooth_proxy
}  // namespace esphome
