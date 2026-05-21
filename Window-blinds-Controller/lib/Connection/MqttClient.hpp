#pragma once

#include <esp_err.h>
#include <mqtt_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

class BlindsCommandQueue;

class MqttClient{
private:
    static constexpr uint32_t startTaskStack_ = 3072;
    static constexpr UBaseType_t startTaskPriority_ = 2;

    BlindsCommandQueue& commandQueue_;
    esp_mqtt_client_handle_t client_ = nullptr;
    EventGroupHandle_t wifiEvents_ = nullptr;
    TaskHandle_t startTaskHandle_ = nullptr;
    bool initialized_ = false;
    bool clientStarted_ = false;

    static void startTask(void* userCtx);
    static void mqttEventHandler(void* handlerArgs, esp_event_base_t eventBase, int32_t eventId, void* eventData);
    void startTaskLoop();
    void handleMqttEvent(esp_mqtt_event_id_t eventId, esp_mqtt_event_handle_t event);

public:
    explicit MqttClient(BlindsCommandQueue& commandQueue);
    esp_err_t init(const char* brokerUri, const char* username, const char* password);
    esp_err_t start(EventGroupHandle_t wifiEvents);
};
