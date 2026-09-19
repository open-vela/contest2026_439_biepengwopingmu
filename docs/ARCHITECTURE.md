# VelaGesture — Architecture

## System Overview

VelaGesture is a multimodal intelligent gesture interaction terminal
built on the ESP32-P4 platform running openvela (NuttX-based RTOS).
It was migrated from an existing multi-platform gesture terminal
prototype to the embedded P4 hardware.

## Layer Diagram

```
+----------------------------------------------------------+
|                   Hardware Layer                          |
|  ESP32-P4 Function-EV-Board V1.6                         |
|  - MIPI-CSI Camera (OV2640/OV5640)                       |
|  - MIPI-DSI Display (1024x600)                           |
|  - I2S Audio Codec (Mic + Speaker)                       |
|  - 32MB PSRAM, Touch Panel                               |
+----------------------------------------------------------+
         |                    |                    |
         v                    v                    v
+------------------+  +----------------+  +------------------+
| Camera Driver    |  | Audio Driver   |  | Display / LVGL   |
| /dev/video0      |  | /dev/audio/*   |  | Framebuffer      |
+------------------+  +----------------+  +------------------+
         |                    |
         v                    v
+----------------------------------------------------------+
|               Application Layer (velagesture)             |
|                                                          |
|  +------------------+   +------------------+             |
|  | Gesture          |   | Audio I/O        |             |
|  | Perception       |   | (half-duplex)    |             |
|  | - Frame capture  |   | - PCM capture    |             |
|  | - Inference      |   | - PCM playback   |             |
|  +--------+---------+   +--------+---------+             |
|           |                       |                       |
|           v                       v                       |
|  +------------------+   +------------------+             |
|  | Stable Gate      |   | Speech Intent    |             |
|  | - Consecutive    |   | (server-side     |             |
|  |   frame filter   |   |  ASR pipeline)   |             |
|  +--------+---------+   +--------+---------+             |
|           |                       |                       |
|           v                       v                       |
|  +----------------------------------------------+        |
|  |              AI Agent                         |        |
|  |  - Event queue                               |        |
|  |  - Gesture → action mapping                  |        |
|  |  - Alert confirmation / dismissal            |        |
|  |  - Skill invocation                          |        |
|  +---------------------+------------------------+        |
|                        |                                  |
|                        v                                  |
|  +----------------------------------------------+        |
|  |         Skill Executor                        |        |
|  |  - guxian-control/query_status                |        |
|  |  - guxian-control/execute_action              |        |
|  +---------------------+------------------------+        |
|                        |                                  |
+----------------------------------------------------------+
                         |
                         v
+----------------------------------------------------------+
|              Communication Layer                          |
|                                                          |
|  +----------------------------------------------+        |
|  |          WebSocket Client                     |        |
|  |  P4 ↔ Guxian Business System                 |        |
|  |  - JSON protocol                             |        |
|  |  - Auto-reconnect                            |        |
|  |  - TLS                                       |        |
|  +----------------------------------------------+        |
|                                                          |
+----------------------------------------------------------+
                         |
                         v
+----------------------------------------------------------+
|           Guxian Business System (Server)                 |
|  - Digital twin visualisation                            |
|  - Water conservancy monitoring                          |
|  - Alert management                                      |
|  - Task execution                                        |
+----------------------------------------------------------+
```

## Data Flow — Gesture Event

1. Camera captures a frame (320x240 grayscale).
2. Inference backend classifies the hand gesture.
3. Stable gate filters transient detections.
4. Stable gesture event is emitted.
5. Event is sent to the AI agent queue.
6. Agent decides action (e.g. confirm alert, query status).
7. Agent invokes the appropriate skill.
8. Skill sends a WebSocket message to Guxian.
9. Guxian processes the request and returns a result.
10. Result is displayed on the P4 screen.

## Data Flow — Active Alert

1. Guxian detects an anomaly (e.g. water level critical).
2. Alert is pushed over WebSocket to the P4 device.
3. Agent receives the alert and stores it as pending.
4. Display shows "Alert: water level high — OK to confirm".
5. User raises hand in OK gesture.
6. Stable gate emits OK event.
7. Agent confirms the alert via guxian-control skill.
8. Guxian acknowledges and returns confirmation.
9. Display shows "Alert acknowledged".

## Module Responsibilities

| Module             | Responsibility                                |
|--------------------|-----------------------------------------------|
| gesture_perception | Camera capture + inference                    |
| stable_gate        | Consecutive-frame filtering                   |
| ai_agent           | Event dispatch + decision logic               |
| skill_executor     | Skill loading + invocation                    |
| ws_client          | WebSocket communication                       |
| audio_io           | PCM capture/playback (optional)               |
| main               | Lifecycle management + subsystem coordination |

## Design Decisions

1. **No 21-keypoint intermediate representation** — the P4
   inference path uses a direct frame-to-gesture CNN, avoiding
   the computational cost of keypoint detection on an embedded
   device.

2. **Stable gate is mandatory** — without it, gesture jitter
   would flood the agent and WebSocket with spurious events.

3. **Skills are file-based** — SKILL.md files are human-readable
   and can be updated without recompilation.

4. **Half-duplex audio** — full-duplex with AEC is not feasible
   on the P4 without dedicated DSP hardware.  The half-duplex
   approach is simpler and reliable.

5. **Server-side ASR** — speech recognition runs on the server,
   not on the device.  This keeps the P4 firmware small and
   allows using a high-quality ASR model.
