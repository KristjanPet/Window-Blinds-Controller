#pragma once
#include <driver/uart.h>
#include <esp_log.h>
#include <cstring>
#include "AppConfig.hpp"

class Tmc2209Driver{
private:
    const TMCUARTDriverPins UARTPins_;
    
public:
    Tmc2209Driver(const TMCUARTDriverPins& UARTPins);
    void init();

    bool writeReg(uint8_t reg, uint32_t value);
    bool readReg(uint8_t reg, uint32_t& value);
    bool uartSelfTest();
};
