#pragma once
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <esp_check.h>
#include "Types.hpp"
#include "AppConfig.hpp"

class BlindsCommandQueue{

private:
    QueueHandle_t queue_ = nullptr;
    volatile uint32_t droppedEvents_ = 0;

public:
    esp_err_t init();
    BaseType_t send(BlindsEvent e, TickType_t wait = 0);
    BaseType_t sendFromISR(BlindsEvent e, BaseType_t* hpTaskWoken = nullptr);
    BaseType_t receive(BlindsEvent& e, TickType_t wait = portMAX_DELAY);
};
