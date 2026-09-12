static const uint32_t ESPRESSIF_OUIS[] = {
    0x004B12, 0x007007, 0x00FB4A, 0x048308, 0x04B247, 0x083A8D,
    0x083AF2, 0x089272, 0x08A6F7, 0x08AD0A, 0x08B61F, 0x08D1F9,
    0x08F9E0, 0x0C4EA0, 0x0C8B95, 0x0CB815, 0x0CDC7E, 0x10003B,
    0x10061C, 0x1020BA, 0x1051DB, 0x10521C, 0x1091A8, 0x1097BD,
    0x10B41D, 0x10BDA3, 0x140808, 0x142B2F, 0x14335C, 0x146393,
    0x14C19F, 0x188B0E, 0x18FE34, 0x1C2904, 0x1C6920, 0x1C8B84,
    0x1C8F57, 0x1C9DC2, 0x1CC3AB, 0x1CDBD4, 0x1CE4CB, 0x202565,
    0x2043A8, 0x20500D, 0x206EF1, 0x209BA9, 0x20D5C2, 0x20E7C8,
    0x240AC4, 0x244CAB, 0x24587C, 0x2462AB, 0x246F28, 0x24A160,
    0x24B2DE, 0x24D7EB, 0x24DCC3, 0x24EC4A, 0x2805A5, 0x28372F,
    0x28562F, 0x288485, 0x2C3AE8, 0x2CBCBB, 0x2CF432, 0x3030F9,
    0x3076F5, 0x308398, 0x30AEA4, 0x30C6F7, 0x30C922, 0x30EDA0,
    0x345F45, 0x348518, 0x34865D, 0x349454, 0x34987A, 0x34AB95,
    0x34B472, 0x34B7DA, 0x34CDB0, 0x38182B, 0x383E51, 0x3844BE,
    0x3C0D0D, 0x3C0F02, 0x3C6105, 0x3C71BF, 0x3C8427, 0x3C8A1F,
    0x3CDC75, 0x3CE90E, 0x4022D8, 0x404CCA, 0x409151, 0x40F520,
    0x441793, 0x441BF6, 0x441D64, 0x447B30, 0x44B176, 0x44BD8D,
    0x4827E2, 0x4831B7, 0x483FDA, 0x485519, 0x489D31, 0x48AFF3,
    0x48CA43, 0x48E729, 0x48F6EE, 0x4C11AE, 0x4C7525, 0x4CC382,
    0x4CEBD6, 0x500291, 0x50787D, 0x543204, 0x5443B2, 0x545AA6,
    0x549DEA, 0x582ABD, 0x588C81, 0x58BF25, 0x58CF79, 0x58E6C5,
    0x5C013B, 0x5CCF7F, 0x600194, 0x6055F9, 0x648914, 0x64B708,
    0x64E833, 0x680947, 0x6825DD, 0x686725, 0x689DD2, 0x68B6B3,
    0x68C63A, 0x68EE8F, 0x68FE71, 0x6C3DD8, 0x6CB456, 0x6CC840,
    0x70039F, 0x70041D, 0x704BCA, 0x70AF09, 0x70B8F6, 0x744DBD,
    0x781C3C, 0x782184, 0x78421C, 0x78E36D, 0x78EE4C, 0x7C0C5F,
    0x7C2C67, 0x7C4FAD, 0x7C7398, 0x7C87CE, 0x7C9EBD, 0x7CD544,
    0x7CDFA1, 0x7CE8B1, 0x80456B, 0x8053E0, 0x80646F, 0x806599,
    0x807D3A, 0x80B54E, 0x80F1B2, 0x80F3DA, 0x840D8E, 0x841FE8,
    0x84C7BB, 0x84CCA8, 0x84F3EB, 0x84F703, 0x84FCE6, 0x8813BF,
    0x8856A6, 0x885721, 0x88F155, 0x8C4B14, 0x8C4F00, 0x8C8C29,
    0x8C94DF, 0x8CAAB5, 0x8CBFEA, 0x8CCE4E, 0x8CFD49, 0x901506,
    0x90380C, 0x90649B, 0x907069, 0x9097D5, 0x90B339, 0x90DA72,
    0x90E5B1, 0x943CC6, 0x9451DC, 0x9454C5, 0x94A990, 0x94B555,
    0x94B97E, 0x94E686, 0x983DAE, 0x9888E0, 0x98A316, 0x98C377,
    0x98CDAC, 0x98F4AB, 0x9C139E, 0x9C96D5, 0x9C9C1F, 0x9C9E6E,
    0x9CCC01, 0xA020A6, 0xA0764E, 0xA085E3, 0xA0A3B3, 0xA0B765,
    0xA0DD6C, 0xA0F262, 0xA47B9D, 0xA4CB8F, 0xA4CF12, 0xA4E57C,
    0xA4F00F, 0xA8032A, 0xA842E3, 0xA84674, 0xA848FA, 0xA8FD07,
    0xAC0BFB, 0xAC1518, 0xAC276E, 0xAC67B2, 0xACA704, 0xACD074,
    0xACEBE6, 0xB03FD3, 0xB08184, 0xB0A604, 0xB0A732, 0xB0B21C,
    0xB0CBD8, 0xB43A45, 0xB48A0A, 0xB4A64A, 0xB4BFE9, 0xB4E62D,
    0xB81F3F, 0xB87B4D, 0xB8BB11, 0xB8D61A, 0xB8F009, 0xB8F862,
    0xBCDDC2, 0xBCFF4D, 0xC049EF, 0xC04E30, 0xC05D89, 0xC0CDD6,
    0xC44F33, 0xC45BBE, 0xC49E7E, 0xC4D8D5, 0xC4DD57, 0xC4DEE2,
    0xC82B96, 0xC82E18, 0xC88541, 0xC88A7B, 0xC8C9A3, 0xC8DA29,
    0xC8F09E, 0xCC50E3, 0xCC68C7, 0xCC7B5C, 0xCC7E1F, 0xCC8DA2,
    0xCCBA97, 0xCCDBA7, 0xD09AAF, 0xD0CF13, 0xD0EF76, 0xD40592,
    0xD48AFC, 0xD48C49, 0xD4D4DA, 0xD4E9F4, 0xD4F98D, 0xD8132A,
    0xD83BDA, 0xD885AC, 0xD8A01D, 0xD8BC38, 0xD8BFC0, 0xD8F15B,
    0xDC0675, 0xDC0A69, 0xDC1ED5, 0xDC4F22, 0xDC5475, 0xDC55B1,
    0xDCB4D9, 0xDCDA0C, 0xE05A1B, 0xE072A1, 0xE08CFE, 0xE09806,
    0xE0E2E6, 0xE465B8, 0xE4B063, 0xE4B323, 0xE80690, 0xE831CD,
    0xE83DC1, 0xE868E7, 0xE86BEA, 0xE89F6D, 0xE8DB84, 0xE8F60A,
    0xEC6260, 0xEC64C9, 0xEC94CB, 0xECC9FF, 0xECD61B, 0xECDA3B,
    0xECE334, 0xECFABC, 0xF008D1, 0xF0161D, 0xF024F9, 0xF09E9E,
    0xF0F5BD, 0xF412FA, 0xF42DC9, 0xF4650B, 0xF4CFA2, 0xF843EE,
    0xF85B1B, 0xF8B3B7, 0xFC012C, 0xFCB467, 0xFCE8C0, 0xFCF5C4,
};
static constexpr size_t ESPRESSIF_OUI_COUNT = sizeof(ESPRESSIF_OUIS) / sizeof(ESPRESSIF_OUIS[0]);

