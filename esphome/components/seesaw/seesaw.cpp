#include "seesaw.h"

#include <array>
#include <algorithm>
#include <cinttypes>

#include "esphome/core/log.h"

namespace esphome::seesaw {

static const char *const TAG = "seesaw";

void Seesaw::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Seesaw...");
  if (this->readbuf(SEESAW_STATUS, SEESAW_STATUS_HW_ID, &this->hw_id_, 1) != i2c::ERROR_OK) {
    this->mark_failed();
    return;
  }
  uint8_t buf[4];
  if (this->readbuf(SEESAW_STATUS, SEESAW_STATUS_VERSION, buf, 4) == i2c::ERROR_OK) {
    this->version_ = (uint32_t(buf[0]) << 24) | (uint32_t(buf[1]) << 16) | (uint32_t(buf[2]) << 8) | buf[3];
  }
}

void Seesaw::dump_config() {
  ESP_LOGCONFIG(TAG, "Seesaw:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  HW ID: 0x%02X", this->hw_id_);
  ESP_LOGCONFIG(TAG, "  Version: %" PRIu32, this->version_);
}

int32_t Seesaw::get_encoder_position(uint8_t channel) {
  uint8_t buf[4];
  if (this->readbuf(SEESAW_ENCODER, SEESAW_ENCODER_POSITION + channel, buf, 4) != i2c::ERROR_OK)
    return 0;
  int32_t value = (int32_t(buf[0]) << 24) | (int32_t(buf[1]) << 16) | (int32_t(buf[2]) << 8) | buf[3];
  // Adafruit's own examples negate this value to make clockwise rotation read positive. On the unit
  // this was tested on, clockwise already reads positive without negating -- a second, independent
  // report of the same discrepancy (https://github.com/ssieb/esphome_components/issues/74) suggests
  // this isn't a one-off unit, but a firmware difference across a batch/revision of Seesaw encoders
  // rather than something specific to this board. Revisit if a report turns up showing the opposite.
  return value;
}

void Seesaw::pin_mode(uint8_t pin, gpio::Flags flags) {
  uint32_t mask = 1UL << pin;
  if (flags & gpio::FLAG_OUTPUT) {
    this->write32(SEESAW_GPIO, SEESAW_GPIO_DIRSET_BULK, mask);
  } else if (flags & gpio::FLAG_PULLUP) {
    this->write32(SEESAW_GPIO, SEESAW_GPIO_DIRCLR_BULK, mask);
    this->write32(SEESAW_GPIO, SEESAW_GPIO_PULLENSET, mask);
    this->write32(SEESAW_GPIO, SEESAW_GPIO_BULK_SET, mask);
  } else {
    // plain input, no pull
    this->write32(SEESAW_GPIO, SEESAW_GPIO_DIRCLR_BULK, mask);
  }
}

bool Seesaw::digital_read(uint8_t pin) {
  uint8_t buf[4];
  if (this->readbuf(SEESAW_GPIO, SEESAW_GPIO_BULK, buf, 4) != i2c::ERROR_OK)
    return false;
  uint32_t value = (uint32_t(buf[0]) << 24) | (uint32_t(buf[1]) << 16) | (uint32_t(buf[2]) << 8) | buf[3];
  return (value & (1UL << pin)) != 0;
}

void Seesaw::digital_write(uint8_t pin, bool value) {
  uint32_t mask = 1UL << pin;
  this->write32(SEESAW_GPIO, value ? SEESAW_GPIO_BULK_SET : SEESAW_GPIO_BULK_CLR, mask);
}

void Seesaw::setup_neopixel(uint8_t pin, uint8_t num_leds) {
  this->write8(SEESAW_NEOPIXEL, SEESAW_NEOPIXEL_PIN, pin);
  this->write8(SEESAW_NEOPIXEL, SEESAW_NEOPIXEL_SPEED, 1);  // 1 == 800 KHz
  this->write16(SEESAW_NEOPIXEL, SEESAW_NEOPIXEL_BUF_LENGTH, uint16_t(num_leds) * 3);
}

void Seesaw::write_neopixel_buffer(const uint8_t *buf, uint16_t len) {
  this->write_buf(SEESAW_NEOPIXEL, SEESAW_NEOPIXEL_BUF, buf, len);
}

void Seesaw::show_neopixel() { this->write8(SEESAW_NEOPIXEL, SEESAW_NEOPIXEL_SHOW, 0); }

i2c::ErrorCode Seesaw::write8(SeesawModule mod, uint8_t reg, uint8_t value) {
  return this->write_register16((uint16_t(mod) << 8) | reg, &value, 1);
}

i2c::ErrorCode Seesaw::write16(SeesawModule mod, uint8_t reg, uint16_t value) {
  uint8_t buf[2] = {uint8_t(value >> 8), uint8_t(value)};
  return this->write_register16((uint16_t(mod) << 8) | reg, buf, 2);
}

i2c::ErrorCode Seesaw::write32(SeesawModule mod, uint8_t reg, uint32_t value) {
  uint8_t buf[4] = {uint8_t(value >> 24), uint8_t(value >> 16), uint8_t(value >> 8), uint8_t(value)};
  return this->write_register16((uint16_t(mod) << 8) | reg, buf, 4);
}

i2c::ErrorCode Seesaw::write_buf(SeesawModule mod, uint8_t reg, const uint8_t *data, uint16_t len) {
  if (len > SEESAW_NEOPIXEL_MAX_FRAME - 4) {
    ESP_LOGE(TAG, "NeoPixel buffer too large (%u bytes)", len);
    return i2c::ERROR_TOO_LARGE;
  }
  // [module, register, pixel offset hi, pixel offset lo, data...] -- offset is always 0 since we always
  // write the whole pixel buffer in one shot.
  std::array<uint8_t, SEESAW_NEOPIXEL_MAX_FRAME> frame;
  frame[0] = mod;
  frame[1] = reg;
  frame[2] = 0;
  frame[3] = 0;
  std::copy(data, data + len, frame.begin() + 4);
  return this->write(frame.data(), len + 4);
}

i2c::ErrorCode Seesaw::readbuf(SeesawModule mod, uint8_t reg, uint8_t *buf, uint8_t len) {
  uint8_t addr[2] = {uint8_t(mod), reg};
  i2c::ErrorCode err = this->write(addr, 2);
  if (err != i2c::ERROR_OK)
    return err;
  return this->read(buf, len);
}

void SeesawGPIOPin::setup() { this->pin_mode(this->flags_); }

void SeesawGPIOPin::pin_mode(gpio::Flags flags) { this->parent_->pin_mode(this->pin_, flags); }

bool SeesawGPIOPin::digital_read() { return this->parent_->digital_read(this->pin_) != this->inverted_; }

void SeesawGPIOPin::digital_write(bool value) { this->parent_->digital_write(this->pin_, value != this->inverted_); }

size_t SeesawGPIOPin::dump_summary(char *buffer, size_t len) const {
  return buf_append_printf(buffer, len, 0, "%u via Seesaw", this->pin_);
}

}  // namespace esphome::seesaw
