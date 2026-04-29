#include "HomeSensor.hpp"

#include <esp_log.h>

static const char* TAG = "HOME_SENSOR";

HomeSensor::HomeSensor(gpio_num_t pin, BlindsCommandQueue& commandQueue)
    : pin_(pin), commandQueue_(commandQueue){}

void IRAM_ATTR HomeSensor::sensorIsr(void* arg){
    auto* self = static_cast<HomeSensor*>(arg);
    if(!self){
        return;
    }

    BaseType_t hpTaskWoken = pdFALSE;
    BlindsEvent event = BlindsEvent::HOMING_REACHED;
    BaseType_t sent = self->commandQueue_.sendFromISR(event, &hpTaskWoken);
    if(sent != pdTRUE){
        self->droppedEvents_++;
    }

    if(hpTaskWoken){
        portYIELD_FROM_ISR();
    }
}

esp_err_t HomeSensor::init(){
    // Assumption to verify on bench before connecting to ESP32:
    // NJK signal is conditioned to ESP32-safe levels and behaves as active-low
    // open-collector, so this input uses the internal pull-up and falling edge.
    gpio_config_t sensorIoConf = {
        .pin_bit_mask = (1ULL << pin_),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    ESP_RETURN_ON_ERROR(gpio_config(&sensorIoConf), TAG, "Failed to configure home sensor GPIO");

    activeAtInit_ = (gpio_get_level(pin_) == 0);
    if(activeAtInit_){
        ESP_LOGW(TAG, "Home sensor is active at init"); //TODO move a bit to check if it is even working
    }

    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(pin_, sensorIsr, this),
                        TAG, "Failed to add home sensor ISR handler");

    return ESP_OK;
}

bool HomeSensor::wasActiveAtInit() const{
    return activeAtInit_;
}
