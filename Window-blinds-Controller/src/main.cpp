#include <freertos/FreeRTOS.h>
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

    MotorController motor(AppConfig::motorPins, AppConfig::togglePeriodUs, commandsQueue);
    if(motor.init() != ESP_OK){
        esp_restart();
    };

    Tmc2209Driver motorDriver(AppConfig::UARTDriverPin);

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