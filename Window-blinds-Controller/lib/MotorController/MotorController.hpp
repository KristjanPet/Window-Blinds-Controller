#pragma once
#include <driver/gpio.h>
#include <driver/gptimer.h>
#include <esp_check.h>

struct MotorPins{
    gpio_num_t step;
    gpio_num_t dir;
    gpio_num_t enable;
};

class MotorController
{
private:
    MotorPins pins_;
    static gptimer_handle_t timer_;
    static volatile bool stepLevel_;
    uint32_t togglePeriodUs_;
public:
    MotorController(const MotorPins& pins, const uint32_t& togglePeriodUs);
    esp_err_t init();
};
