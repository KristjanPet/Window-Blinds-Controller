#pragma once
#include <cstdint>
#include <esp_attr.h>
#include <esp_err.h>
#include "BlindsCommandQueue.hpp"
#include "Types.hpp"

class Tmc2209Driver{
private:
    const TMCUARTDriverPins UARTPins_;
    BlindsCommandQueue& commandsQueue_;
    bool initialized_ = false;

    static void IRAM_ATTR diagIsr(void* arg);
    
public:
    Tmc2209Driver(const TMCUARTDriverPins& UARTPins, BlindsCommandQueue& commandsQueue);
    esp_err_t init();

    esp_err_t writeReg(uint8_t reg, uint32_t value);
    esp_err_t readReg(uint8_t reg, uint32_t& value);
    esp_err_t readSgResult(uint16_t& sgResult);
    esp_err_t configureAndVerify();
};
