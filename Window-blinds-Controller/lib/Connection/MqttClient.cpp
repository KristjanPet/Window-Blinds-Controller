#include "MqttClient.hpp"

#include <cstring>
#include <esp_log.h>

#include "AppConfig.hpp"
#include "Wifi.hpp"

static const char* TAG_MQTT = "MQTT";

static const char* emptyToNull(const char* value){
    return value != nullptr && value[0] != '\0' ? value : nullptr;
}

esp_err_t MqttClient::init(const char* brokerUri, const char* username, const char* password){
    if(brokerUri == nullptr || std::strlen(brokerUri) == 0){
        return ESP_ERR_INVALID_ARG;
    }

    if(client_ != nullptr){
        return ESP_OK;
    }

    esp_mqtt_client_config_t config = {};
    config.broker.address.uri = brokerUri;
    config.credentials.client_id = AppConfig::mqttClientId;
    config.credentials.username = emptyToNull(username);
    config.credentials.authentication.password = emptyToNull(password);
    config.network.disable_auto_reconnect = false;

    client_ = esp_mqtt_client_init(&config);
    if(client_ == nullptr){
        return ESP_FAIL;
    }

    esp_err_t err = esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY, &MqttClient::mqttEventHandler, this);
    if(err != ESP_OK){
        return err;
    }

    initialized_ = true;
    return ESP_OK;
}

esp_err_t MqttClient::start(EventGroupHandle_t wifiEvents){
    if(!initialized_ || client_ == nullptr){
        return ESP_ERR_INVALID_STATE;
    }
    if(wifiEvents == nullptr){
        return ESP_ERR_INVALID_ARG;
    }
    if(startTaskHandle_ != nullptr || clientStarted_){
        return ESP_OK;
    }

    wifiEvents_ = wifiEvents;
    if(xTaskCreate(startTask, "MQTTStart", startTaskStack_, this, startTaskPriority_, &startTaskHandle_) != pdPASS){
        startTaskHandle_ = nullptr;
        return ESP_FAIL;
    }

    return ESP_OK;
}

void MqttClient::startTask(void* userCtx){
    auto* self = static_cast<MqttClient*>(userCtx);
    if(self == nullptr){
        vTaskDelete(nullptr);
        return;
    }

    self->startTaskLoop();
}

void MqttClient::startTaskLoop(){
    xEventGroupWaitBits(wifiEvents_, Wifi::connectedEventBit, pdFALSE, pdTRUE, portMAX_DELAY);

    const esp_err_t err = esp_mqtt_client_start(client_);
    if(err == ESP_OK){
        clientStarted_ = true;
        ESP_LOGI(TAG_MQTT, "MQTT client started");
    }
    else{
        ESP_LOGW(TAG_MQTT, "MQTT client start failed: %s", esp_err_to_name(err));
    }

    startTaskHandle_ = nullptr;
    vTaskDelete(nullptr);
}

void MqttClient::mqttEventHandler(void* handlerArgs, esp_event_base_t eventBase, int32_t eventId, void* eventData){
    (void)eventBase;

    auto* self = static_cast<MqttClient*>(handlerArgs);
    if(self == nullptr){
        return;
    }

    auto* event = static_cast<esp_mqtt_event_handle_t>(eventData);
    self->handleMqttEvent(static_cast<esp_mqtt_event_id_t>(eventId), event);
}

void MqttClient::handleMqttEvent(esp_mqtt_event_id_t eventId, esp_mqtt_event_handle_t event){
    switch(eventId){
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT connected");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG_MQTT, "MQTT disconnected");
        break;
    case MQTT_EVENT_ERROR:
        if(event != nullptr && event->error_handle != nullptr){
            ESP_LOGW(TAG_MQTT,
                     "MQTT error type=%d esp_err=%s sock_errno=%d",
                     static_cast<int>(event->error_handle->error_type),
                     esp_err_to_name(event->error_handle->esp_tls_last_esp_err),
                     event->error_handle->esp_transport_sock_errno);
        }
        else{
            ESP_LOGW(TAG_MQTT, "MQTT error");
        }
        break;
    default:
        break;
    }
}
