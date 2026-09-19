/****************************************************************************
 * app/velagesture/src/stable_gate.c
 *
 * Stable gate — filters transient gesture detections.
 *
 * Maintains an internal counter that increments when the same gesture
 * is observed consecutively.  When the counter reaches
 * VG_STABLE_THRESHOLD the gate emits a stable event.  A different
 * gesture resets the counter after VG_STABLE_RESET frames.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdbool.h>
#include <string.h>

#include "velagesture.h"
#include "stable_gate.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_current_gesture;
static int g_stable_count;
static int g_diff_count;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vg_stable_gate_init
 ****************************************************************************/

void vg_stable_gate_init(void)
{
  g_current_gesture = VG_GESTURE_NONE;
  g_stable_count    = 0;
  g_diff_count      = 0;
}

/****************************************************************************
 * Name: vg_stable_gate_feed
 ****************************************************************************/

bool vg_stable_gate_feed(const struct vg_hand_result_s *raw,
                         struct vg_gesture_event_s *event)
{
  if (!raw->hand_detected)
    {
      /* No hand in frame — reset everything */

      g_current_gesture = VG_GESTURE_NONE;
      g_stable_count    = 0;
      g_diff_count      = 0;
      return false;
    }

  if (raw->gesture_id == g_current_gesture)
    {
      /* Same gesture — increment stable count, reset diff count */

      g_stable_count++;
      g_diff_count = 0;

      if (g_stable_count >= VG_STABLE_THRESHOLD &&
          g_current_gesture != VG_GESTURE_NONE)
        {
          /* Gesture is stable — emit event */

          event->gesture      = g_current_gesture;
          event->confidence   = raw->confidence;
          event->stable       = true;
          event->timestamp_ms = vg_timestamp_ms();

          /* Reset counters so the same gesture requires another
           * stabilisation period before it is emitted again. */

          g_stable_count = 0;
          return true;
        }
    }
  else
    {
      /* Different gesture — increment diff count */

      g_diff_count++;

      if (g_diff_count >= VG_STABLE_RESET)
        {
          /* Enough different frames to accept the change */

          g_current_gesture = raw->gesture_id;
          g_stable_count    = 1;
          g_diff_count      = 0;
        }
    }

  return false;
}

/****************************************************************************
 * Name: vg_stable_gate_reset
 ****************************************************************************/

void vg_stable_gate_reset(void)
{
  g_current_gesture = VG_GESTURE_NONE;
  g_stable_count    = 0;
  g_diff_count      = 0;
}
