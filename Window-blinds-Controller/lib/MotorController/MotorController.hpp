#pragma once
#include <climits>
#include <esp_task.h>
#include <driver/gpio.h>
#include <driver/rmt_tx.h>
#include <driver/pulse_cnt.h>
#include <freertos/semphr.h>
#include <esp_check.h>
#include "IMotor.hpp"
#include "BlindsCommandQueue.hpp"
#include "FaultHandler.hpp"
#include "AppConfig.hpp"

/**
 * @brief Low-level motor motion state.
 */
enum class MotorState{
    STOPPED, ///< No pulse output is active.
    UP,      ///< Moving toward the configured maximum/open position.
    DOWN     ///< Moving toward the home/minimum position.
};

/**
 * @brief Low-level stepper motor controller for the blinds mechanism.
 *
 * Owns STEP/DIR/EN GPIO control, RMT pulse generation, and PCNT-based position
 * tracking. Movement completion and fault conditions are reported upward through
 * the command queue and fault handler; high-level blinds policy stays outside
 * this class.
 */
class MotorController : public IMotor{
    
private:
    static constexpr uint32_t rmtResolutionHz_ = 1 * 1000 * 1000; // 1 tick = 1 us
    static constexpr size_t rmtBufferCount_ = 4;
    static constexpr size_t rmtSymbolsPerBuffer_ = 64;
    static constexpr int pcntLimit_ = 30000;
    static constexpr uint32_t refillTaskStack_ = 3072;
    static constexpr UBaseType_t refillTaskPriority_ = 5;

    const MotorPins pins_;
    rmt_channel_handle_t stepChannel_ = nullptr;
    rmt_encoder_handle_t stepEncoder_ = nullptr;
    pcnt_unit_handle_t stepCounter_ = nullptr;
    pcnt_channel_handle_t stepCounterChannel_ = nullptr;
    TaskHandle_t refillTaskHandle_ = nullptr;
    // Serializes rmt_transmit() against channel recreation after an abort.
    SemaphoreHandle_t rmtMutex_ = nullptr;

    MotorState motorState_ = MotorState::STOPPED;
    int32_t currentStep_ = INT_MAX;
    int32_t maxStep_ = INT_MAX;
    int32_t targetStep_ = 0;
    int32_t moveStartStep_ = INT_MAX;
    // Reservation prevents over-queueing; PCNT remains the position source of truth.
    uint32_t targetPulseCount_ = 0;
    uint32_t reservedPulseCount_ = 0;
    uint32_t currentTogglePeriodUs_ = AppConfig::StartTogglePeriodUs;
    uint32_t rampStepCounter_ = 0;
    bool initialized_ = false;
    bool rmtEnabled_ = false;
    bool pcntRunning_ = false;
    bool limitEventQueued_ = false;
    bool abortRequested_ = false;

    struct RmtQueueState{
        // Payload memory must remain valid until the transaction-done callback fires.
        rmt_symbol_word_t buffers[rmtBufferCount_][rmtSymbolsPerBuffer_] = {};
        bool bufferInUse[rmtBufferCount_] = {};
        uint8_t nextToQueue = 0;
        uint8_t nextToRelease = 0;
        // RMT completion does not identify payloads, so buffers are released FIFO.
        uint8_t activeTransactions = 0;
        uint32_t completedTransactions = 0;
        uint32_t releasedTransactions = 0;
    };

    RmtQueueState rmtQueue_;

    BlindsCommandQueue& commandsQueue_;
    FaultHandler& faultHandler_;
    mutable portMUX_TYPE motorStepMux_ = portMUX_INITIALIZER_UNLOCKED;

    static bool IRAM_ATTR rmtDoneCallback(
        rmt_channel_handle_t txChannel,
        const rmt_tx_done_event_data_t *edata,
        void *user_ctx);
    static void refillTask(void* user_ctx);
    void refillTaskLoop();
    esp_err_t configureRmt();
    esp_err_t recreateRmtChannel();
    esp_err_t configurePcnt();
    esp_err_t ensureRmtEnabled();
    esp_err_t disableRmtOutput();
    esp_err_t startPcntCounter();
    esp_err_t stopPcntCounter();
    esp_err_t stopPulseHardware();
    esp_err_t refreshPositionFromPcnt();
    void resetRmtBufferState();
    void releaseCompletedTransactions();
    void buildPulseSymbols(rmt_symbol_word_t* buffer, size_t pulseCount);
    esp_err_t fillRmtQueue();
    esp_err_t completeMovementFromTask();
    esp_err_t failMovementFromTask();
    void resetRamp();
    void updateRampAfterStep();
    esp_err_t startMovement(MotorState state, uint32_t dirLevel);
public:
    /**
     * @brief Create a motor controller with mandatory hardware and reporting dependencies.
     *
     * @param pins STEP, DIR, and enable GPIO assignments.
     * @param commandsQueue Queue used to report motor events to the blinds controller.
     * @param faultHandler Fault recorder used for motor-control failures.
     */
    MotorController(const MotorPins& pins, BlindsCommandQueue& commandsQueue, FaultHandler& faultHandler);

    /**
     * @brief Initialize motor GPIO, RMT pulse generation, PCNT position tracking, and the refill task.
     *
     * @return ESP_OK on success, or an ESP-IDF error from the failed setup step.
     */
    esp_err_t init();

    /**
     * @brief Move toward a controller position target.
     *
     * @param targetStep Target position in controller step units.
     * @param isCalibrating Allows calibration movement toward the home direction;
     *        defaults to false for normal soft-limit-checked movement.
     * @return ESP_OK when movement is started or completed, ESP_ERR_INVALID_ARG for an
     *         out-of-range target, or another ESP-IDF error if hardware start fails.
     */
    esp_err_t move(int32_t targetStep, bool isCalibrating = false) override;

    /**
     * @brief Move toward the currently known maximum controller position.
     *
     * @return ESP_OK when movement is started or completed, or an error from move().
     */
    esp_err_t moveToMax() override;

    /**
     * @brief Stop pulse output and refresh the tracked position from PCNT.
     *
     * @return ESP_OK on success, or the first error encountered while stopping hardware.
     */
    esp_err_t stop() override;

    /**
     * @brief Mark the current position relative to the home reference.
     *
     * @param offset Positive controller-step offset applied away from the detected home point;
     *        defaults to 0 to mark the current position as home.
     */
    void setHoming(int32_t offset = 0) override;

    /**
     * @brief Set the controller soft maximum from the current position.
     *
     * @param offset Positive controller-step offset subtracted from the current position;
     *        defaults to 0 to use the current position as the soft maximum.
     */
    void setMaxStep(int32_t offset = 0) override;

    /**
     * @brief Get the current tracked controller position.
     *
     * @return Current position in controller step units, refreshed from PCNT while moving when possible.
     */
    int32_t getCurrentStep() const override;

    /**
     * @brief Get the currently known controller soft maximum.
     *
     * @return Maximum position in controller step units.
     */
    int32_t getMaxStep() const override;
};
