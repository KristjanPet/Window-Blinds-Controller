#include "BlindsController.hpp"

#include <climits>

#include "AppConfig.hpp"

static const char* TAG = "BLINDS";

BlindsController::BlindsController(IMotor& motor, BlindsCommandQueue& commandQueue):
                                 motor_(motor), commandsQueue_(commandQueue){}

void BlindsController::resetNormalStallRecovery(){
    normalStallRecoveries_ = 0;
    activeTargetStep_ = 0;
    hasActiveTarget_ = false;
    recoveryReturnState_ = BlindsState::IDLE;
}

esp_err_t BlindsController::retryStallRecoveryTarget(){
    esp_err_t err = motor_.stop();
    if(err != ESP_OK){
        state_ = BlindsState::FAULT;
        ESP_LOGE(TAG, "Error stopping stall recovery before retry: %s", esp_err_to_name(err));
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(200));

    const int32_t maxStep = motor_.getMaxStep();
    if(!hasActiveTarget_ || activeTargetStep_ < 0 || activeTargetStep_ > maxStep){
        err = ESP_ERR_INVALID_STATE;
    } else{
        const int32_t currentStep = motor_.getCurrentStep();
        err = motor_.move(activeTargetStep_, recoveryReturnState_ == BlindsState::CALIBRATING_HOME);
        if(err == ESP_OK){
            if(recoveryReturnState_ == BlindsState::CALIBRATING_HOME ||
               recoveryReturnState_ == BlindsState::CALIBRATING_MAX){
                state_ = recoveryReturnState_;
            } else{
                state_ = activeTargetStep_ >= currentStep ? BlindsState::MOVING_UP : BlindsState::MOVING_DOWN;
            }
        }
    }

    if(err != ESP_OK){
        state_ = BlindsState::FAULT;
        ESP_LOGE(TAG, "Error retrying after stall recovery: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t BlindsController::handleNormalStall(int32_t currentStep){
    const BlindsState stalledState = state_;
    const bool stalledMovingUp = stalledState == BlindsState::MOVING_UP ||
                                 stalledState == BlindsState::CALIBRATING_MAX;
    int32_t targetStep = activeTargetStep_;
    const int32_t maxStep = motor_.getMaxStep();

    if(!hasActiveTarget_){
        targetStep = stalledMovingUp ? maxStep : 0;
    }

    esp_err_t err = motor_.stop();
    if(err != ESP_OK){
        state_ = BlindsState::FAULT;
        ESP_LOGE(TAG, "Error stopping after stall: %s", esp_err_to_name(err));
        return err;
    }

    if(maxStep < 0 || targetStep < 0 || targetStep > maxStep){
        state_ = BlindsState::FAULT;
        activeTargetStep_ = targetStep;
        hasActiveTarget_ = true;
        ESP_LOGE(TAG, "Invalid target during stall recovery: target=%d max=%d",
                 targetStep, maxStep);
        return ESP_ERR_INVALID_STATE;
    }

    if(normalStallRecoveries_ >= AppConfig::normalStallMaxRecoveries){
        state_ = BlindsState::FAULT;
        activeTargetStep_ = targetStep;
        hasActiveTarget_ = true;
        ESP_LOGE(TAG, "Normal stall recovery failed after %u tries",
                 static_cast<unsigned>(normalStallRecoveries_));
        return ESP_ERR_INVALID_STATE;
    }

    int32_t backoffTarget = 0;
    if(stalledMovingUp){
        if(currentStep > AppConfig::normalStallBackoffSteps){
            backoffTarget = currentStep - AppConfig::normalStallBackoffSteps;
        }
    }
    else{
        backoffTarget = maxStep;
        if(currentStep <= maxStep - AppConfig::normalStallBackoffSteps){
            backoffTarget = currentStep + AppConfig::normalStallBackoffSteps;
        }
    }

    if(backoffTarget == currentStep){
        state_ = BlindsState::FAULT;
        activeTargetStep_ = targetStep;
        hasActiveTarget_ = true;
        ESP_LOGE(TAG, "Stall recovery backoff target equals current position: %d", currentStep);
        return ESP_ERR_INVALID_STATE;
    }

    activeTargetStep_ = targetStep;
    hasActiveTarget_ = true;
    recoveryReturnState_ = stalledState;
    err = motor_.move(backoffTarget);
    if(err == ESP_OK){
        normalStallRecoveries_++;
        state_ = BlindsState::STALL_RECOVERY;
        ESP_LOGI(TAG, "STALL_DETECTED, backing off to %d", backoffTarget);
    } else{
        state_ = BlindsState::FAULT;
        ESP_LOGE(TAG, "Error backing away after stall: %s", esp_err_to_name(err));
    }

    return err;
}

void BlindsController::handleCommandTask(void* arg){ //using toggle style
    auto* self = static_cast<BlindsController*>(arg);
    BlindsEvent cmd;

    while(true){
        if(self->commandsQueue_.receive(cmd) == pdTRUE){
            self->handleCommand(cmd);
        }
    }
}

esp_err_t BlindsController::handleCommand(BlindsEvent cmd){
    esp_err_t err = ESP_OK;

    switch (cmd){
    case BlindsEvent::STOP:
        err = motor_.stop();
        if(err == ESP_OK ){
            resetNormalStallRecovery();
            if(state_ != BlindsState::FAULT){
                state_ = BlindsState::IDLE;
            } else{ 
                ESP_LOGE(TAG, "Blinds state is FAULT");
            }
        } else{
                ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
        }
        break;
    case BlindsEvent::UP:
        if(state_ != BlindsState::FAULT){
            if(state_ == BlindsState::MOVING_DOWN || state_ == BlindsState::MOVING_UP ||
               state_ == BlindsState::STALL_RECOVERY){
                err = motor_.stop();
                if(err == ESP_OK){
                    resetNormalStallRecovery();
                    state_ = BlindsState::IDLE;
                }
            }
            else if(state_ == BlindsState::CALIBRATING_HOME || state_ == BlindsState::CALIBRATING_MAX){
                err = motor_.stop();
                ESP_LOGE(TAG, "Blinds state set to FAULT");
                state_ = BlindsState::FAULT;
            }
            else{
                err = motor_.moveToMax();
                if(err == ESP_OK){
                    resetNormalStallRecovery();
                    activeTargetStep_ = motor_.getMaxStep();
                    hasActiveTarget_ = true;
                    state_ = BlindsState::MOVING_UP;
                }
            }
            if(err != ESP_OK) ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
        }
        else{
            ESP_LOGE(TAG, "Blinds state is FAULT");
            err = ESP_ERR_INVALID_STATE;
        }
        break;
    case BlindsEvent::DOWN:
        if(state_ != BlindsState::FAULT){
            if(state_ == BlindsState::MOVING_DOWN || state_ == BlindsState::MOVING_UP ||
               state_ == BlindsState::STALL_RECOVERY){
                err = motor_.stop();
                if(err == ESP_OK){
                    resetNormalStallRecovery();
                    state_ = BlindsState::IDLE;
                }
            }
            else if(state_ == BlindsState::CALIBRATING_HOME || state_ == BlindsState::CALIBRATING_MAX){
                err = motor_.stop();
                ESP_LOGE(TAG, "Blinds state set to FAULT");
                state_ = BlindsState::FAULT;
            }
            else{
                err = motor_.move(0);
                if(err == ESP_OK){
                    resetNormalStallRecovery();
                    activeTargetStep_ = 0;
                    hasActiveTarget_ = true;
                    state_ = BlindsState::MOVING_DOWN;
                }
            }
            if(err != ESP_OK) ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
        } 
        else{
            ESP_LOGE(TAG, "Blinds state is FAULT");
            err = ESP_ERR_INVALID_STATE;
        }
        break;
    case BlindsEvent::LIMIT_REACHED: //soft low or top limit reached
        if(state_ == BlindsState::STALL_RECOVERY){
            err = retryStallRecoveryTarget();
        }
        else if (state_ == BlindsState::MOVING_UP || state_ == BlindsState::MOVING_DOWN) {
            err = motor_.stop();
            if(err == ESP_OK ){
                if(state_ != BlindsState::FAULT){
                    resetNormalStallRecovery();
                    state_ = BlindsState::IDLE;
                } else{ 
                    ESP_LOGE(TAG, "Blinds state is FAULT");
                }
            } else{
                    state_ = BlindsState::FAULT;
                    ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
            }
            ESP_LOGI(TAG, "LIMIT REACHED");
        } 
        break;
    case BlindsEvent::CALIBRATE: //start calibrating
        if(state_ != BlindsState::FAULT){ 
            err = motor_.move(0, true);
            if(err == ESP_OK ){
                resetNormalStallRecovery();
                activeTargetStep_ = 0;
                hasActiveTarget_ = true;
                recoveryReturnState_ = BlindsState::CALIBRATING_HOME;
                state_ = BlindsState::CALIBRATING_HOME;
                ESP_LOGI(TAG, "Moving to HOME");
            } else{
                state_ = BlindsState::FAULT;
                ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
            }
        }
        else{
            ESP_LOGE(TAG, "Blinds state is FAULT");
            err = ESP_ERR_INVALID_STATE;
        }
        break;
    case BlindsEvent::HOMING_REACHED:    
        ESP_LOGI(TAG, "HOMING REACHED");
        err = motor_.stop();
        if(err == ESP_OK ){
            if(state_ == BlindsState::CALIBRATING_HOME){
                const int64_t rawReturnStep = static_cast<int64_t>(INT_MAX) -
                                              static_cast<int64_t>(motor_.getCurrentStep()) +
                                              AppConfig::offsetOfMinStep;
                if(rawReturnStep < 0){
                    calibrationReturnStep_ = 0;
                } else if(rawReturnStep > INT_MAX){
                    calibrationReturnStep_ = INT_MAX;
                } else{
                    calibrationReturnStep_ = static_cast<int32_t>(rawReturnStep);
                }
                motor_.setHoming(AppConfig::offsetOfMinStep);
                vTaskDelay(pdMS_TO_TICKS(200));
                err = motor_.moveToMax();
                if(err == ESP_OK){
                    resetNormalStallRecovery();
                    activeTargetStep_ = motor_.getMaxStep();
                    hasActiveTarget_ = true;
                    recoveryReturnState_ = BlindsState::CALIBRATING_MAX;
                    state_ = BlindsState::CALIBRATING_MAX;
                } else{
                    state_ = BlindsState::FAULT;
                    ESP_LOGE(TAG, "Error moving to max during calibration: %s", esp_err_to_name(err));
                }
            }
            else{
                state_ = BlindsState::FAULT;
                ESP_LOGE(TAG, "Homing detected unexpectedly, Blinds state set FAULT");
            }
        } else{
                state_ = BlindsState::FAULT;
                ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
        }
        break;
    case BlindsEvent::STALL_DETECTED: {
        int32_t currentStep = motor_.getCurrentStep();
        if (state_ == BlindsState::CALIBRATING_MAX && currentStep > AppConfig::stepStallThrehold){ //stall detected as limit reached (top)
            err = motor_.stop();
            if(err == ESP_OK ){
                if(state_ != BlindsState::FAULT){
                    motor_.setMaxStep(AppConfig::offsetOfMaxStep);
                    int32_t targetStep = calibrationReturnStep_;
                    if(targetStep > motor_.getMaxStep()){
                        targetStep = motor_.getMaxStep();
                    }
                    err = motor_.move(targetStep);
                    if(err == ESP_OK){
                        resetNormalStallRecovery();
                        const int32_t currentStep = motor_.getCurrentStep();
                        activeTargetStep_ = targetStep;
                        hasActiveTarget_ = true;
                        state_ = targetStep >= currentStep ? BlindsState::MOVING_UP : BlindsState::MOVING_DOWN;
                        ESP_LOGI(TAG, "Calibration complete");
                    } else{
                        state_ = BlindsState::FAULT;
                        ESP_LOGE(TAG, "Error returning after calibration: %s", esp_err_to_name(err));
                    }
                } else{ 
                    ESP_LOGE(TAG, "Blinds state is FAULT");
                }
            } else{
                state_ = BlindsState::FAULT;
                ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
            }
        }
        else if(state_ == BlindsState::MOVING_UP || state_ == BlindsState::MOVING_DOWN
                || state_ == BlindsState::CALIBRATING_HOME || state_ == BlindsState::CALIBRATING_MAX){ //stall detected during normal movment or calibration
            err = handleNormalStall(currentStep);
        }
        break;
    }
    case BlindsEvent::HOMING_CHECK:
        motor_.setHoming();
        err = motor_.moveToMax();
        if(err != ESP_OK) state_ = BlindsState::FAULT;
        vTaskDelay(pdMS_TO_TICKS(400));
        err = motor_.stop();
        if(err != ESP_OK) state_ = BlindsState::FAULT;
        break;
    case BlindsEvent::FAULT:
        state_ = BlindsState::FAULT;
        ESP_LOGE(TAG, "Blinds state is FAULT");
        break;
    default:
        err = ESP_ERR_INVALID_ARG;
        break;
    }

    return err;
}

BlindsState BlindsController::getState() const{
    return state_;
}

void BlindsController::setState(BlindsState state){
    state_ = state;
}
