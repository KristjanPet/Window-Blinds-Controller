#pragma once
#include <cstdint>
#include <esp_err.h>
#include "Types.hpp"

class Tmc2209Driver{
private:
    const TMCUARTDriverPins UARTPins_;
    bool initialized_ = false;
    
public:
    Tmc2209Driver(const TMCUARTDriverPins& UARTPins);
    esp_err_t init();

    esp_err_t writeReg(uint8_t reg, uint32_t value);
    esp_err_t readReg(uint8_t reg, uint32_t& value);
    esp_err_t configureAndVerify();
};
