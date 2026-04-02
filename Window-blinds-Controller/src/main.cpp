#include <freertos/FreeRTOS.h>
#include "MotorController.hpp"
#include "ButtonHandler.hpp"
#include "BlindsController.hpp"
#include "BlindsCommandQueue.hpp"

static const char *TAG_MAIN = "MAIN";

extern "C" void app_main(void) {

    BlindsCommandQueue commandsQueue;
    if(commandsQueue.init() != ESP_OK){
        esp_restart();
    };

    MotorPins motorPins = {GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_25}; //step, dir, enable
    MotorController motor(motorPins, 120, commandsQueue);
    if(motor.init() != ESP_OK){
        esp_restart();
    };

    BlindsController blinds(motor, commandsQueue);

    ButtonPins buttonPins = {GPIO_NUM_33, GPIO_NUM_32}; //up, down
    ButtonHandler buttonHandler(&buttonPins, commandsQueue);
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