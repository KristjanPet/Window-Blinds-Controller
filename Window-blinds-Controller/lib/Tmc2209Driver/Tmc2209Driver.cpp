#include "Tmc2209Driver.hpp"

#include <cstring>
#include <driver/uart.h>
#include <esp_check.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "AppConfig.hpp"
#include "Tmc2209Constants.hpp"

static const char* TAG = "TMC2209";
using namespace Tmc2209Constants;

Tmc2209Driver::Tmc2209Driver(const TMCUARTDriverPins& UARTPins): UARTPins_(UARTPins){}

static uint8_t tmcCrc(const uint8_t* data, size_t len)
{
    uint8_t crc = 0;
    // len includes CRC byte at the end; CRC byte must be 0 while calculating
    for (size_t i = 0; i < len - 1; i++) {
        uint8_t currentByte = data[i];
        for (int j = 0; j < 8; j++) {
            if (((crc >> 7) ^ (currentByte & 0x01)) != 0) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc = (crc << 1);
            }
            currentByte >>= 1;
        }
    }
    return crc;
}

static bool bytesEqual(const uint8_t* lhs, const uint8_t* rhs, size_t len)
{
    return memcmp(lhs, rhs, len) == 0;
}

static uint32_t decodeRegisterValue(const uint8_t* frame)
{
    return (static_cast<uint32_t>(frame[TMC_REGISTER_VALUE_INDEX]) << 24) |
           (static_cast<uint32_t>(frame[TMC_REGISTER_VALUE_INDEX + 1]) << 16) |
           (static_cast<uint32_t>(frame[TMC_REGISTER_VALUE_INDEX + 2]) << 8) |
           (static_cast<uint32_t>(frame[TMC_REGISTER_VALUE_INDEX + 3]));
}

esp_err_t Tmc2209Driver::init(){
    uart_config_t uartConfig = {};
    uartConfig.baud_rate = UART_BAUD_RATE;
    uartConfig.data_bits = UART_DATA_8_BITS;
    uartConfig.parity = UART_PARITY_DISABLE;
    uartConfig.stop_bits = UART_STOP_BITS_1;
    uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uartConfig.source_clk = UART_SCLK_DEFAULT;

    ESP_RETURN_ON_ERROR(uart_driver_install(UART_PORT, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE, 0, nullptr, 0),
                        TAG, "Failed to install UART driver");
    ESP_RETURN_ON_ERROR(uart_param_config(UART_PORT, &uartConfig), TAG, "Failed to configure UART");
    ESP_RETURN_ON_ERROR(uart_set_pin(UART_PORT, UARTPins_.TX, UARTPins_.RX,
                                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE),
                        TAG, "Failed to set UART pins");
    ESP_RETURN_ON_ERROR(uart_flush(UART_PORT), TAG, "Failed to flush UART");

    initialized_ = true;
    return ESP_OK;
}

