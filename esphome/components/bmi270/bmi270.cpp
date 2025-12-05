#include "bmi270.h"
#include <esphome/components/bmi270/bmi2/bmi2.h>
#include <esphome/components/bmi270/bmi2/bmi270.h>
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome {
namespace bmi270 {

static const char *const TAG = "bmi270";

// NOTE: The register map and chip ID below have been copied from the BMI160
// implementation as a starting point. The BMI270 datasheet uses slightly
// different registers and chip ID. Replace CHIPID and registers below with
// BMI270-specific values if needed.

const uint8_t BMI270_REGISTER_CHIPID = 0x00;
// BMI270 chip ID value (from common BMI270/BMI2xx implementations)
// BMI270 chip ID is provided by the vendored bmi270 header as BMI270_CHIP_ID

const uint8_t BMI270_REGISTER_CMD = 0x7E;
enum class Cmd : uint8_t {
  START_FOC = 0x03,
  ACCL_SET_PMU_MODE = 0b00010000,  // last 2 bits are mode
  GYRO_SET_PMU_MODE = 0b00010100,  // last 2 bits are mode
  MAG_SET_PMU_MODE = 0b00011000,   // last 2 bits are mode
  PROG_NVM = 0xA0,
  FIFO_FLUSH = 0xB0,
  INT_RESET = 0xB1,
  SOFT_RESET = 0xB6,
  STEP_CNT_CLR = 0xB2,
};
enum class GyroPmuMode : uint8_t {
  SUSPEND = 0b00,
  NORMAL = 0b01,
  LOW_POWER = 0b10,
};
enum class AcclPmuMode : uint8_t {
  SUSPEND = 0b00,
  NORMAL = 0b01,
  FAST_STARTUP = 0b11,
};
enum class MagPmuMode : uint8_t {
  SUSPEND = 0b00,
  NORMAL = 0b01,
  LOW_POWER = 0b10,
};

const uint8_t BMI270_REGISTER_ACCEL_CONFIG = 0x40;
enum class AcclFilterMode : uint8_t {
  POWER_SAVING = 0b00000000,
  PERF = 0b10000000,
};
enum class AcclBandwidth : uint8_t {
  OSR4_AVG1 = 0b00000000,
  OSR2_AVG2 = 0b00010000,
  NORMAL_AVG4 = 0b00100000,
  RES_AVG8 = 0b00110000,
  RES_AVG16 = 0b01000000,
  RES_AVG32 = 0b01010000,
  RES_AVG64 = 0b01100000,
  RES_AVG128 = 0b01110000,
};
enum class AccelOutputDataRate : uint8_t {
  HZ_25_32 = 0b0001,  // 25/32 Hz
  HZ_25_16 = 0b0010,  // 25/16 Hz
  HZ_25_8 = 0b0011,   // 25/8 Hz
  HZ_25_4 = 0b0100,   // 25/4 Hz
  HZ_25_2 = 0b0101,   // 25/2 Hz
  HZ_25 = 0b0110,     // 25 Hz
  HZ_50 = 0b0111,     // 50 Hz
  HZ_100 = 0b1000,    // 100 Hz
  HZ_200 = 0b1001,    // 200 Hz
  HZ_400 = 0b1010,    // 400 Hz
  HZ_800 = 0b1011,    // 800 Hz
  HZ_1600 = 0b1100,   // 1600 Hz
};
const uint8_t BMI270_REGISTER_ACCEL_RANGE = 0x41;
enum class AccelRange : uint8_t {
  RANGE_2G = 0b0011,
  RANGE_4G = 0b0101,
  RANGE_8G = 0b1000,
  RANGE_16G = 0b1100,
};

const uint8_t BMI270_REGISTER_GYRO_CONFIG = 0x42;
enum class GyroBandwidth : uint8_t {
  OSR4 = 0x00,
  OSR2 = 0x10,
  NORMAL = 0x20,
};
enum class GyroOuputDataRate : uint8_t {
  HZ_25 = 0x06,
  HZ_50 = 0x07,
  HZ_100 = 0x08,
  HZ_200 = 0x09,
  HZ_400 = 0x0A,
  HZ_800 = 0x0B,
  HZ_1600 = 0x0C,
  HZ_3200 = 0x0D,
};
const uint8_t BMI270_REGISTER_GYRO_RANGE = 0x43;
enum class GyroRange : uint8_t {
  RANGE_2000_DPS = 0x0,  // ±2000 °/s
  RANGE_1000_DPS = 0x1,
  RANGE_500_DPS = 0x2,
  RANGE_250_DPS = 0x3,
  RANGE_125_DPS = 0x4,
};

const uint8_t BMI270_REGISTER_DATA_GYRO_X_LSB = 0x0C;
const uint8_t BMI270_REGISTER_DATA_GYRO_X_MSB = 0x0D;
const uint8_t BMI270_REGISTER_DATA_GYRO_Y_LSB = 0x0E;
const uint8_t BMI270_REGISTER_DATA_GYRO_Y_MSB = 0x0F;
const uint8_t BMI270_REGISTER_DATA_GYRO_Z_LSB = 0x10;
const uint8_t BMI270_REGISTER_DATA_GYRO_Z_MSB = 0x11;
const uint8_t BMI270_REGISTER_DATA_ACCEL_X_LSB = 0x12;
const uint8_t BMI270_REGISTER_DATA_ACCEL_X_MSB = 0x13;
const uint8_t BMI270_REGISTER_DATA_ACCEL_Y_LSB = 0x14;
const uint8_t BMI270_REGISTER_DATA_ACCEL_Y_MSB = 0x15;
const uint8_t BMI270_REGISTER_DATA_ACCEL_Z_LSB = 0x16;
const uint8_t BMI270_REGISTER_DATA_ACCEL_Z_MSB = 0x17;
const uint8_t BMI270_REGISTER_DATA_TEMP_LSB = 0x20;
const uint8_t BMI270_REGISTER_DATA_TEMP_MSB = 0x21;

const float GRAVITY_EARTH = 9.80665f;

// --- BMI2 platform wrappers -------------------------------------------------
// These adapt the BMI2 SDK read/write/delay callbacks to esphome's I2CDevice.
static int8_t bmi2_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
  if (intf_ptr == nullptr)
    return BMI2_E_NULL_PTR;
  // intf_ptr points to the BMI270Component instance; call its safe wrapper
  auto comp = static_cast<BMI270Component *>(intf_ptr);
  if (comp == nullptr)
    return BMI2_E_NULL_PTR;
  i2c::ErrorCode err = comp->bmi2_read_register_cb(reg_addr, reg_data, (size_t) len);
  return (err == i2c::ERROR_OK) ? BMI2_INTF_RET_SUCCESS : BMI2_E_COM_FAIL;
}

static int8_t bmi2_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) {
  if (intf_ptr == nullptr)
    return BMI2_E_NULL_PTR;
  auto comp = static_cast<BMI270Component *>(intf_ptr);
  if (comp == nullptr)
    return BMI2_E_NULL_PTR;
  i2c::ErrorCode err = comp->bmi2_write_register_cb(reg_addr, reg_data, (size_t) len);
  return (err == i2c::ERROR_OK) ? BMI2_INTF_RET_SUCCESS : BMI2_E_COM_FAIL;
}

