#include "BlindsController.hpp"

static const char* TAG = "BLINDS";

BlindsController::BlindsController(MotorController* motor): motor_(motor){}

void BlindsController::sendCommand(MoveCommand cmd){
    switch (cmd)
    {
    case MoveCommand::STOP:
        motor_->stop();
        state_ = BlindsState::IDLE;
        break;
    case MoveCommand::UP:
        if(state_ != BlindsState::IDLE){   
            motor_->stop();
            state_ = BlindsState::IDLE;
        }
        else{
            motor_->moveUp();
            state_ = BlindsState::MOVING_UP;
        }
        break;
    case MoveCommand::DOWN:
        if(state_ != BlindsState::IDLE){   
            motor_->stop();
            state_ = BlindsState::IDLE;
        }
        else{
            motor_->moveDown();
            state_ = BlindsState::MOVING_DOWN;
        }
        break;
    default:
        motor_->stop();
        state_ = BlindsState::FAULT;
        break;
    }
}