#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include "../seesaw.h"

namespace esphome::seesaw {

class SeesawEncoderSensor final : public sensor::Sensor, public PollingComponent, public Parented<Seesaw> {
 public:
  void update() override;
  void set_channel(uint8_t channel) { this->channel_ = channel; }

 protected:
  uint8_t channel_;
};

}  // namespace esphome::seesaw
