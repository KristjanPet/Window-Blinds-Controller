#pragma once
#include <cstdint>
#include <esp_attr.h>
#include <esp_err.h>
#include "BlindsCommandQueue.hpp"
#include "Types.hpp"

/**
 * @brief TMC2209 UART configuration and DIAG event adapter.
 *
 * Owns the TMC2209 UART port and DIAG GPIO interrupt. Register access stays in
 * the driver layer, while DIAG/stall notifications are reported upward through
 * the blinds command queue.
 */
class Tmc2209Driver{
private:
    const TMCUARTDriverPins UARTPins_;
    BlindsCommandQueue& commandsQueue_;
    bool initialized_ = false;

    static void IRAM_ATTR diagIsr(void* arg);
    
public:
    /**
     * @brief Create a TMC2209 driver with UART/DIAG pins and event reporting.
     *
     * @param UARTPins GPIO assignments for UART TX/RX and DIAG input.
     * @param commandsQueue Queue used to report stall/DIAG events.
     */
    Tmc2209Driver(const TMCUARTDriverPins& UARTPins, BlindsCommandQueue& commandsQueue);

    /**
     * @brief Initialize UART communication and DIAG GPIO interrupt handling.
     *
     * @return ESP_OK on success, or an ESP-IDF error from UART/GPIO/ISR setup.
     */
    esp_err_t init();

    /**
     * @brief Write a 32-bit TMC2209 register over UART.
     *
     * @param reg Register address.
     * @param value Register value to write.
     * @return ESP_OK on success, ESP_ERR_INVALID_STATE if init() has not
     *         completed, ESP_FAIL for short writes, or an ESP-IDF UART error.
     */
    esp_err_t writeReg(uint8_t reg, uint32_t value);

    /**
     * @brief Read a 32-bit TMC2209 register over UART.
     *
     * @param reg Register address.
     * @param value Destination populated with the decoded register value.
     * @return ESP_OK on success, ESP_ERR_INVALID_STATE if init() has not
     *         completed, ESP_ERR_TIMEOUT for missing echo/reply bytes,
     *         ESP_ERR_INVALID_RESPONSE for bad echo/header data,
     *         ESP_ERR_INVALID_CRC for CRC mismatch, or another ESP-IDF UART error.
     */
    esp_err_t readReg(uint8_t reg, uint32_t& value);

    /**
     * @brief Read and mask the TMC2209 SG_RESULT value.
     *
     * @param sgResult Destination populated with the StallGuard result.
     * @return ESP_OK on success, or an error propagated from readReg().
     */
    esp_err_t readSgResult(uint16_t& sgResult);

    /**
     * @brief Apply the configured TMC2209 register setup and verify communication.
     *
     * @return ESP_OK on success, ESP_ERR_INVALID_RESPONSE for failed verification,
     *         or an error propagated from register reads/writes.
     */
    esp_err_t configureAndVerify();
};
