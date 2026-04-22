#pragma once
#include "Types.hpp"

namespace AppConfig{
    //queues settings
    constexpr uint8_t commandsQueueDepth = 10;
    constexpr uint8_t buttonsQueueDepth = 10;

    //motor settings
    constexpr MotorPins motorPins = {GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_25}; //step, dir, enable
    constexpr uint32_t togglePeriodUs = 100; //in uS
    constexpr uint32_t maxStep = 1000 * 63; //max num of steps

    //motor driver main settings
    constexpr TMCUARTDriverPins UARTDriverPin = {GPIO_NUM_17, GPIO_NUM_16}; //TX, RX
    constexpr uint32_t motorGConfig = (0u << 2 | 1u << 6 | 1u << 7); //global configs, 2 - Stealth/spread, 6 - UART control, 7 - controll microsteps

    //buttons settings
    constexpr ButtonPins buttonPins = {GPIO_NUM_33, GPIO_NUM_32}; //up, down
    constexpr uint8_t debouncTime = 30; //mS
}