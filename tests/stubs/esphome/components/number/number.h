#pragma once
#include "esphome/core/preferences.h"
// The slice of esphome::number::Number the threshold dial uses.
namespace esphome::number {
class NumberTraits {
 public:
  void set_min_value(float v) { this->min_ = v; }
  void set_max_value(float v) { this->max_ = v; }
  float get_min_value() const { return this->min_; }
  float get_max_value() const { return this->max_; }

 protected:
  float min_{0.0f};
  float max_{100.0f};
};
class Number {
 public:
  virtual ~Number() = default;
  void publish_state(float state) {
    this->state = state;
    this->publishes++;
  }
  // What a Home Assistant "set value" ends up calling.
  void test_set(float value) { this->control(value); }
  template<typename T> ESPPreferenceObject make_entity_preference() { return {}; }
  NumberTraits traits;
  float state{0.0f};
  int publishes{0};

 protected:
  virtual void control(float value) = 0;
};
}  // namespace esphome::number
#define LOG_NUMBER(prefix, type, obj) ((void) (prefix), (void) (type), (void) (obj))
