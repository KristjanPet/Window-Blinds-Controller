#pragma once
#include <cstdint>
#include <driver/gpio.h>

enum class BlindsEvent{
    STOP,
    UP,
    DOWN,
    MOVE_TO_PERCENT,
    LIMIT_REACHED,
    CALIBRATE,
    HOMING_REACHED,
    STALL_DETECTED,
    HOMING_CHECK,
    FAULT
};

struct BlindsCommand{
    BlindsEvent event;
    uint8_t percent;
};

struct MotorPins{
    gpio_num_t step;
    gpio_num_t dir;
    gpio_num_t enable;
};

struct ButtonPins{
    gpio_num_t up;
    gpio_num_t down;
};

struct TMCUARTDriverPins{
    gpio_num_t TX;
    gpio_num_t RX;
    gpio_num_t diag;
};
