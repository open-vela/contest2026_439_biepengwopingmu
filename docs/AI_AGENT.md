# VelaGesture — AI Agent

## Overview

The AI agent is the central decision-making component of
VelaGesture.  It receives events from multiple sources (gesture,
WebSocket, audio), applies policy rules, and dispatches actions
via the skill executor.

## Architecture

```
  Gesture Events       WebSocket Alerts     Speech Intents
       |                     |                    |
       +----------+----------+----------+---------+
                  |
                  v
           +-------------+
           | Event Queue  |  (ring buffer, 32 slots)
           +------+------+
                  |
                  v
           +-------------+
           | AI Agent    |
           | Policy      |
           | Engine      |
           +------+------+
                  |
                  v
           +-------------+
           | Skill       |
           | Executor    |
           +------+------+
                  |
                  v
           +-------------+
           | WebSocket   |
           | Client      |
           +-------------+
```

## Event Types

| Event Type      | Source       | Description                     |
|-----------------|--------------|---------------------------------|
| GESTURE         | Perception   | Stable gesture detected         |
| ALERT           | WebSocket    | Server pushed an alert          |
| SPEECH_INTENT   | Audio/ASR    | Recognised speech command       |
| USER_CMD        | Serial/CLI   | Manual command from NSH         |

## Policy Rules

### Gesture → Action Mapping

| Gesture     | Condition           | Action                          |
|-------------|---------------------|---------------------------------|
| OK          | Pending alert exists | Confirm alert via skill         |
| OK          | No pending alert     | Ignore                          |
| Fist        | Pending alert exists | Dismiss alert                   |
| Fist        | No pending alert     | Ignore                          |
| Open Palm   | Always              | Query system status via skill   |
| Victory     | —                   | Reserved (no action)            |
| Pinch       | —                   | Reserved (no action)            |

### Alert Handling

When a WebSocket alert arrives:
1. The agent stores it as the "pending alert".
2. The agent sends an ACK back to the server.
3. The display shows the alert message.
4. The agent waits for user input (gesture or voice).

### Speech Intent Handling

| Intent Pattern   | Action                           |
|------------------|----------------------------------|
| "status" / "状态" | Query system status              |
| "confirm" / "确认"| Confirm pending alert            |

## LLM Backend

The AI agent can be configured to use an LLM backend for more
complex decision-making.  The LLM integration is server-side —
the device sends structured events to the server, and the server
returns structured actions.

In the current implementation, the policy engine is rule-based
(not LLM-driven) for reliability and latency on the embedded
device.  The LLM backend is used on the server side for:
- Natural language understanding of speech intents
- Complex multi-step task planning
- Context-aware alert prioritisation

## Configuration

The agent does not require runtime configuration.  Policy rules
are compiled into the firmware.  Server-side LLM configuration
is managed by the Guxian business system.

## Thread Model

The agent runs as a dedicated thread with its own event queue.
Events are posted to the queue by:
- `vg_agent_handle_gesture()` — from the perception thread
- `vg_agent_handle_alert()` — from the WebSocket callback
- `vg_agent_post_event()` — from any subsystem

The agent processes events sequentially (FIFO order).  This
simplifies the state machine and avoids race conditions.