static void bmi2_delay_us(uint32_t period, void * /*intf_ptr*/) { delayMicroseconds(period); }

static float lsb_to_mps2(int16_t val, float g_range, uint8_t bit_width) {
  double power = 2;
  float half_scale = (float) ((pow((double) power, (double) bit_width) / 2.0));
  return (GRAVITY_EARTH * val * g_range) / half_scale;
}

static float lsb_to_dps(int16_t val, float dps, uint8_t bit_width) {
  double power = 2;
  float half_scale = (float) ((pow((double) power, (double) bit_width) / 2.0));
  return (dps / (half_scale)) * (val);
}

void BMI270Component::internal_setup_(int stage) {
  switch (stage) {
    case 0:
      // If we haven't performed a soft reset yet, do it now and wait.
      if (!this->did_reset_) {
        ESP_LOGD(TAG, "Performing soft reset of BMI270");
        if (!this->write_byte(BMI270_REGISTER_CMD, (uint8_t) Cmd::SOFT_RESET)) {
          ESP_LOGW(TAG, "Failed to write SOFT_RESET");
          this->mark_failed();
          return;
        }
        // wait for reset to complete per datasheet
        this->set_timeout(100, [this]() {
          this->did_reset_ = true;
          this->internal_setup_(0);
        });
        return;
      }

      uint8_t chipid;
      if (!this->read_byte(BMI270_REGISTER_CHIPID, &chipid) || (chipid != BMI270_CHIP_ID)) {
        ESP_LOGW(TAG, "Unexpected CHIP ID: 0x%02X", chipid);
        this->mark_failed();
        return;
      }
      ESP_LOGD(TAG, "Read CHIP ID: 0x%02X", chipid);

      // Try BMI2 initialization (preferred). If it fails, fall back to legacy
      // register-based setup below.
      if (!this->is_initialized_) {
        ESP_LOGD(TAG, "Initializing BMI2 API for BMI270");
        // populate bmi2 device callbacks and pointers
        this->sensor_.read = bmi2_i2c_read;
        this->sensor_.write = bmi2_i2c_write;
        this->sensor_.delay_us = bmi2_delay_us;
        this->sensor_.intf_ptr = static_cast<void *>(this);
        this->sensor_.intf = BMI2_I2C_INTF;
        this->sensor_.read_write_len = 128;
        // Do not set config_file_ptr/size here; bmi270_init will populate device
        int8_t rslt = bmi270_init(&this->sensor_);
        if (rslt != BMI2_OK) {
          ESP_LOGW(TAG, "BMI2 init failed (%d), falling back to legacy init", rslt);
          // leave is_initialized_ false -> legacy path will run
        } else {
          ESP_LOGD(TAG, "BMI2 initialized OK");
          // configure accel & gyro with sane defaults and enable them
          struct bmi2_sens_config configs[2];
          configs[0].type = BMI2_ACCEL;
          configs[1].type = BMI2_GYRO;
          if (bmi2_get_sensor_config(configs, 2, &this->sensor_) == BMI2_OK) {
            configs[0].cfg.acc.odr = BMI2_ACC_ODR_200HZ;
            configs[0].cfg.acc.range = BMI2_ACC_RANGE_2G;
            configs[0].cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;
            configs[0].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;

            configs[1].cfg.gyr.odr = BMI2_GYR_ODR_200HZ;
            configs[1].cfg.gyr.range = BMI2_GYR_RANGE_2000;
            configs[1].cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;
            configs[1].cfg.gyr.noise_perf = BMI2_POWER_OPT_MODE;
            configs[1].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;

            if (bmi2_set_sensor_config(configs, 2, &this->sensor_) == BMI2_OK) {
              uint8_t sens_list[2] = {BMI2_ACCEL, BMI2_GYRO};
              if (bmi2_sensor_enable(sens_list, 2, &this->sensor_) == BMI2_OK) {
                ESP_LOGD(TAG, "BMI2 accel+gyro enabled");
                this->is_initialized_ = true;
                this->sensors_active_ = true;
                // store some scalars for conversions
                this->accel_sensitivity_ = (float) configs[0].cfg.acc.range;  // e.g., 2
                this->gyro_sensitivity_ = (float) (configs[1].cfg.gyr.range == BMI2_GYR_RANGE_2000 ? 2000 : 0);
                this->setup_complete_ = true;
                return;
              }
            }
          }
          ESP_LOGW(TAG, "BMI2 configuration/enable failed, falling back to legacy register init");
        }
      }

      ESP_LOGV(TAG, "  Bringing accelerometer out of sleep");
      if (!this->write_byte(BMI270_REGISTER_CMD, (uint8_t) Cmd::ACCL_SET_PMU_MODE | (uint8_t) AcclPmuMode::NORMAL)) {
        this->mark_failed();
        return;
      }
      ESP_LOGV(TAG, "  Waiting for accelerometer to wake up");
      // need to wait (max delay in datasheet) because we can't send commands while another is in progress
      // min 5ms, 10ms - short wait is enough for accelerometer
      this->set_timeout(10, [this]() { this->internal_setup_(1); });
      break;

    case 1:
      ESP_LOGV(TAG, "  Bringing gyroscope out of sleep");
      if (!this->write_byte(BMI270_REGISTER_CMD, (uint8_t) Cmd::GYRO_SET_PMU_MODE | (uint8_t) GyroPmuMode::NORMAL)) {
        this->mark_failed();
        return;
      }
      ESP_LOGV(TAG, "  Waiting for gyroscope to wake up");
      // wait between 51 & 81ms according to datasheet — use 100ms to be safe
      this->set_timeout(100, [this]() { this->internal_setup_(2); });
      break;

    case 2:
      ESP_LOGV(TAG, "  Setting up Gyro Config");
      uint8_t gyro_config = (uint8_t) GyroBandwidth::OSR4 | (uint8_t) GyroOuputDataRate::HZ_25;
      ESP_LOGV(TAG, "  Output gyro_config: 0b" BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(gyro_config));
      if (!this->write_byte(BMI270_REGISTER_GYRO_CONFIG, gyro_config)) {
        this->mark_failed();
        return;
      }
      ESP_LOGV(TAG, "  Setting up Gyro Range");
      uint8_t gyro_range = (uint8_t) GyroRange::RANGE_2000_DPS;
      ESP_LOGV(TAG, "  Output gyro_range: 0b" BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(gyro_range));
      if (!this->write_byte(BMI270_REGISTER_GYRO_RANGE, gyro_range)) {
        this->mark_failed();
        return;
      }

      ESP_LOGV(TAG, "  Setting up Accel Config");
      uint8_t accel_config =
          (uint8_t) AcclFilterMode::PERF | (uint8_t) AcclBandwidth::RES_AVG16 | (uint8_t) AccelOutputDataRate::HZ_25;
      ESP_LOGV(TAG, "  Output accel_config: 0b" BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(accel_config));
      if (!this->write_byte(BMI270_REGISTER_ACCEL_CONFIG, accel_config)) {
        this->mark_failed();
        return;
      }
      ESP_LOGV(TAG, "  Setting up Accel Range");
      uint8_t accel_range = (uint8_t) AccelRange::RANGE_16G;
      ESP_LOGV(TAG, "  Output accel_range: 0b" BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(accel_range));
      if (!this->write_byte(BMI270_REGISTER_ACCEL_RANGE, accel_range)) {
        this->mark_failed();
        return;
      }

      // Read back config registers to verify writes succeeded
      uint8_t read_val;
      if (this->read_byte(BMI270_REGISTER_GYRO_CONFIG, &read_val)) {
        ESP_LOGD(TAG, "GYRO_CONFIG readback: 0x%02X", read_val);
      } else {
        ESP_LOGW(TAG, "Failed to read back GYRO_CONFIG");
      }
      if (this->read_byte(BMI270_REGISTER_GYRO_RANGE, &read_val)) {
        ESP_LOGD(TAG, "GYRO_RANGE readback: 0x%02X", read_val);
      } else {
        ESP_LOGW(TAG, "Failed to read back GYRO_RANGE");
      }
      if (this->read_byte(BMI270_REGISTER_ACCEL_CONFIG, &read_val)) {
        ESP_LOGD(TAG, "ACCEL_CONFIG readback: 0x%02X", read_val);
      } else {
        ESP_LOGW(TAG, "Failed to read back ACCEL_CONFIG");
      }
      if (this->read_byte(BMI270_REGISTER_ACCEL_RANGE, &read_val)) {
        ESP_LOGD(TAG, "ACCEL_RANGE readback: 0x%02X", read_val);
      } else {
        ESP_LOGW(TAG, "Failed to read back ACCEL_RANGE");
      }

      this->setup_complete_ = true;
  }
}

