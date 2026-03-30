#include "ButtonHandler.hpp"

static const char* TAG = "BUTTON";

ButtonHandler::ButtonHandler(ButtonPins* pins, BlindsController& blindsCtrl)
             : pins_(*pins), blindsCtrl_(blindsCtrl){}

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
            esp_err_t err;
            gpio_num_t pin;
            BlindsEvent cmd;

            switch (btn){
                case ButtonPressed::UP:
                    pin = self->pins_.up;
                    cmd = BlindsEvent::UP;
                    break;
                case ButtonPressed::DOWN:
                    pin = self->pins_.down;
                    cmd = BlindsEvent::DOWN;
                    break;
                default:
                    continue;
            }

            if (gpio_get_level(pin)) {
                err = xQueueSend(self->blindsCtrl_.commandsQueue_, &cmd, 0);
                if(err != pdTRUE){
                    ESP_LOGE(TAG, "Error sending command: %s", esp_err_to_name(err));
                }

                while (gpio_get_level(pin)) { //TODO remove after implementing schmit trigger
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }

            while(xQueueReceive(self->buttonQueue_, &btn, 0) == pdTRUE) {} //drains extra bounces
        }
    }
}