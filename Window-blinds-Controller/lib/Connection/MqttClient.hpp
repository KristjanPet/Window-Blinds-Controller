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

/**
 * @brief MQTT command input and fault-status publisher for the blinds controller.
 *
 * Subscribes to the configured command topic and forwards valid remote commands
 * into the same BlindsCommandQueue used by local inputs. Fault status publishing
 * is driven by FaultHandler notifications and WiFi connectivity events.
 */
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
    /**
     * @brief Create an MQTT client with command and fault-reporting dependencies.
     *
     * @param commandQueue Queue used to submit accepted MQTT commands.
     * @param faultHandler Fault source used for status publishing.
     */
    MqttClient(BlindsCommandQueue& commandQueue, FaultHandler& faultHandler);

    /**
     * @brief Parse a raw MQTT command payload into a blinds command.
     *
     * Accepts case-insensitive "up", "down", and "stop" tokens, or a numeric
     * percentage from 0 through 100.
     *
     * @param data Pointer to payload bytes.
     * @param dataLen Number of payload bytes.
     * @param command Destination populated when parsing succeeds.
     * @return true when command was populated with a valid command.
     */
    static bool parseCommandPayload(const char* data, int dataLen, BlindsCommand& command);

    /**
     * @brief Build a compact JSON fault-status payload.
     *
     * @param faultHandler Fault source to query.
     * @param buffer Destination buffer for a null-terminated JSON payload.
     * @param bufferSize Size of buffer in bytes.
     * @return true when the payload fit in buffer, false for invalid or too-small buffers.
     */
    static bool buildFaultStatusPayload(const FaultHandler& faultHandler, char* buffer, size_t bufferSize);

    /**
     * @brief Initialize the ESP-IDF MQTT client.
     *
     * Empty username/password strings are treated as absent credentials.
     *
     * @param brokerUri MQTT broker URI.
     * @param username Optional MQTT username.
     * @param password Optional MQTT password.
     * @return ESP_OK on success, ESP_ERR_INVALID_ARG for an empty broker URI,
     *         ESP_FAIL if client allocation fails, or an ESP-IDF MQTT error.
     */
    esp_err_t init(const char* brokerUri, const char* username, const char* password);

    /**
     * @brief Start MQTT support after WiFi connectivity is available.
     *
     * Creates the status task and a start task that waits on Wifi::connectedEventBit
     * before starting the MQTT client.
     *
     * @param wifiEvents WiFi connection event group.
     * @return ESP_OK when startup tasks are created or already running,
     *         ESP_ERR_INVALID_STATE before init(), ESP_ERR_INVALID_ARG for a null
     *         event group, or ESP_FAIL if task creation fails.
     */
    esp_err_t start(EventGroupHandle_t wifiEvents);
};
