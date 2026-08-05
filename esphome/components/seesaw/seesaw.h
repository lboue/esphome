#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome::seesaw {

// Seesaw "module" addresses -- the high byte of the 16-bit register address.
enum SeesawModule : uint8_t {
  SEESAW_STATUS = 0x00,
  SEESAW_GPIO = 0x01,
  SEESAW_NEOPIXEL = 0x0E,
  SEESAW_ENCODER = 0x11,
};

enum : uint8_t {
  SEESAW_STATUS_HW_ID = 0x01,
  SEESAW_STATUS_VERSION = 0x02,
};

enum : uint8_t {
  SEESAW_ENCODER_INTENSET = 0x10,
  SEESAW_ENCODER_POSITION = 0x30,
};

enum : uint8_t {
  SEESAW_GPIO_DIRSET_BULK = 0x02,
  SEESAW_GPIO_DIRCLR_BULK = 0x03,
  SEESAW_GPIO_BULK = 0x04,
  SEESAW_GPIO_BULK_SET = 0x05,
  SEESAW_GPIO_BULK_CLR = 0x06,
  SEESAW_GPIO_PULLENSET = 0x0B,
};

enum : uint8_t {
  SEESAW_NEOPIXEL_PIN = 0x01,
  SEESAW_NEOPIXEL_SPEED = 0x02,
  SEESAW_NEOPIXEL_BUF_LENGTH = 0x03,
  SEESAW_NEOPIXEL_BUF = 0x04,
  SEESAW_NEOPIXEL_SHOW = 0x05,
};

// mod + reg + 2-byte pixel offset + up to 64 RGB pixels -- comfortably covers every NeoPixel-equipped
// Seesaw board Adafruit currently sells (the biggest is a 60-pixel ring).
static constexpr size_t SEESAW_NEOPIXEL_MAX_FRAME = 4 + 64 * 3;

/// Hub component for a single Adafruit "Seesaw" I2C co-processor. Add `sensor`/`light` sub-platforms,
/// or use a `seesaw` pin (via the pin schema registry) with any core platform that takes a `pin:`, e.g.
/// `binary_sensor: platform: gpio`.
class Seesaw : public i2c::I2CDevice, public Component {
 public:
  void setup() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::IO; }

  int32_t get_encoder_position(uint8_t channel);

  void pin_mode(uint8_t pin, gpio::Flags flags);
  bool digital_read(uint8_t pin);
  void digital_write(uint8_t pin, bool value);

  void setup_neopixel(uint8_t pin, uint8_t num_leds);
  void write_neopixel_buffer(const uint8_t *buf, uint16_t len);
  void show_neopixel();

 protected:
  i2c::ErrorCode write8(SeesawModule mod, uint8_t reg, uint8_t value);
  i2c::ErrorCode write16(SeesawModule mod, uint8_t reg, uint16_t value);
  i2c::ErrorCode write32(SeesawModule mod, uint8_t reg, uint32_t value);
  // Writes `len` bytes of `data` starting at (mod, reg); used for the NeoPixel pixel buffer.
  i2c::ErrorCode write_buf(SeesawModule mod, uint8_t reg, const uint8_t *data, uint16_t len);
  // Seesaw needs the module+register write and the data read to be two distinct I2C transactions --
  // the co-processor needs a brief moment to prepare the response before it can be clocked out.
  i2c::ErrorCode readbuf(SeesawModule mod, uint8_t reg, uint8_t *buf, uint8_t len);

  uint8_t hw_id_{0};
  uint32_t version_{0};
};

/// A single GPIO pin on a Seesaw chip, usable anywhere a normal `pin:` is accepted.
class SeesawGPIOPin : public GPIOPin {
 public:
  void setup() override;
  void pin_mode(gpio::Flags flags) override;
  gpio::Flags get_flags() const override { return this->flags_; }
  bool digital_read() override;
  void digital_write(bool value) override;
  size_t dump_summary(char *buffer, size_t len) const override;

  void set_parent(Seesaw *parent) { this->parent_ = parent; }
  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_inverted(bool inverted) { this->inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { this->flags_ = flags; }

 protected:
  Seesaw *parent_;
  uint8_t pin_;
  bool inverted_;
  gpio::Flags flags_;
};

}  // namespace esphome::seesaw
