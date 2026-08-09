# Firmware Project Structure

This folder contains the ESP32-S3 PlatformIO firmware project for the window blinds controller.

## Firmware architecture

The firmware is split into small C++ modules with clear responsibilities:

| Module | Responsibility |
|---|---|
| `BlindsController` | High-level state machine, calibration, movement policy, stall recovery |
| `MotorController` | STEP/DIR/EN control, RMT pulse generation, PCNT position tracking |
| `Tmc2209Driver` | UART register access, driver configuration, DIAG/stall event forwarding |
| `BlindsCommandQueue` | Central FreeRTOS command/event queue |
| `ButtonHandler` | Local wall-button input |
| `HomeSensor` | Reference sensor and homing validation |
| `MqttClient` | Remote commands and fault status publishing |
| `FaultHandler` | Thread-safe first-fault recording |

## Key engineering decisions

### RMT for step pulse generation
Stepper pulses are generated using the ESP32 RMT peripheral instead of software delay loops, reducing timing jitter and CPU load.

### PCNT for position tracking
The ESP32 PCNT peripheral is used to track generated step pulses and maintain position information.

### Event-driven control
Buttons, MQTT commands, motor events, home sensor events, and stall events are normalized into a central FreeRTOS command queue.

### Fault-first design
The system records the first fault and enters a safe fault state instead of silently ignoring failed operations.

### Testable architecture
High-level logic depends on interfaces/fakes where possible, allowing controller behavior and fault paths to be tested without real hardware.