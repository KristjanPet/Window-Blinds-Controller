# Firmware Project Structure

This folder contains the ESP32-S3 PlatformIO firmware project for the window blinds controller.

```text
Window-blinds-Controller/
|-- README.md
|-- platformio.ini
|-- CMakeLists.txt
|-- sdkconfig.esp32-s3
|-- src/
|   |-- CMakeLists.txt
|   `-- main.cpp
|-- lib/
|   |-- BlindsController/
|   |-- MotorController/
|   |-- Tmc2209Driver/
|   |-- ButtonHandler/
|   |-- HomeSensor/
|   |-- Connection/
|   `-- Common/
|-- test/
|   |-- test_blinds_controller/
|   |-- test_blinds_command_queue/
|   |-- test_motor_controller/
|   |-- test_mqtt_client/
|   |-- test_home_sensor/
|   |-- test_fault_handler/
|   `-- fakes/
`-- logs/
    `-- TMC2209_current_readings.log
```

- `platformio.ini` defines the ESP32-S3 board, ESP-IDF framework, upload settings, and build flags.
- `src/main.cpp` initializes the app, hardware modules, FreeRTOS tasks, Wi-Fi, MQTT, and startup calibration.
- `lib/BlindsController/` contains the high-level blinds state machine and command queue.
- `lib/MotorController/` handles stepper movement, RMT pulse generation, PCNT position tracking, and the motor abstraction used by tests.
- `lib/Tmc2209Driver/` configures the TMC2209 stepper driver over UART and forwards DIAG/stall events.
- `lib/ButtonHandler/` and `lib/HomeSensor/` handle local GPIO inputs for manual control and calibration.
- `lib/Connection/` contains Wi-Fi and MQTT support, including the example secrets header.
- `lib/Common/` contains shared configuration, types, and centralized fault handling.
- `test/` contains PlatformIO unit tests and fakes for controller behavior, command handling, MQTT parsing, motor logic, sensor logic, and fault reporting.
- `logs/` stores captured hardware/debug readings used during development.
