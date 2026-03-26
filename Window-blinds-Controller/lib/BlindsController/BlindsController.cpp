#include "BlindsController.hpp"

// static const char* TAG = "BLINDS";

BlindsController::BlindsController(MotorController* motor): motor_(motor){}

esp_err_t BlindsController::sendCommand(MoveCommand cmd){
    esp_err_t err;
    switch (cmd)
    {
    case MoveCommand::STOP:
        err = motor_->stop();
        if(err == ESP_OK) state_ = BlindsState::IDLE;
        else return err;
        break;
    case MoveCommand::UP:
        if(state_ != BlindsState::FAULT){
            if(state_ != BlindsState::IDLE){   
                err = motor_->stop();
                if(err == ESP_OK) state_ = BlindsState::IDLE;
                else return err;
            }
            else{
                err = motor_->moveUp();
                if(err == ESP_OK) state_ = BlindsState::MOVING_UP;
                else return err;
            }
        }
        else{
            return ESP_FAIL;
        }
        break;
    case MoveCommand::DOWN:
        if(state_ != BlindsState::FAULT){
            if(state_ != BlindsState::IDLE){   
                err = motor_->stop();
                if(err == ESP_OK) state_ = BlindsState::IDLE;
                else return err;
            }
            else{
                err = motor_->moveDown();
                if(err == ESP_OK) state_ = BlindsState::MOVING_DOWN;
                else return err;
            }
        } 
        else{
            return ESP_FAIL;
        }
        break;
    default:
        err = motor_->stop();
        if(err == ESP_OK) state_ = BlindsState::FAULT;
        else return err;
        break;
    }

    return ESP_OK;
}