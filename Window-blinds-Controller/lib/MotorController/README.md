# MotorController

This module controls the stepper motor hardware used to move the window blinds.

Stepper motor used for this project: [Nema 17 17HS4401](https://www.alldatasheet.com/datasheet-pdf/pdf/1245671/NINGBO/17HS4401.html)

## Purpose

`MotorController` is the low-level motor-control layer. It is responsible for:

* STEP/DIR/EN GPIO control
* step pulse generation using the ESP32 RMT peripheral
* position tracking using the ESP32 PCNT peripheral
* starting, stopping, and aborting motor movement
* reporting movement completion and motor-control faults to `BlindsCommandQueue`

High-level movement decisions, calibration logic, stall recovery, and fault-state behavior are handled by `BlindsController`.

## Used signals

| Signal       | ESP32-S3 pin | Purpose                                 |
| ------------ | -----------: | --------------------------------------- |
| `STEP`       |     `GPIO15` | Step pulse output to the TMC2209 driver |
| `DIR`        |      `GPIO7` | Motor direction control                 |
| `EN`         |      `GPIO8` | Motor driver enable control             |

## Design notes

### RMT step generation

Stepper pulses are generated with the ESP32 RMT peripheral instead of software delay loops.

This gives more stable pulse timing and reduces CPU load while the motor is moving.

### PCNT position tracking

The ESP32 PCNT peripheral is used to count generated step pulses and track the current blind position.

`MotorController` keeps the current position and known maximum position, but the meaning of those positions is defined by the calibration process in `BlindsController`.

### Movement model

The controller supports:

* moving to a target step position
* moving to the known maximum position
* stopping movement
* setting the current position as the home reference
* setting the current position as the maximum travel limit

Soft-limit checks are used during normal movement. Calibration movement is allowed to move outside normal limits when finding the home/reference point.

## Fault handling

Motor-control errors are reported through `FaultHandler` and movement events are sent to `BlindsCommandQueue`.

This keeps the low-level motor driver separate from the high-level recovery policy. For example, `MotorController` can report that movement failed, while `BlindsController` decides whether to retry, recover from a stall, or enter the fault state.
