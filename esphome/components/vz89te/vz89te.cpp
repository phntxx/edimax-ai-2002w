#include "vz89te.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace vz89te {

static const char *const TAG = "vz89te";

static const uint8_t GET_STATUS_CMD[6] = { 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t READ_CMD[1] = { 0xE1 };

void VZ89TEComponent::set_values_(const uint8_t *buffer) {
  if (buffer[0] != 0) {
    this->voc_val_ = (buffer[0] - 13) * (1000.0 / 229);
  }

  if (buffer[1] != 0) {
    this->co2_val_ = (buffer[1] - 13) * (1600.0 / 229) + 400;
  }
}

void VZ89TEComponent::setup() {
  if (this->write(nullptr, 0) != i2c::ERROR_OK) {
    ESP_LOGD(TAG, "Communication failed!");
    this->mark_failed();
    return;
  }
}

void VZ89TEComponent::update() {
  uint8_t buffer[7];

  if (this->write(GET_STATUS_CMD, sizeof(GET_STATUS_CMD), false) != i2c::ERROR_OK) {
    this->status_set_warning();
    return;
  }

  if (this->write(READ_CMD, sizeof(READ_CMD), true) != i2c::ERROR_OK) {
    this->status_set_warning();
    return;
  }

  if (this->read(buffer, sizeof(buffer)) != i2c::ERROR_OK) {
    this->status_set_warning();
    return;
  }

  this->set_values_(buffer);

  if (this->co2_ != nullptr) {
    this->co2_->publish_state(this->co2_val_);
  }
  if (this->voc_ != nullptr) {
    this->voc_->publish_state(this->voc_val_);
  }

  this->status_clear_warning();
}

void VZ89TEComponent::publish_nans_() {
  if (this->co2_ != nullptr) {
    this->co2_->publish_state(NAN);
  }
  if (this->voc_ != nullptr) {
    this->voc_->publish_state(NAN);
  }
}

void VZ89TEComponent::dump_config() {

  ESP_LOGCONFIG(TAG, "MiCS VZ-89TE:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);

  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with MiCS VZ-89TE failed!");
  }
  
  LOG_SENSOR("  ", "CO2", this->co2_);
  LOG_SENSOR("  ", "VOC", this->voc_);
}

}  // namespace vz89te
}  // namespace esphome