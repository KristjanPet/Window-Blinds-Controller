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
    gpio_config_t sensorIoConf = {
        .pin_bit_mask = (1ULL << pin_),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    ESP_RETURN_ON_ERROR(gpio_config(&sensorIoConf), TAG, "Failed to configure home sensor GPIO");

    return ESP_OK;
}

esp_err_t HomeSensor::sensorCheck(){
    workingAtInit_ = (gpio_get_level(pin_) == 1);
    if(workingAtInit_){ //sensor active or not working
        commandQueue_.send(BlindsEvent::HOMING_CHECK);
        vTaskDelay(pdMS_TO_TICKS(500));
        workingAtInit_ = (gpio_get_level(pin_) == 1);

        if(workingAtInit_){ //TODO needs fix probably schmit trigger
            commandQueue_.send(BlindsEvent::FAULT);
            ESP_LOGE(TAG, "Homing senzor not working");
        }
        else{
            ESP_RETURN_ON_ERROR(gpio_isr_handler_add(pin_, sensorIsr, this), TAG, "Failed to add home sensor ISR handler");
        }
    }
    else{
        ESP_RETURN_ON_ERROR(gpio_isr_handler_add(pin_, sensorIsr, this), TAG, "Failed to add home sensor ISR handler");
    }

    return ESP_OK;
}
