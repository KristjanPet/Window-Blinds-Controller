#include "ButtonHandler.hpp"

ButtonHandler::ButtonHandler()
{}

esp_err_t ButtonHandler::init(){
    gpio_config_t buttIoConf = {
        .pin_bit_mask = (1ULL << pins_.up) | (1ULL << pins_.down),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&buttIoConf);

    buttonQueue_ = xQueueCreate(10, sizeof(uint32_t));

    gpio_install_isr_service(0); //TODO handle error

    gpio_isr_handler_add(pins_.down, buttonIsr, (void*)ButtonPressed::DOWN);
    gpio_isr_handler_add(pins_.up, buttonIsr, (void*)ButtonPressed::UP);

    return ESP_OK;
}