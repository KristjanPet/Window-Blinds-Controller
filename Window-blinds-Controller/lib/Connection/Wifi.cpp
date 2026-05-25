#include "Wifi.hpp"

#include <cstring>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi_default.h>
#include <nvs_flash.h>

static const char* TAG_WIFI = "WIFI";

static esp_err_t initNvs(){
    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND){
        err = nvs_flash_erase();
        if(err != ESP_OK){
            return err;
        }
        err = nvs_flash_init();
    }

    return err;
}

esp_err_t Wifi::init(){
    if(initialized_){
        return ESP_OK;
    }

    esp_err_t err = initNvs();
    if(err != ESP_OK){
        return err;
    }

    if(connectionEvents_ == nullptr){
        connectionEvents_ = xEventGroupCreate();
        if(connectionEvents_ == nullptr){
            return ESP_ERR_NO_MEM;
        }
        xEventGroupSetBits(connectionEvents_, disconnectedEventBit);
    }

    err = esp_netif_init();
    if(err != ESP_OK && err != ESP_ERR_INVALID_STATE){
        return err;
    }

    err = esp_event_loop_create_default();
    if(err != ESP_OK && err != ESP_ERR_INVALID_STATE){
        return err;
    }

    if(netif_ == nullptr){
        esp_netif_config_t netifConfig = ESP_NETIF_DEFAULT_WIFI_STA();
        netif_ = esp_netif_new(&netifConfig);
        if(netif_ == nullptr){
            return ESP_ERR_NO_MEM;
        }

        err = esp_netif_attach_wifi_station(netif_);
        if(err != ESP_OK){
            return err;
        }

        err = esp_wifi_set_default_wifi_sta_handlers();
        if(err != ESP_OK){
            return err;
        }
    }

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&config);
    if(err != ESP_OK){
        return err;
    }

    err = registerEventHandlers();
    if(err != ESP_OK){
        return err;
    }

    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if(err != ESP_OK){
        return err;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if(err != ESP_OK){
        return err;
    }

    initialized_ = true;
    return ESP_OK;
}

esp_err_t Wifi::startAndConnect(const char* ssid, const char* password){
    if(ssid == nullptr || password == nullptr){
        return ESP_ERR_INVALID_ARG;
    }

    const size_t ssidLength = std::strlen(ssid);
    const size_t passwordLength = std::strlen(password);
    if(ssidLength == 0 || ssidLength >= sizeof(wifi_sta_config_t::ssid)){
        return ESP_ERR_INVALID_ARG;
    }
    if(passwordLength >= sizeof(wifi_sta_config_t::password)){
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = init();
    if(err != ESP_OK){
        return err;
    }

    wifi_config_t wifiConfig = {};
    std::memcpy(wifiConfig.sta.ssid, ssid, ssidLength);
    std::memcpy(wifiConfig.sta.password, password, passwordLength);
    wifiConfig.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wifiConfig.sta.failure_retry_cnt = 3;

    err = esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
    if(err != ESP_OK){
        return err;
    }

    if(connectionEvents_ != nullptr){
        xEventGroupClearBits(connectionEvents_, connectedEventBit);
        xEventGroupSetBits(connectionEvents_, disconnectedEventBit);
    }

    if(!started_){
        err = esp_wifi_start();
        if(err != ESP_OK){
            return err;
        }
        started_ = true;
    }

    ESP_LOGI(TAG_WIFI, "Connecting to SSID: %s", ssid);
    err = esp_wifi_connect();
    if(err != ESP_OK && err != ESP_ERR_WIFI_CONN){
        return err;
    }

    return ESP_OK;
}

esp_err_t Wifi::registerEventHandlers(){
    if(eventHandlersRegistered_){
        return ESP_OK;
    }

    esp_err_t err = esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &Wifi::eventHandler,
                                                        this,
                                                        &wifiEventHandler_);
    if(err != ESP_OK){
        return err;
    }

    err = esp_event_handler_instance_register(IP_EVENT,
                                              IP_EVENT_STA_GOT_IP,
                                              &Wifi::eventHandler,
                                              this,
                                              &ipEventHandler_);
    if(err != ESP_OK){
        const esp_err_t cleanupErr = esp_event_handler_instance_unregister(WIFI_EVENT,
                                                                           ESP_EVENT_ANY_ID,
                                                                           wifiEventHandler_);
        wifiEventHandler_ = nullptr;
        return cleanupErr == ESP_OK ? err : cleanupErr;
    }

    eventHandlersRegistered_ = true;
    return ESP_OK;
}

void Wifi::eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData){
    Wifi* wifi = static_cast<Wifi*>(arg);
    if(wifi == nullptr){
        return;
    }

    if(eventBase == WIFI_EVENT){
        wifi->handleWifiEvent(eventId);
    }
    else if(eventBase == IP_EVENT){
        wifi->handleIpEvent(eventId, eventData);
    }
}

void Wifi::handleWifiEvent(int32_t eventId){
    if(eventId == WIFI_EVENT_STA_START){
        ESP_LOGI(TAG_WIFI, "WiFi station started");
        return;
    }

    if(eventId == WIFI_EVENT_STA_DISCONNECTED){
        if(connectionEvents_ != nullptr){
            xEventGroupClearBits(connectionEvents_, connectedEventBit);
            xEventGroupSetBits(connectionEvents_, disconnectedEventBit);
        }
        ESP_LOGW(TAG_WIFI, "WiFi disconnected, retrying");
        const esp_err_t err = esp_wifi_connect();
        if(err != ESP_OK){
            ESP_LOGW(TAG_WIFI, "WiFi reconnect request failed: %s", esp_err_to_name(err));
        }
    }
}

void Wifi::handleIpEvent(int32_t eventId, void* eventData){
    if(eventId != IP_EVENT_STA_GOT_IP){
        return;
    }

    if(connectionEvents_ != nullptr){
        xEventGroupClearBits(connectionEvents_, disconnectedEventBit);
        xEventGroupSetBits(connectionEvents_, connectedEventBit);
    }

    const ip_event_got_ip_t* event = static_cast<const ip_event_got_ip_t*>(eventData);
    if(event == nullptr){
        ESP_LOGI(TAG_WIFI, "WiFi connected, got IP");
        return;
    }

    ESP_LOGI(TAG_WIFI, "WiFi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
}


bool Wifi::isConnected() const{
    if(connectionEvents_ == nullptr){
        return false;
    }

    return (xEventGroupGetBits(connectionEvents_) & connectedEventBit) != 0;
}

EventGroupHandle_t Wifi::connectionEvents() const{
    return connectionEvents_;
}
