#include "BlindsController.hpp"

static const char* TAG = "BLINDS";

BlindsController::BlindsController(IMotor& motor, BlindsCommandQueue& commandQueue):
                                 motor_(motor), commandsQueue_(commandQueue){}

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
        if (state_ == BlindsState::MOVING_UP || state_ == BlindsState::MOVING_DOWN) {
            err = motor_.stop();
            if(err == ESP_OK ){
                if(state_ != BlindsState::FAULT){
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
            //TODO normal stall detected
            ESP_LOGI(TAG, "STALL_DETECTED");
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
            if(state_ == BlindsState::MOVING_DOWN || state_ == BlindsState::MOVING_UP){   
                err = motor_.stop();
                if(err == ESP_OK) state_ = BlindsState::IDLE;
            }
            else if(state_ == BlindsState::CALIBRATING_HOME || state_ == BlindsState::CALIBRATING_MAX){
                err = motor_.stop();
                ESP_LOGE(TAG, "Blinds state set to FAULT");
                state_ = BlindsState::FAULT;
            }
            else{
                err = motor_.moveToMax();
                if(err == ESP_OK) state_ = BlindsState::MOVING_UP;
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
            if(state_ == BlindsState::MOVING_DOWN || state_ == BlindsState::MOVING_UP){   
                err = motor_.stop();
                if(err == ESP_OK) state_ = BlindsState::IDLE;
            }
            else if(state_ == BlindsState::CALIBRATING_HOME || state_ == BlindsState::CALIBRATING_MAX){
                err = motor_.stop();
                ESP_LOGE(TAG, "Blinds state set to FAULT");
                state_ = BlindsState::FAULT;
            }
            else{
                err = motor_.move(0);
                if(err == ESP_OK) state_ = BlindsState::MOVING_DOWN;
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
