# VelaGesture — OpenVela Integration

## What is OpenVela

OpenVela is an open-source IoT operating system based on Apache
NuttX, developed and maintained by the openvela community.  It
provides a POSIX-compliant real-time operating system with
extensive hardware support, networking, graphics, multimedia, and
AI capabilities.

## How VelaGesture Uses OpenVela

### 1. RTOS Foundation

VelaGesture runs as a NuttX application on the openvela platform.
It uses:
- **POSIX threads** (`pthread`) for concurrent subsystem execution
- **POSIX I/O** (`open/read/write/close`) for device access
- **POSIX signals** (`SIGINT/SIGTERM`) for clean shutdown
- **POSIX clocks** (`clock_gettime(CLOCK_MONOTONIC)`) for
  timestamps

### 2. NuttX Application Framework

The application is registered via the NuttX application framework:
- `Kconfig` defines build configuration options
- `Makefile` / `CMakeLists.txt` describe the build
- `Make.defs` registers the application with the build system
- The `main()` function is called when the user runs `velagesture`
  at the NSH prompt

### 3. Device Drivers

VelaGesture uses the following NuttX/openvela device drivers:
- **Camera** (`/dev/video0`) — MIPI-CSI camera driver for
  frame capture
- **Audio** (`/dev/audio/pcm_in0`, `/dev/audio/pcm_out0`) —
  I2S/codec driver for microphone and speaker
- **Display** — MIPI-DSI framebuffer for LVGL-based UI

### 4. Networking

The WebSocket client uses the `libwebsockets` library from
`apps/netutils/libwebsockets`, which is part of the openvela
application ecosystem.

### 5. AI Agent Framework

The `packages/ai_agent` project in the openvela workspace provides
the AI agent framework.  VelaGesture's agent module interfaces
with this framework for:
- Event-driven agent architecture
- Skill-based action dispatch
- LLM backend configuration (server-side)

### 6. Build System Integration

The application is built using openvela's unified build system:

```
./build.sh <board-config> [-j8]
```

The team repository's manifest (`contest2026_439_biepengwopingmu.xml`)
uses `<linkfile>` entries to symlink the application code into the
correct location within the openvela build tree:

```
app/velagesture → packages/demos/contest2026_439_velagesture
```

This means the team's code stays in their own repository while
being seamlessly integrated into the openvela build.

### 7. Board Support

The ESP32-P4 Function-EV-Board is supported through the vendor
BSP at `vendor/espressif/`.  The board configuration for the
contest is at:

```
vendor/espressif/boards/esp32p4/ev/configs/vela/
```

VelaGesture does not modify the BSP — it uses the board
configuration as-is and accesses hardware through standard
NuttX device interfaces.

## Why OpenVela

1. **POSIX compliance** — standard APIs, portable code
2. **ESP32-P4 support** — existing BSP for the target hardware
3. **Rich ecosystem** — networking, graphics, multimedia libraries
4. **AI agent framework** — built-in support for agent-based
   applications
5. **Community** — active development and contest support
