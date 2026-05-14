#include "BlindsController.hpp"

#include "AppConfig.hpp"

static const char* TAG = "BLINDS";

BlindsController::BlindsController(IMotor& motor, BlindsCommandQueue& commandQueue):
                                 motor_(motor), commandsQueue_(commandQueue){}

void BlindsController::resetNormalStallRecovery(){
    normalStallRecoveries_ = 0;
    activeTarget_ = BlindsTarget::NONE;
}

esp_err_t BlindsController::retryStallRecoveryTarget(){
    esp_err_t err = motor_.stop();
    if(err != ESP_OK){
        state_ = BlindsState::FAULT;
        ESP_LOGE(TAG, "Error stopping stall recovery before retry: %s", esp_err_to_name(err));
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(400));

    switch(activeTarget_){
    case BlindsTarget::MAX:
        err = motor_.moveToMax();
        if(err == ESP_OK){
            state_ = BlindsState::MOVING_UP;
        }
        break;
    case BlindsTarget::MIN:
        err = motor_.move(0);
        if(err == ESP_OK){
            state_ = BlindsState::MOVING_DOWN;
        }
        break;
    case BlindsTarget::NONE:
    default:
        err = ESP_ERR_INVALID_STATE;
        break;
    }

    if(err != ESP_OK){
        state_ = BlindsState::FAULT;
        ESP_LOGE(TAG, "Error retrying after stall recovery: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t BlindsController::handleNormalStall(int32_t currentStep){
    const BlindsState stalledState = state_;
    BlindsTarget target = activeTarget_;

    if(target == BlindsTarget::NONE){
        target = stalledState == BlindsState::MOVING_UP ? BlindsTarget::MAX : BlindsTarget::MIN;
    }

    esp_err_t err = motor_.stop();
    if(err != ESP_OK){
        state_ = BlindsState::FAULT;
        ESP_LOGE(TAG, "Error stopping after stall: %s", esp_err_to_name(err));
        return err;
    }

    if(normalStallRecoveries_ >= AppConfig::normalStallMaxRecoveries){
        state_ = BlindsState::FAULT;
        activeTarget_ = target;
        ESP_LOGE(TAG, "Normal stall recovery failed after %u tries",
                 static_cast<unsigned>(normalStallRecoveries_));
        return ESP_ERR_INVALID_STATE;
    }

    int32_t backoffTarget = 0;
    if(stalledState == BlindsState::MOVING_UP){
        if(currentStep > AppConfig::normalStallBackoffSteps){
            backoffTarget = currentStep - AppConfig::normalStallBackoffSteps;
        }
    }
    else{
        const int32_t maxStep = motor_.getMaxStep();
        if(maxStep < 0){
            state_ = BlindsState::FAULT;
            activeTarget_ = target;
            ESP_LOGE(TAG, "Invalid max step during stall recovery: %d", maxStep);
            return ESP_ERR_INVALID_STATE;
        }
        backoffTarget = maxStep;
        if(currentStep <= maxStep - AppConfig::normalStallBackoffSteps){
            backoffTarget = currentStep + AppConfig::normalStallBackoffSteps;
        }
    }

    if(backoffTarget == currentStep){
        state_ = BlindsState::FAULT;
        activeTarget_ = target;
        ESP_LOGE(TAG, "Stall recovery backoff target equals current position: %d", currentStep);
        return ESP_ERR_INVALID_STATE;
    }

    activeTarget_ = target;
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
    case BlindsEvent::CALIBRATE:
        if(state_ != BlindsState::FAULT){ 
            err = motor_.move(0, true);
            if(err == ESP_OK ){
                resetNormalStallRecovery();
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
    case BlindsEvent::LIMIT_REACHED:
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
    case BlindsEvent::STALL_DETECTED: {
        int32_t currentStep = motor_.getCurrentStep();
        if (state_ == BlindsState::CALIBRATING_MAX && currentStep > AppConfig::stepStallThrehold){
            err = motor_.stop();
            if(err == ESP_OK ){
                if(state_ != BlindsState::FAULT){
                    motor_.setMaxStep(AppConfig::offsetOfMaxStep);
                    err = motor_.moveToMax();
                    if(err == ESP_OK){
                        state_ = BlindsState::IDLE;
                        ESP_LOGI(TAG, "Calibration complete");
                    } else{
                        state_ = BlindsState::FAULT;
                        ESP_LOGE(TAG, "Error backing away from max limit: %s", esp_err_to_name(err));
                    }
                } else{ 
                    ESP_LOGE(TAG, "Blinds state is FAULT");
                }
            } else{
                state_ = BlindsState::FAULT;
                ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
            }
        }
        else if(state_ == BlindsState::MOVING_UP || state_ == BlindsState::MOVING_DOWN){
            err = handleNormalStall(currentStep);
        }
        else{
            ESP_LOGE(TAG, "Stall detected unexpectedly");
        }
        break;
    }
    case BlindsEvent::HOMING_REACHED:    
        ESP_LOGI(TAG, "HOMING REACHED");
        err = motor_.stop();
        if(err == ESP_OK ){
            if(state_ == BlindsState::CALIBRATING_HOME){
                motor_.setHoming(AppConfig::offsetOfMinStep);
                vTaskDelay(pdMS_TO_TICKS(200));
                err = motor_.moveToMax();
                if(err == ESP_OK){
                    state_ = BlindsState::CALIBRATING_MAX;
                } else{
                    state_ = BlindsState::FAULT;
                    ESP_LOGE(TAG, "Error moving to max during calibration: %s", esp_err_to_name(err));
                }
            }
            else{
                // state_ = BlindsState::FAULT;
                ESP_LOGE(TAG, "Homing detected unexpectedly, Blinds state set FAULT");
            }
        } else{
                state_ = BlindsState::FAULT;
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
                    activeTarget_ = BlindsTarget::MAX;
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
                    activeTarget_ = BlindsTarget::MIN;
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
