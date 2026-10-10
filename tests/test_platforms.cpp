// Host tests for the sensor and number platforms and the two actions.
//
// Like test_filter.cpp, these compile the REAL sources (sensor/, number/,
// automation.h) against the stubs in tests/stubs/, with a hand-set clock and a
// one-slot "flash" so a reboot can be simulated.
//
//   tests/run.sh

#include <cmath>
#include <cstdio>
#include <string>

#include "../components/ble_advert_filter/automation.h"
#include "../components/ble_advert_filter/ble_advert_filter.h"
#include "../components/ble_advert_filter/number/rssi_threshold_number.h"
#include "../components/ble_advert_filter/sensor/ble_advert_filter_sensor.h"
#include "esphome/core/hal.h"

using esphome::ble_advert_filter::BLEAdvertFilter;
using esphome::ble_advert_filter::BLEAdvertFilterSensor;
using esphome::ble_advert_filter::ClearIrksAction;
using esphome::ble_advert_filter::RSSIThresholdNumber;
using esphome::ble_advert_filter::SetIrksAction;
using esphome::ble_device_base::RawAdvertisement;

namespace {

int failures = 0;
void check(bool ok, const char *what) {
  std::printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok)
    failures++;
}

const char *IRK_A = "ec0234a357c8ad05341010a60a397d9b";
const char *IRK_B = "ffeeddccbbaa99887766554433221100";
const uint8_t FLAGS[] = {0x02, 0x01, 0x06};

struct Rig {
  esphome::bluetooth_proxy::BluetoothProxy proxy;
  BLEAdvertFilter filter;
  Rig(int8_t threshold = -75) {
    filter.set_parent(&proxy);
    filter.set_rssi_threshold(threshold);
    filter.setup();
  }
  void advert(int8_t rssi, int n = 1) {
    for (int i = 0; i < n; i++) {
      RawAdvertisement a{0xC01122334455ULL, FLAGS, sizeof(FLAGS), rssi, 1};
      filter.should_forward(a);
    }
  }
};

struct NumberProbe : RSSIThresholdNumber {
  NumberProbe(BLEAdvertFilter *f, float min, float max, bool restore) {
    this->set_parent(f);
    this->traits.set_min_value(min);
    this->traits.set_max_value(max);
    this->set_restore_value(restore);
  }
};

}  // namespace

int main() {
  std::printf("== sensor platform ==\n");
  {
    Rig r;
    esphome::test_millis() = 1000;
    BLEAdvertFilterSensor s;
    esphome::sensor::Sensor fwd, drop, rate, irks;
    s.set_parent(&r.filter);
    s.set_rate_sensor(BLEAdvertFilterSensor::FORWARDED, &fwd);
    s.set_rate_sensor(BLEAdvertFilterSensor::DROPPED, &drop);
    s.set_drop_rate_sensor(&rate);
    s.set_irk_count_sensor(&irks);
    r.advert(-60, 5);  // before setup: not part of the first rate
    s.setup();

    esphome::test_millis() += 60000;
    s.update();
    check(fwd.publishes == 1 && fwd.state == 0.0f && std::isnan(rate.state),
          "nothing heard since setup: 0 adv/min, and drop rate unknown (not 0%)");

    r.advert(-60, 30);
    r.advert(-90, 10);
    esphome::test_millis() += 30000;  // half a minute
    s.update();
    check(fwd.state == 60.0f && drop.state == 20.0f, "rates are per minute, scaled by the real elapsed time");
    check(std::fabs(rate.state - 25.0f) < 0.01f, "drop rate is dropped / heard over the interval (10 of 40 = 25%)");
    check(irks.state == 0.0f, "irk_count reports the loaded keys");

    const size_t before = fwd.publishes;
    s.update();  // same millis: no time has passed
    check(fwd.publishes == before, "an update with no elapsed time publishes nothing");
  }
  {
    // drop_rate keeps its own baselines, so it works without the rate sensors.
    Rig r;
    esphome::test_millis() = 0;
    BLEAdvertFilterSensor s;
    esphome::sensor::Sensor rate;
    s.set_parent(&r.filter);
    s.set_drop_rate_sensor(&rate);
    s.setup();
    r.advert(-60, 3);
    r.advert(-90, 1);
    esphome::test_millis() += 60000;
    s.update();
    check(std::fabs(rate.state - 25.0f) < 0.01f, "drop_rate alone, without forwarded/dropped configured");
  }

  std::printf("\n== number platform (rssi_threshold dial) ==\n");
  {
    esphome::test_flash() = {};
    Rig r(-75);
    NumberProbe n(&r.filter, -90, -30, true);
    n.setup();
    check(n.state == -75.0f && r.filter.get_rssi_threshold() == -75, "first boot: the dial starts at the YAML value");

    n.test_set(-82);
    check(r.filter.get_rssi_threshold() == -82 && n.state == -82.0f, "moving the dial changes the filter at once");

    // Reboot: a fresh filter built from the same YAML, and a fresh dial.
    Rig r2(-75);
    NumberProbe n2(&r2.filter, -90, -30, true);
    n2.setup();
    check(r2.filter.get_rssi_threshold() == -82 && n2.state == -82.0f,
          "after a reboot the saved value is applied to the filter, not just shown");

    // The range was narrowed since the value was saved.
    Rig r3(-75);
    NumberProbe n3(&r3.filter, -80, -30, true);
    n3.setup();
    check(r3.filter.get_rssi_threshold() == -80, "a saved value outside a narrowed range is clamped into it");
  }
  {
    esphome::test_flash() = {};
    Rig r(-75);
    NumberProbe n(&r.filter, -90, -30, false);
    n.setup();
    n.test_set(-60);
    Rig r2(-75);
    NumberProbe n2(&r2.filter, -90, -30, false);
    n2.setup();
    check(r2.filter.get_rssi_threshold() == -75, "restore_value: false starts from the YAML value every boot");
  }

  std::printf("\n== actions ==\n");
  {
    Rig r;
    SetIrksAction<> set;
    set.set_parent(&r.filter);
    set.set_irks(std::string("phone: ") + IRK_A + "\nwatch: " + IRK_B);
    set.play();
    check(r.filter.get_irk_count() == 2, "set_irks with a fixed value loads every key in it");

    SetIrksAction<std::string> from_entity;
    from_entity.set_parent(&r.filter);
    from_entity.set_irks(std::function<std::string(std::string)>([](std::string x) { return x; }));
    from_entity.play(std::string("unavailable"));
    check(r.filter.get_irk_count() == 2, "set_irks with no key in the text keeps the current list");
    from_entity.play(std::string(IRK_B));
    check(r.filter.get_irk_count() == 1, "set_irks from a lambda replaces the list");

    ClearIrksAction<> clear;
    clear.set_parent(&r.filter);
    clear.play();
    check(r.filter.get_irk_count() == 0, "clear_irks empties it");
  }

  std::printf("\n%s\n", failures ? "FAILED" : "all passed");
  return failures ? 1 : 0;
}
