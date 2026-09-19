/****************************************************************************
 * app/velagesture/include/ai_agent.h
 *
 * AI agent integration — bridges gesture events and inbound alerts
 * into the openvela ai_agent framework.
 *
 * The agent receives gesture events, applies policy (e.g. "OK
 * gesture confirms an alert"), and dispatches skill invocations.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef APP_VELAGESTURE_INCLUDE_AI_AGENT_H
#define APP_VELAGESTURE_INCLUDE_AI_AGENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "velagesture.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Agent event — fed into the agent from external sources (gesture,
 * WebSocket alerts, audio intents). */

struct vg_agent_event_s
{
  enum
  {
    VG_AGENT_EVT_GESTURE,
    VG_AGENT_EVT_ALERT,
    VG_AGENT_EVT_SPEECH_INTENT,
    VG_AGENT_EVT_USER_CMD
  } type;

  union
  {
    struct vg_gesture_event_s gesture;
    struct
    {
      char alert_id[64];
      char message[256];
      int  severity;
    } alert;
    struct
    {
      char intent[128];
      float confidence;
    } speech;
    char user_cmd[256];
  };
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: vg_agent_start
 *
 * Description:
 *   Start the agent event-processing thread.  The agent blocks on
 *   an internal event queue and processes events in order.
 *
 ****************************************************************************/

int vg_agent_start(struct vg_context_s *ctx);

/****************************************************************************
 * Name: vg_agent_stop
 *
 * Description:
 *   Signal the agent thread to exit and wait for it to join.
 *
 ****************************************************************************/

void vg_agent_stop(void);

/****************************************************************************
 * Name: vg_agent_post_event
 *
 * Description:
 *   Post an event into the agent's input queue.  This function is
 *   thread-safe and may be called from the gesture, WebSocket, or
 *   audio subsystems.
 *
 ****************************************************************************/

int vg_agent_post_event(const struct vg_agent_event_s *evt);

/****************************************************************************
 * Name: vg_agent_handle_gesture
 *
 * Description:
 *   High-level handler: given a stable gesture event, decide what
 *   action to take (confirm alert, trigger skill, ignore, etc.).
 *
 ****************************************************************************/

int vg_agent_handle_gesture(struct vg_context_s *ctx,
                            const struct vg_gesture_event_s *event);

/****************************************************************************
 * Name: vg_agent_handle_alert
 *
 * Description:
 *   Process an inbound alert from the Guxian server.  Typically
 *   this prompts the user for confirmation and queues a skill call.
 *
 ****************************************************************************/

int vg_agent_handle_alert(struct vg_context_s *ctx,
                          const char *alert_id,
                          const char *message,
                          int severity);

#endif /* APP_VELAGESTURE_INCLUDE_AI_AGENT_H */
