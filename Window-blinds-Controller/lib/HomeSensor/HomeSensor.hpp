#pragma once

#include <cstdint>

#include <driver/gpio.h>
#include <esp_check.h>
#include <freertos/FreeRTOS.h>
#include <portmacro.h>

#include "BlindsCommandQueue.hpp"
#include "Types.hpp"

class HomeSensor{
private:
    const gpio_num_t pin_;
    BlindsCommandQueue& commandQueue_;
    uint32_t droppedEvents_ = 0;
    bool workingAtInit_ = false;

    static void IRAM_ATTR sensorIsr(void* arg);

public:
    HomeSensor(gpio_num_t pin, BlindsCommandQueue& commandQueue);
    esp_err_t init();
    esp_err_t sensorCheck();
};