// BMI2 I2C wrapper implementations (public methods)
i2c::ErrorCode BMI270Component::bmi2_read_register_cb(uint8_t reg, uint8_t *data, size_t len) {
  if (this->bus_ == nullptr) {
    return i2c::ERROR_NOT_INITIALIZED;
  }
  return this->read_register(reg, data, len);
}

i2c::ErrorCode BMI270Component::bmi2_write_register_cb(uint8_t reg, const uint8_t *data, size_t len) {
  if (this->bus_ == nullptr) {
    return i2c::ERROR_NOT_INITIALIZED;
  }
  return this->write_register(reg, data, len);
}

void BMI270Component::setup() { this->internal_setup_(0); }
void BMI270Component::dump_config() {
  ESP_LOGCONFIG(TAG, "BMI270:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
  }
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Acceleration X", this->accel_x_sensor_);
  LOG_SENSOR("  ", "Acceleration Y", this->accel_y_sensor_);
  LOG_SENSOR("  ", "Acceleration Z", this->accel_z_sensor_);
  LOG_SENSOR("  ", "Gyro X", this->gyro_x_sensor_);
  LOG_SENSOR("  ", "Gyro Y", this->gyro_y_sensor_);
  LOG_SENSOR("  ", "Gyro Z", this->gyro_z_sensor_);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
}

