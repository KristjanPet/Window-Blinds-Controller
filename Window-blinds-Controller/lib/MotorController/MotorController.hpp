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
    gptimer_handle_t timer_ = nullptr;

    volatile bool stepLevel_ = false;
    MotorState motorState_ = MotorState::STOPPED;
    int32_t currentStep_ = INT_MAX;
    int32_t maxStep_ = INT_MAX;
    int32_t targetStep_ = 0;
    volatile bool softLimitHit_ = false;
    uint32_t currentTogglePeriodUs_ = AppConfig::StartTogglePeriodUs;
    uint32_t rampStepCounter_ = 0;

    BlindsCommandQueue& commandsQueue_;
    portMUX_TYPE motorStepMux_ = portMUX_INITIALIZER_UNLOCKED;

    static bool IRAM_ATTR stepTimerCallback(
        gptimer_handle_t timer,
        const gptimer_alarm_event_data_t *edata,
        void *user_ctx);
    static esp_err_t IRAM_ATTR setAlarmAt(gptimer_handle_t timer, uint64_t alarmCount);
    void resetRamp();
    void updateRampAfterStep();
    esp_err_t startMovement(MotorState state, uint32_t dirLevel);
public:
    MotorController(const MotorPins& pins, BlindsCommandQueue& commandsQueue);
    esp_err_t init();
    esp_err_t move(int32_t targetStep, bool isCalibrating = false) override;
    esp_err_t stop() override;
    void setHoming(int32_t offset = 0) override;
    void setMaxStep(int32_t offset = 0) override;
};
