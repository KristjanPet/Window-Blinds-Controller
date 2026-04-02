#pragma once
#include <esp_task.h>
#include <driver/gpio.h>
#include <driver/gptimer.h>
#include <esp_check.h>
#include "IMotor.hpp"
#include "BlindsCommandQueue.hpp"

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
    uint32_t togglePeriodUs_;
    gptimer_handle_t timer_ = nullptr;

    volatile bool stepLevel_ = false;
    MotorState motorState_ = MotorState::STOPPED;
    int32_t currentStep_ = 0;
    static constexpr uint32_t maxStep_ = 1000 * 63;
    volatile bool softLimitHit_ = false; //TODO vn dej

    BlindsCommandQueue& commandsQueue_;
    portMUX_TYPE motorStepMux_ = portMUX_INITIALIZER_UNLOCKED;

    static bool IRAM_ATTR stepTimerCallback(
        gptimer_handle_t timer,
        const gptimer_alarm_event_data_t *edata,
        void *user_ctx);
public:
    TaskHandle_t listenForEdgeStepTaskHandle = nullptr;

    MotorController(const MotorPins& pins, const uint32_t& togglePeriodUs, BlindsCommandQueue& commandsQueue);
    esp_err_t init();
    esp_err_t moveUp() override;
    esp_err_t moveDown() override;
    esp_err_t stop() override;
    static void listenForEdgeStepTask(void *arg);
};
