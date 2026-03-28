#include "BlindsController.hpp"

// static const char* TAG = "BLINDS";

BlindsController::BlindsController(IMotor& motor): motor_(motor){}

esp_err_t BlindsController::handleCommand(MoveCommand cmd){ //using toggle style
    esp_err_t err = ESP_OK;
    switch (cmd){
    case MoveCommand::STOP:
        err = motor_.stop();
        if(err == ESP_OK ){
            if(state_ != BlindsState::FAULT){
                state_ = BlindsState::IDLE;
            }
        }
        return err;
    case MoveCommand::UP:
        if(state_ != BlindsState::FAULT){
            if(state_ != BlindsState::IDLE){   
                err = motor_.stop();
                if(err == ESP_OK) state_ = BlindsState::IDLE;
            }
            else{
                err = motor_.moveUp();
                if(err == ESP_OK) state_ = BlindsState::MOVING_UP;
            }
        }
        else{
            return ESP_ERR_INVALID_STATE;
        }
        return err;
    case MoveCommand::DOWN:
        if(state_ != BlindsState::FAULT){
            if(state_ != BlindsState::IDLE){   
                err = motor_.stop();
                if(err == ESP_OK) state_ = BlindsState::IDLE;
            }
            else{
                err = motor_.moveDown();
                if(err == ESP_OK) state_ = BlindsState::MOVING_DOWN;
            }
        } 
        else{
            return ESP_ERR_INVALID_STATE;
        }
        return err;
    }
    return ESP_ERR_INVALID_ARG;
}

BlindsState BlindsController::getState(){
    return state_;
}

void BlindsController::setState(BlindsState state){
    state_ = state;
}