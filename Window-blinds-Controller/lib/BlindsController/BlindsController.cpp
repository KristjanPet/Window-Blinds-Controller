#include "BlindsController.hpp"

static const char* TAG = "BLINDS";

BlindsController::BlindsController(IMotor& motor, QueueHandle_t& commandQueue):
                                 motor_(motor), commandsQueue_(commandQueue){}

esp_err_t BlindsController::init(){
    commandsQueue_ = xQueueCreate(10, sizeof(BlindsEvent));
    if(commandsQueue_ == NULL){
        ESP_LOGE(TAG, "Creating commands queue failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void BlindsController::handleCommandTask(void* arg){ //using toggle style
    auto* self = static_cast<BlindsController*>(arg);
    BlindsEvent cmd;
    esp_err_t err;

    while(true){
        if(xQueueReceive(self->commandsQueue_, &cmd, portMAX_DELAY) == pdTRUE){
            switch (cmd){
            case BlindsEvent::STOP:
                err = self->motor_.stop();
                if(err == ESP_OK ){
                    if(self->state_ != BlindsState::FAULT){
                        self->state_ = BlindsState::IDLE;
                    } else{ 
                        ESP_LOGE(TAG, "Blinds state is FAULT");
                    }
                } else{
                     ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
                }
                break;
            case BlindsEvent::LIMIT_REACHED:
                err = self->motor_.stop();
                if(err == ESP_OK ){
                    if(self->state_ != BlindsState::FAULT){
                        self->state_ = BlindsState::IDLE;
                    } else{ 
                        ESP_LOGE(TAG, "Blinds state is FAULT");
                    }
                } else{
                     ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
                }
                break;
            case BlindsEvent::UP:
                if(self->state_ != BlindsState::FAULT){
                    if(self->state_ != BlindsState::IDLE){   
                        err = self->motor_.stop();
                        if(err == ESP_OK) self->state_ = BlindsState::IDLE;
                    }
                    else{
                        err = self->motor_.moveUp();
                        if(err == ESP_OK) self->state_ = BlindsState::MOVING_UP;
                    }
                    if(err != ESP_OK) ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
                }
                else{
                    ESP_LOGE(TAG, "Blinds state is FAULT");
                }
                break;
            case BlindsEvent::DOWN:
                if(self->state_ != BlindsState::FAULT){
                    if(self->state_ != BlindsState::IDLE){   
                        err = self->motor_.stop();
                        if(err == ESP_OK) self->state_ = BlindsState::IDLE;
                    }
                    else{
                        err = self->motor_.moveDown();
                        if(err == ESP_OK) self->state_ = BlindsState::MOVING_DOWN;
                    }
                    if(err != ESP_OK) ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
                } 
                else{
                    ESP_LOGE(TAG, "Blinds state is FAULT");
                }
                break;
            }
        }
    }
}

BlindsState BlindsController::getState(){
    return state_;
}

void BlindsController::setState(BlindsState state){
    state_ = state;
}