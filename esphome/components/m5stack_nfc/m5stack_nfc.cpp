#include "m5stack_nfc.h"
#include "m5stack_nfc_protocol.h"

#include "esphome/core/log.h"

namespace esphome {
namespace m5stack_nfc {

static const char *const TAG = "m5stack_nfc";

void M5StackNFCComponent::setup() {
  ESP_LOGI(TAG, "Initializing M5Stack NFC");

  uint8_t command = 0x00;
  uint8_t response[4];

  if (!this->write_read(&command, 1, response, sizeof(response))) {
    ESP_LOGE(TAG, "M5Stack NFC not responding");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "M5Stack NFC detected");
  initialized_ = true;
}

void M5StackNFCComponent::update() {
  if (!initialized_) {
    return;
  }

  NFCTag tag;

  if (this->read_tag(tag)) {
    ESP_LOGI(TAG, "NFC tag detected");

    ESP_LOGI(TAG, "UID length: %u", tag.uid_length);

    for (uint8_t i = 0; i < tag.uid_length; i++) {
      ESP_LOGI(TAG, "UID[%u]=0x%02X", i, tag.uid[i]);
    }
  }
}

void M5StackNFCComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "M5Stack NFC");
  LOG_I2C_DEVICE(this);
}

bool M5StackNFCComponent::read_tag(NFCTag &tag) {
  // TODO: implémenter le protocole I2C M5Stack NFC

  tag.uid_length = 0;

  return false;
}

}  // namespace m5stack_nfc
}  // namespace esphome
