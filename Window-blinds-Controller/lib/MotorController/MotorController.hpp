#pragma once
#include <driver/gpio.h>

struct MotorPins{
    gpio_num_t step;
    gpio_num_t dir;
    gpio_num_t enable;
};

class MotorController
{
private:
    MotorPins pins_;
public:
    MotorController(const MotorPins& pins);
};
