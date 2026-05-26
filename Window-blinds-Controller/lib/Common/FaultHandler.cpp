#include "FaultHandler.hpp"

void FaultHandler::record(FaultSource source, FaultReason reason, esp_err_t espErr){
    taskENTER_CRITICAL(&faultMux_);
    if(!hasFault_){
        fault_ = {source, reason, espErr};
        hasFault_ = true;
    }
    taskEXIT_CRITICAL(&faultMux_);
}

void FaultHandler::recordFromISR(FaultSource source, FaultReason reason, esp_err_t espErr){
    taskENTER_CRITICAL_ISR(&faultMux_);
    if(!hasFault_){
        fault_ = {source, reason, espErr};
        hasFault_ = true;
    }
    taskEXIT_CRITICAL_ISR(&faultMux_);
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
