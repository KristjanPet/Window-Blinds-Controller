#pragma once
#include "Types.hpp"

/**
 * @brief Compile-time application configuration for pins, timing, and control limits.
 *
 * These values are shared across hardware drivers and high-level control logic.
 * Hardware-dependent values should be verified on the actual ESP32/TMC2209 setup
 * before being treated as safe operating limits.
 */
namespace AppConfig{
    /** @brief Queue depths for command and button event buffering. */
    constexpr uint8_t commandsQueueDepth = 10;
    constexpr uint8_t buttonsQueueDepth = 10;

    /** @brief Motor STEP/DIR/EN pins and pulse ramp timing. */
    constexpr MotorPins motorPins = {GPIO_NUM_15, GPIO_NUM_7, GPIO_NUM_8}; //step, dir, enable
    constexpr uint32_t togglePeriodUs = 100; //in uS
    constexpr uint32_t StartTogglePeriodUs = 200; //in uS
    constexpr uint32_t rampStepInterval = 30; //real STEP rising edges per 1 uS ramp change
    static_assert(togglePeriodUs > 0, "togglePeriodUs must be greater than zero");
    static_assert(StartTogglePeriodUs > 0, "StartTogglePeriodUs must be greater than zero");
    static_assert(StartTogglePeriodUs >= togglePeriodUs, "StartTogglePeriodUs must not be faster than togglePeriodUs");
    static_assert(rampStepInterval > 0, "rampStepInterval must be greater than zero");

    /** @brief Calibration offsets and stall-recovery limits for blinds travel. */
    constexpr int32_t offsetOfMaxStep = 800; //offset from stall detected
    constexpr int32_t offsetOfMinStep = 1000; //offset from home detected
    constexpr int32_t normalStallBackoffSteps = 1000 * 10;
    constexpr uint8_t normalStallMaxRecoveries = 3;
    constexpr int32_t stepStallThreshold = 1000 * 63; //above this threshold, stall is considerd as max limit
    static_assert(offsetOfMaxStep > 0, "offsetOfMaxStep must be greater than zero");
    static_assert(offsetOfMinStep > 0, "offsetOfMinStep must be greater than zero");
    static_assert(normalStallBackoffSteps > 0, "normalStallBackoffSteps must be greater than zero");
    static_assert(normalStallMaxRecoveries > 0, "normalStallMaxRecoveries must be greater than zero");

    /** @brief TMC2209 UART/DIAG pins and register configuration values. */
    constexpr TMCUARTDriverPins UARTDriverPin = {GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_9}; //TX, RX, DIAG
    constexpr uint32_t motorGConfig = (0u << 2 | 1u << 6 | 1u << 7); //global configs, 2 - Stealth/spread, 6 - UART control, 7 - controll microsteps
    constexpr uint8_t stallGuardThreshold = 95; //SGTHRS, stall at SG_RESULT <= stallGuardThreshold * 2

    /** @brief Physical button pins and debounce timing. */
    constexpr ButtonPins buttonPins = {GPIO_NUM_12, GPIO_NUM_13}; //up, down
    constexpr uint8_t debouncTime = 30; //mS

    /** @brief MQTT client identity, topics, and publish options. */
    constexpr const char* mqttClientId = "window-blinds-controller";
    constexpr const char* mqttCommandTopic = "window-blinds/command";
    constexpr const char* mqttStatusTopic = "window-blinds/status";
    constexpr int mqttQos = 0;
    constexpr bool mqttRetain = false;
    static_assert(mqttQos >= 0 && mqttQos <= 2, "mqttQos must be 0, 1, or 2");

    /** @brief Home/reference sensor GPIO pin. */
    constexpr gpio_num_t homeSensorPin = GPIO_NUM_14;
}