bool BLEAdvertFilter::is_espressif_oui_(uint64_t addr) {
  const uint32_t oui = static_cast<uint32_t>((addr >> 24) & 0xFFFFFF);
  size_t lo = 0, hi = ESPRESSIF_OUI_COUNT;
  while (lo < hi) {
    const size_t mid = lo + (hi - lo) / 2;
    if (ESPRESSIF_OUIS[mid] < oui) {
      lo = mid + 1;
    } else if (ESPRESSIF_OUIS[mid] > oui) {
      hi = mid;
    } else {
      return true;
    }
  }
  return false;
}

bool BLEAdvertFilter::payload_blocked_(const uint8_t *data, uint16_t len) const {
  // AD structures are [length][type][payload...], length covering type+payload.
  // One pass checks both the local name and the manufacturer id so a dropped
  // advertisement is never walked twice.
  uint16_t i = 0;
  while (i < len) {
    const uint8_t field_len = data[i];
    if (field_len == 0)
      break;  // Zero length terminates the payload.
    // Reject a structure that claims to run past the buffer (truncated packet).
    if (static_cast<uint32_t>(i) + 1u + field_len > len)
      break;
    const uint8_t type = data[i + 1];
    // 0xFF manufacturer specific data: first two payload bytes are the
    // Bluetooth SIG company identifier, little-endian.
    if (type == 0xFF && field_len >= 3) {
      const uint16_t company = static_cast<uint16_t>(data[i + 2]) | (static_cast<uint16_t>(data[i + 3]) << 8);
      // HomeKit accessories advertise under Apple's company id with subtype
      // 0x06. Blocklisting Apple to kill phone/AirPods/AirTag noise would take
      // them with it, and they carry no IRK to rescue them, so exempt HAP
      // explicitly rather than forcing users to choose between the two.
      const bool is_hap = this->allow_homekit_ && company == 0x004C && field_len >= 4 && data[i + 4] == 0x06;
      if (!is_hap) {
        for (const uint16_t blocked : this->manufacturer_blocklist_) {
          if (blocked == company)
            return true;
        }
      }
    }
    // 0x09 complete local name, 0x08 shortened local name.
    if ((type == 0x09 || type == 0x08) && field_len > 1) {
      const char *name = reinterpret_cast<const char *>(&data[i + 2]);
      const uint8_t name_len = field_len - 1;
      for (const char *needle : this->name_blocklist_) {
        const size_t needle_len = strlen(needle);
        if (needle_len == 0 || needle_len > name_len)
          continue;
        // Case-insensitive substring search; the name is not NUL-terminated,
        // so this walks it by length rather than using strstr().
        for (uint8_t start = 0; start + needle_len <= name_len; start++) {
          size_t k = 0;
          while (k < needle_len &&
                 static_cast<char>(tolower(static_cast<unsigned char>(name[start + k]))) == needle[k])
            k++;
          if (k == needle_len)
            return true;
        }
      }
    }
    i += field_len + 1;
  }
  return false;
}

