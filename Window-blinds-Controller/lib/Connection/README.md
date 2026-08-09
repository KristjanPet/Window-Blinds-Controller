# MQTT communication

## Topics

| Topic | Direction | Purpose |
|---|---|---|
| `window-blinds/command` | Subscribe | Remote movement command |
| `window-blinds/status` | Publish | Fault/status report |

## Accepted command payloads

| Payload | Meaning |
|---|---|
| `up` | Move up/open |
| `down` | Move down/closed |
| `stop` | Stop movement |
| `0` - `100` | Move to percentage position |