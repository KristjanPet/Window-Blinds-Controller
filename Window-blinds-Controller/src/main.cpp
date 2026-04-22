#include <esp_log.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "MotorController.hpp"
#include "ButtonHandler.hpp"
#include "BlindsController.hpp"
#include "BlindsCommandQueue.hpp"
#include "Tmc2209Driver.hpp"
#include "AppConfig.hpp"

static const char *TAG_MAIN = "MAIN";

extern "C" void app_main(void) {

    BlindsCommandQueue commandsQueue;
    if(commandsQueue.init() != ESP_OK){
        esp_restart();
    };

    esp_err_t err = gpio_install_isr_service(0);
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

    MotorController motor(AppConfig::motorPins, AppConfig::togglePeriodUs, commandsQueue);
    if(motor.init() != ESP_OK){
        esp_restart();
    };

    BlindsController blinds(motor, commandsQueue);

    ButtonHandler buttonHandler(AppConfig::buttonPins, commandsQueue);
    if(buttonHandler.init() != ESP_OK){
        esp_restart();
    };

    if(xTaskCreate(ButtonHandler::buttonTask, "Button", 2048, &buttonHandler, 3, NULL) != pdPASS){
        ESP_LOGE(TAG_MAIN, "Failed to create Button task, restarting...");
        esp_restart();
    }
    if(xTaskCreate(BlindsController::handleCommandTask, "Blinds", 4096, &blinds, 4, NULL) != pdPASS){
        ESP_LOGE(TAG_MAIN, "Failed to create handle command task, restarting...");
        esp_restart();
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG_MAIN, "Init complete, running program....");

    while (true){
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
}