// The Bluetooth Base UUID, big-endian, with the 16-bit slot (bytes 2-3) zeroed.
// A SIG-allocated short UUID advertised in 128-bit form is this with those two
// bytes filled in, so a device using the long form of 0xFFF6 still matches a
// 16-bit allowlist entry.
static const uint8_t BT_BASE_UUID[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                                         0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

bool BLEAdvertFilter::uuid128_matches_(const uint8_t *le_bytes) const {
  // Advertisements carry 128-bit UUIDs little-endian; flip to canonical order
  // once, then compare.
  uint8_t be[16];
  for (uint8_t k = 0; k < 16; k++)
    be[k] = le_bytes[15 - k];

  for (const auto &allowed : this->service_uuid128_) {
    if (memcmp(be, allowed.data(), 16) == 0)
      return true;
  }

  // Long form of a SIG short UUID: everything but bytes 2-3 matches the base.
  if (!this->service_uuid_allowlist_.empty() && memcmp(be, BT_BASE_UUID, 2) == 0 &&
      memcmp(be + 4, BT_BASE_UUID + 4, 12) == 0) {
    const uint16_t shortened = static_cast<uint16_t>(be[3]) | (static_cast<uint16_t>(be[2]) << 8);
    for (const uint16_t allowed : this->service_uuid_allowlist_) {
      if (allowed == shortened)
        return true;
    }
  }
  return false;
}

bool BLEAdvertFilter::payload_has_allowed_service_uuid_(const uint8_t *data, uint16_t len) const {
  // Same [length][type][payload...] walk as payload_blocked_. A service UUID can
  // appear in several places and a device in pairing mode does not consistently
  // use one, so every form is checked:
  //   0x02/0x03 incomplete/complete 16-bit UUID list   (n * 2 bytes)
  //   0x14      16-bit solicitation list               (n * 2 bytes)
  //   0x16      service data, 16-bit UUID              (2 bytes + data)
  //   0x06/0x07 incomplete/complete 128-bit UUID list  (n * 16 bytes)
  //   0x15      128-bit solicitation list              (n * 16 bytes)
  //   0x21      service data, 128-bit UUID             (16 bytes + data)
  const bool have16 = !this->service_uuid_allowlist_.empty();
  const bool have128 = !this->service_uuid128_.empty();
  uint16_t i = 0;
  while (i < len) {
    const uint8_t field_len = data[i];
    if (field_len == 0)
      break;  // Zero length terminates the payload.
    // Reject a structure that claims to run past the buffer (truncated packet).
    if (static_cast<uint32_t>(i) + 1u + field_len > len)
      break;
    const uint8_t type = data[i + 1];
    // Payload is the field minus its type byte. The list types carry a packed
    // array; the service-data types carry exactly one UUID then opaque bytes,
    // so those stop after the first entry.
    const uint8_t payload_len = field_len - 1;

    if (have16 && (type == 0x02 || type == 0x03 || type == 0x14 || type == 0x16)) {
      const uint8_t entries = payload_len / 2;
      const uint8_t limit = (type == 0x16) ? (entries > 0 ? 1 : 0) : entries;
      for (uint8_t p = 0; p < limit; p++) {
        const uint16_t uuid =
            static_cast<uint16_t>(data[i + 2 + p * 2]) | (static_cast<uint16_t>(data[i + 3 + p * 2]) << 8);
        for (const uint16_t allowed : this->service_uuid_allowlist_) {
          if (allowed == uuid)
            return true;
        }
      }
    }

    // The long form can match a 16-bit entry via the base UUID, so this arm runs
    // whenever either list is configured.
    if ((have128 || have16) && (type == 0x06 || type == 0x07 || type == 0x15 || type == 0x21)) {
      const uint8_t entries = payload_len / 16;
      const uint8_t limit = (type == 0x21) ? (entries > 0 ? 1 : 0) : entries;
      for (uint8_t p = 0; p < limit; p++) {
        if (this->uuid128_matches_(&data[i + 2 + p * 16]))
          return true;
      }
    }

    i += field_len + 1;
  }
  return false;
}

bool BLEAdvertFilter::address_is_rpa_(uint64_t addr, uint8_t addr_type) {
  // addr_type 0 is public; a public address is never resolvable no matter what
  // its top bits look like.
  if (addr_type == 0)
    return false;
  return (((addr >> 40) & 0xC0) == 0x40);
}

bool BLEAdvertFilter::address_is_non_resolvable_(uint64_t addr, uint8_t addr_type) {
  // Same guard as address_is_rpa_: a public address is never a private one, no
  // matter what its leading bits look like.
  if (addr_type == 0)
    return false;
  return (((addr >> 40) & 0xC0) == 0x00);
}

bool BLEAdvertFilter::irk_matches_(uint64_t addr) const {
  // Bluetooth Core "ah": hash = e(IRK, 0-padding | prand)[low 24 bits], where
  // the RPA is prand (top 3 bytes) | hash (bottom 3 bytes).
  uint8_t plaintext[16] = {0};
  uint8_t ciphertext[16];
  plaintext[13] = (addr >> 40) & 0xff;
  plaintext[14] = (addr >> 32) & 0xff;
  plaintext[15] = (addr >> 24) & 0xff;
  for (const auto &irk : this->irks_) {
    ble_device_base::aes128_encrypt_block(irk.data(), plaintext, ciphertext);
    if (ciphertext[15] == (addr & 0xff) && ciphertext[14] == ((addr >> 8) & 0xff) &&
        ciphertext[13] == ((addr >> 16) & 0xff))
      return true;
  }
  return false;
}