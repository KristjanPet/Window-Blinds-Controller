#include "BlindsCommandQueue.hpp"

static const char* TAG = "Command Queue";
static constexpr uint32_t DROPPED_EVENT_FAULT_THRESHOLD = 5;

static BlindsEvent recoveryEventFor(uint32_t droppedEvents){
    return droppedEvents >= DROPPED_EVENT_FAULT_THRESHOLD ? BlindsEvent::FAULT : BlindsEvent::STOP;
}

static BaseType_t sendRecoveryToFront(QueueHandle_t queue, BlindsEvent event){
    if(xQueueSendToFront(queue, &event, 0) == pdTRUE){
        return pdTRUE;
    }

    BlindsEvent discarded;
    if(xQueueReceive(queue, &discarded, 0) != pdTRUE){
        return pdFALSE;
    }

    return xQueueSendToFront(queue, &event, 0);
}

static BaseType_t sendRecoveryToFrontFromISR(QueueHandle_t queue, BlindsEvent event, BaseType_t* hpTaskWoken){
    if(xQueueSendToFrontFromISR(queue, &event, hpTaskWoken) == pdTRUE){
        return pdTRUE;
    }

    BlindsEvent discarded;
    if(xQueueReceiveFromISR(queue, &discarded, hpTaskWoken) != pdTRUE){
        return pdFALSE;
    }

    return xQueueSendToFrontFromISR(queue, &event, hpTaskWoken);
}

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

    BaseType_t sent = xQueueSend(queue_, &e, wait);
    if(sent != pdTRUE){
        droppedEvents_ = droppedEvents_ + 1;
        BlindsEvent recoveryEvent = recoveryEventFor(droppedEvents_);

        ESP_LOGE(TAG, "Send failed, dropped events: %lu", static_cast<unsigned long>(droppedEvents_));
        if(sendRecoveryToFront(queue_, recoveryEvent) != pdTRUE){
            ESP_LOGE(TAG, "Failed to queue recovery event");
        }
    }

    return sent;
}

BaseType_t BlindsCommandQueue::sendFromISR(BlindsEvent e, BaseType_t* hpTaskWoken){
    if(queue_ == nullptr){
        droppedEvents_ = droppedEvents_ + 1;
        return pdFALSE;
    }

    BaseType_t sent = xQueueSendFromISR(queue_, &e, hpTaskWoken);
    if(sent != pdTRUE){
        droppedEvents_ = droppedEvents_ + 1;
        BlindsEvent recoveryEvent = recoveryEventFor(droppedEvents_);
        sendRecoveryToFrontFromISR(queue_, recoveryEvent, hpTaskWoken);
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
