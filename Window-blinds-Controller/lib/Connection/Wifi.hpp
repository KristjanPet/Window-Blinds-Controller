#pragma once

#include <cstdint>
#include <esp_err.h>
#include <esp_netif_types.h>
#include <esp_wifi.h>

struct WifiNetwork{
    static constexpr uint8_t maxSsidLength = 32;

    char ssid[maxSsidLength + 1] = {};
    int8_t rssi = 0;
    uint8_t channel = 0;
    wifi_auth_mode_t authMode = WIFI_AUTH_OPEN;

    uint8_t signalQualityPercent() const;
};

class Wifi{
private:
    bool initialized_ = false;
    esp_netif_t* netif_ = nullptr;

public:
    esp_err_t init();
    esp_err_t scan(WifiNetwork* results, uint16_t maxResults, uint16_t& found);
    void logScanResults(const WifiNetwork* results, uint16_t displayed, uint16_t found) const;
};
