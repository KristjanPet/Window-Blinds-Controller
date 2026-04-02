#pragma once
#include "Types.hpp"

namespace AppConfig{
    //queues settings
    constexpr uint8_t commandsQueueDepth = 10;
    constexpr uint8_t buttonsQueueDepth = 10;

    //motor settings
    constexpr MotorPins motorPins = {GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_25}; //step, dir, enable
    constexpr uint32_t togglePeriodUs = 120; //in uS
    constexpr uint32_t maxStep = 1000 * 63; //max num of steps

    //motor driver settings
    constexpr TMCUARTDriverPins UARTDriverPin = {GPIO_NUM_17, GPIO_NUM_16};

    //buttons settings
    constexpr ButtonPins buttonPins = {GPIO_NUM_33, GPIO_NUM_32}; //up, down
    constexpr uint8_t debouncTime = 30; //mS
}