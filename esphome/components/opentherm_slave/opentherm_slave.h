#pragma once

#include <memory>
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "opentherm_protocol.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

namespace esphome::opentherm_slave {

// Emulates the slave (boiler) side of the OpenTherm protocol, so a thermostat or gateway talking
// OpenTherm sees a working boiler. The ISR/timer bit-level engine lives in opentherm_protocol.h; only
// the request/response logic below is specific to acting as the boiler.
class OpenthermSlaveHub : public Component {
 public:
  void set_in_pin(InternalGPIOPin *in_pin) { this->in_pin_ = in_pin; }
  void set_out_pin(InternalGPIOPin *out_pin) { this->out_pin_ = out_pin; }

  void set_fault(bool fault) { this->fault_ = fault; }
  void set_ch_active(bool active) { this->ch_active_ = active; }
  void set_dhw_active(bool active) { this->dhw_active_ = active; }
  void set_flame_active(bool active) { this->flame_active_ = active; }
  void set_modulation_level(float level) { this->modulation_level_ = level; }

#ifdef USE_SENSOR
  void set_ch_setpoint_sensor(sensor::Sensor *ch_setpoint_sensor) { this->ch_setpoint_sensor_ = ch_setpoint_sensor; }
#endif

  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void setup() override;
  void loop() override;
  void dump_config() override;

 protected:
  InternalGPIOPin *in_pin_{nullptr};
  InternalGPIOPin *out_pin_{nullptr};
  std::unique_ptr<OpenTherm> opentherm_;

  bool fault_{false};
  bool ch_active_{false};
  bool dhw_active_{false};
  bool flame_active_{false};
  float modulation_level_{0.0f};

#ifdef USE_SENSOR
  sensor::Sensor *ch_setpoint_sensor_{nullptr};
#endif

  // Turns a decoded request from the master into the response to send back, in place.
  void build_response_(OpenthermData &data);
};

}  // namespace esphome::opentherm_slave
