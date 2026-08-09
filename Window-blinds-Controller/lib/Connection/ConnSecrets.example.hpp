#pragma once

/**
 * @brief Template values for local WiFi and MQTT credentials.
 *
 * Copy this file to ConnSecrets.hpp and fill in deployment-specific values.
 * Keep real credentials out of version control.
 */
namespace ConnSecrets{
    /** @brief WiFi network SSID. */
    constexpr const char* wifiSsid = "";

    /** @brief WiFi network password. */
    constexpr const char* wifiPassword = "";
    
    /** @brief MQTT broker URI used by MqttClient. */
    constexpr const char* mqttBrokerUri = "mqtt://192.168.1.10";

    /** @brief Optional MQTT username. */
    constexpr const char* mqttUsername = "";

    /** @brief Optional MQTT password. */
    constexpr const char* mqttPassword = "";
}
