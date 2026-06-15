#pragma once
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <esp_check.h>
#include "Types.hpp"
#include "AppConfig.hpp"
#include "FaultHandler.hpp"

/**
 * @brief Central FreeRTOS queue for blinds commands and hardware events.
 *
 * Normalizes button, MQTT, motor, and sensor inputs into BlindsCommand messages.
 * Queue send failures are recorded as faults and trigger recovery events so
 * safety-critical stop/fault handling is not silently lost.
 */
class BlindsCommandQueue{

private:
    QueueHandle_t queue_ = nullptr;
    volatile uint32_t droppedEvents_ = 0;
    FaultHandler& faultHandler_;

public:
    /**
     * @brief Create a command queue wrapper with a fault-reporting dependency.
     *
     * @param faultHandler Fault recorder used for unavailable or overflowing queue paths.
     */
    explicit BlindsCommandQueue(FaultHandler& faultHandler);

    /**
     * @brief Allocate the underlying FreeRTOS queue.
     *
     * @return ESP_OK on success, or ESP_FAIL if queue allocation fails.
     */
    esp_err_t init();

    /**
     * @brief Send an event without payload data from task context.
     *
     * @param e Event to queue.
     * @param wait Maximum FreeRTOS ticks to wait for queue space; defaults to 0 for non-blocking send.
     * @return pdTRUE if queued, otherwise pdFALSE.
     */
    BaseType_t send(BlindsEvent e, TickType_t wait = 0);

    /**
     * @brief Send a full command payload from task context.
     *
     * @param command Command payload to queue.
     * @param wait Maximum FreeRTOS ticks to wait for queue space; defaults to 0 for non-blocking send.
     * @return pdTRUE if queued, otherwise pdFALSE.
     */
    BaseType_t send(const BlindsCommand& command, TickType_t wait = 0);

    /**
     * @brief Send a MOVE_TO_PERCENT command from task context.
     *
     * @param percent Target position percentage.
     * @param wait Maximum FreeRTOS ticks to wait for queue space; defaults to 0 for non-blocking send.
     * @return pdTRUE if queued, otherwise pdFALSE.
     */
    BaseType_t sendMoveToPercent(uint8_t percent, TickType_t wait = 0);

    /**
     * @brief Send an event without payload data from ISR context.
     *
     * @param e Event to queue.
     * @param hpTaskWoken Optional FreeRTOS wake flag updated by ISR-safe queue calls;
     *        defaults to nullptr when the caller does not need wake tracking.
     * @return pdTRUE if queued, otherwise pdFALSE.
     */
    BaseType_t sendFromISR(BlindsEvent e, BaseType_t* hpTaskWoken = nullptr);

    /**
     * @brief Receive a full command payload from task context.
     *
     * @param command Destination populated with the received command.
     * @param wait Maximum FreeRTOS ticks to wait for a command; defaults to
     *        portMAX_DELAY for an indefinite wait.
     * @return pdTRUE if a command was received, otherwise pdFALSE.
     */
    BaseType_t receive(BlindsCommand& command, TickType_t wait = portMAX_DELAY);

    /**
     * @brief Receive only the event portion of the next command.
     *
     * @param e Destination populated with the received event.
     * @param wait Maximum FreeRTOS ticks to wait for a command; defaults to
     *        portMAX_DELAY for an indefinite wait.
     * @return pdTRUE if an event was received, otherwise pdFALSE.
     */
    BaseType_t receive(BlindsEvent& e, TickType_t wait = portMAX_DELAY);
};
