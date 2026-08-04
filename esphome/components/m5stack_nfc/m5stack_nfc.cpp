#include "m5stack_nfc.h"
#include "m5stack_nfc_protocol.h"

#include "esphome/core/log.h"

namespace esphome {
namespace m5stack_nfc {

static const char *const TAG = "m5stack_nfc";

void M5StackNFCComponent::setup() {
  ESP_LOGI(TAG, "Initializing M5Stack NFC");

  // Communication avec le STM32 via I2C @ 0x50
}

void M5StackNFCComponent::update() {
  // Poll NFC
}

void M5StackNFCComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "M5Stack NFC");
  LOG_I2C_DEVICE(this);
}

}  // namespace m5stack_nfc
}  // namespace esphome
