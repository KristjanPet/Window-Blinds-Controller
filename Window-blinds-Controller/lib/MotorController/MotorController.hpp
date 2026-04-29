#pragma once
#include <esp_task.h>
#include <driver/gpio.h>
#include <driver/gptimer.h>
#include <esp_check.h>
#include "IMotor.hpp"
#include "BlindsCommandQueue.hpp"
#include "AppConfig.hpp"

enum class MotorState{
    STOPPED,
    UP,
    DOWN
};

class MotorController : public IMotor{
    
private:
    const MotorPins pins_;
    uint32_t togglePeriodUs_;
    gptimer_handle_t timer_ = nullptr;

    volatile bool stepLevel_ = false;
    MotorState motorState_ = MotorState::STOPPED;
    int32_t currentStep_ = 1; //TODO temp
    volatile bool softLimitHit_ = false;

    BlindsCommandQueue& commandsQueue_;
    portMUX_TYPE motorStepMux_ = portMUX_INITIALIZER_UNLOCKED;

    static bool IRAM_ATTR stepTimerCallback(
        gptimer_handle_t timer,
        const gptimer_alarm_event_data_t *edata,
        void *user_ctx);
public:
    MotorController(const MotorPins& pins, const uint32_t& togglePeriodUs, BlindsCommandQueue& commandsQueue);
    esp_err_t init();
    esp_err_t moveUp() override;
    esp_err_t moveDown() override;
    esp_err_t stop() override;
};
