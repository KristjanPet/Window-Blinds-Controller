#pragma once
#include <esp_check.h>
#include "Types.hpp"

class IMotor{
public:
    virtual esp_err_t move(int32_t targetStep, bool isCalibrating = false) = 0;
    virtual esp_err_t stop() = 0;
    virtual void setHoming(int32_t offset = 0) = 0;
    virtual void setMaxStep(int32_t offset = 0) = 0;
    virtual int32_t getCurrentStep() const = 0;
    virtual ~IMotor() = default;
};