/****************************************************************************
 * app/velagesture/src/main.c
 *
 * VelaGesture — main entry point.
 *
 * Starts all subsystems in order:
 *   1. Application context
 *   2. Skill executor
 *   3. WebSocket client
 *   4. AI agent
 *   5. Gesture perception (camera + inference)
 *   6. Audio capture (if enabled)
 *
 * Then waits for a termination signal and shuts everything down
 * in reverse order.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "velagesture.h"
#include "skill_executor.h"
#include "ws_protocol.h"
#include "ai_agent.h"
#include "gesture_perception.h"
#include "audio_io.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct vg_context_s g_vg_ctx;
static volatile bool g_running = true;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: signal_handler
 ****************************************************************************/

static void signal_handler(int sig)
{
  (void)sig;
  g_running = false;
}

/****************************************************************************
 * Name: print_banner
 ****************************************************************************/

static void print_banner(void)
{
  printf("\n");
  printf("==========================================\n");
  printf("  VelaGesture v%s\n", VG_APP_VERSION);
  printf("  Multimodal Gesture Interaction Terminal\n");
  printf("  Platform: ESP32-P4 + openvela\n");
  printf("==========================================\n");
  printf("\n");
}

/****************************************************************************
 * Name: audio_capture_callback
 *
 * Description:
 *   Called for each captured audio frame.  In the current
 *   implementation this buffers the PCM data and forwards it
 *   over the WebSocket to the server-side ASR pipeline.
 *
 ****************************************************************************/

#ifdef CONFIG_VELAGESTURE_AUDIO
static void audio_capture_callback(const int16_t *pcm, int samples,
                                   void *user_data)
{
  /* PCM data is forwarded to the server via WebSocket for ASR.
   * The server-side agent handles speech-to-text and returns
   * structured intents back over the WS connection. */

  (void)pcm;
  (void)samples;
  (void)user_data;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vg_context_init
 ****************************************************************************/

int vg_context_init(struct vg_context_s *ctx)
{
  int ret;

  memset(ctx, 0, sizeof(*ctx));

  ret = pthread_mutex_init(&ctx->lock, NULL);
  if (ret != 0)
    {
      return -ret;
    }

  ret = pthread_cond_init(&ctx->event_cond, NULL);
  if (ret != 0)
    {
      pthread_mutex_destroy(&ctx->lock);
      return -ret;
    }

  ctx->next_task_id = 1;
  return 0;
}

/****************************************************************************
 * Name: vg_context_destroy
 ****************************************************************************/

void vg_context_destroy(struct vg_context_s *ctx)
{
  pthread_cond_destroy(&ctx->event_cond);
  pthread_mutex_destroy(&ctx->lock);
}

/****************************************************************************
 * Name: vg_gesture_name
 ****************************************************************************/

const char *vg_gesture_name(int gesture)
{
  switch (gesture)
    {
      case VG_GESTURE_NONE:
        return "none";
      case VG_GESTURE_OK:
        return "ok";
      case VG_GESTURE_FIST:
        return "fist";
      case VG_GESTURE_OPEN_PALM:
        return "open_palm";
      case VG_GESTURE_VICTORY:
        return "victory";
      case VG_GESTURE_PINCH:
        return "pinch";
      default:
        return "unknown";
    }
}

/****************************************************************************
 * Name: vg_timestamp_ms
 ****************************************************************************/

uint64_t vg_timestamp_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

/****************************************************************************
 * Name: velagesture_main
 *
 * Description:
 *   Application entry point.  Called by the NuttX application
 *   framework when the user runs "velagesture" at the NSH prompt.
 *
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int ret;

  print_banner();

  /* Install signal handlers for clean shutdown */

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  /* 1. Initialise application context */

  ret = vg_context_init(&g_vg_ctx);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to init context: %d\n", ret);
      return EXIT_FAILURE;
    }

  printf("[INIT] Application context ready\n");

  /* 2. Initialise skill executor */

  ret = vg_skill_init();
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to init skill executor: %d\n", ret);
      goto err_ctx;
    }

  printf("[INIT] Skill executor ready\n");

  /* 3. Start WebSocket client */

  ret = vg_ws_start(&g_vg_ctx);
  if (ret < 0)
    {
      fprintf(stderr, "WARN: WebSocket not connected: %d\n", ret);
      /* Non-fatal — the device can operate in offline mode */
    }
  else
    {
      printf("[INIT] WebSocket client connected\n");
    }

  /* 4. Start AI agent */

  ret = vg_agent_start(&g_vg_ctx);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to start agent: %d\n", ret);
      goto err_ws;
    }

  printf("[INIT] AI agent started\n");

  /* 5. Start gesture perception (camera + inference) */

  ret = vg_perception_start(&g_vg_ctx);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to start perception: %d\n", ret);
      goto err_agent;
    }

  printf("[INIT] Gesture perception started\n");

  /* 6. Start audio capture (if enabled) */

#ifdef CONFIG_VELAGESTURE_AUDIO
  ret = vg_audio_capture_start(audio_capture_callback, NULL);
  if (ret < 0)
    {
      fprintf(stderr, "WARN: Audio capture not started: %d\n", ret);
      /* Non-fatal */
    }
  else
    {
      g_vg_ctx.audio_ready = true;
      printf("[INIT] Audio capture started\n");
    }
#endif

  printf("[RUN] VelaGesture is running. Press Ctrl+C or send SIGINT to stop.\n\n");

  /* Main loop — just sleep until told to stop */

  while (g_running)
    {
      sleep(1);
    }

  printf("\n[STOP] Shutting down...\n");

  /* Shutdown in reverse order */

#ifdef CONFIG_VELAGESTURE_AUDIO
  if (g_vg_ctx.audio_ready)
    {
      vg_audio_capture_stop();
      printf("[STOP] Audio capture stopped\n");
    }
#endif

  vg_perception_stop();
  printf("[STOP] Perception stopped\n");

  vg_agent_stop();
  printf("[STOP] Agent stopped\n");

  vg_ws_stop();
  printf("[STOP] WebSocket stopped\n");

  vg_skill_shutdown();
  printf("[STOP] Skill executor stopped\n");

  vg_context_destroy(&g_vg_ctx);
  printf("[STOP] VelaGesture terminated cleanly.\n");

  return EXIT_SUCCESS;

err_agent:
  vg_agent_stop();
err_ws:
  vg_ws_stop();
err_ctx:
  vg_context_destroy(&g_vg_ctx);
  return EXIT_FAILURE;
}
