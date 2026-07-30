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

| Signal             | ESP32-S3 pin | Idle level | Active level | Activation edge | Purpose                                                |
| ------------------ | -----------: | ---------: | -----------: | --------------: | ------------------------------------------------------ |
| Home sensor output |     `GPIO14` |       HIGH |          LOW |         Falling | Detects the home/reference position during calibration |

The sensor signal passes through two 74HC14 inverter gates before reaching the
ESP32-S3. The two inversions cancel, so the conditioned path preserves the
sensor active-low polarity. The final 74HC14 output actively drives GPIO14 at
3.3 V, so the firmware leaves the ESP32 internal pull resistors disabled.

## Behavior

When the sensor changes from HIGH to LOW, `HomeSensor` sends a
`HOMING_REACHED` event to the command queue. Returning HIGH does not generate a
homing event.

During startup, the sensor state is checked before normal operation. If GPIO14
is already LOW, a `HOMING_CHECK` event is generated so the controller can move
away and verify that the sensor is not stuck.

## Design note

The module only detects and reports the sensor state. It does not decide how calibration should continue. That decision stays in `BlindsController`.
