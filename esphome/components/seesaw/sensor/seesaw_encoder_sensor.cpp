#include "seesaw_encoder_sensor.h"

namespace esphome::seesaw {

void SeesawEncoderSensor::update() { this->publish_state(this->parent_->get_encoder_position(this->channel_)); }

}  // namespace esphome::seesaw
