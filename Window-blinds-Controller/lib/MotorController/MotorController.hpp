#pragma once
#include <driver/gpio.h>
#include <driver/gptimer.h>
#include <esp_check.h>
#include "IMotor.hpp"

enum class MotorState{
    STOPPED,
    UP,
    DOWN
};

struct MotorPins{
    gpio_num_t step;
    gpio_num_t dir;
    gpio_num_t enable;
};

class MotorController : public IMotor{
    
private:
    MotorPins pins_;
    gptimer_handle_t timer_ = nullptr;
    volatile bool stepLevel_ = false;
    uint32_t togglePeriodUs_;
    MotorState motorState_ = MotorState::STOPPED; //TODO maybe not needed
    int32_t currentStep_ = 0;
    static const uint32_t maxStep_ = 1000;

    static bool IRAM_ATTR stepTimerCallback(
        gptimer_handle_t timer,
        const gptimer_alarm_event_data_t *edata,
        void *user_ctx);
public:
    MotorController(const MotorPins& pins, const uint32_t& togglePeriodUs);
    esp_err_t init();
    esp_err_t moveUp() override;
    esp_err_t moveDown() override;
    esp_err_t stop() override;
};
