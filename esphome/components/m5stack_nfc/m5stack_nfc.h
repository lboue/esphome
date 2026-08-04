#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace m5stack_nfc {

class M5StackNFCComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

 protected:
  bool send_command(uint8_t command);
  bool read_response(uint8_t *data, size_t len);
};

}  // namespace m5stack_nfc
}  // namespace esphome
