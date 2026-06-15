#pragma once
#include <esp_check.h>
#include "Types.hpp"

/**
 * @brief Hardware-independent motor interface used by BlindsController.
 *
 * Keeps high-level blinds policy testable without GPIO, RMT, PCNT, or driver
 * hardware. Implementations own the low-level movement details and report errors
 * through esp_err_t.
 */
class IMotor{
public:
    /**
     * @brief Move toward a controller position target.
     *
     * @param targetStep Target position in controller step units.
     * @param isCalibrating Allows implementation-specific calibration movement;
     *        defaults to false for normal bounded movement.
     * @return ESP_OK on success, or an implementation-specific error.
     */
    virtual esp_err_t move(int32_t targetStep, bool isCalibrating = false) = 0;

    /**
     * @brief Move toward the currently known maximum position.
     *
     * @return ESP_OK on success, or an implementation-specific error.
     */
    virtual esp_err_t moveToMax() = 0;

    /**
     * @brief Stop motion and refresh any tracked position state.
     *
     * @return ESP_OK on success, or an implementation-specific error.
     */
    virtual esp_err_t stop() = 0;

    /**
     * @brief Mark the current position relative to the home reference.
     *
     * @param offset Position offset applied by the implementation; defaults to 0
     *        to use the current position as the home reference.
     */
    virtual void setHoming(int32_t offset = 0) = 0;

    /**
     * @brief Set the known maximum position from the current position.
     *
     * @param offset Position offset applied by the implementation; defaults to 0
     *        to use the current position as the maximum reference.
     */
    virtual void setMaxStep(int32_t offset = 0) = 0;

    /**
     * @brief Get the current tracked position.
     *
     * @return Current position in controller step units.
     */
    virtual int32_t getCurrentStep() const = 0;

    /**
     * @brief Get the currently known maximum position.
     *
     * @return Maximum position in controller step units.
     */
    virtual int32_t getMaxStep() const = 0;

    /**
     * @brief Default virtual destructor for interface cleanup.
     */
    virtual ~IMotor() = default;
};
