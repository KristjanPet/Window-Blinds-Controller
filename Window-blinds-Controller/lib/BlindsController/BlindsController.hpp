#pragma once
#include <cstdint>
#include "MotorController.hpp"
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
    MotorController& motor_;

public:
    BlindsController(MotorController& motor);
    esp_err_t handleCommand(MoveCommand cmd);
};
