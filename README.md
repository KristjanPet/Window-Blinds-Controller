# Window-Blinds-Controller

## About
A compact embedded project for controlling window blinds with an **ESP32** and a **stepper motor**.

It is designed as a clean, reliable demonstration of practical embedded development: hardware control, calibration, fault detection, and basic smart-home connectivity.

---

## Project Features
- 🎛️ Wall-button controls for opening and closing the blinds
- 🧭 Automatic homing/reference calibration with a dedicated home sensor
- 〰️ Stepper motor control using ESP32 RMT pulse generation and PCNT position tracking
- ⚙️ TMC2209 UART driver setup with DIAG/stall event handling
- 🛡️ Stall detection, recovery logic, and centralized fault reporting
- 📡 Wi-Fi and MQTT support for remote up, down, stop, and percentage-position commands
- 🧱 OOP-based separation between motor control, sensors, buttons, connectivity, and blinds logic
- 🔁 FreeRTOS task-based architecture with a central command queue
- 🧪 PlatformIO unit tests for controller behavior, MQTT parsing, command queue, motor logic, and fault handling
- 📁 Clean PlatformIO/CMake project structure
- 📄 Doxygen-friendly API comments and project documentation setup

---

## Project Structure
```text
Window-Blinds-Controller/
|-- README.md
|-- LICENSE
|-- Doxyfile
|-- Images/
|   `-- schematics.png
`-- Window-blinds-Controller/
    |-- platformio.ini
    |-- CMakeLists.txt
    |-- sdkconfig.esp32-s3
    |-- src/
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
    |   |-- test_fault_handler/
    |   `-- fakes/
    `-- logs/
        `-- TMC2209_current_readings.log
```

- `src/main.cpp` initializes the ESP32 app, hardware modules, FreeRTOS tasks, Wi-Fi, MQTT, and startup calibration.
- `lib/` contains the embedded modules for blinds logic, motor control, driver communication, buttons, sensors, connectivity, shared types, and fault handling.
- `test/` contains PlatformIO unit tests and fakes for controller behavior, command handling, MQTT parsing, motor logic, and fault reporting.
- `Images/` stores the hardware wiring schematic used in the documentation.
- `Doxyfile` configures API documentation generation for the project.

---

## hardware wiring scheme

![schematics](Images/schematics.png)
