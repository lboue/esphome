/*
 * OpenTherm protocol bit-level engine (ESP32 only). See opentherm_protocol.h for provenance.
 */

#include "opentherm_protocol.h"
#include "esp_err.h"

namespace esphome::opentherm_slave {

ESPHOME_LOG_TAG(TAG, "opentherm_slave.protocol");

OpenTherm::OpenTherm(InternalGPIOPin *in_pin, InternalGPIOPin *out_pin, int32_t device_timeout)
    : in_pin_(in_pin),
      out_pin_(out_pin),
      mode_(OperationMode::IDLE),
      error_type_(ProtocolErrorType::NO_ERROR),
      capture_(0),
      clock_(0),
      data_(0),
      bit_pos_(0),
      timeout_counter_(-1),
      device_timeout_(device_timeout) {
  this->isr_in_pin_ = in_pin->to_isr();
  this->isr_out_pin_ = out_pin->to_isr();
}

bool OpenTherm::initialize() {
  this->in_pin_->pin_mode(gpio::FLAG_INPUT);
  this->in_pin_->setup();
  this->out_pin_->pin_mode(gpio::FLAG_OUTPUT);
  this->out_pin_->setup();
  this->out_pin_->digital_write(true);

  return this->init_esp32_timer_();
}

void OpenTherm::listen() {
  this->stop_timer_();
  this->timeout_counter_ = this->device_timeout_ * 5;  // timer_ ticks at 5 ticks/ms

  this->mode_ = OperationMode::LISTEN;
  this->data_ = 0;
  this->bit_pos_ = 0;

  this->start_read_timer_();
}

void OpenTherm::send(OpenthermData &data) {
  this->stop_timer_();
  this->data_ = data.type;
  this->data_ = (this->data_ << 12) | data.id;
  this->data_ = (this->data_ << 8) | data.valueHB;
  this->data_ = (this->data_ << 8) | data.valueLB;
  if (!check_parity_(this->data_)) {
    this->data_ = this->data_ | 0x80000000;
  }

  this->clock_ = 1;     // clock starts at HIGH
  this->bit_pos_ = 33;  // count down (33 == start bit, 32-1 data, 0 == stop bit)
  this->mode_ = OperationMode::WRITE;

  this->start_write_timer_();
}

bool OpenTherm::get_message(OpenthermData &data) {
  if (this->mode_ == OperationMode::RECEIVED) {
    data.type = (this->data_ >> 28) & 0x7;
    data.id = (this->data_ >> 16) & 0xFF;
    data.valueHB = (this->data_ >> 8) & 0xFF;
    data.valueLB = this->data_ & 0xFF;
    return true;
  }
  return false;
}

void OpenTherm::stop() {
  this->stop_timer_();
  this->mode_ = OperationMode::IDLE;
  this->out_pin_->digital_write(true);
}

void IRAM_ATTR OpenTherm::read_() {
  this->data_ = 0;
  this->bit_pos_ = 0;
  this->mode_ = OperationMode::READ;
  this->capture_ = 1;         // reset counter and add as if read start bit
  this->clock_ = 1;           // clock is high at the start of comm
  this->start_read_timer_();  // get us into 1/4 of manchester code. 5 timer ticks constitute 1 ms, which is 1 bit
                              // period in OpenTherm.
}

bool IRAM_ATTR OpenTherm::timer_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
  auto *arg = static_cast<OpenTherm *>(user_ctx);
  if (arg->mode_ == OperationMode::LISTEN) {
    if (arg->timeout_counter_ == 0) {
      arg->mode_ = OperationMode::ERROR_TIMEOUT;
      arg->stop_timer_();
      return false;
    }
    bool const value = arg->isr_in_pin_.digital_read();
    if (value) {  // incoming data (rising signal)
      arg->read_();
    }
    if (arg->timeout_counter_ > 0) {
      arg->timeout_counter_--;
    }
  } else if (arg->mode_ == OperationMode::READ) {
    bool const value = arg->isr_in_pin_.digital_read();
    uint8_t const last = (arg->capture_ & 1);
    if (value != last) {
      // transition of signal from last sampling
      if (arg->clock_ == 1 && arg->capture_ > 0xF) {
        // no transition in the middle of the bit
        arg->mode_ = OperationMode::ERROR_PROTOCOL;
        arg->error_type_ = ProtocolErrorType::NO_TRANSITION;
        arg->stop_timer_();
        return false;
      } else if (arg->clock_ == 1 || arg->capture_ > 0xF) {
        // transition in the middle of the bit OR no transition between two bit, both are valid data points
        if (arg->bit_pos_ == BitPositions::STOP_BIT) {
          // expecting stop bit
          auto stop_bit_error = arg->verify_stop_bit_(last);
          if (stop_bit_error == ProtocolErrorType::NO_ERROR) {
            arg->mode_ = OperationMode::RECEIVED;
            arg->stop_timer_();
            return false;
          } else {
            // end of data not verified, invalid data
            arg->mode_ = OperationMode::ERROR_PROTOCOL;
            arg->error_type_ = stop_bit_error;
            arg->stop_timer_();
            return false;
          }
        } else {
          // normal data point at clock high
          arg->bit_read_(last);
          arg->clock_ = 0;
        }
      } else {
        // clock low, not a data point, switch clock
        arg->clock_ = 1;
      }
      arg->capture_ = 1;  // reset counter
    } else if (arg->capture_ > 0xFF) {
      // no change for too long, invalid manchester encoding
      arg->mode_ = OperationMode::ERROR_PROTOCOL;
      arg->error_type_ = ProtocolErrorType::NO_CHANGE_TOO_LONG;
      arg->stop_timer_();
      return false;
    }
    arg->capture_ = (arg->capture_ << 1) | value;
  } else if (arg->mode_ == OperationMode::WRITE) {
    // write data to pin
    if (arg->bit_pos_ == 33 || arg->bit_pos_ == 0) {  // start bit
      arg->write_bit_(1, arg->clock_);
    } else {  // data bits
      arg->write_bit_(read_bit(arg->data_, arg->bit_pos_ - 1), arg->clock_);
    }
    if (arg->clock_ == 0) {
      if (arg->bit_pos_ <= 0) {            // check termination
        arg->mode_ = OperationMode::SENT;  // all data written
        arg->stop_timer_();
      }
      arg->bit_pos_--;
      arg->clock_ = 1;
    } else {
      arg->clock_ = 0;
    }
  }

