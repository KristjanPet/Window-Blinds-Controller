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
    if(queue_ == nullptr){
        ESP_LOGE(TAG, "Send failed: queue not initialized");
        return pdFALSE;
    }

    return xQueueSend(queue_, &e, wait);
}

BaseType_t BlindsCommandQueue::sendFromISR(BlindsEvent e, BaseType_t* hpTaskWoken){
    if(queue_ == nullptr){
        droppedFromIsr_ = droppedFromIsr_ + 1;
        return pdFALSE;
    }

    BaseType_t sent = xQueueSendFromISR(queue_, &e, hpTaskWoken);
    if(sent != pdTRUE){
        droppedFromIsr_ = droppedFromIsr_ + 1;
    }

    return sent;
}

BaseType_t BlindsCommandQueue::receive(BlindsEvent& e, TickType_t wait){
    if(queue_ == nullptr){
        ESP_LOGE(TAG, "Receive failed: queue not initialized");
        return pdFALSE;
    }

    return xQueueReceive(queue_, &e, wait);
}

uint32_t BlindsCommandQueue::getDroppedFromISR() const{
    return droppedFromIsr_;
}
