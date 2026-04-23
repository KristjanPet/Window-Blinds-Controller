#pragma once
#include <cstddef>
#include <cstdint>
#include <driver/uart.h>
#include <freertos/FreeRTOS.h>

namespace Tmc2209Constants{

constexpr uart_port_t UART_PORT = UART_NUM_2;
constexpr int UART_BAUD_RATE = 115200;
constexpr size_t UART_RX_BUFFER_SIZE = 256;
constexpr size_t UART_TX_BUFFER_SIZE = 256;
constexpr TickType_t UART_TX_TIMEOUT = pdMS_TO_TICKS(20);
constexpr TickType_t UART_ECHO_TIMEOUT = pdMS_TO_TICKS(20);
constexpr TickType_t UART_REPLY_TIMEOUT = pdMS_TO_TICKS(50);

constexpr uint8_t TMC_SYNC = 0x05;
constexpr uint8_t TMC_ADDR = 0x00;
constexpr uint8_t TMC_WRITE_BIT = 0x80;
constexpr uint8_t TMC_READ_MASK = 0x7F;
constexpr uint8_t TMC_REPLY_MASTER_ADDR = 0xFF;
constexpr uint8_t TMC_EXPECTED_VERSION = 0x21;

constexpr size_t TMC_READ_FRAME_SIZE = 4;
constexpr size_t TMC_REPLY_FRAME_SIZE = 8;
constexpr size_t TMC_WRITE_FRAME_SIZE = 8;
constexpr size_t TMC_REGISTER_VALUE_INDEX = 3;
constexpr size_t TMC_CRC_INDEX_READ = TMC_READ_FRAME_SIZE - 1;
constexpr size_t TMC_CRC_INDEX_REPLY = TMC_REPLY_FRAME_SIZE - 1;
constexpr size_t TMC_CRC_INDEX_WRITE = TMC_WRITE_FRAME_SIZE - 1;

constexpr uint8_t REG_GCONF = 0x00;
constexpr uint8_t REG_IFCNT = 0x02;
constexpr uint8_t REG_IOIN = 0x06;
constexpr uint8_t REG_IHOLD_IRUN = 0x10;
constexpr uint8_t REG_TPWMTHRS = 0x13;
constexpr uint8_t REG_TCOOLTHRS = 0x14;
constexpr uint8_t REG_SGTHRS = 0x40;
constexpr uint8_t REG_SG_RESULT = 0x41;
constexpr uint8_t REG_CHOPCONF = 0x6C;

constexpr uint32_t TPWMTHRS_CONFIG = 0;
constexpr uint32_t TCOOLTHRS_CONFIG = 1000;
constexpr uint32_t SGTHRS_MASK = 0xFF;
constexpr uint32_t SG_RESULT_MASK = 0x03FF;

constexpr uint8_t CHOPCONF_MRES_SHIFT = 24;
constexpr uint32_t CHOPCONF_MRES_MASK = 0xFu << CHOPCONF_MRES_SHIFT;
constexpr uint32_t CHOPCONF_BASE = 0x10000053;
constexpr uint32_t CHOPCONF_MICROSTEPS_1_8 = 5u << CHOPCONF_MRES_SHIFT;

constexpr uint8_t IHOLD_SHIFT = 0;
constexpr uint8_t IRUN_SHIFT = 8;
constexpr uint8_t IHOLDDELAY_SHIFT = 16;
constexpr uint32_t IHOLD_VALUE = 8u;
constexpr uint32_t IRUN_VALUE = 31u;
constexpr uint32_t IHOLDDELAY_VALUE = 8u;
constexpr uint32_t IHOLD_IRUN_CONFIG =
    (IHOLD_VALUE << IHOLD_SHIFT) |
    (IRUN_VALUE << IRUN_SHIFT) |
    (IHOLDDELAY_VALUE << IHOLDDELAY_SHIFT);

constexpr uint8_t CONFIG_WRITE_COUNT = 6;

} // namespace Tmc2209Constants
