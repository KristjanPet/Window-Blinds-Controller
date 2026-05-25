#include "MotorController.hpp"

#include <climits>

static const char* TAG = "Motor";

static esp_err_t keepFirstError(esp_err_t firstErr, esp_err_t nextErr){
    return firstErr == ESP_OK ? nextErr : firstErr;
}

static int32_t positionFromCount(int32_t startStep, MotorState state, int pcntCount){
    int64_t position = startStep;
    if(state == MotorState::UP){
        position += pcntCount;
    }
    else if(state == MotorState::DOWN){
        position -= pcntCount;
    }

    if(position > INT_MAX){
        return INT_MAX;
    }
    if(position < INT_MIN){
        return INT_MIN;
    }
    return static_cast<int32_t>(position);
}

MotorController::MotorController(const MotorPins& pins, BlindsCommandQueue& commandsQueue)
                             : pins_(pins), commandsQueue_(commandsQueue){}

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

bool IRAM_ATTR MotorController::rmtDoneCallback(rmt_channel_handle_t txChannel,
                                                const rmt_tx_done_event_data_t *edata,
                                                void *user_ctx){
    (void)txChannel;
    (void)edata;

    auto *self = static_cast<MotorController*>(user_ctx);
    if(self == nullptr){
        return false;
    }

    // ISR work stays minimal: mark completion and wake the refill task.
    taskENTER_CRITICAL_ISR(&self->motorStepMux_);
    self->rmtQueue_.completedTransactions++;
    taskEXIT_CRITICAL_ISR(&self->motorStepMux_);

    BaseType_t hpTaskWoken = pdFALSE;
    if(self->refillTaskHandle_ != nullptr){
        vTaskNotifyGiveFromISR(self->refillTaskHandle_, &hpTaskWoken);
    }

    return hpTaskWoken == pdTRUE;
}

void MotorController::refillTask(void* user_ctx){
    auto *self = static_cast<MotorController*>(user_ctx);
    if(self == nullptr){
        vTaskDelete(nullptr);
        return;
    }

    self->refillTaskLoop();
}

void MotorController::refillTaskLoop(){
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        releaseCompletedTransactions();
        const esp_err_t fillRet = fillRmtQueue();
        if(fillRet != ESP_OK){
            ESP_LOGE(TAG, "RMT refill failed: %s", esp_err_to_name(fillRet));
            failMovementFromTask();
            continue;
        }

        bool shouldComplete = false;
        taskENTER_CRITICAL(&motorStepMux_);
        shouldComplete = motorState_ != MotorState::STOPPED &&
                         reservedPulseCount_ >= targetPulseCount_ &&
                         rmtQueue_.activeTransactions == 0 &&
                         !limitEventQueued_ &&
                         !abortRequested_;
        taskEXIT_CRITICAL(&motorStepMux_);

        if(shouldComplete){
            const esp_err_t completeRet = completeMovementFromTask();
            if(completeRet != ESP_OK){
                ESP_LOGE(TAG, "Movement completion failed: %s", esp_err_to_name(completeRet));
                failMovementFromTask();
            }
        }
    }
}

esp_err_t MotorController::configureRmt(){
    if(stepChannel_ != nullptr){
        return ESP_OK;
    }

    rmt_tx_channel_config_t txConfig = {};
    txConfig.gpio_num = pins_.step;
    txConfig.clk_src = RMT_CLK_SRC_DEFAULT;
    txConfig.resolution_hz = rmtResolutionHz_;
    txConfig.mem_block_symbols = rmtSymbolsPerBuffer_;
    txConfig.trans_queue_depth = rmtBufferCount_;
    txConfig.intr_priority = 0;
    txConfig.flags.io_loop_back = 1;

    ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&txConfig, &stepChannel_), TAG, "RMT channel create failed");

    if(stepEncoder_ == nullptr){
        rmt_copy_encoder_config_t encoderConfig = {};
        ESP_RETURN_ON_ERROR(rmt_new_copy_encoder(&encoderConfig, &stepEncoder_), TAG, "RMT copy encoder create failed");
    }

    rmt_tx_event_callbacks_t callbacks = {};
    callbacks.on_trans_done = rmtDoneCallback;
    ESP_RETURN_ON_ERROR(rmt_tx_register_event_callbacks(stepChannel_, &callbacks, this), TAG, "RMT callback register failed");

    return ESP_OK;
}

