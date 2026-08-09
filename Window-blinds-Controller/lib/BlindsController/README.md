# BlindsController

This module contains the high-level control logic for the window blinds.

## Purpose

`BlindsController` is responsible for:

* processing button, MQTT, sensor, motor, and stall events
* controlling the blinds state machine
* running the homing and calibration sequence
* converting percentage commands into target step positions
* handling normal movement commands
* managing stall recovery
* entering the fault state when recovery is not possible

Low-level motor movement is delegated to `IMotor`, which is implemented by `MotorController`.

## Controller states

| State              | Description                                        |
| ------------------ | -------------------------------------------------- |
| `IDLE`             | No active movement or calibration                  |
| `CALIBRATING_HOME` | Moving toward the home/reference sensor            |
| `CALIBRATING_MAX`  | Finding the maximum travel position after homing   |
| `MOVING_UP`        | Moving toward the open/maximum position            |
| `MOVING_DOWN`      | Moving toward the home/minimum position            |
| `STALL_RECOVERY`   | Backing away after a stall before retrying         |
| `FAULT`            | Fault state; normal movement commands are rejected |

## Calibration sequence

Calibration is used to find the valid travel range of the blinds.

1. The controller moves the blinds toward the home/reference sensor.
2. When the home sensor is reached, the current position is stored as the minimum reference position.
3. The controller then moves the blinds in the opposite direction to find the maximum travel position.
4. When the maximum end is detected, the controller stores the maximum step position with a safety offset.
5. After calibration, normal movement commands can use soft limits and percentage-based target positions.

## Stall recovery

During normal movement, a stall event can be reported by the TMC2209 driver through the DIAG/StallGuard signal.

When this happens, the controller:

1. stops normal movement,
2. backs away from the detected stall position,
3. retries movement toward the original target,
4. counts the number of recovery attempts.

If the number of recovery attempts exceeds the configured limit, the controller enters the fault state.

## Fault state

The controller enters `FAULT` when it can no longer safely continue normal operation.

Typical reasons include:

* calibration failure
* repeated stall recovery failure
* invalid recovery target
* motor movement failure
* motor stop failure
* command queue failure
* home sensor validation failure

In the fault state, normal movement commands are rejected so the system does not continue operating from an unknown or unsafe state.

## Design note

`BlindsController` owns the movement policy, not the hardware.

It decides what should happen next, while `MotorController`, `HomeSensor`, `Tmc2209Driver`, and `MqttClient` handle the hardware or communication details.

This separation makes the controller logic easier to test with fake motor implementations and simulated command events.
