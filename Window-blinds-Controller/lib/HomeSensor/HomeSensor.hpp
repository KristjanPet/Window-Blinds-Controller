#pragma once

#include <cstdint>

#include <driver/gpio.h>
#include <esp_check.h>
#include <freertos/FreeRTOS.h>
#include <portmacro.h>

#include "BlindsCommandQueue.hpp"
#include "FaultHandler.hpp"
#include "Types.hpp"

/**
 * @brief GPIO-backed home/reference sensor for blinds calibration.
 *
 * Owns the active-low home sensor GPIO input and ISR registration. The
 * dual-inverter conditioned signal is HIGH while idle and LOW when the sensor is
 * active. Sensor edges are reported upward as HOMING_REACHED events through the
 * command queue; startup sensor
 * validation reports HOMING_CHECK and fault events while recording stuck-sensor
 * failures with the fault handler.
 */
class HomeSensor{
private:
    const gpio_num_t pin_;
    BlindsCommandQueue& commandQueue_;
    FaultHandler& faultHandler_;
    bool workingAtInit_ = false;

    static void IRAM_ATTR sensorIsr(void* arg);

public:
    /**
     * @brief Create a home sensor with mandatory hardware and reporting dependencies.
     *
     * @param pin GPIO connected to the home/reference sensor output.
     * @param commandQueue Queue used to report homing and fault events to the blinds controller.
     * @param faultHandler Fault recorder used for home-sensor validation failures.
     */
    HomeSensor(gpio_num_t pin, BlindsCommandQueue& commandQueue, FaultHandler& faultHandler);

    /**
     * @brief Configure the home sensor GPIO input.
     *
     * @return ESP_OK on success, or an ESP-IDF error from GPIO configuration.
     */
    esp_err_t init();

    /**
     * @brief Validate startup sensor state and register the GPIO ISR handler.
     *
     * If the sensor is active at startup, emits HOMING_CHECK so the controller can
     * move away and recheck the input before treating the sensor as stuck.
     *
     * @return ESP_OK when the sensor is ready, ESP_ERR_INVALID_STATE when the
     *         sensor remains active after the startup check, ESP_FAIL when a
     *         required fault event cannot be queued, or an ESP-IDF error from ISR
     *         registration.
     */
    esp_err_t sensorCheck();
};
