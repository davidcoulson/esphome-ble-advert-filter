#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/bluetooth_proxy/bluetooth_proxy.h"

#include <array>
#include <vector>

namespace esphome::ble_advert_filter {

/// On-device advertisement filtering for bluetooth_proxy.
///
/// Installs itself into the proxy's AdvertisementFilter slot, so advertisements
/// are dropped before they are queued for the API rather than being shipped
/// across the network for Home Assistant to discard.
///
/// Filters run cheapest-first; see should_forward().
class BLEAdvertFilter : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_BLUETOOTH; }

  void set_parent(bluetooth_proxy::BluetoothProxy *parent) { this->parent_ = parent; }

  /// Runtime-tunable: the matching number entity restores on boot and overwrites
  /// whatever the YAML set, so tune there rather than reflashing.
  void set_rssi_threshold(int8_t rssi) { this->rssi_threshold_ = rssi; }
  int8_t get_rssi_threshold() const { return this->rssi_threshold_; }

  void set_allow_espressif(bool allow) { this->allow_espressif_ = allow; }
  void set_drop_non_resolvable(bool drop) { this->drop_non_resolvable_ = drop; }
  void set_allow_homekit(bool allow) { this->allow_homekit_ = allow; }
  void set_irks_hex(const char *hex) { this->irks_hex_ = hex; }
  void add_blocked_name(const char *needle) { this->name_blocklist_.push_back(needle); }
  void add_blocked_manufacturer(uint16_t company) { this->manufacturer_blocklist_.push_back(company); }
  /// Address that bypasses every filter.
  void add_allowed_mac(uint64_t addr) { this->mac_allowlist_.push_back(addr); }
  /// Address this proxy ignores entirely, everything else proceeding normally.
  ///
  /// For multi-proxy bonding conflicts: when several proxies are in range of a
  /// device that requires bonding, only one can hold the bond cleanly and the
  /// rest cause connection thrashing. Home Assistant picks a proxy from the ones
  /// reporting the device, so dropping its advertisements here takes this proxy
  /// out of the running without turning it into a single-purpose bridge.
  void add_blocked_mac(uint64_t addr) { this->mac_blocklist_.push_back(addr); }
  /// Turn mac_allowlist from a bypass list into an exclusive one: nothing but
  /// those addresses is forwarded.
  ///
  /// Reproduces the observable behaviour of the ESP-IDF controller whitelist
  /// (esphome/esphome#14353) on platforms that have no such hardware path -
  /// rp2, bk72xx, ln882x. On ESP32 prefer that PR's esp32_ble filter when it
  /// lands: filtering in the controller also saves the CPU and power spent
  /// parsing packets, which a host-side filter like this one cannot.
  void set_allowlist_exclusive(bool exclusive) { this->allowlist_exclusive_ = exclusive; }
  void add_allowed_service_uuid(uint16_t uuid) { this->service_uuid_allowlist_.push_back(uuid); }
  void add_allowed_service_uuid128(const char *hex) { this->service_uuid128_hex_.push_back(hex); }

  uint32_t get_adv_forwarded() const { return this->adv_forwarded_; }
  uint32_t get_adv_dropped() const { return this->adv_dropped_; }
  uint32_t get_adv_dropped_rpa() const { return this->adv_dropped_rpa_; }
  uint32_t get_adv_allowed_service_uuid() const { return this->adv_allowed_service_uuid_; }

  /// The predicate itself. False drops the advertisement.
  bool should_forward(const ble_device_base::RawAdvertisement &adv);

 protected:
  static bool address_is_rpa_(uint64_t addr, uint8_t addr_type);
  static bool address_is_non_resolvable_(uint64_t addr, uint8_t addr_type);
  static bool is_espressif_oui_(uint64_t addr);
  bool irk_matches_(uint64_t addr) const;
  bool payload_blocked_(const uint8_t *data, uint16_t len) const;
  bool payload_has_allowed_service_uuid_(const uint8_t *data, uint16_t len) const;
  bool uuid128_matches_(const uint8_t *le_bytes) const;

  bluetooth_proxy::BluetoothProxy *parent_{nullptr};

  uint32_t adv_forwarded_{0};
  uint32_t adv_dropped_{0};
  uint32_t adv_dropped_rpa_{0};
  uint32_t adv_allowed_service_uuid_{0};

  const char *irks_hex_{nullptr};
  std::vector<std::array<uint8_t, 16>> irks_;
  std::vector<const char *> name_blocklist_;
  std::vector<uint64_t> mac_allowlist_;
  std::vector<uint64_t> mac_blocklist_;
  std::vector<uint16_t> service_uuid_allowlist_;
  std::vector<const char *> service_uuid128_hex_;
  std::vector<std::array<uint8_t, 16>> service_uuid128_;
  std::vector<uint16_t> manufacturer_blocklist_;

  int8_t rssi_threshold_{-127};
  bool allow_espressif_{true};
  bool drop_non_resolvable_{false};
  bool allow_homekit_{true};
  bool allowlist_exclusive_{false};
};

}  // namespace esphome::ble_advert_filter
