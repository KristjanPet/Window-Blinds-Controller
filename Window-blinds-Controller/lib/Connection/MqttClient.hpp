#pragma once

#include <cstddef>

#include <esp_err.h>
#include <mqtt_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include "FaultHandler.hpp"
#include "Types.hpp"

class BlindsCommandQueue;

class MqttClient{
private:
    static constexpr uint32_t startTaskStack_ = 3072;
    static constexpr UBaseType_t startTaskPriority_ = 2;

    BlindsCommandQueue& commandQueue_;
    FaultHandler& faultHandler_;
    esp_mqtt_client_handle_t client_ = nullptr;
    EventGroupHandle_t wifiEvents_ = nullptr;
    TaskHandle_t startTaskHandle_ = nullptr;
    TaskHandle_t statusTaskHandle_ = nullptr;
    bool initialized_ = false;
    bool clientStarted_ = false;
    bool mqttConnected_ = false;

    static void startTask(void* userCtx);
    static void statusTask(void* userCtx);
    static void mqttEventHandler(void* handlerArgs, esp_event_base_t eventBase, int32_t eventId, void* eventData);
    void startTaskLoop();
    void statusTaskLoop();
    esp_err_t publishFaultStatus();
    void handleMqttEvent(esp_mqtt_event_id_t eventId, esp_mqtt_event_handle_t event);

public:
    MqttClient(BlindsCommandQueue& commandQueue, FaultHandler& faultHandler);
    static bool parseCommandPayload(const char* data, int dataLen, BlindsCommand& command);
    static bool buildFaultStatusPayload(const FaultHandler& faultHandler, char* buffer, size_t bufferSize);
    esp_err_t init(const char* brokerUri, const char* username, const char* password);
    esp_err_t start(EventGroupHandle_t wifiEvents);
};
