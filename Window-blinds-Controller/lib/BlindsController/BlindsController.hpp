#pragma once
#include <freertos/FreeRTOS.h>
#include "IMotor.hpp"
#include "BlindsCommandQueue.hpp"
#include "Types.hpp"

enum class BlindsState{
    IDLE,
    MOVING_UP,
    MOVING_DOWN,
    FAULT
};

class BlindsController{

private:
    BlindsState state_ = BlindsState::IDLE;
    IMotor& motor_;
    BlindsCommandQueue& commandsQueue_;
    
public:
    BlindsController(IMotor& motor, BlindsCommandQueue& commandQueue);
    static void handleCommandTask(void* arg);
    esp_err_t postEvent(BlindsEvent event);
    BlindsState getState();
    void setState(BlindsState state);
};
