#pragma once

#include "IMotor.hpp"

enum class LastAction{
    NONE,
    UP,
    DOWN,
    STOP
};

class FakeMotor : public IMotor{
private:
    LastAction lastAction = LastAction::NONE;
    esp_err_t nextMoveResult = ESP_OK;
    esp_err_t nextStopResult = ESP_OK;
    int32_t currentStep = 100;
public:
    LastAction getLastAction(){return lastAction;};
    void setNextResult(esp_err_t result){
        nextMoveResult = result;
        nextStopResult = result;
    }
    void setNextMoveResult(esp_err_t result){nextMoveResult = result;}
    void setNextStopResult(esp_err_t result){nextStopResult = result;}
    void setCurrentStep(int32_t step){currentStep = step;}

    void reset(){
        lastAction = LastAction::NONE;
        nextMoveResult = ESP_OK;
        nextStopResult = ESP_OK;
        currentStep = 100;
    }

    esp_err_t move(int32_t targetStep, bool isCalibrating = false) override {
        if(targetStep == -1 || (!isCalibrating && targetStep >= currentStep)){
            lastAction = LastAction::UP;
        }
        else{
            lastAction = LastAction::DOWN;
        }
        return nextMoveResult;
    }

    esp_err_t stop() override {
        lastAction = LastAction::STOP;
        return nextStopResult;
    }

    void setHoming(int32_t offset = 0) override {currentStep = 0 - offset;}
    void setMaxStep(int32_t offset = 0) override {(void)offset;}
    int32_t getCurrentStep() const override {return currentStep;}
};
