#include "MotorController.hpp"

static const char* TAG = "Motor";

MotorController::MotorController(const MotorPins& pins, const uint32_t& togglePeriodUs, BlindsCommandQueue& commandsQueue)
                             : pins_(pins), togglePeriodUs_(togglePeriodUs), commandsQueue_(commandsQueue){}

bool IRAM_ATTR MotorController::stepTimerCallback( gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    (void)timer; //not in use
    (void)edata;

    auto *self = static_cast<MotorController*>(user_ctx);
    if (!self) {
        return false;
    }

    if((self->currentStep_ > 0 && self->motorState_ == MotorState::DOWN) ||
        (self->currentStep_ < maxStep_ && self->motorState_ == MotorState::UP)){
        self->stepLevel_ = !self->stepLevel_;
        gpio_set_level(self->pins_.step, self->stepLevel_);

        if(self->motorState_ == MotorState::DOWN && self->stepLevel_){
            self->currentStep_--;
        }
        else if(self->motorState_ == MotorState::UP && self->stepLevel_){
            self->currentStep_++;
        }
    }
    else if (self->softLimitHit_ == false){ //limits the trigger while motor is stoping
        BlindsEvent cmd = BlindsEvent::LIMIT_REACHED;
        self->commandsQueue_.sendFromISR(cmd, nullptr);
        self->softLimitHit_ = true;
    }

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
    ESP_RETURN_ON_ERROR(gpio_config(&motorIoConf), TAG, "Motor io pin config failed");

    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.step, 0), TAG, "Seting step pin failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.dir, 0), TAG, "Seting dir pin failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.enable, 1), TAG, "Seting enable pin failed");

    //init timer
    gptimer_config_t timer_config = {};
    timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT;
    timer_config.direction = GPTIMER_COUNT_UP;
    timer_config.resolution_hz = 1 * 1000 * 1000; // 1 tick = 1 us

    ESP_RETURN_ON_ERROR(gptimer_new_timer(&timer_config, &timer_), TAG, "timer create failed");

    gptimer_event_callbacks_t cbs = {};
    cbs.on_alarm = stepTimerCallback;
    ESP_RETURN_ON_ERROR(gptimer_register_event_callbacks(timer_, &cbs, this), TAG, "register callbacks failed");

    gptimer_alarm_config_t alarm_config = {};
    alarm_config.reload_count = 0;
    alarm_config.alarm_count = togglePeriodUs_;
    alarm_config.flags.auto_reload_on_alarm = true;

    ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(timer_, &alarm_config), TAG, "set alarm failed");
    ESP_RETURN_ON_ERROR(gptimer_enable(timer_), TAG, "timer enable failed");

    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.enable, 0), TAG, "Seting enable pin failed");

    return ESP_OK;
}

esp_err_t MotorController::stop(){
    ESP_RETURN_ON_ERROR(gptimer_stop(timer_), TAG, "Failed to stop gptimer");
    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.step, 0), TAG, "Failed seting step pin");
    motorState_ = MotorState::STOPPED;
    stepLevel_ = false;
    softLimitHit_ = false;
    ESP_LOGI(TAG, "STOP, step: %d", currentStep_);

    return ESP_OK;
}

esp_err_t MotorController::moveDown(){
    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.dir, 0), TAG, "Failed seting dir pin");
    ESP_RETURN_ON_ERROR(gptimer_start(timer_), TAG, "Failed starting gptimer");
    motorState_ = MotorState::DOWN;
    ESP_LOGI(TAG, " DOWN");
    return ESP_OK;
}

esp_err_t MotorController::moveUp(){
    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.dir, 1), TAG, "Failed seting dir pin");
    ESP_RETURN_ON_ERROR(gptimer_start(timer_), TAG, "Failed starting gptimer");
    motorState_ = MotorState::UP;
    ESP_LOGI(TAG, " UP");

    return ESP_OK;
}
