/****************************************************************************
 * app/velagesture/src/ai_agent.c
 *
 * AI agent — event-driven decision engine that connects gesture
 * events, server alerts, and speech intents to skill execution.
 *
 * Core scenario: the Guxian business system pushes an alert over
 * WebSocket → the agent prompts the user → user confirms with OK
 * gesture → agent invokes guxian-control skill → result is sent
 * back and displayed.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "velagesture.h"
#include "ai_agent.h"
#include "skill_executor.h"
#include "ws_protocol.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define AGENT_EVT_QUEUE_SIZE  32
#define AGENT_PROMPT_MSG_LEN  256

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Pending alert — stored when an alert arrives and we're waiting
 * for user confirmation via gesture. */

struct pending_alert_s
{
  bool     active;
  char     alert_id[64];
  char     message[256];
  int      severity;
  uint64_t received_at;
};

struct agent_state_s
{
  struct vg_context_s       *ctx;
  pthread_t                  thread;
  volatile bool              running;

  /* Event queue (ring buffer) */

  struct vg_agent_event_s    queue[AGENT_EVT_QUEUE_SIZE];
  int                        q_head;
  int                        q_tail;
  int                        q_count;
  pthread_mutex_t            q_lock;
  pthread_cond_t             q_cond;

  /* Pending alert state */

