#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_check.h"
#include "esp_log.h"
#include <esp_timer.h>

namespace MotorPins {
    static constexpr gpio_num_t Step    = GPIO_NUM_26;
    static constexpr gpio_num_t Dir     = GPIO_NUM_27;
    static constexpr gpio_num_t Enable  = GPIO_NUM_25;
}

static gptimer_handle_t motorStepTimer = nullptr;
static volatile bool motorStepLevel = false;

namespace ButtonPins {
    static constexpr gpio_num_t Up   = GPIO_NUM_33;
    static constexpr gpio_num_t Down = GPIO_NUM_32;
}

TimerHandle_t debounce_timer;

static QueueHandle_t button_queue;

static void IRAM_ATTR buttonIsr(void *arg){
    gpio_num_t btn = static_cast<gpio_num_t>(reinterpret_cast<uintptr_t>(arg));

    BaseType_t hpTaskWoken = pdFALSE;
    xQueueSendFromISR(button_queue, &btn, &hpTaskWoken);

    if(hpTaskWoken){
        portYIELD_FROM_ISR();
    }
}

void button_task(void *arg){
    gpio_num_t btn;

    while(true){
        if(xQueueReceive(button_queue, &btn, portMAX_DELAY) == pdTRUE){
            vTaskDelay(pdMS_TO_TICKS(30)); //debounce time

            switch (btn)
            {
            case ButtonPins::Up:
                if(gpio_get_level(ButtonPins::Up)){
                    ESP_LOGI("BUTTON", "UP button pressed");
                }
                break;
            case ButtonPins::Down:
                if(gpio_get_level(ButtonPins::Down)){
                    ESP_LOGI("BUTTON", "DOWN button pressed");
                }
                break;
            default:
                break;
            }

            while(xQueueReceive(button_queue, &btn, 0) == pdTRUE) {} //drains extra bounces
        }
    }
}

static void init(){
    //stepper motor pins init
    gpio_config_t motorIoConf = {
        .pin_bit_mask = (1ULL << MotorPins::Step) | (1ULL << MotorPins::Dir) | (1ULL << MotorPins::Enable),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&motorIoConf);

    gpio_config_t buttIoConf = {
        .pin_bit_mask = (1ULL << ButtonPins::Up) | (1ULL << ButtonPins::Down),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&buttIoConf);

    button_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_install_isr_service(0); //TODO handle error

    gpio_isr_handler_add(ButtonPins::Down, buttonIsr, (void*) ButtonPins::Down);
    gpio_isr_handler_add(ButtonPins::Up, buttonIsr, (void*) ButtonPins::Up);

    if(xTaskCreate(button_task, "Button", 2048, NULL, 10, NULL) == pdPASS){}
}

extern "C" void app_main(void) {
    init();
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI("MAIN", "Init complete, running program....");
    uint32_t buttonCounter;

    while (true){
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
}