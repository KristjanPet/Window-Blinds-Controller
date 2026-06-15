#pragma once
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_check.h>
#include <portmacro.h>
#include "BlindsCommandQueue.hpp"
#include "AppConfig.hpp"

class ButtonHandler;

/**
 * @brief Debounced button identity passed from ISR to the button task.
 */
enum class ButtonPressed : uint8_t{
    UP,   ///< Up/open button.
    DOWN  ///< Down/close button.
};

/**
 * @brief Stable context object passed to a GPIO ISR registration.
 */
struct ButtonIsrContext{
    ButtonHandler* self;    ///< Button handler instance that owns the queue.
    ButtonPressed button;   ///< Button represented by this ISR context.
};

/**
 * @brief GPIO button reader and debounce task for manual blinds commands.
 *
 * Owns the physical button GPIO inputs, ISR-to-task button queue, and debounce
 * handling. Emits high-level UP/DOWN events through BlindsCommandQueue instead
 * of directly controlling the motor.
 */
class ButtonHandler{

private:
    const ButtonPins pins_;
    QueueHandle_t buttonQueue_ = nullptr;
    ButtonIsrContext upCtx_;
    ButtonIsrContext downCtx_;
    BlindsCommandQueue& commandQueue_;

    static void IRAM_ATTR buttonIsr(void *arg);
public:
    /**
     * @brief Create a button handler with button pins and command output queue.
     *
     * @param pins GPIO assignments for the up and down buttons.
     * @param commandQueue Queue used to report debounced button commands.
     */
    ButtonHandler(const ButtonPins& pins, BlindsCommandQueue& commandQueue);

    /**
     * @brief Configure button GPIO interrupts and allocate the button queue.
     *
     * @return ESP_OK on success, ESP_FAIL on queue allocation failure, or an
     *         ESP-IDF error from GPIO/ISR setup.
     */
    esp_err_t init();

    /**
     * @brief FreeRTOS task entry point that debounces button interrupts.
     *
     * @param arg Pointer to the ButtonHandler instance.
     */
    static void buttonTask(void *arg);
};
