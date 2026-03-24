#include "ButtonHandler.hpp"

ButtonHandler::ButtonHandler(ButtonPins* pins, MotorController* motor)
             : pins_(*pins), motor_(motor){}

void IRAM_ATTR ButtonHandler::buttonIsr(void *arg){
    ButtonIsrContext *ctx = static_cast<ButtonIsrContext*>(arg);

    BaseType_t hpTaskWoken = pdFALSE;
    ButtonPressed btn = ctx->button;

    xQueueSendFromISR(ctx->self->buttonQueue_, &btn, &hpTaskWoken);

    if(hpTaskWoken){
        portYIELD_FROM_ISR();
    }
}

esp_err_t ButtonHandler::init(){
    gpio_config_t buttIoConf = {
        .pin_bit_mask = (1ULL << pins_.up) | (1ULL << pins_.down),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&buttIoConf);

    buttonQueue_ = xQueueCreate(10, sizeof(ButtonPressed));

    upCtx_ = {this, ButtonPressed::UP};
    downCtx_ = {this, ButtonPressed::DOWN};

    gpio_install_isr_service(0); //TODO handle error

    gpio_isr_handler_add(pins_.down, buttonIsr, &downCtx_);
    gpio_isr_handler_add(pins_.up, buttonIsr, &upCtx_);

    return ESP_OK;
}

void ButtonHandler::buttonTask(void *arg){
    auto *self = static_cast<ButtonHandler*>(arg);
    ButtonPressed btn;

    while(true){
        if(xQueueReceive(self->buttonQueue_, &btn, portMAX_DELAY) == pdTRUE){
            vTaskDelay(pdMS_TO_TICKS(30)); //debounce time

            switch (btn)
            {
            case ButtonPressed::UP:
                if(gpio_get_level(self->pins_.up)){   
                    self->motor_->moveMotorUp();
                }
                break;
            case ButtonPressed::DOWN:
                if(gpio_get_level(self->pins_.down)){
                    self->motor_->moveMotorDown();
                }
                break;
            default:
                break;
            }

            while(xQueueReceive(self->buttonQueue_, &btn, 0) == pdTRUE) {} //drains extra bounces
        }
    }
}