esp_err_t Tmc2209Driver::writeReg(uint8_t reg, uint32_t value)
{
    if (!initialized_) {
        ESP_LOGE(TAG, "writeReg called before init");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t frame[TMC_WRITE_FRAME_SIZE] = {};
    frame[0] = TMC_SYNC;
    frame[1] = TMC_ADDR;
    frame[2] = reg | TMC_WRITE_BIT;
    frame[3] = (value >> 24) & 0xFF;
    frame[4] = (value >> 16) & 0xFF;
    frame[5] = (value >> 8) & 0xFF;
    frame[6] = value & 0xFF;
    frame[TMC_CRC_INDEX_WRITE] = tmcCrc(frame, sizeof(frame));

    ESP_RETURN_ON_ERROR(uart_flush_input(UART_PORT), TAG, "Failed to flush UART input before write");

    int written = uart_write_bytes(UART_PORT, reinterpret_cast<const char*>(frame), sizeof(frame));
    if (written != static_cast<int>(sizeof(frame))) {
        ESP_LOGE(TAG, "writeReg(0x%02X): expected to write %u bytes, wrote %d",
                 reg, static_cast<unsigned>(sizeof(frame)), written);
        return ESP_FAIL;
    }

    ESP_RETURN_ON_ERROR(uart_wait_tx_done(UART_PORT, UART_TX_TIMEOUT), TAG, "UART write timeout");

    return ESP_OK;
}

esp_err_t Tmc2209Driver::readReg(uint8_t reg, uint32_t& value)
{
    if (!initialized_) {
        ESP_LOGE(TAG, "readReg called before init");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t req[TMC_READ_FRAME_SIZE] = {};
    req[0] = TMC_SYNC;
    req[1] = TMC_ADDR;
    req[2] = reg & TMC_READ_MASK;
    req[TMC_CRC_INDEX_READ] = tmcCrc(req, sizeof(req));

    ESP_RETURN_ON_ERROR(uart_flush_input(UART_PORT), TAG, "Failed to flush UART input before read");

    int written = uart_write_bytes(UART_PORT, reinterpret_cast<const char*>(req), sizeof(req));
    if (written != static_cast<int>(sizeof(req))) {
        ESP_LOGE(TAG, "readReg(0x%02X): expected to write %u request bytes, wrote %d",
                 reg, static_cast<unsigned>(sizeof(req)), written);
        return ESP_FAIL;
    }

    ESP_RETURN_ON_ERROR(uart_wait_tx_done(UART_PORT, UART_TX_TIMEOUT), TAG, "UART read request timeout");

    uint8_t echo[TMC_READ_FRAME_SIZE] = {};
    int echoRead = uart_read_bytes(UART_PORT, echo, sizeof(echo), UART_ECHO_TIMEOUT);
    if (echoRead != static_cast<int>(sizeof(echo))) {
        ESP_LOGE(TAG, "readReg(0x%02X): expected %u echo bytes, got %d",
                 reg, static_cast<unsigned>(sizeof(echo)), echoRead);
        return ESP_ERR_TIMEOUT;
    }

    if (!bytesEqual(req, echo, sizeof(req))) {
        ESP_LOGE(TAG, "Bad echo: %02X %02X %02X %02X",
                 echo[0], echo[1], echo[2], echo[3]);
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint8_t reply[TMC_REPLY_FRAME_SIZE] = {};
    int read = uart_read_bytes(UART_PORT, reply, sizeof(reply), UART_REPLY_TIMEOUT);
    if (read != static_cast<int>(sizeof(reply))) {
        ESP_LOGE(TAG, "readReg(0x%02X): expected %u reply bytes, got %d",
                 reg, static_cast<unsigned>(sizeof(reply)), read);
        return ESP_ERR_TIMEOUT;
    }

    if (reply[0] != TMC_SYNC || reply[1] != TMC_REPLY_MASTER_ADDR || reply[2] != reg) {
        ESP_LOGE(TAG, "Bad reply header: %02X %02X %02X", reply[0], reply[1], reply[2]);
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint8_t crc = reply[TMC_CRC_INDEX_REPLY];
    uint8_t calcBuf[TMC_REPLY_FRAME_SIZE];
    memcpy(calcBuf, reply, sizeof(calcBuf));
    calcBuf[TMC_CRC_INDEX_REPLY] = 0x00;
    uint8_t calc = tmcCrc(calcBuf, sizeof(calcBuf));
    if (crc != calc) {
        ESP_LOGE(TAG, "CRC mismatch: got %02X calc %02X", crc, calc);
        return ESP_ERR_INVALID_CRC;
    }

    value = decodeRegisterValue(reply);

    return ESP_OK;
}

esp_err_t Tmc2209Driver::readSgResult(uint16_t& sgResult)
{
    uint32_t rawValue = 0;
    esp_err_t err = readReg(REG_SG_RESULT, rawValue);
    if (err != ESP_OK) {
        return err;
    }

    sgResult = static_cast<uint16_t>(rawValue & SG_RESULT_MASK);
    return ESP_OK;
}

esp_err_t Tmc2209Driver::configureAndVerify()
{
    uint32_t ifcntBefore = 0;
    uint32_t ifcntAfter  = 0;
    uint32_t ioin        = 0;

    ESP_RETURN_ON_ERROR(readReg(REG_IFCNT, ifcntBefore), TAG, "UART FAIL: can't read IFCNT");
    ESP_RETURN_ON_ERROR(writeReg(REG_GCONF, AppConfig::motorGConfig), TAG, "UART FAIL: can't write GCONF");

    uint32_t currentChopconf = 0;
    ESP_RETURN_ON_ERROR(readReg(REG_CHOPCONF, currentChopconf), TAG, "UART FAIL: can't read CHOPCONF");
    ESP_LOGI(TAG, "CHOPCONF before config = 0x%08lX", (unsigned long)currentChopconf);

    uint32_t configuredChopconf = CHOPCONF_BASE;
    configuredChopconf &= ~CHOPCONF_MRES_MASK;
    configuredChopconf |= CHOPCONF_MICROSTEPS_1_8;

    ESP_RETURN_ON_ERROR(writeReg(REG_CHOPCONF, configuredChopconf), TAG, "UART FAIL: can't write CHOPCONF");
    ESP_RETURN_ON_ERROR(writeReg(REG_IHOLD_IRUN, IHOLD_IRUN_CONFIG), TAG, "UART FAIL: can't write IHOLD_IRUN");
    ESP_RETURN_ON_ERROR(readReg(REG_IFCNT, ifcntAfter), TAG, "UART FAIL: can't read IFCNT after config");

    uint8_t expectedIfcnt = (ifcntBefore + CONFIG_WRITE_COUNT) & 0xFF;
    if ((ifcntAfter & 0xFF) != expectedIfcnt) {
        ESP_LOGE(TAG, "UART FAIL: IFCNT before=%lu after=%lu expected=%u",
                 (unsigned long)ifcntBefore, (unsigned long)ifcntAfter, expectedIfcnt);
        return ESP_ERR_INVALID_RESPONSE;
    }

    ESP_LOGI(TAG, "IFCNT before=%lu after=%lu", (unsigned long)ifcntBefore, (unsigned long)ifcntAfter);
    ESP_RETURN_ON_ERROR(readReg(REG_IOIN, ioin), TAG, "UART FAIL: can't read IOIN");

    uint8_t version = (ioin >> 24) & 0xFF;
    ESP_LOGI(TAG, "IOIN = 0x%08lX, VERSION = 0x%02X", (unsigned long)ioin, version);

    if (version != TMC_EXPECTED_VERSION) {
        ESP_LOGE(TAG, "Unexpected TMC2209 version: got 0x%02X expected 0x%02X",
                 version, TMC_EXPECTED_VERSION);
        return ESP_ERR_INVALID_RESPONSE;
    }

    ESP_LOGI(TAG, "TMC2209 UART configured and verified");
    return ESP_OK;
}

void Tmc2209Driver::sgResultTask(void* arg)
{
    auto* self = static_cast<Tmc2209Driver*>(arg);
    if (!self) {
        ESP_LOGE(TAG, "SG_RESULT task started without driver context");
        vTaskDelete(nullptr);
        return;
    }

    while (true) {
        uint16_t sgResult = 0;
        esp_err_t err = self->readSgResult(sgResult);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "%u", sgResult);
        } else {
            ESP_LOGE(TAG, "Failed to read SG_RESULT: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(SG_RESULT_LOG_INTERVAL_MS));
    }
}
