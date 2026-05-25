#include "BlindsCommandQueue.hpp"

static const char* TAG = "Command Queue";
static constexpr uint32_t DROPPED_EVENT_FAULT_THRESHOLD = 5;

static BlindsCommand commandFor(BlindsEvent event){
    return {event, 0};
}

static BlindsEvent recoveryEventFor(uint32_t droppedEvents){
    return droppedEvents >= DROPPED_EVENT_FAULT_THRESHOLD ? BlindsEvent::FAULT : BlindsEvent::STOP;
}

static BaseType_t sendRecoveryToFront(QueueHandle_t queue, BlindsEvent event){
    BlindsCommand command = commandFor(event);
    if(xQueueSendToFront(queue, &command, 0) == pdTRUE){
        return pdTRUE;
    }

    BlindsCommand discarded;
    if(xQueueReceive(queue, &discarded, 0) != pdTRUE){
        return pdFALSE;
    }

    return xQueueSendToFront(queue, &command, 0);
}

static BaseType_t sendRecoveryToFrontFromISR(QueueHandle_t queue, BlindsEvent event, BaseType_t* hpTaskWoken){
    BlindsCommand command = commandFor(event);
    if(xQueueSendToFrontFromISR(queue, &command, hpTaskWoken) == pdTRUE){
        return pdTRUE;
    }

    BlindsCommand discarded;
    if(xQueueReceiveFromISR(queue, &discarded, hpTaskWoken) != pdTRUE){
        return pdFALSE;
    }

    return xQueueSendToFrontFromISR(queue, &command, hpTaskWoken);
}

esp_err_t BlindsCommandQueue::init(){
    queue_ = xQueueCreate(AppConfig::commandsQueueDepth, sizeof(BlindsCommand));
    if(queue_ == NULL){
        ESP_LOGE(TAG, "Creating commands queue failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

BaseType_t BlindsCommandQueue::send(BlindsEvent e, TickType_t wait){
    return send(commandFor(e), wait);
}

BaseType_t BlindsCommandQueue::send(const BlindsCommand& command, TickType_t wait){
    if(queue_ == nullptr){
        ESP_LOGE(TAG, "Send failed: queue not initialized");
        return pdFALSE;
    }

    BaseType_t sent = xQueueSend(queue_, &command, wait);
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

BaseType_t BlindsCommandQueue::sendMoveToPercent(uint8_t percent, TickType_t wait){
    return send({BlindsEvent::MOVE_TO_PERCENT, percent}, wait);
}

BaseType_t BlindsCommandQueue::sendFromISR(BlindsEvent e, BaseType_t* hpTaskWoken){
    if(queue_ == nullptr){
        droppedEvents_ = droppedEvents_ + 1;
        return pdFALSE;
    }

    BlindsCommand command = commandFor(e);
    BaseType_t sent = xQueueSendFromISR(queue_, &command, hpTaskWoken);
    if(sent != pdTRUE){
        droppedEvents_ = droppedEvents_ + 1;
        BlindsEvent recoveryEvent = recoveryEventFor(droppedEvents_);
        sendRecoveryToFrontFromISR(queue_, recoveryEvent, hpTaskWoken);
    }

    return sent;
}

BaseType_t BlindsCommandQueue::receive(BlindsCommand& command, TickType_t wait){
    if(queue_ == nullptr){
        ESP_LOGE(TAG, "Receive failed: queue not initialized");
        return pdFALSE;
    }

    return xQueueReceive(queue_, &command, wait);
}

BaseType_t BlindsCommandQueue::receive(BlindsEvent& e, TickType_t wait){
    BlindsCommand command = commandFor(BlindsEvent::STOP);
    const BaseType_t received = receive(command, wait);
    if(received == pdTRUE){
        e = command.event;
    }

    return received;
}
