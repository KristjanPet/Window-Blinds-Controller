# AGENTS.md

Guidance for Codex and other AI coding agents working on this repository.

This file is part of the repository on purpose. It defines how AI agents should work on this embedded firmware project: small diffs, clear ownership, hardware-aware reasoning, tests where possible, and no speculative rewrites.

---

## Project Context

This is a real embedded firmware project for an automatic window blinds controller.

- Platform: ESP32 DevKitC / ESP32-WROOM
- Framework: ESP-IDF via PlatformIO
- Language: C++ with ESP-IDF APIs
- RTOS: FreeRTOS
- Real-time constraints: yes
- ISR usage: yes
- Motor control is timing-sensitive
- Stepper driver: TMC2209
- Motor interface: STEP / DIR / EN, with UART configuration planned
- Inputs: physical buttons, later home/reference sensor
- Future features: homing/recalibration, scheduling, MQTT/remote control, persistent config
- Memory constraints: moderate; avoid heap use in critical paths
- C++ policy: exceptions disabled, RTTI disabled

Treat this as production-style embedded firmware, but avoid unnecessary enterprise overengineering.

---

## Default Agent Workflow

For any non-trivial change:

1. Inspect the relevant files first.
2. Explain the current design briefly.
3. Identify the smallest safe change.
4. Propose a short plan before coding.
5. Implement only the approved or clearly requested scope.
6. Keep the diff small and reviewable.
7. Run or suggest relevant build/tests.
8. Summarize:
   - what changed
   - why it changed
   - risks
   - verification steps

If a change touches motor timing, ISR code, queues, state machines, hardware drivers, or safety behavior, be extra conservative.

If hardware behavior is uncertain, do not guess. State the uncertainty clearly and ask for measurement, schematic, datasheet excerpt, or bench verification.

---

## Build and Test Commands

Use PlatformIO commands where applicable.

- Build: `pio run -e esp32doit-devkit-v1`
- Run tests: `pio test -e esp32doit-devkit-v1`
- Clean build if needed: `pio run -t clean`
- Generate compilation database if needed: `pio run -e esp32doit-devkit-v1 -t compiledb`

If the environment name changes in `platformio.ini`, use the actual environment name from that file.

Do not edit generated build output such as `.pio/`.

If commands fail because tools or hardware are missing, report that clearly and continue with static review where possible.

---

## Architecture Principles

Keep responsibilities separated.

### `BlindsController`

- Owns the blinds state machine.
- Owns command/event decisions.
- Decides what should happen.
- Should stay testable without hardware.
- Should not directly access low-level GPIO, timer, or driver internals.

### `MotorController`

- Owns low-level motor hardware control.
- Owns STEP / DIR / EN and timer interaction.
- Should not own high-level blinds policy.
- Reports hardware events upward instead of changing blinds state directly.
- Must preserve timing-sensitive behavior unless explicitly changed.

### `ButtonHandler`

- Reads and debounces physical buttons.
- Emits events or commands.
- Should not decide high-level motor behavior.
- Should not bypass the central command/event path.

### Event / Queue Layer

- Routes events between modules.
- Keeps ownership clear.
- Avoid exposing queue internals directly unless explicitly designed.
- Avoid hidden cross-module control.

### Tests / Fakes

- Pure logic should be testable with fake interfaces.
- Hardware should be isolated behind small interfaces when practical.
- Fakes should model only the behavior needed for the test.

---

## Current Design Direction

The intended architecture is event-driven:

1. Inputs produce events.
2. `BlindsController` consumes events.
3. `BlindsController` decides state transitions.
4. `MotorController` performs low-level motor actions.
5. Hardware events, such as a soft limit being reached, are reported back as events.
6. Only the motor layer touches timing-sensitive motor pins.

Avoid designs where multiple modules can independently control the motor.

Manual buttons, future MQTT commands, schedules, and homing events should all feed the same command/event boundary instead of bypassing safety and state logic.

---

## Hard Rules

Do not violate these without explicit approval:

- Do not introduce dynamic allocation in ISR paths.
- Do not block inside ISR callbacks.
- Do not log inside ISR callbacks unless explicitly approved and proven ISR-safe.
- Do not call non-ISR-safe FreeRTOS APIs from ISR context.
- Do not change motor timing behavior unless explicitly asked.
- Do not introduce hidden motor-control ownership.
- Do not let button code directly decide high-level motion policy.
- Do not let `MotorController` directly modify blinds state.
- Do not ignore `esp_err_t` return values in control paths.
- Do not silently swallow hardware/control failures.
- Do not use exceptions.
- Do not use RTTI.
- Do not introduce large refactors without first proposing a short plan.
- Do not add speculative features beyond the current task.
- Do not modify unrelated files just because they are nearby.

---

## Embedded Safety Rules

When changing motor, ISR, timer, queue, or state-machine code:

