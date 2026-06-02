#include <esp_log.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "MotorController.hpp"
#include "ButtonHandler.hpp"
#include "HomeSensor.hpp"
#include "BlindsController.hpp"
#include "BlindsCommandQueue.hpp"
#include "Tmc2209Driver.hpp"
#include "AppConfig.hpp"
#include "FaultHandler.hpp"
#include "Wifi.hpp"
#include "MqttClient.hpp"
#include "ConnSecrets.hpp"

static const char *TAG_MAIN = "MAIN";

extern "C" void app_main(void) {

    esp_err_t err = ESP_OK;

    FaultHandler faultHandler;
    BlindsCommandQueue commandsQueue(faultHandler);
    err = commandsQueue.init();
    if(err != ESP_OK){
        ESP_LOGE(TAG_MAIN, "Command queue init failed: %s", esp_err_to_name(err));
        esp_restart();
    };

    err = gpio_install_isr_service(0);
    if(err != ESP_OK){
        ESP_LOGE(TAG_MAIN, "Failed to install GPIO ISR service: %s", esp_err_to_name(err));
        esp_restart();
    };

    Tmc2209Driver motorDriver(AppConfig::UARTDriverPin, commandsQueue);
    err = motorDriver.init();
    if(err != ESP_OK){
        ESP_LOGE(TAG_MAIN, "TMC2209 init failed: %s", esp_err_to_name(err));
        esp_restart();
    };
    err = motorDriver.configureAndVerify();
    if(err != ESP_OK){
        ESP_LOGE(TAG_MAIN, "TMC2209 config failed: %s", esp_err_to_name(err));
        esp_restart();
    };

    MotorController motor(AppConfig::motorPins, commandsQueue, faultHandler);
    err = motor.init();
    if(err != ESP_OK){
        ESP_LOGE(TAG_MAIN, "Motor init failed: %s", esp_err_to_name(err));
        esp_restart();
    };

    BlindsController blinds(motor, commandsQueue, faultHandler);

    ButtonHandler buttonHandler(AppConfig::buttonPins, commandsQueue);
    err = buttonHandler.init();
    if(err != ESP_OK){
        ESP_LOGE(TAG_MAIN, "Button handler init failed: %s", esp_err_to_name(err));
        esp_restart();
    };

    HomeSensor homeSensor(AppConfig::homeSensorPin, commandsQueue, faultHandler);
    err = homeSensor.init();
    if(err != ESP_OK){
        ESP_LOGE(TAG_MAIN, "Home sensor init failed: %s", esp_err_to_name(err));
        esp_restart();
    };

    Wifi wifi;
    err = wifi.startAndConnect(ConnSecrets::wifiSsid, ConnSecrets::wifiPassword);
    if(err != ESP_OK){
        ESP_LOGW(TAG_MAIN, "WiFi start/connect failed, continuing without WiFi: %s", esp_err_to_name(err));
    }

    MqttClient mqtt(commandsQueue, faultHandler);
    err = mqtt.init(ConnSecrets::mqttBrokerUri, ConnSecrets::mqttUsername, ConnSecrets::mqttPassword);
    if(err != ESP_OK){
        ESP_LOGW(TAG_MAIN, "MQTT init failed, continuing without MQTT: %s", esp_err_to_name(err));
    }
    else{
        err = mqtt.start(wifi.connectionEvents());
        if(err != ESP_OK){
            ESP_LOGW(TAG_MAIN, "MQTT start failed, continuing without MQTT: %s", esp_err_to_name(err));
        }
    }

    if(xTaskCreate(ButtonHandler::buttonTask, "Button", 2048, &buttonHandler, 3, NULL) != pdPASS){
        ESP_LOGE(TAG_MAIN, "Failed to create Button task, restarting...");
        esp_restart();
    }
    if(xTaskCreate(BlindsController::handleCommandTask, "Blinds", 4096, &blinds, 4, NULL) != pdPASS){
        ESP_LOGE(TAG_MAIN, "Failed to create handle command task, restarting...");
        esp_restart();
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_LOGI(TAG_MAIN, "Init complete, checking sensor....");

    err = homeSensor.sensorCheck();
    if(err != ESP_OK){
        ESP_LOGE(TAG_MAIN, "Homing sensor check failed: %s", esp_err_to_name(err));
        esp_restart();
    };

    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG_MAIN, "calibrating blinds....");

    if(commandsQueue.send(BlindsEvent::CALIBRATE) != pdTRUE){
        ESP_LOGE(TAG_MAIN, "Failed to start calibrating, restarting...");
        esp_restart();
    }


    while (true){
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
}
