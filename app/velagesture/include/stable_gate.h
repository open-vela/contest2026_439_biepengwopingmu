/****************************************************************************
 * app/velagesture/include/stable_gate.h
 *
 * Stable gate — filters transient gesture detections and only emits
 * a vg_gesture_event_s when the same gesture has been observed for
 * VG_STABLE_THRESHOLD consecutive frames.
 *
 * This module sits between gesture perception and the event/agent
 * layer and is critical for preventing false triggers.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef APP_VELAGESTURE_INCLUDE_STABLE_GATE_H
#define APP_VELAGESTURE_INCLUDE_STABLE_GATE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "velagesture.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: vg_stable_gate_init
 *
 * Description:
 *   Initialise the stable gate state.
 *
 ****************************************************************************/

void vg_stable_gate_init(void);

/****************************************************************************
 * Name: vg_stable_gate_feed
 *
 * Description:
 *   Feed a raw hand detection result into the stable gate.  If the
 *   gesture has been held long enough, the gate writes a fully-formed
 *   vg_gesture_event_s into *event and returns true.  Otherwise it
 *   returns false and *event is not modified.
 *
 * Input Parameters:
 *   raw    - The latest raw detection from perception.
 *   event  - Output event (only written when return is true).
 *
 * Returned Value:
 *   true if a stable event was produced, false otherwise.
 *
 ****************************************************************************/

bool vg_stable_gate_feed(const struct vg_hand_result_s *raw,
                         struct vg_gesture_event_s *event);

/****************************************************************************
 * Name: vg_stable_gate_reset
 *
 * Description:
 *   Force-reset the gate, discarding any accumulated stability count.
 *
 ****************************************************************************/

void vg_stable_gate_reset(void);

#endif /* APP_VELAGESTURE_INCLUDE_STABLE_GATE_H */
