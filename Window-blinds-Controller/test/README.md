## Testing

The firmware includes PlatformIO/Unity unit tests for:

- blinds state machine behavior
- button command handling
- stop and limit handling
- stall recovery logic
- calibration failure paths
- percentage-position movement
- MQTT command parsing
- JSON fault-status payload generation
- command queue and fault handling

Run tests:

```bash
pio test -e esp32-s3
