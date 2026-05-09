#pragma once
#include "Types.hpp"

namespace AppConfig{
    //queues settings
    constexpr uint8_t commandsQueueDepth = 10;
    constexpr uint8_t buttonsQueueDepth = 10;

    //motor settings
    constexpr MotorPins motorPins = {GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_25}; //step, dir, enable
    constexpr uint32_t togglePeriodUs = 100; //in uS
    constexpr uint32_t StartTogglePeriodUs = 200; //in uS
    constexpr uint32_t rampStepInterval = 30; //real STEP rising edges per 1 uS ramp change
    static_assert(togglePeriodUs > 0, "togglePeriodUs must be greater than zero");
    static_assert(StartTogglePeriodUs > 0, "StartTogglePeriodUs must be greater than zero");
    static_assert(StartTogglePeriodUs >= togglePeriodUs, "StartTogglePeriodUs must not be faster than togglePeriodUs");
    static_assert(rampStepInterval > 0, "rampStepInterval must be greater than zero");

    //Blinds settings
    constexpr int32_t offsetOfMaxStep = 800; //offset from stall detected
    constexpr int32_t offsetOfMinStep = 800; //offset from home detected
    constexpr int32_t stepStallThrehold = 1000 * 63;
    static_assert(offsetOfMaxStep > 0, "offsetOfMaxStep must be greater than zero");
    static_assert(offsetOfMinStep > 0, "offsetOfMinStep must be greater than zero");

    //motor driver main settings
    constexpr TMCUARTDriverPins UARTDriverPin = {GPIO_NUM_17, GPIO_NUM_16, GPIO_NUM_19}; //TX, RX, DIAG
    constexpr uint32_t motorGConfig = (0u << 2 | 1u << 6 | 1u << 7); //global configs, 2 - Stealth/spread, 6 - UART control, 7 - controll microsteps
    constexpr uint8_t stallGuardThreshold = 100; //SGTHRS, stall at SG_RESULT <= stallGuardThreshold * 2

    //buttons settings
    constexpr ButtonPins buttonPins = {GPIO_NUM_33, GPIO_NUM_32}; //up, down
    constexpr uint8_t debouncTime = 30; //mS

    //home/reference sensor settings
    constexpr gpio_num_t homeSensorPin = GPIO_NUM_18;
}
