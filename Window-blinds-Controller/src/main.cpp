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

static gptimer_handle_t s_timer = nullptr;
static volatile bool s_step_level = false;

static bool IRAM_ATTR step_timer_callback(gptimer_handle_t timer,
                                          const gptimer_alarm_event_data_t *edata,
                                          void *user_ctx)
{
    // Toggle STEP each alarm event
    s_step_level = !s_step_level;
    gpio_set_level(MotorPins::Step, s_step_level);
    return false; // no higher-priority task woken
}

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
    static bool moving = false;

    while(true){
        if(xQueueReceive(button_queue, &btn, portMAX_DELAY) == pdTRUE){
            vTaskDelay(pdMS_TO_TICKS(30)); //debounce time

            switch (btn)
            {
            case ButtonPins::Up:
                if(gpio_get_level(ButtonPins::Up)){
                    if(moving){
                        ESP_ERROR_CHECK(gptimer_stop(s_timer));
                        moving = false;
                    }
                    else{
                        ESP_ERROR_CHECK(gpio_set_level(MotorPins::Dir, 0));
                        ESP_ERROR_CHECK(gptimer_start(s_timer));
                        moving = true;
                    }
                    ESP_LOGI("BUTTON", "UP button pressed");
                }
                break;
            case ButtonPins::Down:
                if(gpio_get_level(ButtonPins::Down)){
                    if(moving){
                        ESP_ERROR_CHECK(gptimer_stop(s_timer));
                        moving = false;
                    }
                    else{
                        ESP_ERROR_CHECK(gpio_set_level(MotorPins::Dir, 1));
                        ESP_ERROR_CHECK(gptimer_start(s_timer));
                        moving = true;
                    }
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

    gpio_set_level(MotorPins::Step, 0);
    gpio_set_level(MotorPins::Dir, 0);
    gpio_set_level(MotorPins::Enable, 1);

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

static esp_err_t init_step_timer(uint32_t toggle_period_us){
    gptimer_config_t timer_config = {};
    timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT;
    timer_config.direction = GPTIMER_COUNT_UP;
    timer_config.resolution_hz = 1000000; // 1 tick = 1 us

    ESP_RETURN_ON_ERROR(gptimer_new_timer(&timer_config, &s_timer), "Stepper", "new timer failed");

    gptimer_event_callbacks_t cbs = {};
    cbs.on_alarm = step_timer_callback;
    ESP_RETURN_ON_ERROR(gptimer_register_event_callbacks(s_timer, &cbs, nullptr), "Stepper", "register callbacks failed");

    gptimer_alarm_config_t alarm_config = {};
    alarm_config.reload_count = 0;
    alarm_config.alarm_count = toggle_period_us;
    alarm_config.flags.auto_reload_on_alarm = true;

    ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(s_timer, &alarm_config), "Stepper", "set alarm failed");
    ESP_RETURN_ON_ERROR(gptimer_enable(s_timer), "Stepper", "timer enable failed");

    return ESP_OK;
}

extern "C" void app_main(void) {
    init();
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI("MAIN", "Init complete, running program....");
    uint32_t buttonCounter;

    ESP_ERROR_CHECK(gpio_set_level(MotorPins::Enable, 0));
    ESP_ERROR_CHECK(init_step_timer(100));

    while (true){
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
}