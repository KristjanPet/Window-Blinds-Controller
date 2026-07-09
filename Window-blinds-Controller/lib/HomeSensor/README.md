# HomeSensor

This module handles the home/reference sensor used during blinds calibration.

Sensor used for this project: [NJK-5002C Hall Effect Sensor Proximity Switch](https://www.handsontec.com/dataspecs/sensor/NJK-5002C-Hall%20Sensor.pdf)

## Purpose

`HomeSensor` is responsible for:

* configuring the home sensor GPIO input
* registering the GPIO interrupt handler
* detecting when the blinds reach the home/reference position
* sending homing events to `BlindsCommandQueue`
* validating the sensor state during startup
* reporting stuck-sensor or validation failures through `FaultHandler`

High-level calibration behavior is handled by `BlindsController`.

## Used signal

| Signal             | ESP32-S3 pin | Purpose                                                |
| ------------------ | -----------: | ------------------------------------------------------ |
| Home sensor output |     `GPIO14` | Detects the home/reference position during calibration |

## Behavior

When the sensor is triggered, `HomeSensor` sends a `HOMING_REACHED` event to the command queue.

During startup, the sensor state is checked before normal operation. If the sensor is already active at startup, a `HOMING_CHECK` event is generated so the controller can move away and verify that the sensor is not stuck.

## Design note

The module only detects and reports the sensor state. It does not decide how calibration should continue. That decision stays in `BlindsController`.
