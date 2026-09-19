// An independent AES-128 forward cipher for the host tests, written separately
// from ESPHome's so that an IRK test passing here says something about the
// filter's byte ordering rather than about one implementation agreeing with
// itself. Checked against FIPS-197 in the test suite.
#include "esphome/components/ble_device_base/ble_aes_ccm.h"

#include <cstring>

namespace {
uint8_t sbox[256];
bool sbox_ready = false;
uint8_t gmul(uint8_t a, uint8_t b) {
  uint8_t p = 0;
  for (int i = 0; i < 8; i++) {
    if (b & 1) p ^= a;
    const bool hi = a & 0x80;
    a = static_cast<uint8_t>(a << 1);
    if (hi) a ^= 0x1b;
    b >>= 1;
  }
  return p;
}
uint8_t rotl8(uint8_t x, int s) { return static_cast<uint8_t>((x << s) | (x >> (8 - s))); }
void init_sbox() {
  // Generated from the field inverse + affine transform rather than pasted.
  uint8_t p = 1, q = 1;
  do {
    p = static_cast<uint8_t>(p ^ (p << 1) ^ ((p & 0x80) ? 0x1b : 0));
    q ^= static_cast<uint8_t>(q << 1);
    q ^= static_cast<uint8_t>(q << 2);
    q ^= static_cast<uint8_t>(q << 4);
    if (q & 0x80) q ^= 0x09;
    sbox[p] = static_cast<uint8_t>(q ^ rotl8(q, 1) ^ rotl8(q, 2) ^ rotl8(q, 3) ^ rotl8(q, 4) ^ 0x63);
  } while (p != 1);
  sbox[0] = 0x63;
  sbox_ready = true;
}
}  // namespace

namespace esphome::ble_device_base {
void aes128_encrypt_block(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]) {
  if (!sbox_ready) init_sbox();
  uint8_t rk[176];
  std::memcpy(rk, key, 16);
  uint8_t rcon = 1;
  for (int i = 16; i < 176; i += 4) {
    uint8_t t[4] = {rk[i - 4], rk[i - 3], rk[i - 2], rk[i - 1]};
    if (i % 16 == 0) {
      const uint8_t t0 = t[0];
      t[0] = static_cast<uint8_t>(sbox[t[1]] ^ rcon);
      t[1] = sbox[t[2]]; t[2] = sbox[t[3]]; t[3] = sbox[t0];
      rcon = gmul(rcon, 2);
    }
    for (int j = 0; j < 4; j++) rk[i + j] = static_cast<uint8_t>(rk[i - 16 + j] ^ t[j]);
  }
  uint8_t s[16];
  for (int i = 0; i < 16; i++) s[i] = static_cast<uint8_t>(in[i] ^ rk[i]);
  for (int round = 1; round <= 10; round++) {
    uint8_t t[16];
    for (int c = 0; c < 4; c++)
      for (int r = 0; r < 4; r++) t[c * 4 + r] = sbox[s[((c + r) % 4) * 4 + r]];  // SubBytes + ShiftRows
    if (round < 10) {
      for (int c = 0; c < 4; c++) {
        const uint8_t *a = t + c * 4;
        s[c * 4 + 0] = static_cast<uint8_t>(gmul(a[0], 2) ^ gmul(a[1], 3) ^ a[2] ^ a[3]);
        s[c * 4 + 1] = static_cast<uint8_t>(a[0] ^ gmul(a[1], 2) ^ gmul(a[2], 3) ^ a[3]);
        s[c * 4 + 2] = static_cast<uint8_t>(a[0] ^ a[1] ^ gmul(a[2], 2) ^ gmul(a[3], 3));
        s[c * 4 + 3] = static_cast<uint8_t>(gmul(a[0], 3) ^ a[1] ^ a[2] ^ gmul(a[3], 2));
      }
    } else {
      std::memcpy(s, t, 16);
    }
    for (int i = 0; i < 16; i++) s[i] ^= rk[round * 16 + i];
  }
  std::memcpy(out, s, 16);
}
}  // namespace esphome::ble_device_base
