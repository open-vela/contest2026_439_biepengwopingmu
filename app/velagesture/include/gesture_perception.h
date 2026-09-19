/****************************************************************************
 * app/velagesture/include/gesture_perception.h
 *
 * Gesture perception subsystem — camera capture and hand-gesture
 * recognition.  This module owns the camera device, runs inference on
 * each frame, and produces raw vg_hand_result_s structures for the
 * stable gate.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef APP_VELAGESTURE_INCLUDE_GESTURE_PERCEPTION_H
#define APP_VELAGESTURE_INCLUDE_GESTURE_PERCEPTION_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "velagesture.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: vg_perception_start
 *
 * Description:
 *   Open the camera device, initialise the inference backend, and
 *   start the perception thread.  The thread loops: capture → infer
 *   → push result to stable gate.
 *
 * Input Parameters:
 *   ctx - Application context (must be initialised).
 *
 * Returned Value:
 *   0 on success, negative errno on failure.
 *
 ****************************************************************************/

int vg_perception_start(struct vg_context_s *ctx);

/****************************************************************************
 * Name: vg_perception_stop
 *
 * Description:
 *   Signal the perception thread to exit and wait for it to join.
 *
 ****************************************************************************/

void vg_perception_stop(void);

/****************************************************************************
 * Name: vg_perception_infer
 *
 * Description:
 *   Run gesture inference on a single frame buffer.  Exposed as a
 *   public API so that unit tests can exercise the recognition path
 *   without a live camera device.
 *
 * Input Parameters:
 *   frame    - Pointer to raw frame data (grayscale or RGB).
 *   width    - Frame width in pixels.
 *   height   - Frame height in pixels.
 *   out      - Output hand detection result.
 *
 * Returned Value:
 *   0 on success, negative errno on failure.
 *
 ****************************************************************************/

int vg_perception_infer(const uint8_t *frame, int width, int height,
                        struct vg_hand_result_s *out);

#endif /* APP_VELAGESTURE_INCLUDE_GESTURE_PERCEPTION_H */
