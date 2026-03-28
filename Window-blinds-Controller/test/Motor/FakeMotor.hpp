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
    esp_err_t moveUp() override;
    esp_err_t moveDown() override;
    esp_err_t stop() override;
    LastAction getLastAction();
    void setNextResult(esp_err_t result);
};
