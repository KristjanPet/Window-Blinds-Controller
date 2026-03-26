#include <freertos/FreeRTOS.h>
#include "MotorController.hpp"
#include "ButtonHandler.hpp"

static const char *TAG_MAIN = "MAIN";

extern "C" void app_main(void) {

    MotorPins motorPins = {GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_25}; //step, dir, enable

    MotorController motor(motorPins, 100);
    motor.init();

    BlindsController blinds(&motor);

    ButtonPins buttonPins = {GPIO_NUM_33, GPIO_NUM_32}; //up, down
    ButtonHandler buttonHandler(&buttonPins, &blinds);
    buttonHandler.init();

    if(xTaskCreate(ButtonHandler::buttonTask, "Button", 4096, &buttonHandler, 10, NULL) == pdPASS){}

    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG_MAIN, "Init complete, running program....");

    while (true){
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
}