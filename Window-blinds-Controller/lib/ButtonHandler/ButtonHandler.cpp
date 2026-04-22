#include "ButtonHandler.hpp"

static const char* TAG = "BUTTON";

ButtonHandler::ButtonHandler(const ButtonPins& pins, BlindsCommandQueue& commandQueue)
             : pins_(pins), commandQueue_(commandQueue){}

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
    ESP_RETURN_ON_ERROR(gpio_config(&buttIoConf), TAG, "Failed to config button gpio");

    buttonQueue_ = xQueueCreate(AppConfig::buttonsQueueDepth, sizeof(ButtonPressed));
    if(buttonQueue_ == NULL){
        ESP_LOGE(TAG, "Creating button queue failed");
        return ESP_FAIL;
    }

    upCtx_ = {this, ButtonPressed::UP};
    downCtx_ = {this, ButtonPressed::DOWN};

    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(pins_.down, buttonIsr, &downCtx_), TAG, "Failed to add DOWN handler to ISR");
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(pins_.up, buttonIsr, &upCtx_), TAG, "Failed to add UP handler to ISR");

    return ESP_OK;
}

void ButtonHandler::buttonTask(void *arg){
    auto *self = static_cast<ButtonHandler*>(arg);
    ButtonPressed btn;

    while(true){
        if(xQueueReceive(self->buttonQueue_, &btn, portMAX_DELAY) == pdTRUE){
            vTaskDelay(pdMS_TO_TICKS(AppConfig::debouncTime)); //debounce time
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
                if(self->commandQueue_.send(cmd, 0) != pdTRUE){
                    ESP_LOGE(TAG, "Error sending button command");
                }

                while (gpio_get_level(pin)) { //TODO remove after implementing schmit trigger
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }

            while(xQueueReceive(self->buttonQueue_, &btn, 0) == pdTRUE) {} //drains extra bounces
        }
    }
}