esp_err_t MotorController::recreateRmtChannel(){
    // rmt_disable() stops the active transaction but does not purge queued descriptors.
    if(stepChannel_ != nullptr){
        ESP_RETURN_ON_ERROR(rmt_del_channel(stepChannel_), TAG, "RMT channel delete failed");
        stepChannel_ = nullptr;
    }

    rmtEnabled_ = false;
    return configureRmt();
}

esp_err_t MotorController::configurePcnt(){
    pcnt_unit_config_t unitConfig = {};
    unitConfig.low_limit = -pcntLimit_;
    unitConfig.high_limit = pcntLimit_;
    unitConfig.flags.accum_count = true;

    ESP_RETURN_ON_ERROR(pcnt_new_unit(&unitConfig, &stepCounter_), TAG, "PCNT unit create failed");
    ESP_RETURN_ON_ERROR(pcnt_unit_set_glitch_filter(stepCounter_, nullptr), TAG, "PCNT glitch filter config failed");

    pcnt_chan_config_t channelConfig = {};
    channelConfig.edge_gpio_num = pins_.step;
    channelConfig.level_gpio_num = -1;

    ESP_RETURN_ON_ERROR(pcnt_new_channel(stepCounter_, &channelConfig, &stepCounterChannel_), TAG, "PCNT channel create failed");
    ESP_RETURN_ON_ERROR(pcnt_channel_set_edge_action(stepCounterChannel_,
                                                     PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                                     PCNT_CHANNEL_EDGE_ACTION_HOLD),
                        TAG,
                        "PCNT edge action config failed");
    ESP_RETURN_ON_ERROR(pcnt_unit_add_watch_point(stepCounter_, pcntLimit_), TAG, "PCNT high watch point failed");
    ESP_RETURN_ON_ERROR(pcnt_unit_add_watch_point(stepCounter_, -pcntLimit_), TAG, "PCNT low watch point failed");
    ESP_RETURN_ON_ERROR(pcnt_unit_enable(stepCounter_), TAG, "PCNT enable failed");
    ESP_RETURN_ON_ERROR(pcnt_unit_clear_count(stepCounter_), TAG, "PCNT clear failed");

    return ESP_OK;
}

esp_err_t MotorController::ensureRmtEnabled(){
    if(stepChannel_ == nullptr){
        return ESP_ERR_INVALID_STATE;
    }
    if(rmtEnabled_){
        return ESP_OK;
    }

    const esp_err_t err = rmt_enable(stepChannel_);
    if(err == ESP_OK){
        rmtEnabled_ = true;
    }

    return err;
}

esp_err_t MotorController::disableRmtOutput(){
    // Active transactions can leave stale descriptors in ESP-IDF's internal queue.
    bool shouldRecreateChannel = false;
    taskENTER_CRITICAL(&motorStepMux_);
    shouldRecreateChannel = rmtQueue_.activeTransactions > 0;
    taskEXIT_CRITICAL(&motorStepMux_);

    if(rmtMutex_ != nullptr){
        xSemaphoreTake(rmtMutex_, portMAX_DELAY);
    }

    esp_err_t err = ESP_OK;
    if(stepChannel_ != nullptr && rmtEnabled_){
        err = rmt_disable(stepChannel_);
        if(err == ESP_OK){
            rmtEnabled_ = false;
            if(shouldRecreateChannel){
                err = recreateRmtChannel();
            }
        }
    }

    if(rmtMutex_ != nullptr){
        xSemaphoreGive(rmtMutex_);
    }

    if(err == ESP_OK){
        resetRmtBufferState();
    }

    return err;
}

