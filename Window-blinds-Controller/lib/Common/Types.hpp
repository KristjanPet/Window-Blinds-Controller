#pragma once
#include <driver/gpio.h>

enum class BlindsEvent{
    UP,
    DOWN,
    STOP,
    CALIBRATE,
    LIMIT_REACHED,
    HOMING_REACHED
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
