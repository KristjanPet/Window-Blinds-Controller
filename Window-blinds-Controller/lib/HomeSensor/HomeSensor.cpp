#include "HomeSensor.hpp"

#include <esp_log.h>

static const char* TAG = "HOME_SENSOR";
static constexpr int HOME_SENSOR_ACTIVE_LEVEL = 0;

HomeSensor::HomeSensor(gpio_num_t pin, BlindsCommandQueue& commandQueue, FaultHandler& faultHandler)
    : pin_(pin), commandQueue_(commandQueue), faultHandler_(faultHandler){}

void IRAM_ATTR HomeSensor::sensorIsr(void* arg){
    auto* self = static_cast<HomeSensor*>(arg);
    if(!self){
        return;
    }

    BaseType_t hpTaskWoken = pdFALSE;
    BlindsEvent event = BlindsEvent::HOMING_REACHED;
    self->commandQueue_.sendFromISR(event, &hpTaskWoken);

    if(hpTaskWoken){
        portYIELD_FROM_ISR();
    }
}

esp_err_t HomeSensor::init(){
    gpio_config_t sensorIoConf = {
        .pin_bit_mask = (1ULL << pin_),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    ESP_RETURN_ON_ERROR(gpio_config(&sensorIoConf), TAG, "Failed to configure home sensor GPIO");

    return ESP_OK;
}

esp_err_t HomeSensor::sensorCheck(){
    workingAtInit_ = (gpio_get_level(pin_) == HOME_SENSOR_ACTIVE_LEVEL);
    if(workingAtInit_){ //sensor active or not working
        if(commandQueue_.send(BlindsEvent::HOMING_CHECK) != pdTRUE){
            ESP_LOGE(TAG, "Failed to send homing check event");
            return ESP_FAIL;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        workingAtInit_ = (gpio_get_level(pin_) == HOME_SENSOR_ACTIVE_LEVEL);

        if(workingAtInit_){ 
            faultHandler_.record(FaultSource::HomeSensor,
                                 FaultReason::StuckHomeSensor,
                                 ESP_ERR_INVALID_STATE);
            if(commandQueue_.send(BlindsEvent::FAULT) != pdTRUE){
                ESP_LOGE(TAG, "Failed to send homing fault event");
                return ESP_FAIL;
            }
            ESP_LOGE(TAG, "Homing senzor not working");
            return ESP_ERR_INVALID_STATE;
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
