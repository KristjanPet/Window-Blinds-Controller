#pragma once
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include "IMotor.hpp"
#include "BlindsCommandQueue.hpp"
#include "FaultHandler.hpp"
#include "Types.hpp"

/**
 * @brief High-level blinds state machine states.
 */
enum class BlindsState{
    IDLE,             ///< No active movement or calibration.
    CALIBRATING_HOME, ///< Moving toward the home/reference sensor.
    CALIBRATING_MAX,  ///< Moving toward the maximum travel point after homing.
    MOVING_UP,        ///< Moving toward the configured maximum/open position.
    MOVING_DOWN,      ///< Moving toward the home/minimum position.
    STALL_RECOVERY,   ///< Backing away after a detected stall before retrying.
    FAULT             ///< Fault state; normal movement commands are rejected.
};

/**
 * @brief High-level blinds command and calibration state machine.
 *
 * Consumes button, sensor, motor, and remote events from the command queue,
 * decides state transitions, and delegates hardware motion to IMotor. The class
 * owns blinds policy but does not touch GPIO, timers, or motor-driver internals.
 */
class BlindsController{

private:
    BlindsState state_ = BlindsState::IDLE;
    IMotor& motor_;
    BlindsCommandQueue& commandsQueue_;
    FaultHandler& faultHandler_;
    int32_t activeTargetStep_ = 0;
    bool hasActiveTarget_ = false;
    BlindsState recoveryReturnState_ = BlindsState::IDLE;
    uint8_t normalStallRecoveries_ = 0;
    int32_t calibrationReturnStep_ = 0;

    void enterFault(FaultReason reason, esp_err_t err);
    void resetNormalStallRecovery();
    esp_err_t handleNormalStall(int32_t currentStep);
    esp_err_t retryStallRecoveryTarget();
    esp_err_t targetStepFromPercent(uint8_t percent, int32_t& targetStep) const;
    esp_err_t moveToTargetStep(int32_t targetStep);
    esp_err_t handleMoveToPercent(uint8_t percent);
    
public:
    /**
     * @brief Create a blinds controller with mandatory motor and reporting dependencies.
     *
     * @param motor Motor interface used to execute movement decisions.
     * @param commandQueue Queue that supplies input and hardware events.
     * @param faultHandler Fault recorder used when controller decisions fail.
     */
    BlindsController(IMotor& motor, BlindsCommandQueue& commandQueue, FaultHandler& faultHandler);

    /**
     * @brief FreeRTOS task entry point that continuously receives and handles commands.
     *
     * @param arg Pointer to the BlindsController instance.
     */
    static void handleCommandTask(void* arg);

    /**
     * @brief Handle a command/event without additional payload data.
     *
     * @param cmd Event to process.
     * @return ESP_OK on success, ESP_ERR_INVALID_STATE for rejected commands, or
     *         an error propagated from the motor or queue-dependent control path.
     */
    esp_err_t handleCommand(BlindsEvent cmd);

    /**
     * @brief Handle a command/event with optional payload data.
     *
     * @param command Command payload to process.
     * @return ESP_OK on success, ESP_ERR_INVALID_ARG for unsupported commands or
     *         invalid payloads, ESP_ERR_INVALID_STATE for rejected commands, or an
     *         error propagated from the motor/control path.
     */
    esp_err_t handleCommand(const BlindsCommand& command);

    /**
     * @brief Get the current high-level blinds state.
     *
     * @return Current controller state.
     */
    BlindsState getState() const;

    /**
     * @brief Override the current high-level blinds state.
     *
     * Intended for tests and tightly controlled recovery paths.
     *
     * @param state New controller state.
     */
    void setState(BlindsState state);
};
