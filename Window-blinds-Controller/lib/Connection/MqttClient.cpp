#include "MqttClient.hpp"

#include <cstring>
#include <esp_log.h>

#include "AppConfig.hpp"
#include "BlindsCommandQueue.hpp"
#include "Wifi.hpp"

static const char* TAG_MQTT = "MQTT";

static const char* emptyToNull(const char* value){
    return value != nullptr && value[0] != '\0' ? value : nullptr;
}

static bool isAsciiSpace(char value){
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

static char toLowerAscii(char value){
    if(value >= 'A' && value <= 'Z'){
        return static_cast<char>(value - 'A' + 'a');
    }

    return value;
}

static bool tokenEqualsIgnoreCase(const char* data, int len, const char* token){
    if(static_cast<int>(std::strlen(token)) != len){
        return false;
    }

    for(int i = 0; i < len; ++i){
        if(toLowerAscii(data[i]) != token[i]){
            return false;
        }
    }

    return true;
}

static bool parseCommandPayload(const char* data, int dataLen, BlindsEvent& command){
    if(data == nullptr || dataLen <= 0){
        return false;
    }

    int start = 0;
    int end = dataLen;
    while(start < end && isAsciiSpace(data[start])){
        ++start;
    }
    while(end > start && isAsciiSpace(data[end - 1])){
        --end;
    }

    const int len = end - start;
    const char* token = data + start;
    if(tokenEqualsIgnoreCase(token, len, "up")){
        command = BlindsEvent::UP;
        return true;
    }
    if(tokenEqualsIgnoreCase(token, len, "down")){
        command = BlindsEvent::DOWN;
        return true;
    }
    if(tokenEqualsIgnoreCase(token, len, "stop")){
        command = BlindsEvent::STOP;
        return true;
    }

    return false;
}

static const char* commandName(BlindsEvent command){
    switch(command){
    case BlindsEvent::UP:
        return "up";
    case BlindsEvent::DOWN:
        return "down";
    case BlindsEvent::STOP:
        return "stop";
    default:
        return "unknown";
    }
}

static bool commandTopicMatches(esp_mqtt_event_handle_t event){
    if(event == nullptr || event->topic == nullptr){
        return false;
    }

    const int commandTopicLen = static_cast<int>(std::strlen(AppConfig::mqttCommandTopic));
    return event->topic_len == commandTopicLen &&
           std::memcmp(event->topic, AppConfig::mqttCommandTopic, commandTopicLen) == 0;
}

MqttClient::MqttClient(BlindsCommandQueue& commandQueue): commandQueue_(commandQueue){}

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
    case MQTT_EVENT_CONNECTED: {
        ESP_LOGI(TAG_MQTT, "MQTT connected");
        const int msgId = esp_mqtt_client_subscribe(client_, AppConfig::mqttCommandTopic, AppConfig::mqttQos);
        if(msgId < 0){
            ESP_LOGW(TAG_MQTT, "MQTT command topic subscribe failed");
        }
        else{
            ESP_LOGI(TAG_MQTT, "Subscribed to MQTT command topic, msg_id=%d", msgId);
        }
        break;
    }
    case MQTT_EVENT_DATA: {
        if(event == nullptr || !commandTopicMatches(event)){
            break;
        }
        if(event->retain){
            ESP_LOGW(TAG_MQTT, "Ignoring retained MQTT command");
            break;
        }
        if(event->current_data_offset != 0 || event->data_len != event->total_data_len){
            ESP_LOGW(TAG_MQTT, "Ignoring fragmented MQTT command");
            break;
        }

        BlindsEvent command = BlindsEvent::STOP;
        if(!parseCommandPayload(event->data, event->data_len, command)){
            ESP_LOGW(TAG_MQTT, "Ignoring invalid MQTT command");
            break;
        }

        if(commandQueue_.send(command, 0) != pdTRUE){
            ESP_LOGE(TAG_MQTT, "Failed to queue MQTT command: %s", commandName(command));
        }
        else{
            ESP_LOGI(TAG_MQTT, "Queued MQTT command: %s", commandName(command));
        }
        break;
    }
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