esp_err_t MotorController::startPcntCounter(){
    if(stepCounter_ == nullptr){
        return ESP_ERR_INVALID_STATE;
    }

    if(pcntRunning_){
        ESP_RETURN_ON_ERROR(pcnt_unit_stop(stepCounter_), TAG, "PCNT stop before start failed");
        pcntRunning_ = false;
    }

    ESP_RETURN_ON_ERROR(pcnt_unit_clear_count(stepCounter_), TAG, "PCNT clear before start failed");
    ESP_RETURN_ON_ERROR(pcnt_unit_start(stepCounter_), TAG, "PCNT start failed");
    pcntRunning_ = true;

    return ESP_OK;
}

esp_err_t MotorController::stopPcntCounter(){
    if(stepCounter_ == nullptr || !pcntRunning_){
        return ESP_OK;
    }

    const esp_err_t err = pcnt_unit_stop(stepCounter_);
    if(err == ESP_OK){
        pcntRunning_ = false;
    }

    return err;
}

esp_err_t MotorController::stopPulseHardware(){
    esp_err_t firstErr = ESP_OK;
    firstErr = keepFirstError(firstErr, disableRmtOutput());
    firstErr = keepFirstError(firstErr, refreshPositionFromPcnt());
    firstErr = keepFirstError(firstErr, stopPcntCounter());
    firstErr = keepFirstError(firstErr, gpio_set_level(pins_.step, 0));
    return firstErr;
}

esp_err_t MotorController::refreshPositionFromPcnt(){
    if(stepCounter_ == nullptr){
        return ESP_ERR_INVALID_STATE;
    }

    MotorState state = MotorState::STOPPED;
    int32_t startStep = 0;
    bool shouldRefresh = false;

    taskENTER_CRITICAL(&motorStepMux_);
    state = motorState_;
    startStep = moveStartStep_;
    shouldRefresh = pcntRunning_ && state != MotorState::STOPPED;
    taskEXIT_CRITICAL(&motorStepMux_);

    if(!shouldRefresh){
        return ESP_OK;
    }

    int pcntCount = 0;
    ESP_RETURN_ON_ERROR(pcnt_unit_get_count(stepCounter_, &pcntCount), TAG, "PCNT get count failed");
    const int32_t refreshedStep = positionFromCount(startStep, state, pcntCount);

    taskENTER_CRITICAL(&motorStepMux_);
    currentStep_ = refreshedStep;
    taskEXIT_CRITICAL(&motorStepMux_);

    return ESP_OK;
}

void MotorController::resetRmtBufferState(){
    taskENTER_CRITICAL(&motorStepMux_);
    for(size_t i = 0; i < rmtBufferCount_; i++){
        rmtQueue_.bufferInUse[i] = false;
    }
    rmtQueue_.nextToQueue = 0;
    rmtQueue_.nextToRelease = 0;
    rmtQueue_.activeTransactions = 0;
    rmtQueue_.releasedTransactions = rmtQueue_.completedTransactions;
    taskEXIT_CRITICAL(&motorStepMux_);
}

void MotorController::releaseCompletedTransactions(){
    taskENTER_CRITICAL(&motorStepMux_);
    const uint32_t completed = rmtQueue_.completedTransactions;
    while(rmtQueue_.releasedTransactions < completed && rmtQueue_.activeTransactions > 0){
        rmtQueue_.bufferInUse[rmtQueue_.nextToRelease] = false;
        rmtQueue_.nextToRelease = static_cast<uint8_t>((rmtQueue_.nextToRelease + 1) % rmtBufferCount_);
        rmtQueue_.activeTransactions--;
        rmtQueue_.releasedTransactions++;
    }

    if(rmtQueue_.activeTransactions == 0 && rmtQueue_.releasedTransactions < completed){
        rmtQueue_.releasedTransactions = completed;
    }
    taskEXIT_CRITICAL(&motorStepMux_);
}

