#include "m5stack_nfc.h"
#include "m5stack_nfc_protocol.h"

#include "esphome/core/log.h"

// https://github.com/m5stack/M5Unit-NFC/blob/main/examples/UnitUnified/NFCA/Detect/main/Detect.cpp
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

    ESP_LOGI(TAG, "UID length: %d", tag.uid_length);

    std::string uid;

    for (uint8_t i = 0; i < tag.uid_length; i++) {
      char buffer[4];
      snprintf(buffer, sizeof(buffer), "%02X", tag.uid[i]);
      uid += buffer;

      if (i < tag.uid_length - 1) {
        uid += ":";
      }
    }

    ESP_LOGI(TAG, "UID: %s", uid.c_str());
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

bool M5StackNFCComponent::read_ic_identity() {
  uint8_t value;

  auto err = this->read_register(st25r3916::REG_IC_IDENTITY, &value, 1);

  if (err != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Failed to read IC identity");
    return false;
  }

  ESP_LOGI(TAG, "ST25R3916 identity: 0x%02X", value);

  return value == st25r3916::IC_IDENTITY_ST25R3916;
}

}  // namespace m5stack_nfc
}  // namespace esphome
