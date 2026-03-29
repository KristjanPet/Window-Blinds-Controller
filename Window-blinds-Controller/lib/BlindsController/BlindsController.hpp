#pragma once
#include <cstdint>
#include <freertos/queue.h>
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
    QueueHandle_t commandsQueue = nullptr;

public:
    BlindsController(IMotor& motor);
    static void handleCommandTask(void* arg);
    BlindsState getState();
    void setState(BlindsState state);
};
