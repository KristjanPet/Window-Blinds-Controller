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
    esp_err_t nextResult = ESP_OK;
public:
    LastAction getLastAction(){return lastAction;};
    void setNextResult(esp_err_t result){nextResult = result;}

    void reset(){
        lastAction = LastAction::NONE;
        nextResult = ESP_OK;
    }

    esp_err_t moveUp() override {
        lastAction = LastAction::UP;
        return nextResult;
    }

    esp_err_t moveDown() override {
        lastAction = LastAction::DOWN;
        return nextResult;
    }

    esp_err_t stop() override {
        lastAction = LastAction::STOP;
        return nextResult;
    }
};
