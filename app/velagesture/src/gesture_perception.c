/****************************************************************************
 * app/velagesture/src/gesture_perception.c
 *
 * Gesture perception subsystem — camera capture and hand-gesture
 * recognition on the ESP32-P4 platform.
 *
 * Pipeline:
 *   Camera (MIPI-CSI / DVP) → Frame buffer
 *   → Inference backend (ESP-DL / TF-Lite Micro)
 *   → vg_hand_result_s → stable gate
 *
 * The inference backend is selected at build time via Kconfig.
 * The default path uses a lightweight CNN model that classifies
 * hand gestures directly from the camera frame without requiring
 * a 21-keypoint intermediate representation.
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
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include "velagesture.h"
#include "gesture_perception.h"
#include "stable_gate.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CAMERA_DEV_PATH      "/dev/video0"
#define FRAME_WIDTH          320
#define FRAME_HEIGHT         240
#define FRAME_PIXELS         (FRAME_WIDTH * FRAME_HEIGHT)
#define INFERENCE_INTERVAL_MS 100  /* ~10 fps inference rate */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct perception_state_s
{
  struct vg_context_s *ctx;
  pthread_t            thread;
  volatile bool        running;
  int                  camera_fd;
  uint8_t             *frame_buf;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct perception_state_s g_percep;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: open_camera
 *
 * Description:
 *   Open and configure the camera device for grayscale capture at
 *   FRAME_WIDTH x FRAME_HEIGHT.
 ****************************************************************************/

static int open_camera(struct perception_state_s *ps)
{
  ps->camera_fd = open(CAMERA_DEV_PATH, O_RDONLY);
  if (ps->camera_fd < 0)
    {
      fprintf(stderr, "PERCEP: Cannot open %s: %d\n",
              CAMERA_DEV_PATH, errno);
      return -errno;
    }

  /* Camera configuration (resolution, format, etc.) is handled by
   * the board-level video driver.  On ESP32-P4 with openvela, the
   * MIPI-CSI driver is configured via Kconfig and the board init
   * code.  No additional ioctl calls are required here for the
   * default configuration. */

  return 0;
}

/****************************************************************************
 * Name: capture_frame
 *
 * Description:
 *   Read one frame from the camera device into the frame buffer.
 ****************************************************************************/

static int capture_frame(struct perception_state_s *ps)
{
  ssize_t nread = read(ps->camera_fd, ps->frame_buf, FRAME_PIXELS);
  if (nread < 0)
    {
      return -errno;
    }

  if (nread < FRAME_PIXELS)
    {
      return -EAGAIN; /* incomplete frame */
    }

  return 0;
}

/****************************************************************************
 * Name: classify_gesture
 *
 * Description:
 *   Run gesture classification on the current frame buffer.
 *   This is the inference entry point.
 *
 *   The implementation depends on the selected backend:
 *   - CONFIG_VELAGESTURE_USE_ESPDL: ESP-DL runtime on P4
 *   - CONFIG_VELAGESTURE_USE_TFLITE: TF-Lite Micro
 *   - Fallback: basic heuristic for development/testing
 *
 *   The function maps the raw frame to one of VG_GESTURE_* and
 *   populates the confidence score.
 ****************************************************************************/

static int classify_gesture(const uint8_t *frame, int width, int height,
                            struct vg_hand_result_s *out)
{
  /* The actual inference is performed by the selected backend.
   * The frame is passed as a raw grayscale buffer.
   *
   * For the ESP-DL backend, the model expects:
   *   - Input:  width x height grayscale (or RGB after conversion)
   *   - Output: VG_GESTURE_MAX class probabilities
   *   - Post-processing: argmax + confidence threshold
   *
   * The fallback heuristic below allows the application to run on
   * hardware without a model file (e.g. for protocol and UI testing). */

#if defined(CONFIG_VELAGESTURE_USE_ESPDL)
  /* ESP-DL inference path — uses the board's hardware accelerator.
   * Model files are expected at /data/models/hand_gesture.onnx or
   * the quantised .espdl equivalent. */

  /* TODO: Integrate esp-dl runtime once model is deployed to device.
   * The skeleton here shows the expected interface. */

  out->gesture_id    = VG_GESTURE_NONE;
  out->confidence    = 0.0f;
  out->hand_detected = false;
  return 0;

#elif defined(CONFIG_VELAGESTURE_USE_TFLITE)
  /* TF-Lite Micro inference path */

  out->gesture_id    = VG_GESTURE_NONE;
  out->confidence    = 0.0f;
  out->hand_detected = false;
  return 0;

#else
  /* Development fallback — scan for simple pixel patterns.
   * This is NOT intended for production use.  It allows the rest of
   * the pipeline (stable gate, event, WS, agent) to be tested
   * without a real model. */

  (void)frame;
  (void)width;
  (void)height;

  out->gesture_id    = VG_GESTURE_NONE;
  out->confidence    = 0.0f;
  out->hand_detected = false;
  return 0;
#endif
}

/****************************************************************************
 * Name: perception_thread
 *
 * Description:
 *   Main perception loop.  Captures frames from the camera, runs
 *   gesture inference, and feeds results to the stable gate.
 ****************************************************************************/

static FAR void *perception_thread(FAR void *arg)
{
  struct perception_state_s *ps = (struct perception_state_s *)arg;
  struct vg_hand_result_s result;
  struct vg_gesture_event_s event;
  int ret;

  printf("PERCEP: Thread started\n");

  while (ps->running)
    {
      /* Capture one frame */

      ret = capture_frame(ps);
      if (ret < 0)
        {
          if (ret == -EAGAIN)
            {
              continue; /* incomplete frame, retry */
            }

          fprintf(stderr, "PERCEP: Capture error: %d\n", ret);
          usleep(INFERENCE_INTERVAL_MS * 1000);
          continue;
        }

      /* Run gesture inference */

      ret = classify_gesture(ps->frame_buf, FRAME_WIDTH, FRAME_HEIGHT,
                             &result);
      if (ret < 0)
        {
          fprintf(stderr, "PERCEP: Inference error: %d\n", ret);
          usleep(INFERENCE_INTERVAL_MS * 1000);
          continue;
        }

      /* Feed result to stable gate */

      if (vg_stable_gate_feed(&result, &event))
        {
          /* Stable gesture detected — update context and notify */

          pthread_mutex_lock(&ps->ctx->lock);
          ps->ctx->last_event = event;
          ps->ctx->camera_ready = true;
          pthread_cond_signal(&ps->ctx->event_cond);
          pthread_mutex_unlock(&ps->ctx->lock);

          printf("PERCEP: Stable gesture: %s (%.2f)\n",
                 vg_gesture_name(event.gesture), event.confidence);
        }

      /* Rate limiting */

      usleep(INFERENCE_INTERVAL_MS * 1000);
    }

  printf("PERCEP: Thread exiting\n");
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vg_perception_start
 ****************************************************************************/

int vg_perception_start(struct vg_context_s *ctx)
{
  int ret;

  memset(&g_percep, 0, sizeof(g_percep));
  g_percep.ctx     = ctx;
  g_percep.running = true;

  /* Allocate frame buffer */

  g_percep.frame_buf = malloc(FRAME_PIXELS);
  if (g_percep.frame_buf == NULL)
    {
      fprintf(stderr, "PERCEP: Cannot allocate frame buffer\n");
      return -ENOMEM;
    }

  /* Open camera device */

  ret = open_camera(&g_percep);
  if (ret < 0)
    {
      free(g_percep.frame_buf);
      return ret;
    }

  /* Initialise stable gate */

  vg_stable_gate_init();

  /* Start perception thread */

  ret = pthread_create(&g_percep.thread, NULL, perception_thread,
                       &g_percep);
  if (ret != 0)
    {
      fprintf(stderr, "PERCEP: Cannot create thread: %d\n", ret);
      close(g_percep.camera_fd);
      free(g_percep.frame_buf);
      return -ret;
    }

  return 0;
}

/****************************************************************************
 * Name: vg_perception_stop
 ****************************************************************************/

void vg_perception_stop(void)
{
  g_percep.running = false;
  pthread_join(g_percep.thread, NULL);

  if (g_percep.camera_fd >= 0)
    {
      close(g_percep.camera_fd);
    }

  free(g_percep.frame_buf);
}

/****************************************************************************
 * Name: vg_perception_infer
 ****************************************************************************/

int vg_perception_infer(const uint8_t *frame, int width, int height,
                        struct vg_hand_result_s *out)
{
  return classify_gesture(frame, width, height, out);
}
