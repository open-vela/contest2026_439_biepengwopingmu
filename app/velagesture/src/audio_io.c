/****************************************************************************
 * app/velagesture/src/audio_io.c
 *
 * Audio I/O — half-duplex microphone capture and speaker playback.
 *
 * Capture:  /dev/audio/pcm_in0 → 16 kHz / 16-bit / mono PCM
 *            → callback → WebSocket → server-side ASR
 *
 * Playback: TTS PCM ← WebSocket ← /dev/audio/pcm_out0
 *
 * No AEC, no barge-in, no full-duplex synchronisation.
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
#include <nuttx/audio/audio.h>

#include "velagesture.h"
#include "audio_io.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct audio_capture_state_s
{
  vg_audio_capture_cb_t callback;
  void                 *user_data;
  pthread_t             thread;
  volatile bool         running;
  int                   fd;
  int16_t              *buf;
};

struct audio_playback_state_s
{
  int   fd;
  bool  active;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct audio_capture_state_s  g_capture;
static struct audio_playback_state_s g_playback;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: capture_thread
 *
 * Description:
 *   Reads PCM frames from the microphone device and delivers them
 *   to the registered callback.
 ****************************************************************************/

static FAR void *capture_thread(FAR void *arg)
{
  struct audio_capture_state_s *ac = (struct audio_capture_state_s *)arg;
  int samples = VG_AUDIO_FRAME_SAMPLES;

  printf("AUDIO: Capture thread started (%d Hz, %d-bit, %d ch)\n",
         VG_AUDIO_SAMPLE_RATE, VG_AUDIO_BITS, VG_AUDIO_CHANNELS);

  while (ac->running)
    {
      ssize_t nread = read(ac->fd, ac->buf,
                           samples * sizeof(int16_t));
      if (nread < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }
          fprintf(stderr, "AUDIO: Capture read error: %d\n", errno);
          break;
        }

      int got_samples = nread / sizeof(int16_t);
      if (got_samples > 0 && ac->callback != NULL)
        {
          ac->callback(ac->buf, got_samples, ac->user_data);
        }
    }

  printf("AUDIO: Capture thread exiting\n");
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vg_audio_capture_start
 ****************************************************************************/

int vg_audio_capture_start(vg_audio_capture_cb_t cb, void *user_data)
{
  int ret;

  memset(&g_capture, 0, sizeof(g_capture));
  g_capture.callback  = cb;
  g_capture.user_data = user_data;
  g_capture.running   = true;

  /* Allocate frame buffer */

  g_capture.buf = malloc(VG_AUDIO_FRAME_BYTES);
  if (g_capture.buf == NULL)
    {
      return -ENOMEM;
    }

  /* Open microphone device */

  g_capture.fd = open(VG_AUDIO_DEV_CAPTURE, O_RDONLY);
  if (g_capture.fd < 0)
    {
      fprintf(stderr, "AUDIO: Cannot open %s: %d\n",
              VG_AUDIO_DEV_CAPTURE, errno);
      free(g_capture.buf);
      return -errno;
    }

  /* Configure audio format — the NuttX audio driver is configured
   * via ioctl calls.  The exact sequence depends on the codec
   * driver on the P4 board.  The defaults (set by Kconfig for the
   * board) are expected to be 16 kHz / 16-bit / mono. */

  /* Start capture thread */

  ret = pthread_create(&g_capture.thread, NULL, capture_thread,
                       &g_capture);
  if (ret != 0)
    {
      close(g_capture.fd);
      free(g_capture.buf);
      return -ret;
    }

  return 0;
}

/****************************************************************************
 * Name: vg_audio_capture_stop
 ****************************************************************************/

void vg_audio_capture_stop(void)
{
  g_capture.running = false;
  pthread_join(g_capture.thread, NULL);

  if (g_capture.fd >= 0)
    {
      close(g_capture.fd);
    }

  free(g_capture.buf);
}

/****************************************************************************
 * Name: vg_audio_playback_start
 ****************************************************************************/

int vg_audio_playback_start(void)
{
  memset(&g_playback, 0, sizeof(g_playback));

  g_playback.fd = open(VG_AUDIO_DEV_PLAYBACK, O_WRONLY);
  if (g_playback.fd < 0)
    {
      fprintf(stderr, "AUDIO: Cannot open %s: %d\n",
              VG_AUDIO_DEV_PLAYBACK, errno);
      return -errno;
    }

  g_playback.active = true;
  return 0;
}

/****************************************************************************
 * Name: vg_audio_playback_write
 ****************************************************************************/

int vg_audio_playback_write(const int16_t *pcm, int samples)
{
  if (!g_playback.active || g_playback.fd < 0)
    {
      return -ENODEV;
    }

  size_t bytes = samples * sizeof(int16_t);
  ssize_t nwritten = write(g_playback.fd, pcm, bytes);
  if (nwritten < 0)
    {
      return -errno;
    }

  return (int)(nwritten / sizeof(int16_t));
}

/****************************************************************************
 * Name: vg_audio_playback_stop
 ****************************************************************************/

void vg_audio_playback_stop(void)
{
  if (g_playback.fd >= 0)
    {
      close(g_playback.fd);
    }

  g_playback.active = false;
}
