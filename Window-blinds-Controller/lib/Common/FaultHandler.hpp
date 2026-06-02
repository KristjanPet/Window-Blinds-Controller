#pragma once

#include <cstdint>

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

enum class FaultSource : uint8_t{
    BlindsController,
    MotorController,
    HomeSensor,
    CommandQueue
};

enum class FaultReason : uint8_t{
    None,
    MotorStopFailed,
    MotorMoveFailed,
    StallRecoveryExhausted,
    InvalidStallRecoveryTarget,
    StallRecoveryBackoffInvalid,
    UnexpectedCalibrationCommand,
    StuckHomeSensor,
    RmtRefillFailed,
    MovementCompletionFailed,
    CommandQueueOverflow,
    CommandQueueUnavailable,
    ExplicitFaultEvent
};

struct FaultRecord{
    FaultSource source;
    FaultReason reason;
    esp_err_t espErr;
};

class FaultHandler{
private:
    mutable portMUX_TYPE faultMux_ = portMUX_INITIALIZER_UNLOCKED;
    bool hasFault_ = false;
    FaultRecord fault_ = {FaultSource::BlindsController, FaultReason::None, ESP_OK};
    TaskHandle_t changeTask_ = nullptr;

public:
    void setChangeTask(TaskHandle_t task);
    void record(FaultSource source, FaultReason reason, esp_err_t espErr);
    void recordFromISR(FaultSource source, FaultReason reason, esp_err_t espErr);
    bool hasFault() const;
    bool getFault(FaultRecord& fault) const;
};
