# VelaGesture — WebSocket Protocol

## Overview

All communication between the VelaGesture P4 device and the Guxian
business system uses a single WebSocket connection with JSON-encoded
messages.

**Connection:** `wss://<host>:<port>/ws/velagesture`

## Message Format

Every message is a JSON object with a `type` field that determines
the message semantics.

### Device → Server (P4 → Guxian)

#### gesture

Sent when a stable gesture event is detected.

```json
{
  "type": "gesture",
  "gesture": "ok",
  "confidence": 0.96,
  "stable": true,
  "timestamp": 1726734600000
}
```

| Field      | Type    | Description                              |
|------------|---------|------------------------------------------|
| gesture    | string  | Gesture name: "ok", "fist", "open_palm", "victory", "pinch" |
| confidence | float   | Recognition confidence (0.0 – 1.0)       |
| stable     | boolean | Always true for stable gate events       |
| timestamp  | integer | Monotonic timestamp in milliseconds      |

#### task

Sent when the agent requests a skill execution on the server.

```json
{
  "type": "task",
  "task_id": 1,
  "skill": "guxian-control",
  "action": "execute_action",
  "payload": {
    "action": "confirm_alert",
    "alert_id": "alert-20260919-001"
  }
}
```

| Field    | Type    | Description                        |
|----------|---------|------------------------------------|
| task_id  | integer | Unique task identifier             |
| skill    | string  | Skill name                         |
| action   | string  | Skill action                       |
| payload  | object  | Action-specific parameters         |

#### status

Sent periodically or in response to queries.

```json
{
  "type": "status",
  "device_id": "p4-gesture-001",
  "uptime_s": 3600,
  "last_gesture": "open_palm",
  "task_count": 5
}
```

#### error

Sent when the device encounters an error.

```json
{
  "type": "error",
  "code": -1,
  "message": "Camera initialization failed"
}
```

### Server → Device (Guxian → P4)

#### alert

Sent when the Guxian system detects an anomaly.

```json
{
  "type": "alert",
  "alert_id": "alert-20260919-001",
  "message": "Water level exceeds threshold at Gate 7",
  "severity": 2,
  "timestamp": "2026-09-19T10:30:00Z",
  "context": {
    "location": "spillway-gate-7",
    "value": 85.3,
    "threshold": 80.0
  }
}
```

| Field     | Type    | Description                              |
|-----------|---------|------------------------------------------|
| alert_id  | string  | Unique alert identifier                  |
| message   | string  | Human-readable alert description         |
| severity  | integer | 1=info, 2=warning, 3=critical            |
| timestamp | string  | ISO 8601 timestamp                       |
| context   | object  | Optional additional context              |

#### task_result

Sent in response to a task request.

```json
{
  "type": "task_result",
  "task_id": 1,
  "status": "success",
  "result": {
    "action": "confirm_alert",
    "acknowledged": true,
    "timestamp": "2026-09-19T10:30:05Z"
  }
}
```

| Field    | Type    | Description                              |
|----------|---------|------------------------------------------|
| task_id  | integer | Matches the request task_id              |
| status   | string  | "success" or "error"                     |
| result   | object  | Result data                              |

#### ack

Acknowledgement for received messages.

```json
{
  "type": "ack",
  "ref_type": "alert",
  "ref_id": "alert-20260919-001",
  "status": "received"
}
```

## Audio Data

Audio frames (PCM) are sent as binary WebSocket frames:

```
[Binary frame]
  Header (4 bytes):
    - type: uint8  (0x01 = audio_pcm)
    - flags: uint8 (0x00)
    - length: uint16 (little-endian)
  Payload:
    - PCM samples (16-bit, little-endian, 16 kHz mono)
```

## Connection Lifecycle

1. Device opens WSS connection to server.
2. Server accepts and sends initial `ack`.
3. Device sends periodic `status` messages (every 30s).
4. Device sends `gesture` events as they occur.
5. Server sends `alert` messages when anomalies are detected.
6. Device sends `task` messages to execute actions.
7. Server responds with `task_result`.
8. If connection drops, device reconnects with exponential
   backoff (3s, 6s, 12s, ... max 60s).
