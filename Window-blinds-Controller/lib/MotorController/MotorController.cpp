#include "MotorController.hpp"

MotorController::MotorController(const MotorPins& pins, const uint32_t& togglePeriodUs)
                             : pins_(pins), togglePeriodUs_(togglePeriodUs) {}

bool IRAM_ATTR MotorController::stepTimerCallback( gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    (void) timer; //not in use
    (void) edata;

    auto *self = static_cast<MotorController*>(user_ctx);
    if (!self) return false;
    self->stepLevel_ = !self->stepLevel_;
    gpio_set_level(self->pins_.step, self->stepLevel_);
    return false; 
}

esp_err_t MotorController::init(){
    //stepper motor pins init
    gpio_config_t motorIoConf = {
        .pin_bit_mask = (1ULL << pins_.step) | (1ULL << pins_.dir) | (1ULL << pins_.enable),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&motorIoConf);

    gpio_set_level(pins_.step, 0);
    gpio_set_level(pins_.dir, 0);
    gpio_set_level(pins_.enable, 1);

    //init timer
    gptimer_config_t timer_config = {};
    timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT;
    timer_config.direction = GPTIMER_COUNT_UP;
    timer_config.resolution_hz = 1 * 1000 * 1000; // 1 tick = 1 us

    ESP_RETURN_ON_ERROR(gptimer_new_timer(&timer_config, &timer_), "Stepper", "timer create failed");

    gptimer_event_callbacks_t cbs = {};
    cbs.on_alarm = stepTimerCallback;
    ESP_RETURN_ON_ERROR(gptimer_register_event_callbacks(timer_, &cbs, this), "Stepper", "register callbacks failed");

    gptimer_alarm_config_t alarm_config = {};
    alarm_config.reload_count = 0;
    alarm_config.alarm_count = togglePeriodUs_;
    alarm_config.flags.auto_reload_on_alarm = true;

    ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(timer_, &alarm_config), "Stepper", "set alarm failed");
    ESP_RETURN_ON_ERROR(gptimer_enable(timer_), "Stepper", "timer enable failed");

    ESP_ERROR_CHECK(gpio_set_level(pins_.enable, 0));

    return ESP_OK;
}

esp_err_t MotorController::moveMotorDown(){
    if(moving){
        ESP_ERROR_CHECK(gptimer_stop(timer_));
        moving = false;
        ESP_LOGI("MOTOR", "motor stoped");
    }
    else{
        ESP_ERROR_CHECK(gpio_set_level(pins_.dir, 0));
        ESP_ERROR_CHECK(gptimer_start(timer_));
        moving = true;
        ESP_LOGI("MOTOR", "motor going down");
    }
    return ESP_OK;
}

esp_err_t MotorController::moveMotorUp(){
    if(moving){
        ESP_ERROR_CHECK(gptimer_stop(timer_));
        moving = false;
        ESP_LOGI("MOTOR", "motor stoped");
    }
    else{
        ESP_ERROR_CHECK(gpio_set_level(pins_.dir, 1));
        ESP_ERROR_CHECK(gptimer_start(timer_));
        moving = true;
        ESP_LOGI("MOTOR", "motor going up");
    }
    return ESP_OK;
}