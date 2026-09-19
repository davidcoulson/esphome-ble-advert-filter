#pragma once
#include <cstdint>
namespace esphome::ble_device_base {
void aes128_encrypt_block(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]);
}
