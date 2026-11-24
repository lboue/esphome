/*
 * OpenTherm protocol bit-level engine (ESP32 only). Forked from the `opentherm` component's
 * `opentherm.h`/`opentherm.cpp` (ESP32 gptimer path only, ESP8266 timer1 path dropped since this
 * component's CONFIG_SCHEMA restricts it to ESP32), which was itself originally taken from
 * https://github.com/jpraus/arduino-opentherm. Kept as a private copy under this component's own
 * namespace, rather than a cross-component dependency, because `opentherm` is a master-role hub with
 * its own required (non-optional) pin configuration: depending on it would force every user of this
 * (slave/boiler) component to also configure an unrelated, unwanted master hub. Original code is
 * licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International Public
 * License, which is compatible with GPLv3 license, which covers the C++ part of the ESPHome project.
 */

#pragma once

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include "driver/gptimer.h"

namespace esphome::opentherm_slave {

template<class T> constexpr T read_bit(T value, uint8_t bit) { return (value >> bit) & 0x01; }

enum OperationMode {
  IDLE = 0,  // no operation

  LISTEN = 1,    // waiting for transmission to start
  READ = 2,      // reading 32-bit data frame
  RECEIVED = 3,  // data frame received with valid start and stop bit

  WRITE = 4,  // writing data to output
  SENT = 5,   // all data written to output

  ERROR_PROTOCOL = 8,  // protocol error, can happen only during READ
  ERROR_TIMEOUT = 9,   // timeout while waiting for a request, only during LISTEN
  ERROR_TIMER = 10,    // error operating the ESP32 timer
};

enum ProtocolErrorType {
  NO_ERROR = 0,            // No error
  NO_TRANSITION = 1,       // No transition in the middle of the bit
  INVALID_STOP_BIT = 2,    // Stop bit wasn't present when expected
  PARITY_ERROR = 3,        // Parity check didn't pass
  NO_CHANGE_TOO_LONG = 4,  // No level change for too much timer ticks
};

enum TimerErrorType {
  NO_TIMER_ERROR = 0,           // No error
  SET_ALARM_VALUE_ERROR = 1,    // No transition in the middle of the bit
  TIMER_START_ERROR = 2,        // Stop bit wasn't present when expected
  TIMER_PAUSE_ERROR = 3,        // Parity check didn't pass
  SET_COUNTER_VALUE_ERROR = 4,  // No level change for too much timer ticks
};

enum MessageType {
  READ_DATA = 0,
  READ_ACK = 4,
  WRITE_DATA = 1,
  WRITE_ACK = 5,
  INVALID_DATA = 2,
  DATA_INVALID = 6,
  UNKNOWN_DATAID = 7,
};

// Only the Data-IDs this component actually answers are listed; everything else gets
// MessageType::UNKNOWN_DATAID regardless of its id.
enum MessageId {
  STATUS = 0,
  CH_SETPOINT = 1,
  MODULATION_LEVEL = 17,
};

enum BitPositions { STOP_BIT = 33 };

/**
 * Structure to hold an OpenTherm data packet. Use get_f88() or set_f88() to read/write the fixed-point
 * value of a message (the only representation this component needs for Status/CH setpoint/modulation).
 */
struct OpenthermData {
  uint8_t type;
  uint8_t id;
  uint8_t valueHB;
  uint8_t valueLB;

  OpenthermData() : type(0), id(0), valueHB(0), valueLB(0) {}

  float get_f88();
  void set_f88(float value);
};

/**
 * Bit-level OpenTherm engine: listens for an incoming 32-bit frame or sends one, using an ESP32
 * gptimer-driven ISR for Manchester bit timing. Role-agnostic — a master uses it by sending a request
 * then listening for the response; this component (a slave) uses it the other way round.
 */
class OpenTherm {
 public:
  OpenTherm(InternalGPIOPin *in_pin, InternalGPIOPin *out_pin, int32_t device_timeout = 800);

  /// Set up pins and the timer.
  bool initialize();

  /**
   * Start listening for an OpenTherm data packet on the input pin. has_message() becomes true once a
   * valid packet is captured. If device_timeout (from the constructor) is >= 0 and no packet arrives
   * within that many milliseconds, is_error()/is_timeout() become true instead. A negative
   * device_timeout means wait indefinitely.
   */
  void listen();

  /// True once listen() has captured a valid data packet.
  bool has_message() { return mode_ == OperationMode::RECEIVED; }

  /// Retrieve the packet captured by listen(); may be called repeatedly until stop() is called.
  bool get_message(OpenthermData &data);

  /// Send a data packet on the output pin. is_sent() becomes true once the transfer completes.
  void send(OpenthermData &data);

  /// Stop listening/sending, stop the timer, and reset internal state.
  void stop();

  bool is_sent() { return mode_ == OperationMode::SENT; }
  bool is_idle() { return mode_ == OperationMode::IDLE; }
  bool is_error() {
    return mode_ == OperationMode::ERROR_TIMEOUT || mode_ == OperationMode::ERROR_PROTOCOL || mode_ == ERROR_TIMER;
  }
  bool is_timeout() { return mode_ == OperationMode::ERROR_TIMEOUT; }
  bool is_protocol_error() { return mode_ == OperationMode::ERROR_PROTOCOL; }
  bool is_timer_error() { return mode_ == OperationMode::ERROR_TIMER; }
  bool is_active() { return mode_ == LISTEN || mode_ == READ || mode_ == WRITE; }

  OperationMode get_mode() { return mode_; }

  void report_and_reset_timer_error();

  const char *operation_mode_to_str(OperationMode mode);
  const char *message_type_to_str(MessageType message_type);

  static bool timer_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

 private:
  InternalGPIOPin *in_pin_;
  InternalGPIOPin *out_pin_;
  ISRInternalGPIOPin isr_in_pin_;
  ISRInternalGPIOPin isr_out_pin_;

  gptimer_handle_t timer_handle_{nullptr};
  gptimer_alarm_config_t alarm_config_{
      .alarm_count = 0,
      .reload_count = 0,
      .flags = {.auto_reload_on_alarm = true},
  };

  OperationMode mode_;
  ProtocolErrorType error_type_;
  uint32_t capture_;
  uint8_t clock_;
  uint32_t data_;
  uint8_t bit_pos_;
  int32_t timeout_counter_;  // <0 no timeout
  int32_t device_timeout_;

  esp_err_t timer_error_ = ESP_OK;
  TimerErrorType timer_error_type_ = TimerErrorType::NO_TIMER_ERROR;

  bool init_esp32_timer_();
  void start_esp32_timer_(uint64_t alarm_value);
  void stop_timer_();

  void read_();               // data detected, start reading
  void start_read_timer_();   // reading timer_ to sample at 1/5 of manchester code bit length (at 5kHz)
  void start_write_timer_();  // writing timer_ to send manchester code (at 2kHz)
  bool check_parity_(uint32_t val);

  void bit_read_(uint8_t value);
  ProtocolErrorType verify_stop_bit_(uint8_t value);
  void write_bit_(uint8_t high, uint8_t clock);
};

}  // namespace esphome::opentherm_slave
