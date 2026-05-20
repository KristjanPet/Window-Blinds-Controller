#include "Wifi.hpp"

#include <cstring>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi_default.h>
#include <nvs_flash.h>

uint8_t WifiNetwork::signalQualityPercent() const{
    constexpr int8_t minUsableRssi = -100;
    constexpr int8_t maxQualityRssi = -50;

    if(rssi <= minUsableRssi){
        return 0;
    }
    if(rssi >= maxQualityRssi){
        return 100;
    }

    return static_cast<uint8_t>((rssi - minUsableRssi) * 2);
}

esp_err_t Wifi::init(){
    if(initialized_){
        return ESP_OK;
    }

    esp_err_t err = nvs_flash_init();
    if(err != ESP_OK){
        return err;
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

    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if(err != ESP_OK){
        return err;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if(err != ESP_OK){
        return err;
    }

    err = esp_wifi_start();
    if(err != ESP_OK){
        return err;
    }

    initialized_ = true;
    return ESP_OK;
}

esp_err_t Wifi::scan(WifiNetwork* results, uint16_t maxResults, uint16_t& found){
    found = 0;

    if(!initialized_){
        return ESP_ERR_INVALID_STATE;
    }
    if(results == nullptr && maxResults > 0){
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = esp_wifi_scan_start(nullptr, true);
    if(err != ESP_OK){
        return err;
    }

    err = esp_wifi_scan_get_ap_num(&found);
    if(err != ESP_OK){
        const esp_err_t clearErr = esp_wifi_clear_ap_list();
        return clearErr == ESP_OK ? err : clearErr;
    }

    const uint16_t recordsToCopy = found < maxResults ? found : maxResults;
    for(uint16_t i = 0; i < recordsToCopy; ++i){
        wifi_ap_record_t record = {};
        err = esp_wifi_scan_get_ap_record(&record);
        if(err != ESP_OK){
            const esp_err_t clearErr = esp_wifi_clear_ap_list();
            return clearErr == ESP_OK ? err : clearErr;
        }

        std::memset(results[i].ssid, 0, sizeof(results[i].ssid));
        std::memcpy(results[i].ssid, record.ssid, WifiNetwork::maxSsidLength);
        results[i].ssid[WifiNetwork::maxSsidLength] = '\0';
        results[i].rssi = record.rssi;
        results[i].channel = record.primary;
        results[i].authMode = record.authmode;
    }

    if(recordsToCopy < found){
        err = esp_wifi_clear_ap_list();
        if(err != ESP_OK){
            return err;
        }
    }

    return ESP_OK;
}

void Wifi::logScanResults(const WifiNetwork* results, uint16_t displayed, uint16_t found) const{
    static const char* TAG_WIFI = "WIFI";

    if(results == nullptr && displayed > 0){
        ESP_LOGE(TAG_WIFI, "Cannot log WiFi scan results: results buffer is null");
        return;
    }

    ESP_LOGI(TAG_WIFI, "WiFi scan found %u network(s), showing %u", found, displayed);
    for(uint16_t i = 0; i < displayed; ++i){
        ESP_LOGI(TAG_WIFI,
                 "%u: SSID=\"%s\", RSSI=%d dBm, quality=%u%%, channel=%u, auth=%d",
                 static_cast<unsigned>(i + 1),
                 results[i].ssid,
                 static_cast<int>(results[i].rssi),
                 static_cast<unsigned>(results[i].signalQualityPercent()),
                 static_cast<unsigned>(results[i].channel),
                 static_cast<int>(results[i].authMode));
    }
}
