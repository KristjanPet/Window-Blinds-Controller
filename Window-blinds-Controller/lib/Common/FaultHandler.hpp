#pragma once

#include <cstdint>

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/**
 * @brief Module that first reported a system fault.
 */
enum class FaultSource : uint8_t{
    BlindsController, ///< High-level blinds state machine.
    MotorController,  ///< Low-level motor control layer.
    HomeSensor,       ///< Home/reference sensor layer.
    CommandQueue      ///< Central command queue layer.
};

/**
 * @brief Specific reason recorded for a fault.
 */
enum class FaultReason : uint8_t{
    None,                        ///< No fault has been recorded.
    MotorStopFailed,             ///< Motor stop command returned an error.
    MotorMoveFailed,             ///< Motor move command returned an error.
    StallRecoveryExhausted,      ///< Stall recovery retried too many times.
    InvalidStallRecoveryTarget,  ///< Stall recovery target was outside valid range.
    StallRecoveryBackoffInvalid, ///< Stall recovery could not choose a safe backoff target.
    UnexpectedCalibrationCommand, ///< Manual or remote command interrupted calibration.
    StuckHomeSensor,             ///< Home sensor stayed active during startup validation.
    RmtRefillFailed,             ///< Motor RMT pulse refill failed.
    MovementCompletionFailed,    ///< Motor movement completion handling failed.
    CommandQueueOverflow,        ///< Command queue could not accept an event.
    CommandQueueUnavailable,     ///< Command queue was used before initialization.
    ExplicitFaultEvent           ///< FAULT event was received by the controller.
};

/**
 * @brief Stored details for the first recorded fault.
 */
struct FaultRecord{
    FaultSource source; ///< Module that reported the fault.
    FaultReason reason; ///< Project-specific fault reason.
    esp_err_t espErr;   ///< ESP-IDF error associated with the failure.
};

/**
 * @brief Thread-safe first-fault recorder shared by control modules.
 *
 * Preserves the first fault until reset by restart and can notify one task when
 * the first fault is recorded. Provides task-context and ISR-context recording
 * entry points so timing-sensitive callbacks can report failures safely.
 */
class FaultHandler{
private:
    mutable portMUX_TYPE faultMux_ = portMUX_INITIALIZER_UNLOCKED;
    bool hasFault_ = false;
    FaultRecord fault_ = {FaultSource::BlindsController, FaultReason::None, ESP_OK};
    TaskHandle_t changeTask_ = nullptr;

public:
    /**
     * @brief Set the task notified when the first fault is recorded.
     *
     * @param task FreeRTOS task handle to notify, or nullptr to disable notification.
     */
    void setChangeTask(TaskHandle_t task);

    /**
     * @brief Record the first fault from task context.
     *
     * Later faults are ignored so the original cause remains available.
     *
     * @param source Module that detected the fault.
     * @param reason Project-specific failure reason.
     * @param espErr ESP-IDF error associated with the failure.
     */
    void record(FaultSource source, FaultReason reason, esp_err_t espErr);

    /**
     * @brief Record the first fault from ISR context.
     *
     * Uses ISR-safe critical sections and task notification.
     *
     * @param source Module that detected the fault.
     * @param reason Project-specific failure reason.
     * @param espErr ESP-IDF error associated with the failure.
     */
    void recordFromISR(FaultSource source, FaultReason reason, esp_err_t espErr);

    /**
     * @brief Check whether any fault has been recorded.
     *
     * @return true if a fault record is present.
     */
    bool hasFault() const;

    /**
     * @brief Copy the stored fault record if one is present.
     *
     * @param fault Destination for the stored fault details.
     * @return true if fault was populated, false if no fault is recorded.
     */
    bool getFault(FaultRecord& fault) const;
};
