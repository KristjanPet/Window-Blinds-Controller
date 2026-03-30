#pragma once
#include <freertos/FreeRTOS.h>
#include "IMotor.hpp"
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
    QueueHandle_t& commandsQueue_;

public:
    BlindsController(IMotor& motor, QueueHandle_t& commandQueue);
    esp_err_t init();
    static void handleCommandTask(void* arg);
    BlindsState getState();
    void setState(BlindsState state);
};
