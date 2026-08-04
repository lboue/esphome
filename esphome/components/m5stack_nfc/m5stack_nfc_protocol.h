#pragma once

#include <cstdint>

namespace esphome {
namespace m5stack_nfc {
namespace protocol {

static constexpr uint8_t DEFAULT_ADDRESS = 0x50;

// À compléter après analyse de UnitNFC.cpp
enum Command : uint8_t {};

enum Status : uint8_t {};

}  // namespace protocol
}  // namespace m5stack_nfc
}  // namespace esphome