  return false;
}

void IRAM_ATTR OpenTherm::bit_read_(uint8_t value) {
  this->data_ = (this->data_ << 1) | value;
  this->bit_pos_++;
}

ProtocolErrorType IRAM_ATTR OpenTherm::verify_stop_bit_(uint8_t value) {
  if (value) {  // stop bit detected
    return check_parity_(this->data_) ? ProtocolErrorType::NO_ERROR : ProtocolErrorType::PARITY_ERROR;
  } else {  // no stop bit detected, error
    return ProtocolErrorType::INVALID_STOP_BIT;
  }
}

void IRAM_ATTR OpenTherm::write_bit_(uint8_t high, uint8_t clock) {
  if (clock == 1) {                           // left part of manchester encoding
    this->isr_out_pin_.digital_write(!high);  // low means logical 1 to protocol
  } else {                                    // right part of manchester encoding
    this->isr_out_pin_.digital_write(high);   // high means logical 0 to protocol
  }
}

bool OpenTherm::init_esp32_timer_() {
  // 80MHz / 80 = 1MHz resolution (1µs per tick)
  gptimer_config_t config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = 1000000,
  };

  esp_err_t result = gptimer_new_timer(&config, &this->timer_handle_);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create timer: %s", esp_err_to_name(result));
    return false;
  }

  gptimer_event_callbacks_t cbs = {
      .on_alarm = OpenTherm::timer_isr,
  };
  result = gptimer_register_event_callbacks(this->timer_handle_, &cbs, this);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register timer callback: %s", esp_err_to_name(result));
    gptimer_del_timer(this->timer_handle_);
    this->timer_handle_ = nullptr;
    return false;
  }

  result = gptimer_enable(this->timer_handle_);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to enable timer: %s", esp_err_to_name(result));
    gptimer_del_timer(this->timer_handle_);
    this->timer_handle_ = nullptr;
    return false;
  }

  return true;
}