  struct pending_alert_s     pending;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct agent_state_s g_agent;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: agent_enqueue
 *
 * Description:
 *   Add an event to the agent's input queue.
 ****************************************************************************/

static int agent_enqueue(const struct vg_agent_event_s *evt)
{
  pthread_mutex_lock(&g_agent.q_lock);

  if (g_agent.q_count >= AGENT_EVT_QUEUE_SIZE)
    {
      pthread_mutex_unlock(&g_agent.q_lock);
      return -ENOSPC;
    }

  g_agent.queue[g_agent.q_tail] = *evt;
  g_agent.q_tail = (g_agent.q_tail + 1) % AGENT_EVT_QUEUE_SIZE;
  g_agent.q_count++;

  pthread_cond_signal(&g_agent.q_cond);
  pthread_mutex_unlock(&g_agent.q_lock);
  return 0;
}

/****************************************************************************
 * Name: agent_dequeue
 *
 * Description:
 *   Pop the next event from the queue.  Blocks until one is
 *   available or the agent is shutting down.
 ****************************************************************************/

static bool agent_dequeue(struct vg_agent_event_s *evt)
{
  pthread_mutex_lock(&g_agent.q_lock);

  while (g_agent.q_count == 0 && g_agent.running)
    {
      pthread_cond_wait(&g_agent.q_cond, &g_agent.q_lock);
    }

  if (!g_agent.running && g_agent.q_count == 0)
    {
      pthread_mutex_unlock(&g_agent.q_lock);
      return false;
    }

  *evt = g_agent.queue[g_agent.q_head];
  g_agent.q_head = (g_agent.q_head + 1) % AGENT_EVT_QUEUE_SIZE;
  g_agent.q_count--;

  pthread_mutex_unlock(&g_agent.q_lock);
  return true;
}

/****************************************************************************
 * Name: process_gesture_event
 *
 * Description:
 *   Core gesture → action mapping.
 *
 *   - OK gesture:  If there's a pending alert, confirm it and
 *     invoke the guxian-control skill.
 *   - Fist gesture: Cancel / dismiss current alert.
 *   - Open palm:   Trigger status query.
 *   - Victory/Pinch: Reserved for future use.
 ****************************************************************************/

static void process_gesture_event(const struct vg_gesture_event_s *event)
{
  struct vg_skill_result_s result;
  int ret;

  switch (event->gesture)
    {
      case VG_GESTURE_OK:
        if (g_agent.pending.active)
          {
            /* User confirmed the pending alert — execute the
             * associated action via the guxian-control skill. */

            printf("AGENT: OK gesture → confirming alert '%s'\n",
                   g_agent.pending.alert_id);

            char params[256];
            snprintf(params, sizeof(params),
                     "{\"alert_id\":\"%s\",\"action\":\"confirm\"}",
                     g_agent.pending.alert_id);

            ret = vg_skill_execute_action(params, &result);
            if (ret == 0)
              {
                printf("AGENT: Skill result: %s\n", result.payload);

                /* Send result back to Guxian via WebSocket */

                vg_ws_send_status(result.payload);
              }

            g_agent.pending.active = false;
          }
        else
          {
            printf("AGENT: OK gesture (no pending alert, ignored)\n");
          }
        break;

      case VG_GESTURE_FIST:
        if (g_agent.pending.active)
          {
            printf("AGENT: Fist gesture → dismissing alert '%s'\n",
                   g_agent.pending.alert_id);
            g_agent.pending.active = false;

            vg_ws_send_status("{\"status\":\"alert_dismissed\"}");
          }
        break;

      case VG_GESTURE_OPEN_PALM:
        /* Open palm → query system status */

        printf("AGENT: Open palm → querying status\n");
        ret = vg_skill_query_status("system", &result);
        if (ret == 0)
          {
            printf("AGENT: Status: %s\n", result.payload);
            vg_ws_send_status(result.payload);
          }
        break;

      default:
        break;
    }
}

/****************************************************************************
 * Name: process_alert_event
 *
 * Description:
 *   Handle an inbound alert from the Guxian server.  Store it as
 *   the pending alert and notify the user (via display/audio).
 ****************************************************************************/

static void process_alert_event(const char *alert_id, const char *message,
                                int severity)
{
  printf("AGENT: Alert received [%d]: %s\n", severity, message);

  /* Store as pending alert — will be confirmed/dismissed by
   * the next gesture event. */

  g_agent.pending.active = true;
  strlcpy(g_agent.pending.alert_id, alert_id,
          sizeof(g_agent.pending.alert_id));
  strlcpy(g_agent.pending.message, message,
          sizeof(g_agent.pending.message));
  g_agent.pending.severity   = severity;
  g_agent.pending.received_at = vg_timestamp_ms();

  /* Send acknowledgement back to server */

  char ack[128];
  snprintf(ack, sizeof(ack),
           "{\"type\":\"ack\",\"alert_id\":\"%s\",\"status\":\"received\"}",
           alert_id);
  vg_ws_send_status(ack);
}

/****************************************************************************
 * Name: process_speech_intent
 *
 * Description:
 *   Handle a speech intent from the audio/ASR pipeline.
 *   Maps recognised intents to skill calls.
 ****************************************************************************/

static void process_speech_intent(const char *intent, float confidence)
{
  struct vg_skill_result_s result;

  printf("AGENT: Speech intent '%s' (conf=%.2f)\n", intent, confidence);

  if (strstr(intent, "status") != NULL || strstr(intent, "状态") != NULL)
    {
      vg_skill_query_status("system", &result);
      vg_ws_send_status(result.payload);
    }
  else if (strstr(intent, "confirm") != NULL ||
           strstr(intent, "确认") != NULL)
    {
      if (g_agent.pending.active)
        {
          char params[256];
          snprintf(params, sizeof(params),
                   "{\"alert_id\":\"%s\",\"action\":\"confirm\"}",
                   g_agent.pending.alert_id);
          vg_skill_execute_action(params, &result);
          vg_ws_send_status(result.payload);
          g_agent.pending.active = false;
        }
    }
}

/****************************************************************************
 * Name: agent_thread
 *
 * Description:
 *   Main agent event loop.  Dequeues events and dispatches them
 *   to the appropriate handler.
 ****************************************************************************/

static FAR void *agent_thread(FAR void *arg)
{
  struct vg_agent_event_s evt;

  printf("AGENT: Thread started\n");

  while (agent_dequeue(&evt))
    {
      switch (evt.type)
        {
          case VG_AGENT_EVT_GESTURE:
            process_gesture_event(&evt.gesture);
            break;

          case VG_AGENT_EVT_ALERT:
            process_alert_event(evt.alert.alert_id,
                                evt.alert.message,
                                evt.alert.severity);
            break;

          case VG_AGENT_EVT_SPEECH_INTENT:
            process_speech_intent(evt.speech.intent,
                                  evt.speech.confidence);
            break;

          case VG_AGENT_EVT_USER_CMD:
            printf("AGENT: User command: %s\n", evt.user_cmd);
            break;

          default:
            fprintf(stderr, "AGENT: Unknown event type: %d\n", evt.type);
            break;
        }
    }

  printf("AGENT: Thread exiting\n");
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vg_agent_start
 ****************************************************************************/

int vg_agent_start(struct vg_context_s *ctx)
{
  int ret;

  memset(&g_agent, 0, sizeof(g_agent));
  g_agent.ctx     = ctx;
  g_agent.running = true;

  ret = pthread_mutex_init(&g_agent.q_lock, NULL);
  if (ret != 0)
    {
      return -ret;
    }

  ret = pthread_cond_init(&g_agent.q_cond, NULL);
  if (ret != 0)
    {
      pthread_mutex_destroy(&g_agent.q_lock);
      return -ret;
    }

  ret = pthread_create(&g_agent.thread, NULL, agent_thread, &g_agent);
  if (ret != 0)
    {
      pthread_cond_destroy(&g_agent.q_cond);
      pthread_mutex_destroy(&g_agent.q_lock);
      return -ret;
    }

  ctx->agent_ready = true;
  return 0;
}

/****************************************************************************
 * Name: vg_agent_stop
 ****************************************************************************/

void vg_agent_stop(void)
{
  g_agent.running = false;
  pthread_cond_signal(&g_agent.q_cond);
  pthread_join(g_agent.thread, NULL);

  pthread_cond_destroy(&g_agent.q_cond);
  pthread_mutex_destroy(&g_agent.q_lock);

  g_agent.ctx->agent_ready = false;
}

/****************************************************************************
 * Name: vg_agent_post_event
 ****************************************************************************/

int vg_agent_post_event(const struct vg_agent_event_s *evt)
{
  return agent_enqueue(evt);
}

/****************************************************************************
 * Name: vg_agent_handle_gesture
 ****************************************************************************/

int vg_agent_handle_gesture(struct vg_context_s *ctx,
                            const struct vg_gesture_event_s *event)
{
  struct vg_agent_event_s evt;
  memset(&evt, 0, sizeof(evt));
  evt.type    = VG_AGENT_EVT_GESTURE;
  evt.gesture = *event;
  return vg_agent_post_event(&evt);
}

/****************************************************************************
 * Name: vg_agent_handle_alert
 ****************************************************************************/

int vg_agent_handle_alert(struct vg_context_s *ctx,
                          const char *alert_id,
                          const char *message,
                          int severity)
{
  struct vg_agent_event_s evt;
  memset(&evt, 0, sizeof(evt));
  evt.type = VG_AGENT_EVT_ALERT;
  strlcpy(evt.alert.alert_id, alert_id, sizeof(evt.alert.alert_id));
  strlcpy(evt.alert.message, message, sizeof(evt.alert.message));
  evt.alert.severity = severity;
  return vg_agent_post_event(&evt);
}
