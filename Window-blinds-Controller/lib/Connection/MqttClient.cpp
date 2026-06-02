#include "MqttClient.hpp"

#include <cstdio>
#include <cstring>
#include <esp_err.h>
#include <esp_log.h>

#include "AppConfig.hpp"
#include "BlindsCommandQueue.hpp"
#include "Wifi.hpp"

static const char* TAG_MQTT = "MQTT";
static constexpr uint32_t MQTT_STATUS_TASK_STACK = 3072;
static constexpr UBaseType_t MQTT_STATUS_TASK_PRIORITY = 2;
static constexpr size_t MQTT_STATUS_PAYLOAD_BUFFER_SIZE = 160;

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

bool MqttClient::parseCommandPayload(const char* data, int dataLen, BlindsCommand& command){
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
    if(len <= 0){
        return false;
    }

    const char* token = data + start;
    if(tokenEqualsIgnoreCase(token, len, "up")){
        command = {BlindsEvent::MOVE_TO_PERCENT, 100};
        return true;
    }
    if(tokenEqualsIgnoreCase(token, len, "down")){
        command = {BlindsEvent::MOVE_TO_PERCENT, 0};
        return true;
    }
    if(tokenEqualsIgnoreCase(token, len, "stop")){
        command = {BlindsEvent::STOP, 0};
        return true;
    }

    uint16_t percent = 0;
    for(int i = 0; i < len; ++i){
        if(token[i] < '0' || token[i] > '9'){
            return false;
        }

        percent = static_cast<uint16_t>(percent * 10U + static_cast<uint16_t>(token[i] - '0'));
        if(percent > 100){
            return false;
        }
    }

    command = {BlindsEvent::MOVE_TO_PERCENT, static_cast<uint8_t>(percent)};
    return true;
}

static const char* commandName(BlindsEvent command){
    switch(command){
    case BlindsEvent::UP:
        return "up";
    case BlindsEvent::DOWN:
        return "down";
    case BlindsEvent::MOVE_TO_PERCENT:
        return "move_to_percent";
    case BlindsEvent::STOP:
        return "stop";
    default:
        return "unknown";
    }
}

static const char* faultSourceName(FaultSource source){
    switch(source){
    case FaultSource::BlindsController:
        return "blinds_controller";
    case FaultSource::MotorController:
        return "motor_controller";
    case FaultSource::HomeSensor:
        return "home_sensor";
    case FaultSource::CommandQueue:
        return "command_queue";
    default:
        return "unknown";
    }
}

static const char* faultReasonName(FaultReason reason){
    switch(reason){
    case FaultReason::None:
        return "none";
    case FaultReason::MotorStopFailed:
        return "motor_stop_failed";
    case FaultReason::MotorMoveFailed:
        return "motor_move_failed";
    case FaultReason::StallRecoveryExhausted:
        return "stall_recovery_exhausted";
    case FaultReason::InvalidStallRecoveryTarget:
        return "invalid_stall_recovery_target";
    case FaultReason::StallRecoveryBackoffInvalid:
        return "stall_recovery_backoff_invalid";
    case FaultReason::UnexpectedCalibrationCommand:
        return "unexpected_calibration_command";
    case FaultReason::StuckHomeSensor:
        return "stuck_home_sensor";
    case FaultReason::RmtRefillFailed:
        return "rmt_refill_failed";
    case FaultReason::MovementCompletionFailed:
        return "movement_completion_failed";
    case FaultReason::CommandQueueOverflow:
        return "command_queue_overflow";
    case FaultReason::CommandQueueUnavailable:
        return "command_queue_unavailable";
    case FaultReason::ExplicitFaultEvent:
        return "explicit_fault_event";
    default:
        return "unknown";
    }
}

