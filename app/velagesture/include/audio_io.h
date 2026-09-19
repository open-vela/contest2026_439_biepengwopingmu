/****************************************************************************
 * app/velagesture/include/audio_io.h
 *
 * Audio I/O subsystem — half-duplex microphone capture and speaker
 * playback over NuttX audio (I2S / codec) devices.
 *
 * Capture path:  Mic → PCM (16 kHz / 16-bit / mono) → WebSocket → ASR
 * Playback path: TTS PCM ← WebSocket ← Speaker
 *
 * No AEC, no barge-in, no full-duplex synchronisation.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef APP_VELAGESTURE_INCLUDE_AUDIO_IO_H
#define APP_VELAGESTURE_INCLUDE_AUDIO_IO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "velagesture.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VG_AUDIO_DEV_CAPTURE   "/dev/audio/pcm_in0"
#define VG_AUDIO_DEV_PLAYBACK  "/dev/audio/pcm_out0"
#define VG_AUDIO_FRAME_SAMPLES (VG_AUDIO_SAMPLE_RATE * VG_AUDIO_FRAME_MS / 1000)
#define VG_AUDIO_FRAME_BYTES   (VG_AUDIO_FRAME_SAMPLES * (VG_AUDIO_BITS / 8))

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Audio capture callback — invoked for each captured PCM frame. */

typedef void (*vg_audio_capture_cb_t)(const int16_t *pcm, int samples,
                                      void *user_data);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: vg_audio_capture_start
 *
 * Description:
 *   Open the microphone device and start the capture thread.  Each
 *   captured frame is delivered to the callback.
 *
 ****************************************************************************/

int vg_audio_capture_start(vg_audio_capture_cb_t cb, void *user_data);

/****************************************************************************
 * Name: vg_audio_capture_stop
 *
 * Description:
 *   Stop capture and close the device.
 *
 ****************************************************************************/

void vg_audio_capture_stop(void);

/****************************************************************************
 * Name: vg_audio_playback_start
 *
 * Description:
 *   Open the speaker device for playback.  Does not start playing
 *   until vg_audio_playback_write() is called.
 *
 ****************************************************************************/

int vg_audio_playback_start(void);

/****************************************************************************
 * Name: vg_audio_playback_write
 *
 * Description:
 *   Write a buffer of PCM samples to the speaker.  Blocks until the
 *   samples have been accepted by the audio driver.
 *
 ****************************************************************************/

int vg_audio_playback_write(const int16_t *pcm, int samples);

/****************************************************************************
 * Name: vg_audio_playback_stop
 *
 * Description:
 *   Drain the playback buffer and close the device.
 *
 ****************************************************************************/

void vg_audio_playback_stop(void);

#endif /* APP_VELAGESTURE_INCLUDE_AUDIO_IO_H */
