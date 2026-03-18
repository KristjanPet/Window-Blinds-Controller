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

static volatile uint64_t lastIsrTime = 0;
static volatile uint32_t counter = 0;
static QueueHandle_t button_queue;

static void IRAM_ATTR button_isr(void *arg){ //TODO keep ISR short, just send trigger, handle debounce in seprete task
    uint64_t now = esp_timer_get_time();

    if (now - lastIsrTime > 1000000ULL){
        counter++;
        uint32_t cnt = counter;
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(button_queue, &cnt, &higherPriorityTaskWoken);
        lastIsrTime = now;
        if(higherPriorityTaskWoken){
            portYIELD_FROM_ISR();
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

    gpio_isr_handler_add(ButtonPins::Down, button_isr, (void*) ButtonPins::Down);
    gpio_isr_handler_add(ButtonPins::Up, button_isr, (void*) ButtonPins::Up);
}

extern "C" void app_main(void) {
    init();
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI("MAIN", "Init complete, running program....");
    uint32_t buttonCounter;

    while (true){
        if(xQueueReceive(button_queue, &buttonCounter, portMAX_DELAY)){
            ESP_LOGI("MAIN", "Button pressed %d times.\n", buttonCounter);
        }
    }
    
}