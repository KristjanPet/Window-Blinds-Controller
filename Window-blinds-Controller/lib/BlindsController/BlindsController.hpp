#pragma once

#include <cstdint>

enum class BlindsState{
    Uncalibrated,
    Homing,
    Idle,
    MovingUp,
    MovingDown,
    Fault
};

enum class MotorCommand{
    MoveUp,
    MoveDown,
    Stop
};

class BlindsController{

private:
    BlindsState state;
    bool isCalibrated;
    uint32_t currentStep;
    static constexpr uint32_t maxStep = 100; //TODO temp

    bool sendMotorCommand(MotorCommand dir);
    bool canAcceptCommand();
    bool shouldRecalibrate();
public:
    BlindsController();
    void HandleCommands();
};
