#pragma once
#include <cstddef>
// Records what the sensor platform publishes.
namespace esphome::sensor {
class Sensor {
 public:
  void publish_state(float state) {
    this->state = state;
    this->publishes++;
  }
  float state{0.0f};
  size_t publishes{0};
};
}  // namespace esphome::sensor
#define LOG_SENSOR(prefix, type, obj) ((void) (prefix), (void) (type), (void) (obj))