void MotorController::buildPulseSymbols(rmt_symbol_word_t* buffer, size_t pulseCount){
    // Low-then-high preserves the old initial half-period before the first edge.
    for(size_t i = 0; i < pulseCount; i++){
        const uint32_t lowDurationUs = currentTogglePeriodUs_;
        updateRampAfterStep();
        const uint32_t highDurationUs = currentTogglePeriodUs_;

        buffer[i].level0 = 0;
        buffer[i].duration0 = lowDurationUs;
        buffer[i].level1 = 1;
        buffer[i].duration1 = highDurationUs;
    }
}

esp_err_t MotorController::fillRmtQueue(){
    rmt_transmit_config_t txConfig = {};
    txConfig.loop_count = 0;
    txConfig.flags.eot_level = 0;
    txConfig.flags.queue_nonblocking = 1;

    while(true){
        size_t bufferIndex = 0;
        size_t pulseCount = 0;

        taskENTER_CRITICAL(&motorStepMux_);
        if(motorState_ == MotorState::STOPPED ||
           abortRequested_ ||
           rmtQueue_.activeTransactions >= rmtBufferCount_ ||
           reservedPulseCount_ >= targetPulseCount_){
            taskEXIT_CRITICAL(&motorStepMux_);
            return ESP_OK;
        }

        bufferIndex = rmtQueue_.nextToQueue;
        if(rmtQueue_.bufferInUse[bufferIndex]){
            taskEXIT_CRITICAL(&motorStepMux_);
            return ESP_ERR_INVALID_STATE;
        }

        const uint32_t remainingPulses = targetPulseCount_ - reservedPulseCount_;
        pulseCount = remainingPulses < rmtSymbolsPerBuffer_ ? remainingPulses : rmtSymbolsPerBuffer_;
        // Reserve before transmit so concurrent refills cannot queue past target.
        buildPulseSymbols(rmtQueue_.buffers[bufferIndex], pulseCount);
        rmtQueue_.bufferInUse[bufferIndex] = true;
        rmtQueue_.activeTransactions++;
        reservedPulseCount_ += pulseCount;
        rmtQueue_.nextToQueue = static_cast<uint8_t>((rmtQueue_.nextToQueue + 1) % rmtBufferCount_);
        taskEXIT_CRITICAL(&motorStepMux_);

        if(rmtMutex_ != nullptr){
            xSemaphoreTake(rmtMutex_, portMAX_DELAY);
        }

        bool canTransmit = false;
        taskENTER_CRITICAL(&motorStepMux_);
        canTransmit = motorState_ != MotorState::STOPPED && !abortRequested_ && rmtEnabled_;
        taskEXIT_CRITICAL(&motorStepMux_);

        esp_err_t txRet = ESP_OK;
        if(canTransmit){
            txRet = rmt_transmit(stepChannel_,
                                 stepEncoder_,
                                 rmtQueue_.buffers[bufferIndex],
                                 pulseCount * sizeof(rmt_symbol_word_t),
                                 &txConfig);
        }
        if(rmtMutex_ != nullptr){
            xSemaphoreGive(rmtMutex_);
        }

        if(!canTransmit){
            return ESP_OK;
        }
        if(txRet != ESP_OK){
            bool abortRequested = false;
            taskENTER_CRITICAL(&motorStepMux_);
            abortRequested = abortRequested_;
            taskEXIT_CRITICAL(&motorStepMux_);
            if(abortRequested){
                return ESP_OK;
            }
            return txRet;
        }
    }
}

esp_err_t MotorController::completeMovementFromTask(){
    bool shouldComplete = false;
    taskENTER_CRITICAL(&motorStepMux_);
    shouldComplete = motorState_ != MotorState::STOPPED && !limitEventQueued_ && !abortRequested_;
    if(shouldComplete){
        limitEventQueued_ = true;
    }
    taskEXIT_CRITICAL(&motorStepMux_);

    if(!shouldComplete){
        return ESP_OK;
    }

    esp_err_t firstErr = stopPulseHardware();

    taskENTER_CRITICAL(&motorStepMux_);
    motorState_ = MotorState::STOPPED;
    targetPulseCount_ = 0;
    reservedPulseCount_ = 0;
    resetRamp();
    taskEXIT_CRITICAL(&motorStepMux_);

    if(firstErr == ESP_OK){
        if(commandsQueue_.send(BlindsEvent::LIMIT_REACHED) != pdTRUE){
            firstErr = ESP_FAIL;
        }
    }

    return firstErr;
}

