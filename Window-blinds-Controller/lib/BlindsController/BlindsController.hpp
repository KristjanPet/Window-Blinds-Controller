#pragma once
#include <cstdint>
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

public:
    BlindsController(IMotor& motor);
    esp_err_t handleCommand(MoveCommand cmd);
    BlindsState getState();
};
