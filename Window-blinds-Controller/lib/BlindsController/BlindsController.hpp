#pragma once
#include <cstdint>
#include "MotorController.hpp"
#include "Types.hpp"

enum class BlindsState{
    Idle,
    MovingUp,
    MovingDown,
    Fault
};

class BlindsController{

private:
    BlindsState state_;
    MotorController* motor_;
    uint32_t currentStep;
    static constexpr uint32_t maxStep = 100; //TODO temp

public:
    BlindsController(MotorController* motor);
    void sendMotorCommand(MoveCommand cmd);
};
