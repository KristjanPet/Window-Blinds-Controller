#pragma once
#include <cstdint>
#include <driver/gpio.h>

/**
 * @brief Events and commands passed through the central blinds command queue.
 */
enum class BlindsEvent{
    STOP,            ///< Stop current motion.
    UP,              ///< Move toward the configured maximum/open position.
    DOWN,            ///< Move toward the home/minimum position.
    MOVE_TO_PERCENT, ///< Move to the percent value carried by BlindsCommand.
    LIMIT_REACHED,   ///< Motor layer reported that a software target or soft limit was reached.
    CALIBRATE,       ///< Start the homing and maximum-travel calibration sequence.
    HOMING_REACHED,  ///< Home sensor reported the reference point.
    STALL_DETECTED,  ///< TMC2209 DIAG/stall path reported a stall event.
    HOMING_CHECK,    ///< Startup home-sensor validation should move away and recheck.
    FAULT            ///< Explicit fault event for the blinds state machine.
};

/**
 * @brief Queue payload for blinds commands that need optional data.
 */
struct BlindsCommand{
    BlindsEvent event; ///< Command or event type.
    uint8_t percent;   ///< Target percentage for MOVE_TO_PERCENT commands.
};

/**
 * @brief GPIO assignments for STEP/DIR/EN motor control.
 */
struct MotorPins{
    gpio_num_t step;   ///< STEP pulse output pin.
    gpio_num_t dir;    ///< Direction output pin.
    gpio_num_t enable; ///< Driver enable output pin.
};

/**
 * @brief GPIO assignments for the physical manual buttons.
 */
struct ButtonPins{
    gpio_num_t up;   ///< Up/open button input pin.
    gpio_num_t down; ///< Down/close button input pin.
};

/**
 * @brief GPIO assignments for the TMC2209 UART and DIAG signal.
 */
struct TMCUARTDriverPins{
    gpio_num_t TX;   ///< ESP32 UART TX pin connected to the driver.
    gpio_num_t RX;   ///< ESP32 UART RX pin connected to the driver.
    gpio_num_t diag; ///< TMC2209 DIAG input pin used for stall events.
};