i2c::ErrorCode BMI270Component::read_le_int16_(uint8_t reg, int16_t *value, uint8_t len) {
  uint8_t raw_data[len * 2];
  // read using read_register because we have little-endian data, and read_bytes_16 will swap it
  i2c::ErrorCode err = this->read_register(reg, raw_data, len * 2);
  if (err != i2c::ERROR_OK) {
    return err;
  }
  // Debug: log raw bytes read from the device
  ESP_LOGD(TAG, "I2C read reg=0x%02X len=%d ->", reg, len * 2);
  for (int i = 0; i < len * 2; i++) {
    ESP_LOGD(TAG, "  raw[%d]=0x%02X", i, raw_data[i]);
  }
  for (int i = 0; i < len; i++) {
    value[i] = (int16_t) ((uint16_t) raw_data[i * 2] | ((uint16_t) raw_data[i * 2 + 1] << 8));
  }
  return err;
}

void BMI270Component::update() {
  if (!this->setup_complete_) {
    return;
  }
  ESP_LOGV(TAG, "    Updating BMI270");

  // If we initialized the BMI2 API, prefer reading parsed sensor data from it.
  if (this->is_initialized_ && this->sensors_active_) {
    struct bmi2_sens_data sens_data = {{0}};
    int8_t rslt = bmi2_get_sensor_data(&sens_data, &this->sensor_);
    if (rslt != BMI2_OK) {
      ESP_LOGW(TAG, "bmi2_get_sensor_data failed: %d", rslt);
      this->status_set_warning();
      return;
    }

    // Ensure data-ready bits for both accel and gyro
    if (!((sens_data.status & BMI2_DRDY_ACC) && (sens_data.status & BMI2_DRDY_GYR))) {
      ESP_LOGV(TAG, "BMI2 did not report both ACC and GYR data-ready (status=0x%02X)", sens_data.status);
      this->status_set_warning();
      return;
    }

    float accel_x = lsb_to_mps2(sens_data.acc.x, this->accel_sensitivity_ > 0 ? this->accel_sensitivity_ : 2.0f,
                                this->sensor_.resolution);
    float accel_y = lsb_to_mps2(sens_data.acc.y, this->accel_sensitivity_ > 0 ? this->accel_sensitivity_ : 2.0f,
                                this->sensor_.resolution);
    float accel_z = lsb_to_mps2(sens_data.acc.z, this->accel_sensitivity_ > 0 ? this->accel_sensitivity_ : 2.0f,
                                this->sensor_.resolution);

    float gyro_x = lsb_to_dps(sens_data.gyr.x, this->gyro_sensitivity_ > 0 ? this->gyro_sensitivity_ : 2000.0f,
                              this->sensor_.resolution);
    float gyro_y = lsb_to_dps(sens_data.gyr.y, this->gyro_sensitivity_ > 0 ? this->gyro_sensitivity_ : 2000.0f,
                              this->sensor_.resolution);
    float gyro_z = lsb_to_dps(sens_data.gyr.z, this->gyro_sensitivity_ > 0 ? this->gyro_sensitivity_ : 2000.0f,
                              this->sensor_.resolution);

    // Get temperature via BMI2 API if available, otherwise fallback to register read
    float temperature = NAN;
    int16_t bmi2_temp_raw = 0;
    if (bmi2_get_temperature_data(&bmi2_temp_raw, &this->sensor_) == BMI2_OK) {
      // vendor formula: temperature_value = (float)(((float)((int16_t)temperature_data)) / 512.0) + 23.0
      temperature = (float) bmi2_temp_raw / 512.0f + 23.0f;
    } else {
      int16_t raw_temperature;
      if (this->read_le_int16_(BMI270_REGISTER_DATA_TEMP_LSB, &raw_temperature, 1) == i2c::ERROR_OK)
        temperature = (float) raw_temperature / (float) INT16_MAX * 64.5f + 23.f;
    }

    ESP_LOGD(TAG,
             "Got accel={x=%.3f m/s², y=%.3f m/s², z=%.3f m/s²}, "
             "gyro={x=%.3f °/s, y=%.3f °/s, z=%.3f °/s}, temp=%.3f°C",
             accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, temperature);

    if (this->accel_x_sensor_ != nullptr)
      this->accel_x_sensor_->publish_state(accel_x);
    if (this->accel_y_sensor_ != nullptr)
      this->accel_y_sensor_->publish_state(accel_y);
    if (this->accel_z_sensor_ != nullptr)
      this->accel_z_sensor_->publish_state(accel_z);

    if (this->temperature_sensor_ != nullptr && !std::isnan(temperature))
      this->temperature_sensor_->publish_state(temperature);

    if (this->gyro_x_sensor_ != nullptr)
      this->gyro_x_sensor_->publish_state(gyro_x);
    if (this->gyro_y_sensor_ != nullptr)
      this->gyro_y_sensor_->publish_state(gyro_y);
    if (this->gyro_z_sensor_ != nullptr)
      this->gyro_z_sensor_->publish_state(gyro_z);

    this->status_clear_warning();
    return;
  }

  // Legacy register-based fallback (unchanged)
  int16_t data[6];
  if (this->read_le_int16_(BMI270_REGISTER_DATA_GYRO_X_LSB, data, 6) != i2c::ERROR_OK) {
    this->status_set_warning();
    return;
  }

  float gyro_x = (float) data[0] / (float) INT16_MAX * 2000.f;
  float gyro_y = (float) data[1] / (float) INT16_MAX * 2000.f;
  float gyro_z = (float) data[2] / (float) INT16_MAX * 2000.f;
  float accel_x = (float) data[3] / (float) INT16_MAX * 16 * GRAVITY_EARTH;
  float accel_y = (float) data[4] / (float) INT16_MAX * 16 * GRAVITY_EARTH;
  float accel_z = (float) data[5] / (float) INT16_MAX * 16 * GRAVITY_EARTH;

  int16_t raw_temperature;
  if (this->read_le_int16_(BMI270_REGISTER_DATA_TEMP_LSB, &raw_temperature, 1) != i2c::ERROR_OK) {
    this->status_set_warning();
    return;
  }
  float temperature = (float) raw_temperature / (float) INT16_MAX * 64.5f + 23.f;

  ESP_LOGD(TAG,
           "Got accel={x=%.3f m/s², y=%.3f m/s², z=%.3f m/s²}, "
           "gyro={x=%.3f °/s, y=%.3f °/s, z=%.3f °/s}, temp=%.3f°C",
           accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, temperature);

  if (this->accel_x_sensor_ != nullptr)
    this->accel_x_sensor_->publish_state(accel_x);
  if (this->accel_y_sensor_ != nullptr)
    this->accel_y_sensor_->publish_state(accel_y);
  if (this->accel_z_sensor_ != nullptr)
    this->accel_z_sensor_->publish_state(accel_z);

  if (this->temperature_sensor_ != nullptr)
    this->temperature_sensor_->publish_state(temperature);

  if (this->gyro_x_sensor_ != nullptr)
    this->gyro_x_sensor_->publish_state(gyro_x);
  if (this->gyro_y_sensor_ != nullptr)
    this->gyro_y_sensor_->publish_state(gyro_y);
  if (this->gyro_z_sensor_ != nullptr)
    this->gyro_z_sensor_->publish_state(gyro_z);

  this->status_clear_warning();
}
float BMI270Component::get_setup_priority() const { return setup_priority::DATA; }

}  // namespace bmi270
}  // namespace esphome
