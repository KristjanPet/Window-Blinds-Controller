# Firmware Library Architecture

This folder contains the main firmware modules used by the ESP32-S3 window blinds controller.

The library code is split by responsibility: high-level blind logic, low-level motor control, motor-driver communication, user input, connectivity, and shared application types/configuration.

## Runtime flow

```text
Wall buttons / MQTT / Home sensor / TMC2209 DIAG
        |
        v
BlindsCommandQueue
        |
        v
BlindsController
        |
        v
MotorController
        |
        v
STEP/DIR/EN + TMC2209 + stepper motor
```

## Modules

| Module               | Responsibility                                                                                               |
| -------------------- | ------------------------------------------------------------------------------------------------------------ |
| `BlindsController`   | High-level state machine, calibration, movement policy, stall recovery, and fault-state handling             |
| `MotorController`    | Low-level STEP/DIR/EN control, RMT pulse generation, PCNT position tracking, and motor stop/failure handling |
| `Tmc2209Driver`      | UART register access, TMC2209 configuration, DIAG interrupt handling, and stall-event forwarding             |
| `BlindsCommandQueue` | Central FreeRTOS queue used to normalize button, MQTT, sensor, motor, and driver events                      |
| `ButtonHandler`      | Local wall-button input handling and command generation                                                      |
| `HomeSensor`         | Home/reference sensor input used during calibration                                                          |
| `Connection`         | Wi-Fi and MQTT communication for remote commands and fault/status publishing                                 |
| `Common`             | Shared configuration, application types, and fault handling utilities                                        |

## Design goals

* Keep high-level blind behavior separate from low-level hardware control.
* Route all external and hardware events through one command queue.
* Make the controller logic easier to unit test by depending on interfaces where possible.
* Keep hardware-specific values in shared configuration instead of spreading them through the code.
* Record and report faults explicitly instead of silently ignoring failed operations.

## Documentation note

This README gives only the overall library structure.

More detailed explanations, such as motor-control behavior, TMC2209 register usage, MQTT payloads, calibration logic, and fault handling, should be documented in the README files inside the individual module folders.
