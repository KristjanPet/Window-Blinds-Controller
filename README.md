# Window-Blinds-Controller

## About
A compact embedded project for controlling window blinds with an **ESP32** and a **stepper motor**.

It is designed as a clean, reliable demonstration of practical embedded development: hardware control, calibration, fault detection, and basic smart-home connectivity.

---

## Project Features
- 🎛️ Wall-button controls for opening and closing the blinds
- 🧭 Automatic homing/reference calibration with a dedicated home sensor
- 🛞 Stepper motor control using ESP32 RMT pulse generation and PCNT position tracking
- ⚙️ TMC2209 UART driver setup with DIAG/stall event handling
- 🛡️ Stall detection, recovery logic, and centralized fault reporting
- 📡 Wi-Fi and MQTT support for remote up, down, stop, and percentage-position commands
- 🧱 OOP-based separation between motor control, sensors, buttons, connectivity, and blinds logic
- 🔁 FreeRTOS task-based architecture with a central command queue
- 🧪 PlatformIO unit tests for controller behavior, MQTT parsing, command queue, motor logic, and fault handling
- 📁 Clean PlatformIO/CMake project structure
- 📄 Doxygen-friendly API comments and project documentation setup

---

## hardware wiring scheme

![schematics](Images/schematics.png)

