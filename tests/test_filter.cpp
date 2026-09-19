// Host tests for BLEAdvertFilter.
//
// These compile the REAL components/ble_advert_filter/ble_advert_filter.cpp
// against small stubs (tests/stubs/) for the ESPHome APIs it touches, so
// nothing is transcribed and the tests cannot drift from the component. The
// AES behind IRK resolution is an independent implementation, checked here
// against FIPS-197 and the Bluetooth Core spec's own sample data.
//
// tests/run.sh builds this with AddressSanitizer and UBSan, which is what makes
// the payload fuzz at the bottom meaningful: every advert lives in an exactly
// sized heap buffer, so a one-byte overread aborts the run.
//
//   tests/run.sh

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "../components/ble_advert_filter/ble_advert_filter.h"
#include "esphome/components/ble_device_base/ble_aes_ccm.h"

using esphome::ble_advert_filter::BLEAdvertFilter;
using esphome::ble_device_base::RawAdvertisement;
using Bytes = std::vector<uint8_t>;

namespace {

int failures = 0;
void check(bool ok, const char *what) {
  std::printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok)
    failures++;
}

// ---- address helpers. Top two bits of a random address give its class. ----
constexpr uint8_t PUBLIC = 0, RANDOM = 1;
constexpr uint64_t STATIC_ADDR = 0xC01122334455ULL;      // 11.. static random
constexpr uint64_t NRPA_ADDR = 0x0A1122334455ULL;        // 00.. non-resolvable private
constexpr uint64_t STRANGER_RPA = 0x4A1122334455ULL;     // 01.. resolvable, resolves to nothing here
// Bluetooth Core spec Vol 3 Part H App. D.7: this IRK and prand 708194 give
// hash 0dfbaa, i.e. the RPA 70:81:94:0D:FB:AA.
const char *SPEC_IRK = "ec0234a357c8ad05341010a60a397d9b";
constexpr uint64_t SPEC_RPA = 0x7081940DFBAAULL;

// ---- payload builders ----
Bytes ad(uint8_t type, const Bytes &payload) {
  Bytes out{static_cast<uint8_t>(payload.size() + 1), type};
  out.insert(out.end(), payload.begin(), payload.end());
  return out;
}
Bytes operator+(Bytes a, const Bytes &b) {
  a.insert(a.end(), b.begin(), b.end());
  return a;
}
const Bytes FLAGS = ad(0x01, {0x06});
Bytes apple(uint8_t subtype, size_t extra = 4) {
  Bytes p{0x4C, 0x00, subtype};
  p.insert(p.end(), extra, 0xAB);
  return ad(0xFF, p);
}
Bytes ibeacon(uint16_t major, uint16_t minor) {
  Bytes p{0x4C, 0x00, 0x02, 0x15};
  p.insert(p.end(), 16, 0x11);  // UUID
  p.push_back(major >> 8); p.push_back(major & 0xff);
  p.push_back(minor >> 8); p.push_back(minor & 0xff);
  p.push_back(0xC5);            // measured power
  return ad(0xFF, p);
}
Bytes uuid16_list(uint16_t u) { return ad(0x03, {static_cast<uint8_t>(u & 0xff), static_cast<uint8_t>(u >> 8)}); }
Bytes name(const std::string &s) { return ad(0x09, Bytes(s.begin(), s.end())); }

struct Probe : BLEAdvertFilter {
  // Lets a test compare verdicts with the pre-gate forced off.
  void disable_gate() { this->min_rssi_gate_ = -127; }
};

struct Fixture {
  esphome::bluetooth_proxy::BluetoothProxy proxy;
  Probe f;
  Fixture() { f.set_parent(&proxy); }
  // The exact-size heap copy is what lets ASan see an overread.
  bool fwd(uint64_t addr, uint8_t type, int8_t rssi, const Bytes &data) {
    std::unique_ptr<uint8_t[]> buf(new uint8_t[data.size()]);
    if (!data.empty())  // an empty vector's data() may be null, which memcpy must not be given
      std::memcpy(buf.get(), data.data(), data.size());
    RawAdvertisement a{addr, buf.get(), static_cast<uint16_t>(data.size()), rssi, type};
    return f.should_forward(a);
  }
};

