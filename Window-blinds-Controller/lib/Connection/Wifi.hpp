#pragma once

#include <esp_err.h>
#include <esp_event.h>
#include <esp_netif_types.h>
#include <esp_wifi.h>

class Wifi{
private:
    bool initialized_ = false;
    bool started_ = false;
    bool connected_ = false;
    bool eventHandlersRegistered_ = false;
    esp_netif_t* netif_ = nullptr;
    esp_event_handler_instance_t wifiEventHandler_ = nullptr;
    esp_event_handler_instance_t ipEventHandler_ = nullptr;

    static void eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);
    void handleWifiEvent(int32_t eventId);
    void handleIpEvent(int32_t eventId, void* eventData);
    esp_err_t registerEventHandlers();

public:
    esp_err_t init();
    esp_err_t startAndConnect(const char* ssid, const char* password);
};