esp_err_t MotorController::failMovementFromTask(){
    esp_err_t firstErr = stopPulseHardware();

    taskENTER_CRITICAL(&motorStepMux_);
    motorState_ = MotorState::STOPPED;
    limitEventQueued_ = false;
    abortRequested_ = false;
    targetPulseCount_ = 0;
    reservedPulseCount_ = 0;
    resetRamp();
    taskEXIT_CRITICAL(&motorStepMux_);

    if(commandsQueue_.send(BlindsEvent::FAULT) != pdTRUE && firstErr == ESP_OK){
        firstErr = ESP_FAIL;
    }

    return firstErr;
}

esp_err_t MotorController::init(){
    gpio_config_t motorIoConf = {};
    motorIoConf.pin_bit_mask = (1ULL << pins_.step) | (1ULL << pins_.dir) | (1ULL << pins_.enable);
    motorIoConf.mode = GPIO_MODE_OUTPUT;
    motorIoConf.pull_up_en = GPIO_PULLUP_DISABLE;
    motorIoConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    motorIoConf.intr_type = GPIO_INTR_DISABLE;
    ESP_RETURN_ON_ERROR(gpio_config(&motorIoConf), TAG, "Motor io pin config failed");

    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.step, 0), TAG, "Seting step pin failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.dir, 0), TAG, "Seting dir pin failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.enable, 1), TAG, "Seting enable pin failed");

    rmtMutex_ = xSemaphoreCreateMutex();
    if(rmtMutex_ == nullptr){
        ESP_LOGE(TAG, "RMT mutex create failed");
        return ESP_FAIL;
    }

    ESP_RETURN_ON_ERROR(configureRmt(), TAG, "RMT config failed");
    ESP_RETURN_ON_ERROR(configurePcnt(), TAG, "PCNT config failed");

    if(xTaskCreate(refillTask, "MotorRMT", refillTaskStack_, this, refillTaskPriority_, &refillTaskHandle_) != pdPASS){
        ESP_LOGE(TAG, "Motor RMT refill task create failed");
        return ESP_FAIL;
    }

    initialized_ = true;
    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.step, 0), TAG, "Seting step pin failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.enable, 0), TAG, "Seting enable pin failed");

    return ESP_OK;
}

esp_err_t MotorController::stop(){
    taskENTER_CRITICAL(&motorStepMux_);
    abortRequested_ = true;
    taskEXIT_CRITICAL(&motorStepMux_);

    esp_err_t firstErr = stopPulseHardware();

    int32_t getStep = 0;
    taskENTER_CRITICAL(&motorStepMux_);
    motorState_ = MotorState::STOPPED;
    limitEventQueued_ = false;
    abortRequested_ = false;
    targetPulseCount_ = 0;
    reservedPulseCount_ = 0;
    resetRamp();
    getStep = currentStep_;
    taskEXIT_CRITICAL(&motorStepMux_);

    if(firstErr != ESP_OK){
        ESP_LOGE(TAG, "STOP had errors: %s", esp_err_to_name(firstErr));
        return firstErr;
    }

    ESP_LOGI(TAG, "STOP, step: %d", getStep);
    return ESP_OK;
}

