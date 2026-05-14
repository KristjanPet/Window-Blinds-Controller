#pragma once
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include "IMotor.hpp"
#include "BlindsCommandQueue.hpp"
#include "Types.hpp"

enum class BlindsState{
    IDLE,
    CALIBRATING_HOME,
    CALIBRATING_MAX,
    MOVING_UP,
    MOVING_DOWN,
    STALL_RECOVERY,
    FAULT
};

class BlindsController{

private:
    enum class BlindsTarget{
        NONE,
        MIN,
        MAX
    };

    BlindsState state_ = BlindsState::IDLE;
    IMotor& motor_;
    BlindsCommandQueue& commandsQueue_;
    BlindsTarget activeTarget_ = BlindsTarget::NONE;
    uint8_t normalStallRecoveries_ = 0;

    void resetNormalStallRecovery();
    esp_err_t handleNormalStall(int32_t currentStep);
    esp_err_t retryStallRecoveryTarget();
    
public:
    BlindsController(IMotor& motor, BlindsCommandQueue& commandQueue);
    static void handleCommandTask(void* arg);
    esp_err_t handleCommand(BlindsEvent cmd);
    BlindsState getState() const;
    void setState(BlindsState state);
};
