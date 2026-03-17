#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_check.h"
#include "esp_log.h"

static constexpr gpio_num_t StepPin = GPIO_NUM_26;
static constexpr gpio_num_t DirPin = GPIO_NUM_27;
static constexpr gpio_num_t EnPin = GPIO_NUM_25;
static constexpr gpio_num_t UpPin = GPIO_NUM_33;
static constexpr gpio_num_t DownPin = GPIO_NUM_32;

static gptimer_handle_t motorStepTimer = nullptr;
static volatile bool motorStepLevel = false;

static void init(){
    //stepper motor pins init
    gpio_config_t motorIoConf = {
        .pin_bit_mask = (1ULL << StepPin) | (1ULL << DirPin) | (1ULL << EnPin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&motorIoConf);

    gpio_config_t buttIoConf = {
        .pin_bit_mask = (1ULL << UpPin) | (1ULL << DownPin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&buttIoConf);
}

extern "C" void app_main(void) {
    init();
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI("MAIN", "Init complete, running program....");

    while (true){
        gpio_set_level(StepPin, 0);
        vTaskDelay(100);
        gpio_set_level(StepPin, 1);
        vTaskDelay(100);
    }
    
    gpio_set_level(EnPin, 0);
}