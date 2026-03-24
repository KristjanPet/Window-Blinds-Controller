#pragma once
#include <driver/gpio.h>
#include <driver/gptimer.h>
#include <esp_check.h>

struct MotorPins{
    gpio_num_t step;
    gpio_num_t dir;
    gpio_num_t enable;
};

class MotorController{
    
private:
    MotorPins pins_;
    gptimer_handle_t timer_ = nullptr;
    volatile bool stepLevel_ = false;
    uint32_t togglePeriodUs_;
    bool moving_ = false;

    static bool IRAM_ATTR stepTimerCallback(
        gptimer_handle_t timer,
        const gptimer_alarm_event_data_t *edata,
        void *user_ctx);
public:
    MotorController(const MotorPins& pins, const uint32_t& togglePeriodUs);
    esp_err_t init();
    esp_err_t moveUp();
    esp_err_t moveDown();
    esp_err_t stop();
    bool getMoving();
};
