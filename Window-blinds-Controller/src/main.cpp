#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"

static constexpr gpio_num_t StepPin = GPIO_NUM_26;
static constexpr gpio_num_t DirPin = GPIO_NUM_27;
static constexpr gpio_num_t EnPin = GPIO_NUM_25;

static void init(){
    gpio_set_direction(StepPin, GPIO_MODE_OUTPUT);
    gpio_set_direction(DirPin, GPIO_MODE_OUTPUT);
    gpio_set_direction(EnPin, GPIO_MODE_OUTPUT);

    gpio_set_level(EnPin, 0);
    gpio_set_level(DirPin, 0);
    gpio_set_level(StepPin, 0);

}

extern "C" void app_main(void) {
    init();
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI("MAIN", "Init complete, running program....");

    while (true){
        gpio_set_level(StepPin, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(StepPin, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    gpio_set_level(EnPin, 0);
}