/****************************************************************************
 * app/velagesture/include/velagesture.h
 *
 * VelaGesture — 基于 openvela 的多模态智能手势交互终端
 *
 * Main application header. Defines shared data structures and the
 * global application context used across all subsystems.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef APP_VELAGESTURE_INCLUDE_VELAGESTURE_H
#define APP_VELAGESTURE_INCLUDE_VELAGESTURE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Application metadata */

#define VG_APP_NAME           "velagesture"
#define VG_APP_VERSION        "1.0.0"

/* Gesture types */

#define VG_GESTURE_NONE       0
#define VG_GESTURE_OK         1
#define VG_GESTURE_FIST       2
#define VG_GESTURE_OPEN_PALM  3
#define VG_GESTURE_VICTORY    4
#define VG_GESTURE_PINCH      5
#define VG_GESTURE_MAX        6

/* Stable gate parameters — a gesture must be held for N consecutive
 * frames before it is considered stable and emitted as an event. */

#define VG_STABLE_THRESHOLD   5      /* consecutive frames required  */
#define VG_STABLE_RESET       3      /* consecutive different frames to reset */

/* Audio parameters */

#define VG_AUDIO_SAMPLE_RATE  16000
#define VG_AUDIO_CHANNELS     1
#define VG_AUDIO_BITS         16
#define VG_AUDIO_FRAME_MS     20

/* Task state machine */

#define VG_TASK_IDLE          0
#define VG_TASK_PENDING       1
#define VG_TASK_RUNNING       2
#define VG_TASK_DONE          3
#define VG_TASK_ERROR         4

/* Maximum lengths */

#define VG_MAX_SKILL_NAME     64
#define VG_MAX_ACTION_NAME    64
#define VG_MAX_PAYLOAD_LEN    1024

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Raw hand detection result from the perception pipeline.
 * In the current architecture this is produced by the gesture
 * recognition backend (ESP-DL / TF-Lite Micro / custom model). */

struct vg_hand_result_s
{
  int  gesture_id;       /* one of VG_GESTURE_* */
  float confidence;      /* 0.0 – 1.0 */
  bool hand_detected;    /* true if a hand is present in frame */
};

/* Stable-gated gesture event — emitted only when the gesture has
 * been held for VG_STABLE_THRESHOLD consecutive frames.  This is
 * the canonical event consumed by the AI agent and WebSocket layer. */

struct vg_gesture_event_s
{
  int      gesture;       /* VG_GESTURE_* identifier */
  float    confidence;    /* confidence at time of stabilisation */
  bool     stable;        /* always true when emitted by stable gate */
  uint64_t timestamp_ms;  /* monotonic clock, milliseconds */
};

/* Task descriptor — tracks one round-trip from gesture/agent trigger
 * through skill execution and result delivery. */

struct vg_task_s
{
  int       task_id;
  int       state;          /* VG_TASK_* */
  char      skill[VG_MAX_SKILL_NAME];
  char      action[VG_MAX_ACTION_NAME];
  char      payload[VG_MAX_PAYLOAD_LEN];
  int       result_code;
};

/* Application context — singleton owned by main(), shared read-only
 * with subsystem threads.  Each subsystem receives a pointer to this
 * structure at thread creation. */

struct vg_context_s
{
  /* Subsystem state */

  bool camera_ready;
  bool ws_connected;
  bool agent_ready;
  bool audio_ready;

  /* Current gesture (updated by stable gate) */

  struct vg_gesture_event_s last_event;

  /* Task tracking */

  struct vg_task_s current_task;
  int              next_task_id;

  /* Synchronisation */

  pthread_mutex_t lock;
  pthread_cond_t  event_cond;   /* signalled when a new event arrives */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: vg_context_init
 *
 * Description:
 *   Initialise the application context (mutex, condvar, defaults).
 *
 ****************************************************************************/

int vg_context_init(struct vg_context_s *ctx);

/****************************************************************************
 * Name: vg_context_destroy
 *
 * Description:
 *   Tear down the application context, releasing synchronisation
 *   resources.
 *
 ****************************************************************************/

void vg_context_destroy(struct vg_context_s *ctx);

/****************************************************************************
 * Name: vg_gesture_name
 *
 * Description:
 *   Return a human-readable name for a VG_GESTURE_* identifier.
 *
 ****************************************************************************/

const char *vg_gesture_name(int gesture);

/****************************************************************************
 * Name: vg_timestamp_ms
 *
 * Description:
 *   Return the current monotonic time in milliseconds.
 *
 ****************************************************************************/

uint64_t vg_timestamp_ms(void);

#endif /* APP_VELAGESTURE_INCLUDE_VELAGESTURE_H */