- Explain hardware assumptions.
- Explain timing assumptions.
- Explain what can fail.
- Preserve safe stop behavior.
- Prefer fail-safe behavior over convenience.
- Keep ISR work minimal.
- Avoid heap allocation in control paths.
- Avoid unbounded queues, buffers, or retries.
- Avoid long blocking waits in tasks unless intentionally documented.
- Do not assume sensor behavior without measurement.
- Do not treat software soft limits as physical safety limits.
- Do not assume network/server availability for safe local operation.

Software soft limits are temporary protection, not a replacement for physical limit switches, mechanical stops, current limiting, or proper fault handling.

---

## C++ Style Rules

Prefer simple embedded C++.

- Use clear ownership.
- Prefer references when dependency is mandatory.
- Prefer small interfaces only when they improve testability, hardware isolation, or ownership clarity.
- Avoid unnecessary inheritance.
- Avoid clever templates unless there is a clear benefit.
- Prefer explicit state machines over hidden behavior.
- Keep headers lightweight.
- Include only what is needed.
- Avoid global mutable state where possible.
- Keep names precise and boring.
- Use `enum class` for states/events.
- Use fixed-width integer types where appropriate.
- Avoid premature micro-optimizations.
- Prefer readable code over clever code.

Do not introduce interfaces, inheritance, templates, or design patterns just because they are common in desktop C++.

Only add abstraction when it directly improves:

- testability
- hardware isolation
- ownership clarity
- safety
- maintainability of an already-growing module

---

## Error Handling Rules

- Use `esp_err_t` at ESP-IDF-facing driver/control boundaries.
- For pure logic, prefer small explicit result/status enums if they improve testability.
- Return and propagate errors in driver/control layers.
- Do not use `ESP_ERROR_CHECK` inside reusable driver/controller logic unless failure should intentionally abort the whole app.
- Prefer returning errors to the caller.
- Higher-level logic decides whether to retry, stop, enter fault, or log.
- Normal state transitions should happen only after the underlying action succeeds.
- If an underlying action fails, propagate the error and consider entering an explicit fault state.
- Use specific errors when possible, for example invalid state vs generic failure.
- Do not clear fault state just because `STOP` succeeds.
- Do not hide failures behind boolean success/failure if useful error information exists.

---

## ISR and FreeRTOS Rules

For ISR callbacks:

- Do the minimum work.
- Toggle pins only if required.
- Update very small state only if safe.
- Use ISR-safe notification/queue APIs only.
- Defer logging, stopping timers, state changes, and recovery decisions to tasks.
- Do not allocate memory.
- Do not perform long computations.
- Do not call APIs that may block.

For tasks:

- Keep responsibilities narrow.
- Avoid task-to-task coupling through private internals.
- Prefer events over direct cross-module control.
- Avoid blocking behavior unless documented as intentional proof-of-concept behavior.
- Avoid unbounded waits when motor/safety behavior depends on responsiveness.

---

## Motor-Control Rules

Motor control is timing-sensitive.

- Do not change timer frequency, stepping edge behavior, or STEP toggling logic casually.
- Count real motor steps only on the selected STEP edge, not on every toggle.
- Direction-specific limits must allow movement away from the limit.
- Reaching a soft limit should become an event handled by higher-level logic.
- Soft limits are temporary protection, not physical safety.
- Future homing sensor integration must define position trust clearly.
- Motor stop behavior must remain safe even during errors.
- Never let remote, scheduled, or button commands bypass motor safety/state checks.

Any change to motor timing must explain:

- previous behavior
- new behavior
- expected step frequency
- edge-counting assumptions
- verification method, preferably with logic analyzer or oscilloscope

---

## Datasheet and External Documentation Rules

For hardware-register work:

- Do not invent register addresses, bit fields, timings, or electrical limits.
- Use existing project constants or documented datasheet excerpts.
- If datasheet information is missing from the repo, ask for the relevant excerpt or clearly state the uncertainty.
- When changing TMC2209 UART configuration, explain which register/bit field is affected and why.
- Do not rely on memory for register-level code.
- Prefer adding a short source note in documentation when a hardware behavior depends on a datasheet section.

For ESP-IDF behavior:

- Prefer ESP-IDF documentation or existing project patterns.
- Be careful with ISR safety, IRAM requirements, timer behavior, UART behavior, and FreeRTOS API variants.
- If unsure whether an API is ISR-safe or blocking, say so and verify before using it.

---

## Testing Rules

Always suggest tests when changing logic.

For pure logic:

- Use Unity tests.
- Use fake interfaces such as `FakeMotor`.
- Test state transitions.
- Test error propagation.
- Test invalid states.
- Test fault behavior.
- Test command/event arbitration.
- Test boundary cases, not only happy paths.

For hardware drivers:

- Prefer bench verification steps.
- Do not pretend hardware behavior is unit-testable unless abstracted.
- Suggest logic analyzer or oscilloscope checks for timing-sensitive changes.
- Suggest serial/log verification only when it does not interfere with timing-sensitive behavior.

When modifying `BlindsController`, update or suggest unit tests.

When modifying `MotorController`, suggest both:

- minimal unit-testable logic checks if applicable
- on-target bench verification

When modifying TMC2209 UART code, suggest tests or verification for:

