#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace vz89te {

class VZ89TEComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_co2(sensor::Sensor *co2) { co2_ = co2; }
  void set_voc(sensor::Sensor *voc) { voc_ = voc; }

  void setup() override;
  void update() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  float co2_val_ = 0.0;
  float voc_val_ = 0.0;

  sensor::Sensor *co2_{nullptr};
  sensor::Sensor *voc_{nullptr};

  void publish_nans_();
  void set_values_(const uint8_t *buffer);
};

}  // namespace vz89te
}  // namespace esphome