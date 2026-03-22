#include "MotorController.hpp"

MotorController::MotorController(const MotorPins& pins) : pins_(pins)
{
    //stepper motor pins init
    gpio_config_t motorIoConf = {
        .pin_bit_mask = (1ULL << pins_.step) | (1ULL << pins_.dir) | (1ULL << pins_.enable),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&motorIoConf);
}