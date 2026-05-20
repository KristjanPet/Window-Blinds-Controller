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
#include "AppConfig.hpp"

enum class MotorState{
    STOPPED,
    UP,
    DOWN
};

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
    SemaphoreHandle_t rmtMutex_ = nullptr;

    MotorState motorState_ = MotorState::STOPPED;
    int32_t currentStep_ = INT_MAX;
    int32_t maxStep_ = INT_MAX;
    int32_t targetStep_ = 0;
    int32_t moveStartStep_ = INT_MAX;
    uint32_t targetPulseCount_ = 0;
    uint32_t reservedPulseCount_ = 0;
    uint32_t currentTogglePeriodUs_ = AppConfig::StartTogglePeriodUs;
    uint32_t rampStepCounter_ = 0;
    bool initialized_ = false;
    bool rmtEnabled_ = false;
    bool pcntRunning_ = false;
    bool limitEventQueued_ = false;
    bool abortRequested_ = false;

    rmt_symbol_word_t rmtBuffers_[rmtBufferCount_][rmtSymbolsPerBuffer_] = {};
    size_t rmtBufferLengths_[rmtBufferCount_] = {};
    bool rmtBufferInUse_[rmtBufferCount_] = {};
    uint8_t nextBufferToQueue_ = 0;
    uint8_t nextBufferToRelease_ = 0;
    uint8_t activeTransactions_ = 0;
    uint32_t completedTransactions_ = 0;
    uint32_t releasedTransactions_ = 0;

    BlindsCommandQueue& commandsQueue_;
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
    esp_err_t refreshPositionFromPcnt();
    static int32_t positionFromCount(int32_t startStep, MotorState state, int pcntCount);
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
    MotorController(const MotorPins& pins, BlindsCommandQueue& commandsQueue);
    esp_err_t init();
    esp_err_t move(int32_t targetStep, bool isCalibrating = false) override;
    esp_err_t moveToMax() override;
    esp_err_t stop() override;
    void setHoming(int32_t offset = 0) override;
    void setMaxStep(int32_t offset = 0) override;
    int32_t getCurrentStep() const override;
    int32_t getMaxStep() const override;
};