void IRAM_ATTR OpenTherm::start_esp32_timer_(uint64_t alarm_value) {
  // We will report timer errors outside of interrupt handler
  this->timer_error_ = ESP_OK;
  this->timer_error_type_ = TimerErrorType::NO_TIMER_ERROR;

  this->alarm_config_.alarm_count = alarm_value;
  this->timer_error_ = gptimer_set_alarm_action(this->timer_handle_, &this->alarm_config_);
  if (this->timer_error_ != ESP_OK) {
    this->timer_error_type_ = TimerErrorType::SET_ALARM_VALUE_ERROR;
    return;
  }
  this->timer_error_ = gptimer_start(this->timer_handle_);
  if (this->timer_error_ != ESP_OK) {
    this->timer_error_type_ = TimerErrorType::TIMER_START_ERROR;
  }
}

void OpenTherm::report_and_reset_timer_error() {
  if (this->timer_error_ == ESP_OK) {
    return;
  }

  ESP_LOGE(TAG, "Error occurred while manipulating timer (code %d): %s", this->timer_error_type_,
           esp_err_to_name(this->timer_error_));

  this->timer_error_ = ESP_OK;
  this->timer_error_type_ = NO_TIMER_ERROR;
}

// 5 kHz timer_
void IRAM_ATTR OpenTherm::start_read_timer_() {
  InterruptLock const lock;
  this->start_esp32_timer_(200);
}

// 2 kHz timer_
void IRAM_ATTR OpenTherm::start_write_timer_() {
  InterruptLock const lock;
  this->start_esp32_timer_(500);
}

void IRAM_ATTR OpenTherm::stop_timer_() {
  InterruptLock const lock;
  // We will report timer errors outside of interrupt handler
  this->timer_error_ = ESP_OK;
  this->timer_error_type_ = TimerErrorType::NO_TIMER_ERROR;

  this->timer_error_ = gptimer_stop(this->timer_handle_);
  if (this->timer_error_ != ESP_OK) {
    this->timer_error_type_ = TimerErrorType::TIMER_PAUSE_ERROR;
    return;
  }
  this->timer_error_ = gptimer_set_raw_count(this->timer_handle_, 0);
  if (this->timer_error_ != ESP_OK) {
    this->timer_error_type_ = TimerErrorType::SET_COUNTER_VALUE_ERROR;
  }
}

// https://stackoverflow.com/questions/21617970/how-to-check-if-value-has-even-parity-of-bits-or-odd
bool IRAM_ATTR OpenTherm::check_parity_(uint32_t val) {
  val ^= val >> 16;
  val ^= val >> 8;
  val ^= val >> 4;
  val ^= val >> 2;
  val ^= val >> 1;
  return (~val) & 1;
}

#define TO_STRING_MEMBER(name) \
  case name: \
    return #name;

const char *OpenTherm::operation_mode_to_str(OperationMode mode) {
  switch (mode) {
    TO_STRING_MEMBER(IDLE)
    TO_STRING_MEMBER(LISTEN)
    TO_STRING_MEMBER(READ)
    TO_STRING_MEMBER(RECEIVED)
    TO_STRING_MEMBER(WRITE)
    TO_STRING_MEMBER(SENT)
    TO_STRING_MEMBER(ERROR_PROTOCOL)
    TO_STRING_MEMBER(ERROR_TIMEOUT)
    TO_STRING_MEMBER(ERROR_TIMER)
    default:
      return "<INVALID>";
  }
}

const char *OpenTherm::message_type_to_str(MessageType message_type) {
  switch (message_type) {
    TO_STRING_MEMBER(READ_DATA)
    TO_STRING_MEMBER(READ_ACK)
    TO_STRING_MEMBER(WRITE_DATA)
    TO_STRING_MEMBER(WRITE_ACK)
    TO_STRING_MEMBER(INVALID_DATA)
    TO_STRING_MEMBER(DATA_INVALID)
    TO_STRING_MEMBER(UNKNOWN_DATAID)
    default:
      return "<INVALID>";
  }
}

float OpenthermData::get_f88() {
  int16_t const value = (this->valueHB << 8) | this->valueLB;
  return ((float) value) / 256.0f;
}

void OpenthermData::set_f88(float value) {
  int16_t const raw = (int16_t) (value * 256);
  this->valueLB = raw & 0xFF;
  this->valueHB = (raw >> 8) & 0xFF;
}

}  // namespace esphome::opentherm_slave
