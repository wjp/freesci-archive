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

#ifdef HAVE_ALSA

#include <alsa/asoundlib.h>

static guint16 *buffer;

static snd_pcm_t *pcm_handle;

static snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
static snd_pcm_access_t pcm_access = SND_PCM_ACCESS_RW_INTERLEAVED;
static snd_pcm_format_t alsa_format = SND_PCM_FORMAT_S16_LE;
static snd_pcm_hw_params_t *hwparams;
static snd_pcm_sw_params_t *swparams;
static snd_async_handler_t *ahandler;
static snd_output_t *output;

static char *alsa_device = "plughw:0,0"; 

static void async_callback(snd_async_handler_t *ahandler)
{
  snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);
  snd_pcm_sframes_t avail;
  int err, count;

  if ((avail = snd_pcm_avail_update(handle)) < 0) {
    printf("ALSA: Buffer underrun\n");
    snd_pcm_prepare(handle);
    avail = snd_pcm_avail_update(handle);
  }

  while (avail >= BUFFER_SIZE) {
    count = mix_sound(BUFFER_SIZE);
    if ((err = snd_pcm_writei(handle, buffer, count)) < 0) {
      printf("ALSA: Write error: %s\n", snd_strerror(err));
      return;
    }
    avail = snd_pcm_avail_update(handle);
    printf("XXXX Fill buffer, %04x, count: %d actual: %d, avail: %d\n", *buffer, count, err, avail);
  }
}

static int pcmout_alsa_open(guint16 *b, guint16 rate) {
  int channels = 2;
  int periods = 4;
  int err, count;

  buffer = b;

  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_sw_params_alloca(&swparams);

  if ((err = snd_output_stdio_attach(&output, stdout, 0)) < 0) {
    printf("Output failed:  %s\n", snd_strerror(err));
    return -1;
  }

  //  if ((err = snd_pcm_open(&pcm_handle, alsa_device, stream, SND_PCM_ASYNC)) < 0) {
  if ((err = snd_pcm_open(&pcm_handle, alsa_device, stream, SND_PCM_NONBLOCK)) < 0) {
  //  if ((err = snd_pcm_open(&pcm_handle, alsa_device, stream, 0)) < 0) {
    printf("ALSA: Playback open error: %s\n", snd_strerror(err));
    return -1;
  }

  if ((err = snd_pcm_hw_params_any(pcm_handle, hwparams)) < 0) {
    printf("ALSA: Can not configure this PCM device.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_access(pcm_handle, hwparams, pcm_access)) < 0) {
    printf("ALSA: Error setting access.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, alsa_format)) < 0) {
    printf("ALSA: Error setting format.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_rate(pcm_handle, hwparams, rate, 0)) < 0) {
    printf("ALSA: Error setting rate.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 2)) < 0) {
    printf("ALSA: Error setting channels.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods, 0)) < 0) {
    printf("ALSA: Error setting periods.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_buffer_size(pcm_handle, hwparams, BUFFER_SIZE*periods)) < 0) {
    printf("ALSA: Error setting buffersize.\n");
    return -1;
  }

  if ((err = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
    printf("ALSA: Error setting HW params.\n");
    return -1;
  }

  /* register callback */
  if ((err = snd_async_add_pcm_handler(&ahandler, pcm_handle, async_callback, NULL)) < 0) {
    printf("ALSA: Unable to register async handler\n");
    return -1;
  }

  /* software interface settings */
  if ((err = snd_pcm_sw_params_current(pcm_handle, swparams)) < 0) {
    printf("ALSA: Unable to determine current swparams for playback\n");
    return -1;
  }

  /*
  if ((err = snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, BUFFER_SIZE*2)) < 0) {
    printf("ALSA: Unable to set start threshold mode for playback\n");
    return -1;
  }
  */
  if ((err = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, BUFFER_SIZE)) < 0) {
    printf("ALSA: Unable to set avail min for playback\n");
    return -1;
  }

  if ((err = snd_pcm_sw_params(pcm_handle, swparams)) < 0) {
    printf("ALSA: Unable to set sw params for playback\n");
    return -1;
  }

  for (count = 0 ; count < periods ; count++) {
    if ((err = snd_pcm_writei(pcm_handle, buffer, BUFFER_SIZE)) < 0) {
      printf("ALSA: Initial write error %d: %s\n", count, snd_strerror(err));
      return -1;
    }
  }

  /*
  if ((err = snd_pcm_prepare(pcm_handle)) < 0) {
    printf("ALSA: Unable to prepare device\n");
    return -1;
  }

  if ((err = snd_pcm_start(pcm_handle)) < 0) {
    printf("ALSA: Unable to start device\n");
    return -1;
  }
  */

  return 0;
}

static void pcmout_alsa_close() {
  int err;
  
  if ((err = snd_pcm_drop(pcm_handle)) < 0) {
    printf("ALSA:  Can't stop PCM device\n");
  }
  if ((err = snd_pcm_close(pcm_handle)) < 0) {
    printf("ALSA:  Can't close PCM device\n");
  }

  snd_pcm_hw_params_free(hwparams);
  snd_pcm_sw_params_free(swparams);
}

pcmout_driver_t pcmout_driver_alsa = {
  "alsa",
  "v0.01",
  NULL,
  &pcmout_alsa_open,
  &pcmout_alsa_close
};

#endif  /* HAVE_ALSA */
