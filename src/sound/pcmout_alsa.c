/***************************************************************************
 pcmout_sdl.c Copyright (C) 2002 Solomon Peachy (And Claudio Matsuoka)

 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantibility,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.

***************************************************************************/

#include <pcmout.h>

#ifdef SUPPORT_ALSA_PCM

#include <pthread.h>

static pthread_t thread;
static int run = 1;

static gint16 *buffer;

static snd_pcm_t *pcm_handle;

static snd_pcm_hw_params_t *hwparams;
static snd_output_t *output;

static char *alsa_device = "hw:0,0"; 

static void *sound_thread (void *arg)
{
  int count, err;

  while(run) {
    count = mix_sound(BUFFER_SIZE);
    //    printf("XXXX Fill buffer, %04x, count: %d \n", *buffer, count);
    if ((err = snd_pcm_writei(pcm_handle, buffer, count)) < 0) {
      snd_pcm_prepare(pcm_handle);
      printf("ALSA: Write error: %s\n", snd_strerror(err));
    }
  }
  pthread_exit(0);
}

static int pcmout_alsa_open(gint16 *b, guint16 rate, guint8 stereo) 
{
  int channels = (stereo) ? 2 : 1;
  int periods = 8;
  int err;
  snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
  snd_pcm_access_t pcm_access = SND_PCM_ACCESS_RW_INTERLEAVED;
#ifdef WORDS_BIGENDIAN
  snd_pcm_format_t alsa_format = SND_PCM_FORMAT_S16_BE;
#else
  snd_pcm_format_t alsa_format = SND_PCM_FORMAT_S16_LE;
#endif

  buffer = b;

  snd_pcm_hw_params_alloca(&hwparams);

  if ((err = snd_output_stdio_attach(&output, stdout, 0)) < 0) {
    printf("Output failed:  %s\n", snd_strerror(err));
    return -1;
  }

  if ((err = snd_pcm_open(&pcm_handle, alsa_device, stream, 0)) < 0) {
    printf("ALSA: Playback open error: %s\n", snd_strerror(err));
    return -1;
  }

  if ((err = snd_pcm_hw_params_any(pcm_handle, hwparams)) < 0) {
    printf("ALSA: Can not configure this PCM device.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_access(pcm_handle, hwparams, pcm_access)) < 0) {
    printf("ALSA: Error setting access.\n");    return -1;
  }

  if ((err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, alsa_format)) < 0) {
    printf("ALSA: Error setting format.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_rate(pcm_handle, hwparams, rate, 0)) < 0) {
    printf("ALSA: Error setting rate.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, channels)) < 0) {
    printf("ALSA: Error setting channels.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods, 0)) < 0) {
    printf("ALSA: Error setting periods.\n");
    return -1;
  }

  /* buffer size, in frames */
  if ((err = snd_pcm_hw_params_set_buffer_size(pcm_handle, hwparams, BUFFER_SIZE*periods)>>2) < 0) {
    printf("ALSA: Error setting buffersize.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
    printf("ALSA: Error setting HW params.\n");
    return -1;
  }

#if 0
  snd_pcm_dump(pcm_handle, output);
#endif

  pthread_create (&thread, NULL, sound_thread, NULL);
  pthread_detach (thread);

  return 0;
}

static int pcmout_alsa_close() {
  int err;
  
  run = 0;

  pthread_join(thread, NULL);

  if ((err = snd_pcm_drop(pcm_handle)) < 0) {
    printf("ALSA:  Can't stop PCM device\n");
  }
  if ((err = snd_pcm_close(pcm_handle)) < 0) {
    printf("ALSA:  Can't close PCM device\n");
  }

  snd_pcm_hw_params_free(hwparams);
  return 0;
}

pcmout_driver_t pcmout_driver_alsa = {
  "alsa",
  "v0.01",
  NULL,
  &pcmout_alsa_open,
  &pcmout_alsa_close
};

#endif  /* SUPPORT_ALSA_PCM */
