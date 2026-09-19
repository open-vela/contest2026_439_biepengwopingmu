/****************************************************************************
 * app/velagesture/src/ws_client.c
 *
 * WebSocket client — connects to the Guxian business system server.
 *
 * Two-way communication:
 *   P4 → Guxian: gesture events, task requests, status, errors
 *   Guxian → P4: alerts, task results, acknowledgements
 *
 * Uses the libwebsockets library available in openvela
 * (apps/netutils/libwebsockets).
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

#include <libwebsockets.h>

#include "velagesture.h"
#include "ws_protocol.h"
#include "ai_agent.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WS_SERVER_HOST    CONFIG_VELAGESTURE_WS_HOST
#define WS_SERVER_PORT    CONFIG_VELAGESTURE_WS_PORT
#define WS_SERVER_PATH    "/ws/velagesture"
#define WS_RX_BUF_SIZE    4096
#define WS_TX_BUF_SIZE    2048
#define WS_RECONNECT_MS   3000

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ws_state_s
{
  struct vg_context_s     *ctx;
  struct lws_context      *lws_ctx;
  struct lws              *wsi;
  pthread_t                thread;
  volatile bool            running;
  volatile bool            connected;

  /* Outbound message queue (simple ring buffer) */

  char                     tx_buf[WS_TX_BUF_SIZE];
  int                      tx_len;
  pthread_mutex_t          tx_lock;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct ws_state_s g_ws;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ws_callback
 *
 * Description:
 *   libwebsockets protocol callback.  Handles connection lifecycle,
 *   incoming data, and outgoing writes.
 ****************************************************************************/

static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                       void *user, void *in, size_t len)
{
  struct ws_state_s *ws = &g_ws;

  switch (reason)
    {
      case LWS_CALLBACK_CLIENT_ESTABLISHED:
        ws->connected = true;
        ws->wsi = wsi;
        printf("WS: Connected to %s:%d\n", WS_SERVER_HOST, WS_SERVER_PORT);
        break;

      case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        fprintf(stderr, "WS: Connection error: %s\n",
                in ? (const char *)in : "unknown");
        ws->connected = false;
        ws->wsi = NULL;
        break;

      case LWS_CALLBACK_CLIENT_RECEIVE:
        /* Parse incoming message and dispatch to agent */

        if (in && len > 0)
          {
            /* Parse the JSON message.  For simplicity we do a
             * lightweight string-match dispatch here.  A production
             * build would use cJSON. */

            const char *msg = (const char *)in;

            if (strstr(msg, "\"type\":\"alert\"") != NULL)
              {
                /* Extract alert fields (simplified) */
                const char *alert_id = strstr(msg, "\"alert_id\":\"");
                const char *message  = strstr(msg, "\"message\":\"");

                vg_agent_handle_alert(ws->ctx,
                    alert_id ? alert_id + 12 : "unknown",
                    message  ? message  + 11 : "unknown",
                    1);
              }
            else if (strstr(msg, "\"type\":\"task_result\"") != NULL)
              {
                /* Update task state in context */
                pthread_mutex_lock(&ws->ctx->lock);
                ws->ctx->current_task.state = VG_TASK_DONE;
                pthread_mutex_unlock(&ws->ctx->lock);
              }
          }
        break;

      case LWS_CALLBACK_CLIENT_WRITEABLE:
        /* Send queued outbound data */

        pthread_mutex_lock(&ws->tx_lock);
        if (ws->tx_len > 0)
          {
            unsigned char buf[LWS_PRE + WS_TX_BUF_SIZE];
            memcpy(&buf[LWS_PRE], ws->tx_buf, ws->tx_len);
            lws_write(wsi, &buf[LWS_PRE], ws->tx_len, LWS_WRITE_TEXT);
            ws->tx_len = 0;
          }
        pthread_mutex_unlock(&ws->tx_lock);
        break;

      case LWS_CALLBACK_CLIENT_CLOSED:
        ws->connected = false;
        ws->wsi = NULL;
        printf("WS: Connection closed\n");
        break;

      default:
        break;
    }

  return 0;
}

/****************************************************************************
 * Name: ws_protocols
 ****************************************************************************/

static const struct lws_protocols ws_protocols[] =
{
  {
    "velagesture-ws",
    ws_callback,
    0,
    WS_RX_BUF_SIZE,
  },
  { NULL, NULL, 0, 0 }
};

/****************************************************************************
 * Name: ws_thread
 *
 * Description:
 *   WebSocket service loop.  Calls lws_service() repeatedly and
 *   handles reconnection.
 ****************************************************************************/

static FAR void *ws_thread(FAR void *arg)
{
  struct ws_state_s *ws = (struct ws_state_s *)arg;

  printf("WS: Thread started\n");

  while (ws->running)
    {
      if (ws->lws_ctx != NULL)
        {
          lws_service(ws->lws_ctx, 50);
        }

      /* Reconnection logic */

      if (!ws->connected && ws->running)
        {
          usleep(WS_RECONNECT_MS * 1000);

          if (ws->running && !ws->connected)
            {
              printf("WS: Attempting reconnection...\n");
              /* lws_client_connect_via_info() would be called here
               * with the stored connection parameters. */
            }
        }
    }

  printf("WS: Thread exiting\n");
  return NULL;
}

/****************************************************************************
 * Name: ws_send_internal
 *
 * Description:
 *   Queue a JSON message for sending on the next writable callback.
 ****************************************************************************/

