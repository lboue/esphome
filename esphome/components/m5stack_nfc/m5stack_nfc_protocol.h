#pragma once

#include <cstdint>

namespace esphome {
namespace m5stack_nfc {

namespace st25r3916 {

// I2C address of M5Stack Unit NFC
static constexpr uint8_t I2C_ADDRESS = 0x50;

// -----------------------------------------------------------------------------
// ST25R3916 Register map
// -----------------------------------------------------------------------------

// Register space A
static constexpr uint8_t REG_IO_CONFIGURATION = 0x00;
static constexpr uint8_t REG_IO_CONFIGURATION2 = 0x01;

static constexpr uint8_t REG_OPERATION_CONTROL = 0x02;
static constexpr uint8_t REG_MODE_DEFINITION = 0x03;

static constexpr uint8_t REG_BIT_RATE_DEFINITION = 0x04;

static constexpr uint8_t REG_ISO14443A_NFC_106 = 0x05;

static constexpr uint8_t REG_RECEIVER_CONFIGURATION1 = 0x0B;
static constexpr uint8_t REG_RECEIVER_CONFIGURATION2 = 0x0C;
static constexpr uint8_t REG_RECEIVER_CONFIGURATION3 = 0x0D;
static constexpr uint8_t REG_RECEIVER_CONFIGURATION4 = 0x0E;

static constexpr uint8_t REG_MASK_RECEIVE_TIMER = 0x0F;

static constexpr uint8_t REG_TIMER_EMV_CONTROL = 0x12;

static constexpr uint8_t REG_GENERAL_PURPOSE_TIMER1 = 0x13;
static constexpr uint8_t REG_GENERAL_PURPOSE_TIMER2 = 0x14;

// -----------------------------------------------------------------------------
// ST25R3916 Commands (Direct Commands)
// -----------------------------------------------------------------------------

static constexpr uint8_t CMD_SET_DEFAULT = 0xC1;
static constexpr uint8_t CMD_STOP = 0x02;

static constexpr uint8_t CMD_CLEAR_FIFO = 0xC7;

static constexpr uint8_t CMD_TRANSMIT_WITH_CRC = 0x80;
static constexpr uint8_t CMD_TRANSMIT_WITHOUT_CRC = 0x88;

static constexpr uint8_t CMD_RECEIVE = 0x86;

// -----------------------------------------------------------------------------
// FIFO
// -----------------------------------------------------------------------------

static constexpr uint8_t REG_FIFO_STATUS = 0x1A;
static constexpr uint8_t REG_FIFO_READ = 0x1C;
static constexpr uint8_t REG_FIFO_WRITE = 0x1C;

// -----------------------------------------------------------------------------
// Identification
// -----------------------------------------------------------------------------

static constexpr uint8_t REG_IC_IDENTITY = 0x3F;

static constexpr uint8_t IC_IDENTITY_ST25R3916 = 0x05;

}  // namespace st25r3916

}  // namespace m5stack_nfc
}  // namespace esphome
