#include "BlindsController.hpp"

BlindsController::BlindsController(MotorController* motor): motor_(motor){}

void BlindsController::sendCommand(MoveCommand cmd){
    switch (cmd)
    {
    case MoveCommand::STOP:
        motor_->stop();
        break;
    case MoveCommand::UP:
        if(state_ != BlindsState::IDLE){   
            motor_->stop();
        }
        else{
            motor_->moveUp();
        }
        break;
    case MoveCommand::DOWN:
        if(state_ != BlindsState::IDLE){   
            motor_->stop();
        }
        else{
            motor_->moveDown();
        }
        break;
    default:
        motor_->stop();
        break;
    }
}