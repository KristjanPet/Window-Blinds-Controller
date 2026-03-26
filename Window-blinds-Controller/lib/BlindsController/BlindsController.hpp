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
    MotorController* motor_;
    uint32_t currentStep;
    static constexpr uint32_t maxStep = 100; //TODO temp

public:
    BlindsController(MotorController* motor);
    void sendCommand(MoveCommand cmd);
};