bool MqttClient::buildFaultStatusPayload(const FaultHandler& faultHandler, char* buffer, size_t bufferSize){
    if(buffer == nullptr || bufferSize == 0){
        return false;
    }

    FaultRecord fault = {FaultSource::BlindsController, FaultReason::None, ESP_OK};
    int written = 0;
    if(!faultHandler.getFault(fault)){
        written = std::snprintf(buffer, bufferSize, "{\"fault\":false}");
    }
    else{
        written = std::snprintf(buffer,
                                bufferSize,
                                "{\"fault\":true,\"source\":\"%s\",\"reason\":\"%s\",\"esp_err\":%d,\"esp_err_name\":\"%s\"}",
                                faultSourceName(fault.source),
                                faultReasonName(fault.reason),
                                static_cast<int>(fault.espErr),
                                esp_err_to_name(fault.espErr));
    }

    return written >= 0 && static_cast<size_t>(written) < bufferSize;
}

static bool commandTopicMatches(esp_mqtt_event_handle_t event){
    if(event == nullptr || event->topic == nullptr){
        return false;
    }

    const int commandTopicLen = static_cast<int>(std::strlen(AppConfig::mqttCommandTopic));
    return event->topic_len == commandTopicLen &&
           std::memcmp(event->topic, AppConfig::mqttCommandTopic, commandTopicLen) == 0;
}

MqttClient::MqttClient(BlindsCommandQueue& commandQueue, FaultHandler& faultHandler)
    : commandQueue_(commandQueue), faultHandler_(faultHandler){}

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

    if(statusTaskHandle_ == nullptr){
        if(xTaskCreate(statusTask, "MQTTStatus", MQTT_STATUS_TASK_STACK, this, MQTT_STATUS_TASK_PRIORITY, &statusTaskHandle_) != pdPASS){
            statusTaskHandle_ = nullptr;
            return ESP_FAIL;
        }
        faultHandler_.setChangeTask(statusTaskHandle_);
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

void MqttClient::statusTask(void* userCtx){
    auto* self = static_cast<MqttClient*>(userCtx);
    if(self == nullptr){
        vTaskDelete(nullptr);
        return;
    }

    self->statusTaskLoop();
}

void MqttClient::statusTaskLoop(){
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if(mqttConnected_){
            publishFaultStatus();
        }
    }
}

esp_err_t MqttClient::publishFaultStatus(){
    char payload[MQTT_STATUS_PAYLOAD_BUFFER_SIZE] = {};
    if(!buildFaultStatusPayload(faultHandler_, payload, sizeof(payload))){
        ESP_LOGW(TAG_MQTT, "Fault status payload build failed");
        return ESP_ERR_INVALID_SIZE;
    }

    const int msgId = esp_mqtt_client_publish(client_,
                                              AppConfig::mqttStatusTopic,
                                              payload,
                                              0,
                                              AppConfig::mqttQos,
                                              AppConfig::mqttRetain);
    if(msgId < 0){
        ESP_LOGW(TAG_MQTT, "Fault status publish failed: %d", msgId);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG_MQTT, "Published fault status");
    return ESP_OK;
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
        mqttConnected_ = true;
        ESP_LOGI(TAG_MQTT, "MQTT connected");
        const int msgId = esp_mqtt_client_subscribe(client_, AppConfig::mqttCommandTopic, AppConfig::mqttQos);
        if(msgId < 0){
            ESP_LOGW(TAG_MQTT, "MQTT command topic subscribe failed");
        }
        else{
            ESP_LOGI(TAG_MQTT, "Subscribed to MQTT command topic, msg_id=%d", msgId);
        }
        publishFaultStatus();
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

        BlindsCommand command = {BlindsEvent::STOP, 0};
        if(!parseCommandPayload(event->data, event->data_len, command)){
            ESP_LOGW(TAG_MQTT, "Ignoring invalid MQTT command");
            break;
        }

        if(commandQueue_.send(command, 0) != pdTRUE){
            ESP_LOGE(TAG_MQTT, "Failed to queue MQTT command: %s", commandName(command.event));
        }
        else if(command.event == BlindsEvent::MOVE_TO_PERCENT){
            ESP_LOGI(TAG_MQTT, "Queued MQTT command: %u%%", static_cast<unsigned>(command.percent));
        }
        else{
            ESP_LOGI(TAG_MQTT, "Queued MQTT command: %s", commandName(command.event));
        }
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
        mqttConnected_ = false;
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
