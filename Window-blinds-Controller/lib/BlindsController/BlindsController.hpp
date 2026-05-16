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
    BlindsState state_ = BlindsState::IDLE;
    IMotor& motor_;
    BlindsCommandQueue& commandsQueue_;
    int32_t activeTargetStep_ = 0;
    bool hasActiveTarget_ = false;
    BlindsState recoveryReturnState_ = BlindsState::IDLE;
    uint8_t normalStallRecoveries_ = 0;
    int32_t calibrationReturnStep_ = 0;

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
