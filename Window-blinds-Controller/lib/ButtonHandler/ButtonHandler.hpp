#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/projdefs.h>
#include <driver/gpio.h>
#include <esp_check.h>
#include <portmacro.h>
#include "MotorController.hpp"

enum class ButtonPressed : uint8_t{
    UP,
    DOWN
};

struct ButtonPins{
    gpio_num_t up;
    gpio_num_t down;
};

class ButtonHandler;

struct ButtonIsrContext{
    ButtonHandler* self;
    ButtonPressed button;
};

class ButtonHandler{

private:
    ButtonPins pins_;
    TimerHandle_t debounceTimer_;// TODO no need maybe?
    QueueHandle_t buttonQueue_ = nullptr;
    ButtonIsrContext upCtx_;
    ButtonIsrContext downCtx_;
    MotorController* motor_;

    static void IRAM_ATTR buttonIsr(void *arg);
public:
    ButtonHandler(ButtonPins* pins, MotorController* motor);
    esp_err_t init();
    static void buttonTask(void *arg);
};