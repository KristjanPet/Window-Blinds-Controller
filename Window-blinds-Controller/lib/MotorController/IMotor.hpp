#pragma once
#include <esp_check.h>
#include "Types.hpp"

class IMotor{
public:
    virtual esp_err_t move(int32_t targetStep) = 0;
    virtual esp_err_t stop() = 0;
    virtual void setCurrentStep(int32_t currentStep) = 0;
    virtual void setMaxStep(int32_t offset = 0) = 0;
    virtual ~IMotor() = default;
};