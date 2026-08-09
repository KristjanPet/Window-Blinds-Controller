#pragma once

#include <esp_err.h>
#include <esp_event.h>
#include <esp_netif_types.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

/**
 * @brief ESP-IDF WiFi station wrapper with connection event bits.
 *
 * Owns WiFi/NVS/network-interface initialization and exposes a FreeRTOS event
 * group so dependent services can wait for connectivity without owning WiFi
 * internals.
 */
class Wifi{
public:
    /**
     * @brief Event bit set while the station has an IP connection.
     */
    static constexpr EventBits_t connectedEventBit = static_cast<EventBits_t>(1U << 0);

    /**
     * @brief Event bit set while the station is disconnected.
     */
    static constexpr EventBits_t disconnectedEventBit = static_cast<EventBits_t>(1U << 1);

private:
    bool initialized_ = false;
    bool started_ = false;
    bool eventHandlersRegistered_ = false;
    esp_netif_t* netif_ = nullptr;
    EventGroupHandle_t connectionEvents_ = nullptr;
    esp_event_handler_instance_t wifiEventHandler_ = nullptr;
    esp_event_handler_instance_t ipEventHandler_ = nullptr;

    static void eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);
    void handleWifiEvent(int32_t eventId);
    void handleIpEvent(int32_t eventId, void* eventData);
    esp_err_t registerEventHandlers();

public:
    /**
     * @brief Initialize NVS, TCP/IP networking, WiFi station mode, and event handlers.
     *
     * @return ESP_OK on success, ESP_ERR_NO_MEM for allocation failures, or an
     *         ESP-IDF error from WiFi/network setup.
     */
    esp_err_t init();

    /**
     * @brief Start WiFi station mode and request connection to an access point.
     *
     * @param ssid Null-terminated WiFi SSID.
     * @param password Null-terminated WiFi password.
     * @return ESP_OK on successful start/connect request, ESP_ERR_INVALID_ARG for
     *         invalid credentials, or an ESP-IDF error from WiFi setup.
     */
    esp_err_t startAndConnect(const char* ssid, const char* password);

    /**
     * @brief Check the current WiFi connection event bit.
     *
     * @return true when the connected event bit is set.
     */
    bool isConnected() const;

    /**
     * @brief Get the WiFi connection event group.
     *
     * @return FreeRTOS event group handle, or nullptr before init() succeeds.
     */
    EventGroupHandle_t connectionEvents() const;
};
