#include <freertos/FreeRTOS.h>

enum class BlindsState{
    Uncalibrated,
    Homing,
    Idle,
    MovingUp,
    MovingDown,
    Fault
};

class BlindsController{

private:
    BlindsState state;
    bool isCalibrated;
    uint32_t currentStep;
    static constexpr uint32_t maxStep = 100; //TODO temp

public:
    BlindsController(/* args */);
    ~BlindsController();
};