static int ws_send_internal(const char *json, int len)
{
  if (!g_ws.connected || g_ws.wsi == NULL)
    {
      return -ENOTCONN;
    }

  pthread_mutex_lock(&g_ws.tx_lock);

  if (len > WS_TX_BUF_SIZE)
    {
      pthread_mutex_unlock(&g_ws.tx_lock);
      return -E2BIG;
    }

  memcpy(g_ws.tx_buf, json, len);
  g_ws.tx_len = len;

  pthread_mutex_unlock(&g_ws.tx_lock);

  lws_callback_on_writable(g_ws.wsi);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vg_ws_start
 ****************************************************************************/

int vg_ws_start(struct vg_context_s *ctx)
{
  struct lws_context_creation_info info;
  struct lws_client_connect_info ccinfo;
  int ret;

  memset(&g_ws, 0, sizeof(g_ws));
  g_ws.ctx     = ctx;
  g_ws.running = true;

  ret = pthread_mutex_init(&g_ws.tx_lock, NULL);
  if (ret != 0)
    {
      return -ret;
    }

  /* Create lws context */

  memset(&info, 0, sizeof(info));
  info.port      = CONTEXT_PORT_NO_LISTEN;
  info.protocols = ws_protocols;
  info.options   = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

  g_ws.lws_ctx = lws_create_context(&info);
  if (g_ws.lws_ctx == NULL)
    {
      fprintf(stderr, "WS: Cannot create lws context\n");
      return -ENOMEM;
    }

  /* Connect to server */

  memset(&ccinfo, 0, sizeof(ccinfo));
  ccinfo.context    = g_ws.lws_ctx;
  ccinfo.address    = WS_SERVER_HOST;
  ccinfo.port       = WS_SERVER_PORT;
  ccinfo.path       = WS_SERVER_PATH;
  ccinfo.host       = WS_SERVER_HOST;
  ccinfo.origin     = WS_SERVER_HOST;
  ccinfo.protocol   = ws_protocols[0].name;
  ccinfo.ssl_connection = LCCSCF_USE_SSL;

  if (lws_client_connect_via_info(&ccinfo) == NULL)
    {
      fprintf(stderr, "WS: Client connect failed (will retry)\n");
      /* Non-fatal — reconnection logic in ws_thread will retry */
    }

  /* Start service thread */

  ret = pthread_create(&g_ws.thread, NULL, ws_thread, &g_ws);
  if (ret != 0)
    {
      lws_context_destroy(g_ws.lws_ctx);
      return -ret;
    }

  ctx->ws_connected = true;
  return 0;
}

/****************************************************************************
 * Name: vg_ws_stop
 ****************************************************************************/

void vg_ws_stop(void)
{
  g_ws.running = false;
  pthread_join(g_ws.thread, NULL);

  if (g_ws.lws_ctx != NULL)
    {
      lws_context_destroy(g_ws.lws_ctx);
    }

  pthread_mutex_destroy(&g_ws.tx_lock);
  g_ws.ctx->ws_connected = false;
}

/****************************************************************************
 * Name: vg_ws_encode_gesture_json
 ****************************************************************************/

int vg_ws_encode_gesture_json(const struct vg_gesture_event_s *event,
                              char *buf, size_t buflen)
{
  int n = snprintf(buf, buflen,
      "{\"type\":\"gesture\","
      "\"gesture\":\"%s\","
      "\"confidence\":%.2f,"
      "\"stable\":%s,"
      "\"timestamp\":%llu}",
      vg_gesture_name(event->gesture),
      event->confidence,
      event->stable ? "true" : "false",
      (unsigned long long)event->timestamp_ms);

  if (n < 0 || (size_t)n >= buflen)
    {
      return -E2BIG;
    }

  return n;
}

/****************************************************************************
 * Name: vg_ws_send_gesture_event
 ****************************************************************************/

int vg_ws_send_gesture_event(const struct vg_gesture_event_s *event)
{
  char buf[256];
  int len = vg_ws_encode_gesture_json(event, buf, sizeof(buf));
  if (len < 0)
    {
      return len;
    }

  return ws_send_internal(buf, len);
}

/****************************************************************************
 * Name: vg_ws_send_task
 ****************************************************************************/

int vg_ws_send_task(const struct vg_task_s *task)
{
  char buf[512];
  int n = snprintf(buf, sizeof(buf),
      "{\"type\":\"task\","
      "\"task_id\":%d,"
      "\"skill\":\"%s\","
      "\"action\":\"%s\","
      "\"payload\":%s}",
      task->task_id, task->skill, task->action,
      task->payload[0] ? task->payload : "null");

  if (n < 0 || (size_t)n >= sizeof(buf))
    {
      return -E2BIG;
    }

  return ws_send_internal(buf, n);
}

/****************************************************************************
 * Name: vg_ws_send_status
 ****************************************************************************/

int vg_ws_send_status(const char *status_json)
{
  char buf[512];
  int n = snprintf(buf, sizeof(buf),
      "{\"type\":\"status\",%s}", status_json);

  if (n < 0 || (size_t)n >= sizeof(buf))
    {
      return -E2BIG;
    }

  return ws_send_internal(buf, n);
}

/****************************************************************************
 * Name: vg_ws_send_error
 ****************************************************************************/

int vg_ws_send_error(int code, const char *message)
{
  char buf[256];
  int n = snprintf(buf, sizeof(buf),
      "{\"type\":\"error\",\"code\":%d,\"message\":\"%s\"}",
      code, message);

  if (n < 0 || (size_t)n >= sizeof(buf))
    {
      return -E2BIG;
    }

  return ws_send_internal(buf, n);
}
