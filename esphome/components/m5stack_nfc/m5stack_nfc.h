#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace m5stack_nfc {

struct NFCTag {
  uint8_t uid[10];
  uint8_t uid_length;
};

class M5StackNFCComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

 private:
  bool read_tag(NFCTag &tag);
  bool read_version();

  bool initialized_{false};
};

}  // namespace m5stack_nfc
}  // namespace esphome
