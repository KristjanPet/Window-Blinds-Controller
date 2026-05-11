#pragma once
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_check.h>
#include <portmacro.h>
#include "BlindsCommandQueue.hpp"
#include "AppConfig.hpp"

class ButtonHandler;

enum class ButtonPressed : uint8_t{
    UP,
    DOWN
};

struct ButtonIsrContext{
    ButtonHandler* self;
    ButtonPressed button;
};

class ButtonHandler{

private:
    const ButtonPins pins_;
    QueueHandle_t buttonQueue_ = nullptr;
    ButtonIsrContext upCtx_;
    ButtonIsrContext downCtx_;
    BlindsCommandQueue& commandQueue_;

    static void IRAM_ATTR buttonIsr(void *arg);
public:
    ButtonHandler(const ButtonPins& pins, BlindsCommandQueue& commandQueue);
    esp_err_t init();
    static void buttonTask(void *arg);
};
