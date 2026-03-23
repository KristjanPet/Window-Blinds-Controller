#pragma once
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>

struct ButtonPins{
    gpio_num_t up;
    gpio_num_t down;
};

enum class ButtonPressed{
    UP,
    DOWN
};

class ButtonHandler{

private:
    ButtonPins pins_;
    TimerHandle_t debounceTimer_;
    QueueHandle_t buttonQueue_;
public:
    ButtonHandler();
    esp_err_t init();
};