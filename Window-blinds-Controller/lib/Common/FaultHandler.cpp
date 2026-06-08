#include "FaultHandler.hpp"

void FaultHandler::setChangeTask(TaskHandle_t task){
    taskENTER_CRITICAL(&faultMux_);
    changeTask_ = task;
    taskEXIT_CRITICAL(&faultMux_);
}

void FaultHandler::record(FaultSource source, FaultReason reason, esp_err_t espErr){
    TaskHandle_t changeTask = nullptr;

    taskENTER_CRITICAL(&faultMux_);
    if(!hasFault_){
        fault_ = {source, reason, espErr};
        hasFault_ = true;
        changeTask = changeTask_;
    }
    taskEXIT_CRITICAL(&faultMux_);

    if(changeTask != nullptr){
        xTaskNotifyGive(changeTask);
    }
}

void FaultHandler::recordFromISR(FaultSource source, FaultReason reason, esp_err_t espErr){
    TaskHandle_t changeTask = nullptr;

    taskENTER_CRITICAL_ISR(&faultMux_);
    if(!hasFault_){
        fault_ = {source, reason, espErr};
        hasFault_ = true;
        changeTask = changeTask_;
    }
    taskEXIT_CRITICAL_ISR(&faultMux_);

    if(changeTask != nullptr){
        BaseType_t hpTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(changeTask, &hpTaskWoken);
        if(hpTaskWoken){
            portYIELD_FROM_ISR();
        }
    }
}

bool FaultHandler::hasFault() const{
    bool hasFault = false;

    taskENTER_CRITICAL(&faultMux_);
    hasFault = hasFault_;
    taskEXIT_CRITICAL(&faultMux_);

    return hasFault;
}

bool FaultHandler::getFault(FaultRecord& fault) const{
    bool hasFault = false;

    taskENTER_CRITICAL(&faultMux_);
    hasFault = hasFault_;
    if(hasFault_){
        fault = fault_;
    }
    taskEXIT_CRITICAL(&faultMux_);

    return hasFault;
}
