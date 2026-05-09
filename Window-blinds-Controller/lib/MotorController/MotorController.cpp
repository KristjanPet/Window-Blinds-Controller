#include "MotorController.hpp"

static const char* TAG = "Motor";

MotorController::MotorController(const MotorPins& pins, BlindsCommandQueue& commandsQueue)
                             : pins_(pins), commandsQueue_(commandsQueue){}

esp_err_t IRAM_ATTR MotorController::setAlarmAt(gptimer_handle_t timer, uint64_t alarmCount){
    gptimer_alarm_config_t alarmConfig = {};
    alarmConfig.reload_count = 0;
    alarmConfig.alarm_count = alarmCount;
    alarmConfig.flags.auto_reload_on_alarm = false;

    return gptimer_set_alarm_action(timer, &alarmConfig);
}

void MotorController::resetRamp(){
    currentTogglePeriodUs_ = AppConfig::StartTogglePeriodUs;
    rampStepCounter_ = 0;
}

void MotorController::updateRampAfterStep(){
    if(currentTogglePeriodUs_ <= AppConfig::togglePeriodUs){
        return;
    }

    rampStepCounter_++;
    if(rampStepCounter_ < AppConfig::rampStepInterval){
        return;
    }

    rampStepCounter_ = 0;
    currentTogglePeriodUs_--;
}

bool IRAM_ATTR MotorController::stepTimerCallback( gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    auto *self = static_cast<MotorController*>(user_ctx);
    if (!self || !timer || !edata) {
        return false;
    }

    bool notifyLimit = false;
    bool rearmTimer = false;
    uint32_t nextTogglePeriodUs = self->currentTogglePeriodUs_;
    BaseType_t hpTaskWoken = false;

    taskENTER_CRITICAL_ISR(&self->motorStepMux_);
    if((self->currentStep_ > self->targetStep_ && self->motorState_ == MotorState::DOWN) ||
        (self->currentStep_ < self->targetStep_ && self->motorState_ == MotorState::UP)){
        self->stepLevel_ = !self->stepLevel_;
        esp_err_t stepRet = gpio_set_level(self->pins_.step, self->stepLevel_);

        if(stepRet == ESP_OK && self->stepLevel_){
            self->updateRampAfterStep();
            if(self->motorState_ == MotorState::DOWN){
                self->currentStep_--;
            }
            else if(self->motorState_ == MotorState::UP){
                self->currentStep_++;
            }
        }
        if(stepRet == ESP_OK){
            nextTogglePeriodUs = self->currentTogglePeriodUs_;
            rearmTimer = true;
        }
        else if(self->softLimitHit_ == false){
            self->softLimitHit_ = true;
            notifyLimit = true;
        }
    }
    else if (self->softLimitHit_ == false && self->motorState_ != MotorState::STOPPED){ //limits the trigger while motor is stoping
        self->softLimitHit_ = true;
        notifyLimit = true;
    }
    taskEXIT_CRITICAL_ISR(&self->motorStepMux_);

    if(rearmTimer){
        const esp_err_t alarmRet = setAlarmAt(timer, edata->count_value + nextTogglePeriodUs);
        if(alarmRet != ESP_OK){
            taskENTER_CRITICAL_ISR(&self->motorStepMux_);
            if(self->softLimitHit_ == false && self->motorState_ != MotorState::STOPPED){
                self->softLimitHit_ = true;
                notifyLimit = true;
            }
            taskEXIT_CRITICAL_ISR(&self->motorStepMux_);
        }
    }

    if(notifyLimit){
        BlindsEvent cmd = BlindsEvent::LIMIT_REACHED;
        self->commandsQueue_.sendFromISR(cmd, &hpTaskWoken);
    }

    return hpTaskWoken;
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

    ESP_RETURN_ON_ERROR(setAlarmAt(timer_, AppConfig::StartTogglePeriodUs), TAG, "set alarm failed");
    ESP_RETURN_ON_ERROR(gptimer_enable(timer_), TAG, "timer enable failed");

    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.enable, 0), TAG, "Seting enable pin failed");

    return ESP_OK;
}

esp_err_t MotorController::stop(){
    ESP_RETURN_ON_ERROR(gptimer_stop(timer_), TAG, "Failed to stop gptimer");

    int32_t getStep = 0;
    esp_err_t stepRet = ESP_OK;

    taskENTER_CRITICAL(&motorStepMux_);
    motorState_ = MotorState::STOPPED;
    stepLevel_ = false;
    softLimitHit_ = false;
    resetRamp();
    stepRet = gpio_set_level(pins_.step, 0);
    getStep = currentStep_;
    taskEXIT_CRITICAL(&motorStepMux_);

    ESP_RETURN_ON_ERROR(stepRet, TAG, "Failed seting step pin");
    ESP_LOGI(TAG, "STOP, step: %d", getStep);

    return ESP_OK;
}

esp_err_t MotorController::startMovement(MotorState state, uint32_t dirLevel){
    ESP_RETURN_ON_ERROR(gptimer_stop(timer_), TAG, "Failed stopping gptimer before start");

    taskENTER_CRITICAL(&motorStepMux_);
    motorState_ = MotorState::STOPPED;
    stepLevel_ = false;
    softLimitHit_ = false;
    resetRamp();
    taskEXIT_CRITICAL(&motorStepMux_);

    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.dir, dirLevel), TAG, "Failed seting dir pin");
    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.step, 0), TAG, "Failed seting step pin");
    ESP_RETURN_ON_ERROR(gptimer_set_raw_count(timer_, 0), TAG, "Failed resetting gptimer count");
    ESP_RETURN_ON_ERROR(setAlarmAt(timer_, AppConfig::StartTogglePeriodUs), TAG, "Failed setting start alarm");

    taskENTER_CRITICAL(&motorStepMux_);
    motorState_ = state;
    taskEXIT_CRITICAL(&motorStepMux_);

    const esp_err_t startRet = gptimer_start(timer_);
    if(startRet != ESP_OK){
        taskENTER_CRITICAL(&motorStepMux_);
        motorState_ = MotorState::STOPPED;
        taskEXIT_CRITICAL(&motorStepMux_);
        ESP_RETURN_ON_ERROR(startRet, TAG, "Failed starting gptimer");
    }

    return ESP_OK;
}

esp_err_t MotorController::move(int32_t targetStep, bool isCalibrating ){

    if(targetStep < maxStep_ || targetStep >= -1){
        if(targetStep == -1){
            targetStep_ = maxStep_;
        } else {targetStep_ = targetStep;}
        
        if(targetStep_ >= currentStep_){
            ESP_RETURN_ON_ERROR(startMovement(MotorState::UP, 1), TAG, "Failed starting upward movement");
            ESP_LOGI(TAG, " UP");
        }
        else if(targetStep_ <= currentStep_ || isCalibrating){
            ESP_RETURN_ON_ERROR(startMovement(MotorState::DOWN, 0), TAG, "Failed starting downward movement");
            ESP_LOGI(TAG, " DOWN");
        }
    }
    else{
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

void MotorController::setHoming(int32_t offset){
    currentStep_ = 0 - offset;
}

void MotorController::setMaxStep(int32_t offset){
    maxStep_ = currentStep_ - offset;
}

int32_t MotorController::getCurrentStep() const{
    return currentStep_;
}
