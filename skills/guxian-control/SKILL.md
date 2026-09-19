---
name: guxian-control
version: "1.0.0"
description: >
  Control and query the Guxian (古贤) business system from the
  VelaGesture device.  Provides status queries and action execution
  for digital-twin / industrial-control / water-conservancy scenarios.
author: biepengwopingmu
license: Apache-2.0
capabilities:
  - query_status
  - execute_action
---

# guxian-control

## Overview

`guxian-control` is a custom agent skill for the VelaGesture
platform.  It bridges the openvela AI agent with the Guxian (古贤)
business system — a digital-twin and industrial-control platform
used in water conservancy monitoring.

The skill is invoked by the AI agent when:
- The user makes an **OK gesture** to confirm a pending alert.
- The user makes an **open palm** gesture to query system status.
- The user issues a **voice command** related to Guxian operations.

## Capabilities

### query_status

Query the current status of the Guxian business system.

**Parameters:**

| Field  | Type   | Required | Description                       |
|--------|--------|----------|-----------------------------------|
| target | string | yes      | What to query: "system", "device", "scene", or a specific device ID |

**Example:**

```json
{
  "target": "system"
}
```

**Response:**

```json
{
  "status": "ok",
  "devices_online": 42,
  "active_alerts": 1,
  "current_scene": "spillway_monitoring"
}
```

### execute_action

Execute an action on the Guxian business system.

**Parameters:**

| Field     | Type   | Required | Description                              |
|-----------|--------|----------|------------------------------------------|
| action    | string | yes      | Action type: "confirm_alert", "locate_device", "switch_scene", "acknowledge" |
| alert_id  | string | no       | Alert ID (required for alert-related actions) |
| device_id | string | no       | Target device ID (for device actions)    |
| scene_id  | string | no       | Scene ID (for scene switching)           |

**Example — Confirm an alert:**

```json
{
  "action": "confirm_alert",
  "alert_id": "alert-20260919-001"
}
```

**Example — Locate a device:**

```json
{
  "action": "locate_device",
  "device_id": "gate-valve-07"
}
```

**Response:**

```json
{
  "status": "executed",
  "action": "confirm_alert",
  "result": "alert_acknowledged",
  "timestamp": "2026-09-19T10:30:00Z"
}
```

## Installation

Copy this directory to the device:

```
/data/agent/skills/guxian-control/SKILL.md
```

The VelaGesture skill executor automatically discovers skills in
`/data/agent/skills/` at startup.

## Active + Execute Scenario

The primary use case for this skill is the "active + execute"
loop:

1. **Guxian server detects an anomaly** (e.g. water level
   threshold exceeded).
2. **WebSocket push** delivers an alert to the P4 device.
3. **AI agent** receives the alert and prompts the user
   (display + optional audio).
4. **User confirms** with an OK gesture.
5. **Agent invokes** `guxian-control/execute_action` with the
   alert details.
6. **Guxian system** executes the action (e.g. acknowledges the
   alarm, triggers a corrective measure).
7. **Result** is returned to the device and displayed.

This cycle demonstrates the full "perception → decision →
execution → feedback" loop required by the contest.
