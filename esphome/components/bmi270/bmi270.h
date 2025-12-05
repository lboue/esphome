#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"
// BMI2 vendored headers
#include <esphome/components/bmi270/bmi2/bmi2.h>

namespace esphome {
namespace bmi270 {

class BMI270Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;

  void update() override;

  float get_setup_priority() const override;

  void set_accel_x_sensor(sensor::Sensor *accel_x_sensor) { accel_x_sensor_ = accel_x_sensor; }
  void set_accel_y_sensor(sensor::Sensor *accel_y_sensor) { accel_y_sensor_ = accel_y_sensor; }
  void set_accel_z_sensor(sensor::Sensor *accel_z_sensor) { accel_z_sensor_ = accel_z_sensor; }
  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
  void set_gyro_x_sensor(sensor::Sensor *gyro_x_sensor) { gyro_x_sensor_ = gyro_x_sensor; }
  void set_gyro_y_sensor(sensor::Sensor *gyro_y_sensor) { gyro_y_sensor_ = gyro_y_sensor; }
  void set_gyro_z_sensor(sensor::Sensor *gyro_z_sensor) { gyro_z_sensor_ = gyro_z_sensor; }
  void set_power_save_mode(int mode) { this->power_save_mode_ = mode; }

  // Expose a small I2C wrapper used by the BMI2 C callbacks. These ensure the
  // underlying I2C bus pointer is valid before attempting transfers so the
  // BMI2 SDK cannot crash the MCU if the bus hasn't been attached yet.
  i2c::ErrorCode bmi2_read_register_cb(uint8_t reg, uint8_t *data, size_t len);

  i2c::ErrorCode bmi2_write_register_cb(uint8_t reg, const uint8_t *data, size_t len);

 protected:
  // Sensor outputs
  sensor::Sensor *accel_x_sensor_{nullptr};
  sensor::Sensor *accel_y_sensor_{nullptr};
  sensor::Sensor *accel_z_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *gyro_x_sensor_{nullptr};
  sensor::Sensor *gyro_y_sensor_{nullptr};
  sensor::Sensor *gyro_z_sensor_{nullptr};

  // BMI2 runtime structures
  struct bmi2_dev sensor_ {};             // bmi2 device handle
  struct bmi2_sens_config accel_cfg_ {};  // accel config
  struct bmi2_sens_config gyro_cfg_ {};   // gyro config

  // I2C helper
  std::unique_ptr<esphome::i2c::I2CDevice> i2c_dev_{};

  // State tracking
  bool is_initialized_{false};
  int power_save_mode_{0};
  float accel_sensitivity_{0.0f};
  float gyro_sensitivity_{0.0f};
  bool sensors_active_{false};

  // Legacy register-based helpers (kept for fallback)
  void internal_setup_(int stage);
  bool setup_complete_{false};
  bool did_reset_{false};

  /** reads `len` 16-bit little-endian integers from the given i2c register */
  i2c::ErrorCode read_le_int16_(uint8_t reg, int16_t *value, uint8_t len);
};

}  // namespace bmi270
}  // namespace esphome
