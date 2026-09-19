# VelaGesture — Demo Script

**Duration:** ≤ 5 minutes

## Scene Setup

- ESP32-P4 Function-EV-Board V1.6 on desk, powered via USB.
- Camera pointing at presenter's hand area.
- Display showing the Guxian business system dashboard (via
  WebSocket to a laptop/server running the Guxian frontend).
- Serial console visible on a second screen.

## Script

### 1. Boot and System Ready (0:00 – 0:30)

**[Show serial console]**

Power on the board.  Show the openvela boot log:

```
openvela on ESP32-P4 rev v3.2
NuttX 12.x.x
...
nsh>
```

**[Narrate]** "This is the ESP32-P4 Function-EV-Board running
openvela.  The board has a MIPI-CSI camera, a 1024x600 MIPI-DSI
display, and 32MB PSRAM."

Launch the application:

```
nsh> velagesture
```

Show the startup sequence completing successfully.

### 2. Camera and Gesture Detection (0:30 – 1:30)

**[Show the camera feed / display]**

Position hand in front of camera.

**[Show OK gesture]** — hold for ~0.5s.

Serial console shows:

```
PERCEP: Stable gesture: ok (0.96)
```

Display updates to show "Gesture: OK".

**[Show Fist gesture]** — hold for ~0.5s.

```
PERCEP: Stable gesture: fist (0.92)
```

**[Show Open Palm gesture]** — hold for ~0.5s.

```
PERCEP: Stable gesture: open_palm (0.89)
```

**[Narrate]** "The camera detects hand gestures in real-time.
The stable gate filters out momentary misdetections — a gesture
must be held for 5 consecutive frames before it's accepted."

### 3. WebSocket Communication (1:30 – 2:00)

**[Show both screens — serial console + Guxian dashboard]**

After the open palm gesture, the device sends a status query:

```
AGENT: Open palm → querying status
SKILL: Sending query_status via guxian-control
WS: Sent: {"type":"task","skill":"guxian-control","action":"query_status",...}
```

The Guxian dashboard shows the query was received and returns
a status response.

### 4. AI Agent + Skill (2:00 – 2:30)

**[Narrate]** "The gesture event goes through the AI agent, which
decides what action to take.  For an open palm, it queries the
Guxian system status.  The agent uses the guxian-control skill
to interact with the business system."

Show the skill execution:

```
AGENT: Status: {"status":"ok","devices_online":42,...}
```

### 5. Active Alert Scenario (2:30 – 4:00)

**[Trigger an alert on the Guxian server]**

The Guxian server pushes an alert over WebSocket:

```
WS: Received alert: "Water level exceeds threshold at Gate 7"
AGENT: Alert received [2]: Water level exceeds threshold at Gate 7
```

Display shows: "ALERT: Water level high — make OK gesture to confirm"

**[Show Fist gesture first]** — to demonstrate dismissal option:

```
AGENT: Fist gesture → dismissing alert
```

**[Trigger the alert again]**

This time, show OK gesture:

```
AGENT: OK gesture → confirming alert 'alert-20260919-001'
SKILL: Sending execute_action via guxian-control
WS: Sent: {"type":"task","skill":"guxian-control","action":"execute_action",...}
```

The Guxian dashboard shows the alert being acknowledged.

**[Show the result]**

```
AGENT: Skill result: {"status":"executed","result":"alert_acknowledged"}
```

Display shows: "Alert acknowledged — Gate 7"

**[Narrate]** "This is the complete active-execute cycle:
the server pushes an alert, the agent prompts the user, the user
confirms with a gesture, the skill executes the action, and the
result is displayed."

### 6. Voice Interaction (4:00 – 4:30)

**[If audio is enabled]**

Speak a command: "查看状态" (query status)

The audio is captured, sent to the server for ASR, and the
resulting intent triggers a status query:

```
AUDIO: Frame captured (320 samples)
AGENT: Speech intent '查看状态' (conf=0.91)
AGENT: Status: {"status":"ok",...}
```

### 7. Summary (4:30 – 5:00)

**[Narrate]** "VelaGesture runs on the ESP32-P4 with openvela.
It connects gesture recognition, AI agent decision-making, and
the Guxian business system through a unified WebSocket protocol.
The guxian-control skill provides query and execution capabilities.
The active-execute scenario demonstrates a real-world industrial
control workflow."

Show the clean shutdown:

```
^C
[STOP] Shutting down...
[STOP] Perception stopped
[STOP] Agent stopped
[STOP] WebSocket stopped
[STOP] Skill executor stopped
[STOP] VelaGesture terminated cleanly.
```

## Notes

- All timings are approximate.
- If the camera inference is not yet running a real model, skip
  the gesture detection section and show the protocol/agent flow
  using manual event injection via the serial console.
- Do not fabricate performance metrics (FPS, latency, accuracy)
  that have not been measured on real hardware.
