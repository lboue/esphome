#include "opentherm_slave.h"

namespace esphome::opentherm_slave {

ESPHOME_LOG_TAG(TAG, "opentherm_slave");

void OpenthermSlaveHub::setup() {
  // Unlike a master waiting for a reply, a boiler has no deadline of its own to wait for the next
  // request, so a negative device_timeout tells the engine's listen() to never time out.
  this->opentherm_ = std::make_unique<OpenTherm>(this->in_pin_, this->out_pin_, -1);
  if (!this->opentherm_->initialize()) {
    this->mark_failed(LOG_STR("Failed to initialize OpenTherm bus"));
    return;
  }
  this->opentherm_->listen();
}

void OpenthermSlaveHub::loop() {
  if (this->opentherm_->has_message()) {
    OpenthermData data;
    this->opentherm_->get_message(data);
    this->build_response_(data);
    this->opentherm_->send(data);
  } else if (this->opentherm_->is_sent() || this->opentherm_->is_error()) {
    this->opentherm_->listen();
  }
}

void OpenthermSlaveHub::build_response_(OpenthermData &data) {
  switch (data.id) {
    case MessageId::STATUS:
      data.type = MessageType::READ_ACK;
      data.valueHB = 0;
      data.valueLB = (this->fault_ ? 0x01 : 0) | (this->ch_active_ ? 0x02 : 0) | (this->dhw_active_ ? 0x04 : 0) |
                     (this->flame_active_ ? 0x08 : 0);
      break;
    case MessageId::CH_SETPOINT: {
      float const setpoint = data.get_f88();
      data.type = MessageType::WRITE_ACK;  // echoes the value the master wrote
#ifdef USE_SENSOR
      PUBLISH_STATE(this->ch_setpoint_sensor_, setpoint);
#endif
      break;
    }
    case MessageId::MODULATION_LEVEL:
      data.type = MessageType::READ_ACK;
      data.set_f88(this->modulation_level_);
      break;
    default:
      data.type = MessageType::UNKNOWN_DATAID;
      data.valueHB = 0;
      data.valueLB = 0;
      break;
  }
}

void OpenthermSlaveHub::dump_config() {
  ESP_LOGCONFIG(TAG, "OpenTherm Slave:");
  LOG_PIN("  In Pin: ", this->in_pin_);
  LOG_PIN("  Out Pin: ", this->out_pin_);
}

}  // namespace esphome::opentherm_slave