- frame generation
- CRC/datagram correctness
- timeout behavior
- invalid reply handling
- register write confirmation where possible

---

## Development Workflow

Use small diffs.

Before making changes:

1. Inspect the relevant files.
2. Identify the smallest safe change.
3. Explain the reason briefly.
4. Make only the requested or necessary change.
5. Suggest tests or verification.

Avoid:

- large rewrites
- broad architectural changes without approval
- changing unrelated files
- speculative features
- cleaning up everything at once
- mixing formatting-only changes with logic changes
- mixing feature work with refactoring unless explicitly approved

If a refactor is needed, first propose:

- what problem it solves
- which files are affected
- risk level
- expected behavior after refactor
- test plan

Wait for approval before major refactors.

---

## Review Style Expected

When reviewing code, provide:

1. Short overall assessment.
2. Compact list of issues found.
3. Only the top 2 or 3 fixes to do next.

For each fix:

- say what is wrong
- why it matters in embedded/C++
- what better direction looks like
- suggest verification/tests

Prioritize issues in this order:

1. safety
2. timing
3. ownership
4. error handling
5. state-machine correctness
6. testability
7. readability
8. style

Do not dump long lists of minor style issues before safety, timing, ownership, or error-handling issues.

---

## Hardware Assumptions

Do not assume hardware behavior without measurement.

Known project hardware direction:

- ESP32 DevKitC / ESP32-WROOM
- TMC2209 stepper driver
- NEMA17-style stepper motor
- 12 V motor supply
- buck converter for logic supply
- physical buttons
- NJK-style magnetic proximity sensor planned for homing/reference
- logic conditioning may include Schmitt trigger / divider / interface stage

Always mention when a change depends on:

- motor current setting
- sensor polarity
- sensor voltage level
- stepper driver mode
- UART wiring
- timer frequency
- mechanical travel range
- power supply behavior
- ESP32 boot pin behavior
- pull-up / pull-down configuration
- active-high vs active-low logic

For detailed hardware assumptions, prefer a dedicated documentation file such as:

- `docs/hardware.md`
- `docs/pinout.md`
- `docs/tmc2209.md`

Do not invent electrical details not documented in the repo.

---

## Forbidden Shortcuts

Do not:

- add features before the current architecture supports them
- hide hardware uncertainty in code
- assume network/server availability for safe local operation
- use MQTT/scheduler logic inside motor driver code
- write flash continuously during motion
- introduce unbounded logging in repeated control paths
- make state transitions from multiple places
- allow private queues/state to be modified from unrelated modules
- add global flags as a shortcut around ownership problems
- bypass the command/event boundary for convenience
- treat a temporary proof-of-concept as production behavior without documenting it

---

## Preferred Direction for Future Features

### UART TMC2209 Configuration

- Keep it driver-layer only.
- Separate static configuration from runtime control.
- Keep register definitions explicit.
- Validate on bench.
- Do not mix UART diagnostics into high-level blinds policy.
- Do not invent register meanings.
- Prefer a small testable codec/protocol layer if UART framing grows.

### Homing Sensor

- Introduce explicit events/states.
- Define position trust.
- Define timeout/fault behavior.
- Define movement direction during homing.
- Define behavior if the sensor is already active at boot.
- Do not assume sensor repeatability until tested.

### Scheduling

- Keep scheduling logic pure and unit-testable.
- Do not mix time parsing into motor driver code.
- Define conflict rules with manual input.
- Define behavior for missed schedules.
- Define behavior when time source is unavailable or invalid.

### MQTT / Remote Control

- Feed the same command/event boundary as physical buttons.
- Define local/manual priority.
- Define offline behavior.
- Do not let remote commands bypass safety/state logic.
- Do not assume the network is trusted.
- Do not require cloud/server availability for safe local operation.

### Persistent Config

- Avoid writing flash repeatedly during motion or frequent control loops.
- Validate config before applying it.
- Define defaults.
- Define recovery behavior for corrupted config.
- Keep config application separate from motor stepping logic.

---

## Current Priority

Favor this order:

1. Clean architecture and ownership
2. Error handling
3. Unit-testable controller logic
4. Safe motor-control behavior
5. TMC2209 UART configuration
6. Homing/sensor integration
7. Persistent config
8. Scheduler
9. MQTT/remote control

Do not invent features beyond the current task.

---

## Good Agent Behavior

A good agent working on this project should:

- ask for clarification when hardware behavior is unknown
- prefer plans before risky code changes
- keep changes small
- preserve existing behavior unless asked otherwise
- explain embedded risks
- suggest verification steps
- make code more testable without overengineering it
- respect module ownership boundaries
- prioritize safety and correctness over speed

A bad agent working on this project would:

- rewrite architecture without approval
- hide motor-control decisions in unrelated modules
- add global state to bypass design problems
- invent datasheet details
- ignore `esp_err_t`
- add logging in timing-critical paths
- introduce heap use in ISR/control paths
- make remote/scheduled commands bypass local safety
- produce code that the maintainer cannot understand or verify