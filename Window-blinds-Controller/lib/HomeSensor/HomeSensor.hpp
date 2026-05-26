#pragma once

#include <cstdint>

#include <driver/gpio.h>
#include <esp_check.h>
#include <freertos/FreeRTOS.h>
#include <portmacro.h>

#include "BlindsCommandQueue.hpp"
#include "FaultHandler.hpp"
#include "Types.hpp"

class HomeSensor{
private:
    const gpio_num_t pin_;
    BlindsCommandQueue& commandQueue_;
    FaultHandler& faultHandler_;
    bool workingAtInit_ = false;

    static void IRAM_ATTR sensorIsr(void* arg);

public:
    HomeSensor(gpio_num_t pin, BlindsCommandQueue& commandQueue, FaultHandler& faultHandler);
    esp_err_t init();
    esp_err_t sensorCheck();
};