esp_err_t MotorController::startMovement(MotorState state, uint32_t dirLevel){
    if(!initialized_){
        return ESP_ERR_INVALID_STATE;
    }

    ESP_RETURN_ON_ERROR(disableRmtOutput(), TAG, "Failed disabling RMT before start");
    ESP_RETURN_ON_ERROR(refreshPositionFromPcnt(), TAG, "Failed refreshing position before start");
    ESP_RETURN_ON_ERROR(stopPcntCounter(), TAG, "Failed stopping PCNT before start");

    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.dir, dirLevel), TAG, "Failed seting dir pin");
    ESP_RETURN_ON_ERROR(gpio_set_level(pins_.step, 0), TAG, "Failed seting step pin");

    const int64_t rawDistance = static_cast<int64_t>(targetStep_) - static_cast<int64_t>(currentStep_);
    uint32_t pulseCount = 0;
    if(rawDistance < 0){
        pulseCount = static_cast<uint32_t>(-rawDistance);
    }
    else{
        pulseCount = static_cast<uint32_t>(rawDistance);
    }

    ESP_RETURN_ON_ERROR(startPcntCounter(), TAG, "Failed starting PCNT");
    ESP_RETURN_ON_ERROR(ensureRmtEnabled(), TAG, "Failed enabling RMT");

    taskENTER_CRITICAL(&motorStepMux_);
    motorState_ = state;
    abortRequested_ = false;
    moveStartStep_ = currentStep_;
    targetPulseCount_ = pulseCount;
    reservedPulseCount_ = 0;
    limitEventQueued_ = false;
    resetRamp();
    taskEXIT_CRITICAL(&motorStepMux_);

    const esp_err_t fillRet = fillRmtQueue();
    if(fillRet != ESP_OK){
        failMovementFromTask();
        ESP_RETURN_ON_ERROR(fillRet, TAG, "Failed queueing RMT pulses");
    }

    if(pulseCount == 0){
        ESP_RETURN_ON_ERROR(completeMovementFromTask(), TAG, "Failed completing zero-step movement");
    }

    return ESP_OK;
}

esp_err_t MotorController::move(int32_t targetStep, bool isCalibrating ){

    if(targetStep < 0 || targetStep > maxStep_){
        return ESP_ERR_INVALID_ARG;
    }

    targetStep_ = targetStep;

    const int32_t currentStep = getCurrentStep();
    if(targetStep_ >= currentStep){
        ESP_RETURN_ON_ERROR(startMovement(MotorState::UP, 1), TAG, "Failed starting upward movement");
        ESP_LOGI(TAG, " UP");
    }
    else if(targetStep_ <= currentStep || isCalibrating){
        ESP_RETURN_ON_ERROR(startMovement(MotorState::DOWN, 0), TAG, "Failed starting downward movement");
        ESP_LOGI(TAG, " DOWN");
    }

    return ESP_OK;
}

esp_err_t MotorController::moveToMax(){
    return move(maxStep_);
}

void MotorController::setHoming(int32_t offset){
    taskENTER_CRITICAL(&motorStepMux_);
    currentStep_ = 0 - offset;
    moveStartStep_ = currentStep_;
    taskEXIT_CRITICAL(&motorStepMux_);
}

void MotorController::setMaxStep(int32_t offset){
    taskENTER_CRITICAL(&motorStepMux_);
    maxStep_ = currentStep_ - offset;
    taskEXIT_CRITICAL(&motorStepMux_);
}

int32_t MotorController::getCurrentStep() const{
    MotorState state = MotorState::STOPPED;
    int32_t startStep = 0;
    int32_t currentStep = 0;
    bool shouldReadPcnt = false;

    taskENTER_CRITICAL(&motorStepMux_);
    state = motorState_;
    startStep = moveStartStep_;
    currentStep = currentStep_;
    shouldReadPcnt = pcntRunning_ && state != MotorState::STOPPED && stepCounter_ != nullptr;
    taskEXIT_CRITICAL(&motorStepMux_);

    if(!shouldReadPcnt){
        return currentStep;
    }

    int pcntCount = 0;
    if(pcnt_unit_get_count(stepCounter_, &pcntCount) != ESP_OK){
        return currentStep;
    }

    return positionFromCount(startStep, state, pcntCount);
}

int32_t MotorController::getMaxStep() const{
    return maxStep_;
}