// The fleet's production config, as of ble-proxy.yaml 2026.09.19.0.
void fleet_config(Fixture &x) {
  x.f.set_rssi_threshold(-75);
  x.f.set_rssi_floor(-90);
  x.f.set_rssi_service_uuid(-90);
  x.f.set_drop_non_resolvable(true);
  x.f.add_blocked_manufacturer(0x004C);
  x.f.set_allow_homekit(true);
  x.f.set_allow_findmy(true);
  x.f.set_findmy_rssi(-85);
  x.f.set_allow_ibeacon(false);
  x.f.add_ibeacon_major(1, -95);
  x.f.add_allowed_mac(0xE3AC06854000ULL);
  x.f.add_allowed_service_uuid(0xFFF6);
  x.f.add_allowed_service_uuid(0xFEED);
  x.f.add_allowed_service_uuid128("00467768622822724663277478268000");
  x.f.set_irks_hex(SPEC_IRK);
  x.f.setup();
}

}  // namespace

int main() {
  std::printf("== AES and the RPA hash ==\n");
  {
    uint8_t k[16], in[16], out[16];
    for (int i = 0; i < 16; i++) { k[i] = static_cast<uint8_t>(i); in[i] = static_cast<uint8_t>(i * 0x11); }
    esphome::ble_device_base::aes128_encrypt_block(k, in, out);
    const uint8_t want[16] = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30, 0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};
    check(std::memcmp(out, want, 16) == 0, "test AES matches FIPS-197 C.1");
  }
  {
    Fixture x; x.f.set_irks_hex(SPEC_IRK); x.f.setup();
    check(x.fwd(SPEC_RPA, RANDOM, -60, FLAGS), "the Bluetooth spec's sample RPA resolves with the spec's IRK (keys are MSB-first hex)");
    check(x.f.get_adv_forwarded_irk() == 1, "and is counted as an IRK match");
    check(!x.fwd(SPEC_RPA ^ 1, RANDOM, -60, FLAGS), "one bit off in the hash does not resolve");
    check(!x.fwd(STRANGER_RPA, RANDOM, -30, FLAGS) && x.f.get_adv_dropped_rpa() == 2,
          "an unresolved RPA is dropped however close it is");
  }
  {
    Fixture x; x.f.setup();  // no IRKs configured
    check(x.fwd(STRANGER_RPA, RANDOM, -60, FLAGS), "with no IRKs, RPAs are not gated at all (upstream behaviour)");
  }
  {
    Fixture x; x.f.set_irks_hex(SPEC_IRK); x.f.setup();
    check(x.fwd(0x4C1122334455ULL, PUBLIC, -60, FLAGS), "a PUBLIC address in the RPA bit range (Espressif 4C:..) is never treated as an RPA");
  }

  {
    // ESP-IDF reports a device it is bonded to by its IDENTITY address: type 2
    // (public identity) or 3 (static random identity). Neither is private.
    Fixture x; x.f.set_irks_hex(SPEC_IRK); x.f.set_drop_non_resolvable(true); x.f.setup();
    check(x.fwd(0x4C1122334455ULL, 2, -60, FLAGS), "a bonded device's public identity address (type 2) with a 4C: OUI is not an unresolved RPA");
    check(x.fwd(0x0C1122334455ULL, 2, -60, FLAGS), "nor, with a 0C: OUI, a non-resolvable address");
    check(x.fwd(STATIC_ADDR, 3, -60, FLAGS), "a static random identity address (type 3) passes");
  }

  std::printf("\n== an unconfigured filter is a no-op ==\n");
  {
    Fixture x; x.f.setup();
    check(x.f.get_min_rssi_gate() == -127, "no gate");
    check(x.fwd(NRPA_ADDR, RANDOM, -127, apple(0x10) + name("anything")), "everything is forwarded, at any strength");
  }

  std::printf("\n== distance: threshold, floor, per-category limits ==\n");
  {
    Fixture x; x.f.set_rssi_threshold(-75); x.f.setup();
    check(x.fwd(STATIC_ADDR, RANDOM, -75, FLAGS) && !x.fwd(STATIC_ADDR, RANDOM, -76, FLAGS), "threshold is inclusive at the limit");
  }
  {
    Fixture x; x.f.set_rssi_threshold(-75); x.f.set_rssi_floor(-90); x.f.add_allowed_mac(STATIC_ADDR); x.f.setup();
    check(x.fwd(STATIC_ADDR, RANDOM, -89, FLAGS), "an allowlisted MAC bypasses the threshold");
    check(!x.fwd(STATIC_ADDR, RANDOM, -91, FLAGS) && x.f.get_adv_dropped() == 1, "but not the floor");
  }
  {
    Fixture x; x.f.set_rssi_floor(-90); x.f.setup();  // floor only, no threshold
    check(x.fwd(STATIC_ADDR, RANDOM, -90, FLAGS) && !x.fwd(STATIC_ADDR, RANDOM, -91, FLAGS), "a floor with no threshold works on its own");
    check(x.f.get_min_rssi_gate() == -90, "and the gate follows it");
  }
  {
    Fixture x; x.f.set_rssi_threshold(-75); x.f.set_rssi_floor(-90); x.f.set_rssi_irk(-85); x.f.set_irks_hex(SPEC_IRK); x.f.setup();
    check(x.fwd(SPEC_RPA, RANDOM, -84, FLAGS) && !x.fwd(SPEC_RPA, RANDOM, -86, FLAGS), "rssi_irk gives our phones their own, looser limit");
  }
  {
    Fixture x; x.f.set_rssi_threshold(-75); x.f.set_rssi_floor(-60); x.f.add_allowed_mac(STATIC_ADDR); x.f.set_rssi_mac_allowlist(-80); x.f.setup();
    check(!x.fwd(STATIC_ADDR, RANDOM, -70, FLAGS), "a floor stricter than a category limit still binds (runtime misconfig)");
  }

  std::printf("\n== allow / block lists ==\n");
  {
    Fixture x; x.f.add_allowed_mac(STATIC_ADDR); x.f.add_blocked_mac(STATIC_ADDR); x.f.setup();
    check(!x.fwd(STATIC_ADDR, RANDOM, -40, FLAGS), "mac_blocklist beats mac_allowlist");
  }
  {
    Fixture x; x.f.add_allowed_mac(STATIC_ADDR); x.f.set_allowlist_exclusive(true); x.f.setup();
    check(x.fwd(STATIC_ADDR, RANDOM, -60, FLAGS) && !x.fwd(STATIC_ADDR + 1, RANDOM, -60, FLAGS), "allowlist_exclusive forwards only allowlisted devices");
  }
  {
    Fixture x; x.f.set_drop_non_resolvable(true); x.f.setup();
    check(!x.fwd(NRPA_ADDR, RANDOM, -60, FLAGS), "drop_non_resolvable drops a non-resolvable private address");
    check(x.fwd(NRPA_ADDR, PUBLIC, -60, FLAGS), "but never a PUBLIC address that merely looks like one (00:.. OUIs exist)");
    check(x.fwd(STATIC_ADDR, RANDOM, -60, FLAGS), "and never a static random address");
  }
  {
    Fixture x; x.f.add_blocked_name("govee"); x.f.setup();
    check(!x.fwd(STATIC_ADDR, RANDOM, -60, FLAGS + name("My GOVEE_H6001")), "name_blocklist is a case-insensitive substring match");
    check(x.fwd(STATIC_ADDR, RANDOM, -60, FLAGS + name("gove")), "a shorter name does not match a longer needle");
    check(x.fwd(STATIC_ADDR, RANDOM, -60, FLAGS), "no name, no match");
  }

  std::printf("\n== Apple: blocklist and its carve-outs ==\n");
  {
    Fixture x; fleet_config(x);
    check(!x.fwd(STATIC_ADDR, RANDOM, -60, FLAGS + apple(0x10)), "Apple manufacturer data is dropped");
    check(x.fwd(STATIC_ADDR, RANDOM, -60, FLAGS + apple(0x06)), "HomeKit (0x06) is kept");
    check(x.fwd(STATIC_ADDR, RANDOM, -84, FLAGS + apple(0x12, 25)), "FindMy (0x12) is kept, at its own -85 limit, below the -75 threshold");
    check(x.fwd(STATIC_ADDR, RANDOM, -84, FLAGS + apple(0x07, 25)), "AirPods proximity pairing (0x07) rides the same rule");
    check(!x.fwd(STATIC_ADDR, RANDOM, -86, FLAGS + apple(0x12, 25)), "FindMy below its limit is dropped");
    check(x.fwd(SPEC_RPA, RANDOM, -60, FLAGS + apple(0x10)), "OUR phone, resolved by IRK, is protected from the Apple blocklist");
    check(x.fwd(0xE3AC06854000ULL, RANDOM, -89, FLAGS + apple(0x10)), "so is an allowlisted MAC");
  }
  {
    Fixture x; x.f.add_blocked_manufacturer(0x004C); x.f.set_allow_homekit(false); x.f.setup();
    check(!x.fwd(STATIC_ADDR, RANDOM, -60, apple(0x06)), "allow_homekit: false blocks HAP too");
    check(!x.fwd(STATIC_ADDR, RANDOM, -60, apple(0x12, 25)), "FindMy is blocked unless allow_findmy is on");
  }

  std::printf("\n== iBeacon rules ==\n");
  {
    Fixture x; fleet_config(x);
    check(x.fwd(0x246F28AABBCCULL, PUBLIC, -94, FLAGS + ibeacon(1, 1)), "our probes (major 1) are forwarded at -95, overriding the -90 floor");
    check(!x.fwd(0x246F28AABBCCULL, PUBLIC, -96, FLAGS + ibeacon(1, 1)), "but not below the rule's own limit");
    check(!x.fwd(0x246F28AABBCCULL, PUBLIC, -50, FLAGS + ibeacon(2, 1)), "another major is still Apple noise");
    Bytes cut = ibeacon(1, 1); cut.resize(cut.size() - 4); cut[0] -= 4;
    check(!x.fwd(0x246F28AABBCCULL, PUBLIC, -50, FLAGS + cut), "a truncated iBeacon cannot match a major rule");
  }
  {
    Fixture x; x.f.set_rssi_threshold(-75); x.f.set_rssi_floor(-90); x.f.add_blocked_manufacturer(0x004C);
    x.f.add_ibeacon_major(1, -95); x.f.add_ibeacon_major_minor(1, 7, -60); x.f.setup();
    check(!x.fwd(STATIC_ADDR, RANDOM, -70, ibeacon(1, 7)) && x.fwd(STATIC_ADDR, RANDOM, -70, ibeacon(1, 8)), "an exact major+minor rule beats the whole-major rule");
  }
  {
    Fixture x; x.f.set_rssi_threshold(-75); x.f.set_rssi_floor(-90); x.f.add_blocked_manufacturer(0x004C);
    x.f.add_ibeacon_major(1, BLEAdvertFilter::IBEACON_RSSI_INHERIT); x.f.setup();
    check(x.fwd(STATIC_ADDR, RANDOM, -75, ibeacon(1, 1)) && !x.fwd(STATIC_ADDR, RANDOM, -76, ibeacon(1, 1)), "a rule with no rssi only lifts the blocklist; the threshold still applies");
  }

  std::printf("\n== service UUID passthrough ==\n");
  {
    Fixture x; fleet_config(x);
    check(x.fwd(NRPA_ADDR, RANDOM, -89, FLAGS + uuid16_list(0xFFF6)), "Matter commissioning from a non-resolvable address is rescued from drop_non_resolvable");
    check(x.fwd(STRANGER_RPA, RANDOM, -60, FLAGS + uuid16_list(0xFFF6)), "and from the unresolved-RPA drop, as documented");
    check(x.f.get_adv_allowed_service_uuid() == 2, "both counted as UUID passthroughs");
    check(!x.fwd(STRANGER_RPA, RANDOM, -60, FLAGS + uuid16_list(0xFD6F)), "an RPA with a UUID we did NOT allowlist is still dropped");
    check(x.fwd(NRPA_ADDR, RANDOM, -60, ad(0x16, {0xED, 0xFE, 0x01, 0x02})), "16-bit service DATA (0x16) matches on its leading UUID");
    check(!x.fwd(NRPA_ADDR, RANDOM, -60, ad(0x16, {0x01, 0x02, 0xED, 0xFE})), "but not on bytes after it");
    Bytes improv_le{0x00, 0x80, 0x26, 0x78, 0x74, 0x27, 0x63, 0x46, 0x72, 0x22, 0x28, 0x62, 0x68, 0x77, 0x46, 0x00};
    check(x.fwd(NRPA_ADDR, RANDOM, -60, ad(0x07, improv_le)), "a 128-bit UUID matches (little-endian on air)");
    Bytes fff6_long{0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF6, 0xFF, 0x00, 0x00};
    check(x.fwd(NRPA_ADDR, RANDOM, -60, ad(0x07, fff6_long)), "a SIG short in Base-UUID long form matches its 16-bit entry");
    check(!x.fwd(NRPA_ADDR, RANDOM, -91, FLAGS + uuid16_list(0xFFF6)), "the floor still bounds the passthrough");
  }

  std::printf("\n== runtime IRKs ==\n");
  {
    Fixture x; x.f.add_blocked_manufacturer(0x004C); x.f.setup();
    check(x.f.set_irks("David phone: " + std::string(SPEC_IRK) + "\nDad watch: ffeeddccbbaa99887766554433221100") == 2, "named lines load; hex-letter names are not keys");
    check(x.fwd(SPEC_RPA, RANDOM, -60, apple(0x10)) && !x.fwd(STRANGER_RPA, RANDOM, -60, apple(0x10)), "and take effect immediately");
    check(x.f.set_irks("unavailable") == -1 && x.f.set_irks("") == -1 && x.f.get_irk_count() == 2, "input with no key keeps the current list");
    check(x.f.set_irks(std::string(SPEC_IRK) + "0") == -1, "33 hex digits is not a key");
    check(x.f.set_irks(std::string(SPEC_IRK) + SPEC_IRK) == -1, "two keys run together are rejected, not split");
    check(x.f.set_irks(std::string(SPEC_IRK) + "," + SPEC_IRK) == 1, "duplicates collapse");
    {
      std::string many;
      for (int k = 0; k < 40; k++) {
        char key[40];
        std::snprintf(key, sizeof(key), "%032x,", k + 1);
        many += key;
      }
      check(x.f.set_irks(many) == 32 && x.f.get_irk_count() == BLEAdvertFilter::MAX_RUNTIME_IRKS, "a runtime list is capped at MAX_RUNTIME_IRKS");
    }
    x.f.clear_irks();
    check(x.f.get_irk_count() == 0 && x.fwd(STRANGER_RPA, RANDOM, -60, FLAGS), "clear_irks turns RPA gating off");
  }

  std::printf("\n== the pre-gate ==\n");
  {
    Fixture x; fleet_config(x);
    check(x.f.get_min_rssi_gate() == -95, "fleet config gates at -95 (the iBeacon rule is the loosest)");
  }
  {
    Fixture x; x.f.set_rssi_threshold(-75); x.f.set_rssi_floor(-90); x.f.setup();
    check(x.f.get_min_rssi_gate() == -75, "with no allowlists configured the gate tightens to the threshold");
    x.f.add_allowed_mac(STATIC_ADDR);
    check(x.f.get_min_rssi_gate() == -90, "a MAC allowlist opens it to the floor");
  }
  {
    Fixture x; x.f.set_rssi_threshold(-75); x.f.add_allowed_mac(STATIC_ADDR); x.f.setup();
    check(x.f.get_min_rssi_gate() == -127, "a MAC allowlist with no floor is unbounded, so there is no gate");
  }
  {
    // The defect this replaced: the gate was computed once at codegen.
    Fixture x; x.f.set_rssi_threshold(-75); x.f.set_rssi_floor(-90); x.f.add_allowed_mac(STATIC_ADDR); x.f.set_rssi_mac_allowlist(-85); x.f.setup();
    check(x.f.get_min_rssi_gate() == -85, "explicit mac limit -85: gate -85");
    x.f.set_rssi_threshold(-90);  // the Home Assistant number entity, dialled to the floor
    check(x.fwd(STATIC_ADDR + 1, RANDOM, -88, FLAGS), "lowering the threshold at runtime takes effect (the gate follows)");
  }
  {
    Fixture x; x.f.set_rssi_threshold(-75); x.f.set_rssi_floor(-90); x.f.setup();
    x.f.set_irks(SPEC_IRK); x.f.set_rssi_irk(-88);
    check(x.fwd(SPEC_RPA, RANDOM, -87, FLAGS), "IRKs and an irk limit added at runtime open the gate for them");
  }
  {
    // Property: the gate is an optimisation and must never change a verdict.
    std::srand(12345);
    int mismatches = 0, trials = 0;
    const int8_t limits[] = {-127, -95, -90, -85, -80, -75, -60};
    for (int cfg = 0; cfg < 400; cfg++) {
      Fixture a, b;
      for (Fixture *x : {&a, &b}) {
        std::srand(1000 + cfg);
        auto pick = [&] { return limits[std::rand() % 7]; };
        x->f.set_rssi_threshold(pick()); x->f.set_rssi_floor(pick());
        x->f.set_rssi_mac_allowlist(pick()); x->f.set_rssi_irk(pick()); x->f.set_rssi_service_uuid(pick());
        if (std::rand() % 2) x->f.add_allowed_mac(STATIC_ADDR);
        if (std::rand() % 2) x->f.set_irks_hex(SPEC_IRK);
        if (std::rand() % 2) x->f.add_allowed_service_uuid(0xFFF6);
        if (std::rand() % 2) x->f.add_blocked_manufacturer(0x004C);
        if (std::rand() % 2) x->f.add_ibeacon_major(1, std::rand() % 2 ? pick() : BLEAdvertFilter::IBEACON_RSSI_INHERIT);
        if (std::rand() % 3 == 0) { x->f.set_allow_ibeacon(true); x->f.set_ibeacon_any_rssi(std::rand() % 2 ? pick() : BLEAdvertFilter::IBEACON_RSSI_INHERIT); }
        if (std::rand() % 2) { x->f.set_allow_findmy(true); x->f.set_findmy_rssi(std::rand() % 2 ? pick() : BLEAdvertFilter::IBEACON_RSSI_INHERIT); }
        x->f.set_drop_non_resolvable(std::rand() % 2);
        x->f.setup();
      }
      b.f.disable_gate();
      const uint64_t addrs[] = {STATIC_ADDR, NRPA_ADDR, STRANGER_RPA, SPEC_RPA};
      const Bytes payloads[] = {FLAGS, apple(0x10), apple(0x12, 25), ibeacon(1, 1), ibeacon(2, 2), uuid16_list(0xFFF6)};
      for (uint64_t addr : addrs)
        for (const Bytes &p : payloads)
          for (int rssi = -100; rssi <= -55; rssi += 3) {
            trials++;
            if (a.fwd(addr, RANDOM, static_cast<int8_t>(rssi), p) != b.fwd(addr, RANDOM, static_cast<int8_t>(rssi), p))
              mismatches++;
          }
    }
    std::printf("      (%d verdicts compared across 400 random configs)\n", trials);
    check(mismatches == 0, "the pre-gate never changes a verdict");
  }

  std::printf("\n== counters ==\n");
  {
    Fixture x; fleet_config(x);
    x.fwd(STATIC_ADDR, RANDOM, -60, FLAGS); x.fwd(STATIC_ADDR, RANDOM, -99, FLAGS); x.fwd(STRANGER_RPA, RANDOM, -60, FLAGS);
    check(x.f.get_adv_forwarded() == 1 && x.f.get_adv_dropped() == 2 && x.f.get_adv_dropped_gate() == 1 && x.f.get_adv_dropped_rpa() == 1,
          "forwarded + dropped account for every advert, with gate and RPA subsets");
  }
  {
    Fixture x; x.f.setup();
    check(x.proxy.advertisement_filter_.is_set(), "setup() installs the filter into the proxy's slot");
    RawAdvertisement a{STATIC_ADDR, FLAGS.data(), static_cast<uint16_t>(FLAGS.size()), -60, RANDOM};
    check(x.proxy.advertisement_filter_.should_forward(a), "and the slot reaches should_forward()");
  }

  std::printf("\n== malformed payloads (run under ASan/UBSan) ==\n");
  {
    Fixture x; fleet_config(x);
    x.f.add_blocked_name("x");
    std::srand(99);
    static const uint8_t TYPES[] = {0xFF, 0x03, 0x07, 0x16, 0x21, 0x09};
    static const uint8_t SUBTYPES[] = {0x02, 0x06, 0x07, 0x12};
    for (int n = 0; n < 200000; n++) {
      Bytes p(static_cast<size_t>(std::rand() % 64));
      for (auto &b : p) b = static_cast<uint8_t>(std::rand());
      // Bias towards the structures the walkers care about, with lying lengths.
      if (!p.empty() && n % 3 == 0) { p[0] = static_cast<uint8_t>(std::rand() % 40); if (p.size() > 1) p[1] = TYPES[std::rand() % 6]; }
      if (p.size() > 4 && n % 5 == 0) { p[2] = 0x4C; p[3] = 0x00; p[4] = SUBTYPES[std::rand() % 4]; }
      x.fwd(n % 2 ? NRPA_ADDR : STATIC_ADDR, RANDOM, -60, p);
    }
    x.fwd(STATIC_ADDR, RANDOM, -60, {});
    check(true, "200,000 random and truncated payloads: no overread, no UB");
  }

  std::printf("\n%s\n", failures ? "FAILED" : "all passed");
  return failures ? 1 : 0;
}
