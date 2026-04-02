#include "BlindsCommandQueue.hpp"

static const char* TAG = "Command Queue";

esp_err_t BlindsCommandQueue::init(){
    queue_ = xQueueCreate(AppConfig::commandsQueueDepth, sizeof(BlindsEvent));
    if(queue_ == NULL){
        ESP_LOGE(TAG, "Creating commands queue failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

BaseType_t BlindsCommandQueue::send(BlindsEvent e, TickType_t wait){
    return xQueueSend(queue_, &e, wait);
}

BaseType_t BlindsCommandQueue::sendFromISR(BlindsEvent e, BaseType_t* hpTaskWoken){
    return xQueueSendFromISR(queue_, &e, hpTaskWoken);
}

BaseType_t BlindsCommandQueue::receive(BlindsEvent& e, TickType_t wait){
    return xQueueReceive(queue_, &e, wait);
}
