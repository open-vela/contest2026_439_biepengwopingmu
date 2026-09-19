/****************************************************************************
 * app/velagesture/include/ws_protocol.h
 *
 * WebSocket protocol definitions for VelaGesture.
 *
 * Two directions:
 *   P4 → Guxian  (control / event messages)
 *   Guxian → P4  (status / task result / alert messages)
 *
 * All messages are JSON-encoded over a single WebSocket connection.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef APP_VELAGESTURE_INCLUDE_WS_PROTOCOL_H
#define APP_VELAGESTURE_INCLUDE_WS_PROTOCOL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "velagesture.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Message types — P4 → Guxian */

#define VG_WS_MSG_GESTURE      "gesture"
#define VG_WS_MSG_TASK         "task"
#define VG_WS_MSG_STATUS       "status"
#define VG_WS_MSG_ERROR        "error"

/* Message types — Guxian → P4 */

#define VG_WS_MSG_ALERT        "alert"
#define VG_WS_MSG_TASK_RESULT  "task_result"
#define VG_WS_MSG_ACK          "ack"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Parsed inbound message from the Guxian server. */

struct vg_ws_msg_s
{
  const char *type;            /* message type string */
  const char *payload_json;    /* pointer into the original JSON buffer */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: vg_ws_start
 *
 * Description:
 *   Start the WebSocket client thread.  Connects to the Guxian
 *   server at the configured URL and enters a read loop, dispatching
 *   inbound messages to the agent/alert handler.
 *
 ****************************************************************************/

int vg_ws_start(struct vg_context_s *ctx);

/****************************************************************************
 * Name: vg_ws_stop
 *
 * Description:
 *   Close the WebSocket connection and stop the client thread.
 *
 ****************************************************************************/

void vg_ws_stop(void);

/****************************************************************************
 * Name: vg_ws_send_gesture_event
 *
 * Description:
 *   Serialise a gesture event as JSON and send it to the Guxian
 *   server.
 *
 ****************************************************************************/

int vg_ws_send_gesture_event(const struct vg_gesture_event_s *event);

/****************************************************************************
 * Name: vg_ws_send_task
 *
 * Description:
 *   Send a task request to the Guxian server.
 *
 ****************************************************************************/

int vg_ws_send_task(const struct vg_task_s *task);

/****************************************************************************
 * Name: vg_ws_send_status
 *
 * Description:
 *   Send a device status update.
 *
 ****************************************************************************/

int vg_ws_send_status(const char *status_json);

/****************************************************************************
 * Name: vg_ws_send_error
 *
 * Description:
 *   Send an error message.
 *
 ****************************************************************************/

int vg_ws_send_error(int code, const char *message);

/****************************************************************************
 * Name: vg_ws_encode_gesture_json
 *
 * Description:
 *   Encode a gesture event into a JSON string in the caller-supplied
 *   buffer.  Returns the number of bytes written (excluding NUL), or
 *   a negative errno on failure.
 *
 ****************************************************************************/

int vg_ws_encode_gesture_json(const struct vg_gesture_event_s *event,
                              char *buf, size_t buflen);

#endif /* APP_VELAGESTURE_INCLUDE_WS_PROTOCOL_H */
