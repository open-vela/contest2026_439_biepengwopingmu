# VelaGesture Application

Main application for the VelaGesture multimodal gesture interaction
terminal.

Builds as the NuttX application `velagesture` which can be launched
from the NSH prompt:

```
nsh> velagesture
```

## Subsystems

- **Gesture Perception** — camera capture and gesture inference
- **Stable Gate** — filters transient detections, emits stable events
- **AI Agent** — event-driven decision engine
- **WebSocket Client** — bidirectional communication with Guxian
- **Skill Executor** — dispatches guxian-control skill calls
- **Audio I/O** — half-duplex microphone/speaker (optional)

## Configuration

Enable via `menuconfig`:

```
Application Configuration → Demos → VelaGesture multimodal gesture interaction terminal
```

Key options:
- `CONFIG_VELAGESTURE` — master enable
- `CONFIG_VELAGESTURE_WS_HOST` — server address
- `CONFIG_VELAGESTURE_WS_PORT` — server port
- `CONFIG_VELAGESTURE_AUDIO` — enable audio subsystem
- `CONFIG_VELAGESTURE_USE_ESPDL` — use ESP-DL inference backend
- `CONFIG_VELAGESTURE_USE_TFLITE` — use TF-Lite Micro backend
