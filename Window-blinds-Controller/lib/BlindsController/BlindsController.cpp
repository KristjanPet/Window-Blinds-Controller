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
            state_ = BlindsState::CALIBRATING;
            err = motor_.move(0, true);
            if(err == ESP_OK ){
                ESP_LOGI(TAG, "Moving to HOME");
            } else{
                ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
            }
        }
        else{
            ESP_LOGE(TAG, "Blinds state is FAULT");
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
                    ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
            }
            ESP_LOGI(TAG, "LIMIT REACHED");
        } 
        break;
    case BlindsEvent::STALL_DETECTED:
        if (state_ == BlindsState::CALIBRATING){
            err = motor_.stop();
            if(err == ESP_OK ){
                if(state_ != BlindsState::FAULT){
                    motor_.setMaxStep(AppConfig::offsetOfMaxStep);
                    motor_.move(-1);
                    state_ = BlindsState::IDLE;
                } else{ 
                    ESP_LOGE(TAG, "Blinds state is FAULT");
                }
            } else{
                    ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
            }
        }
        else if(state_ == BlindsState::MOVING_UP || state_ == BlindsState::MOVING_DOWN){
            //TODO normal stall detected
        }
        else{
            // ESP_LOGE(TAG, "Stall detected unexpectedly");
        }
        break;
    case BlindsEvent::HOMING_REACHED:    
        ESP_LOGI(TAG, "HOMING REACHED");
        err = motor_.stop();
        if(err == ESP_OK ){
            if(state_ == BlindsState::CALIBRATING){
                motor_.setHoming(AppConfig::offsetOfMinStep);
                vTaskDelay(pdMS_TO_TICKS(200));
                motor_.move(-1);
            }
            else{
                // state_ = BlindsState::FAULT;
                ESP_LOGE(TAG, "Homing detected unexpectedly, Blinds state set FAULT");
            }
        } else{
                ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
        }
        break;
    case BlindsEvent::UP:
        if(state_ != BlindsState::FAULT){
            if(state_ == BlindsState::MOVING_DOWN || state_ == BlindsState::MOVING_UP){   
                err = motor_.stop();
                if(err == ESP_OK) state_ = BlindsState::IDLE;
            }
            else{
                err = motor_.move(-1);
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
