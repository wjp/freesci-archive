/***************************************************************************
 alsa.c Copyright (C) 2004 Walter van Niftrik


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


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Walter van Niftrik <w.f.b.w.v.niftrik@stud.tue.nl>

***************************************************************************/

/* Based on ALSA's pcm.c example. */

#include <sfx_pcm.h>
#include <sfx_audiobuf.h>

#ifdef HAVE_ALSA

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#include <alsa/asoundlib.h>

static const char *device = "hw:0,0"; /* FIXME: Try "default" first */ /* Or "plughw:0,0" */
static snd_pcm_format_t format = SND_PCM_FORMAT_S16;
static unsigned int rate = 44100; /* FIXME */
static unsigned int channels = 2; /* FIXME */
static unsigned int buffer_time = 500000; /* FIXME */
static unsigned int period_time = 100000; /* FIXME */

static snd_pcm_sframes_t buffer_size;
static snd_pcm_sframes_t period_size;

static int frame_size;
static long last_callback_secs, last_callback_usecs;
static int last_callback_len;

static sfx_audio_buf_t audio_buffer;
static void (*alsa_sfx_timer_callback)(void *data);
static void *alsa_sfx_timer_data;

static snd_pcm_t *handle;

static snd_async_handler_t *ahandler;

static snd_pcm_hw_params_t *hwparams;
static snd_pcm_sw_params_t *swparams;

static int
xrun_recovery(snd_pcm_t *handle, int err)
{
	if (err == -EPIPE) {	/* under-run */
		err = snd_pcm_prepare(handle);
		if (err < 0)
			printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
		return 0;
	} else if (err == -ESTRPIPE) {
		while ((err = snd_pcm_resume(handle)) == -EAGAIN)
			sleep(1);	/* wait until the suspend flag is released */
		if (err < 0) {
			err = snd_pcm_prepare(handle);
			if (err < 0)
				printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
		}
		return 0;
	}
	return err;
}

static sfx_timestamp_t
pcmout_alsa_output_timestamp(sfx_pcm_device_t *self)
{
	/* Number of frames enqueued in the output device: */
	int delta = (buffer_size - last_callback_len) / frame_size
		/* Number of frames enqueued in the internal audio buffer: */
		+ audio_buffer.frames_nr;

	return sfx_timestamp_add(sfx_new_timestamp(last_callback_secs,
						   last_callback_usecs,
						   rate),
				 delta);
}

static void
async_direct_callback(snd_async_handler_t *ahandler)
{
	const snd_pcm_channel_area_t *my_areas;
	snd_pcm_uframes_t offset, frames, size;
	snd_pcm_sframes_t avail, commitres;
	snd_pcm_state_t state;
	int first = 0, err;

	sci_gettime(&last_callback_secs, &last_callback_usecs);

	while (1) {
		state = snd_pcm_state(handle);
		if (state == SND_PCM_STATE_XRUN) {
			err = xrun_recovery(handle, -EPIPE);
			if (err < 0) {
				sciprintf("[SND:ALSA] XRUN recovery failed: %s\n", snd_strerror(err));
			}
			first = 1;
		} else if (state == SND_PCM_STATE_SUSPENDED) {
			err = xrun_recovery(handle, -ESTRPIPE);
			if (err < 0) {
				sciprintf("[SND:ALSA] SUSPEND recovery failed: %s\n", snd_strerror(err));
			}
		}

		avail = snd_pcm_avail_update(handle);

		last_callback_len = avail;

		if (alsa_sfx_timer_callback)
			alsa_sfx_timer_callback(alsa_sfx_timer_data);

		if (avail < 0) {
			err = xrun_recovery(handle, avail);
			if (err < 0) {
				sciprintf("[SND:ALSA] avail update failed: %s\n", snd_strerror(err));
			}
			first = 1;
			continue;
		}
		if (avail < period_size) {
			if (first) {
				first = 0;
				err = snd_pcm_start(handle);
				if (err < 0) {
					sciprintf("[SND:ALSA] Start error: %s\n", snd_strerror(err));
				}
			} else {
				break;
			}
			continue;
		}
		size = period_size;
		while (size > 0) {
			frames = size;
			err = snd_pcm_mmap_begin(handle, &my_areas, &offset, &frames);
			if (err < 0) {
				if ((err = xrun_recovery(handle, err)) < 0) {
					sciprintf("[SND:ALSA] MMAP begin avail error: %s\n", snd_strerror(err));
				}
				first = 1;
			}
			sfx_audbuf_read(&audio_buffer, (unsigned char*)((int)my_areas->addr + offset*frame_size), frames);
			commitres = snd_pcm_mmap_commit(handle, offset, frames);
			if (commitres < 0 || commitres != frames) {
				if ((err = xrun_recovery(handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
					sciprintf("[SND:ALSA] MMAP commit error: %s\n", snd_strerror(err));
				}
				first = 1;
			}
			size -= frames;
		}
	}
}

static int
pcmout_alsa_init(sfx_pcm_device_t *self)
{
	unsigned int rrate;
	int err, count, dir;

	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_sw_params_alloca(&swparams);

	err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		sciprintf("[SND:ALSA] Playback open error: %s\n", snd_strerror(err));
		return 0;
	}
	err = snd_pcm_hw_params_any(handle, hwparams);
	if (err < 0) {
		sciprintf("[SND:ALSA] Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}
	err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_MMAP_INTERLEAVED);
	if (err < 0) {
		sciprintf("[SND:ALSA] Access type not available for playback: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}
	err = snd_pcm_hw_params_set_format(handle, hwparams, format);
	if (err < 0) {
		sciprintf("[SND:ALSA] Sample format not available for playback: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}
	err = snd_pcm_hw_params_set_channels(handle, hwparams, channels);
	if (err < 0) {
		sciprintf("[SND:ALSA] Channels count (%i) not available for playback: %s\n", channels, snd_strerror(err));
		return SFX_ERROR;
	}
	rrate = rate;
	err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &rrate, 0);
	if (err < 0) {
		sciprintf("[SND:ALSA] Rate %iHz not available for playback: %s\n", rate, snd_strerror(err));
		return SFX_ERROR;
	}
	if (rrate != rate) {
		sciprintf("[SND:ALSA] Rate doesn't match (requested %iHz, get %iHz)\n", rate, err);
		return SFX_ERROR;
	}
	err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, &dir);
	if (err < 0) {
		sciprintf("[SND:ALSA] Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
		return SFX_ERROR;
	}
	err = snd_pcm_hw_params_get_buffer_size(hwparams, (snd_pcm_uframes_t*)&buffer_size);
	if (err < 0) {
		sciprintf("[SND:ALSA] Unable to get buffer size for playback: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}
	err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, &dir);
	if (err < 0) {
		sciprintf("[SND:ALSA] Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
		return SFX_ERROR;
	}
	err = snd_pcm_hw_params_get_period_size(hwparams, (snd_pcm_uframes_t*)&period_size, &dir);
	if (err < 0) {
		sciprintf("[SND:ALSA] Unable to get period size for playback: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}
	if (period_size >= buffer_size) {
		sciprintf("[SND:ALSA] Period size %i matches or exceeds buffer size %i\n", period_size, buffer_size);
		return SFX_ERROR;
	}
	err = snd_pcm_hw_params(handle, hwparams);
	if (err < 0) {
		sciprintf("[SND:ALSA] Unable to set hw params for playback: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}
	err = snd_pcm_sw_params_current(handle, swparams);
	if (err < 0) {
		sciprintf("[SND:ALSA] Unable to determine current swparams for playback: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, buffer_size);
	if (err < 0) {
		sciprintf("[SND:ALSA] Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}
	err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
	if (err < 0) {
		sciprintf("[SND:ALSA] Unable to set avail min for playback: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}
	err = snd_pcm_sw_params_set_xfer_align(handle, swparams, 1);
	if (err < 0) {
		sciprintf("[SND:ALSA] Unable to set transfer align for playback: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}
	err = snd_pcm_sw_params(handle, swparams);
	if (err < 0) {
		sciprintf("[SND:ALSA] Unable to set sw params for playback: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}

	self->buf_size = buffer_size;
	self->conf.rate = rate;
	self->conf.stereo = channels > 1;
	self->conf.format = SFX_PCM_FORMAT_S16_NATIVE;

	frame_size = SFX_PCM_FRAME_SIZE(self->conf);

	sfx_audbuf_init(&audio_buffer, self->conf);

	err = snd_async_add_pcm_handler(&ahandler, handle, async_direct_callback, NULL);
	if (err < 0) {
		sciprintf("[SND:ALSA] Unable to register async handler\n");
		return SFX_ERROR;
	}

	for (count = 0; count < 2; count++) {
		const snd_pcm_channel_area_t *my_areas;
		snd_pcm_uframes_t offset, frames, size;
		size = period_size;
		snd_pcm_sframes_t commitres;
		while (size > 0) {
			frames = size;
			err = snd_pcm_mmap_begin(handle, &my_areas, &offset, &frames);
			if (err < 0) {
				if ((err = xrun_recovery(handle, err)) < 0) {
					sciprintf("[SND:ALSA] MMAP begin avail error: %s\n", snd_strerror(err));
					return SFX_ERROR;
				}
			}
			sfx_audbuf_read(&audio_buffer, (byte *)((int)my_areas->addr + offset*frame_size), frames);
			commitres = snd_pcm_mmap_commit(handle, offset, frames);
			if (commitres < 0 || commitres != frames) {
				if ((err = xrun_recovery(handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
					sciprintf("[SND:ALSA] MMAP commit error: %s\n", snd_strerror(err));
					return SFX_ERROR;
				}
			}
			size -= frames;
		}
	}

	err = snd_pcm_start(handle);
	if (err < 0) {
		sciprintf("[SND:ALSA] Start error: %s\n", snd_strerror(err));
		return SFX_ERROR;
	}

	return SFX_OK;
}

static int
pcmout_alsa_output(sfx_pcm_device_t *self, byte *buf,
		  int count, sfx_timestamp_t *ts)
{
	if (ts)
		sfx_audbuf_write_timestamp(&audio_buffer, *ts);
	sfx_audbuf_write(&audio_buffer, buf, count);
	return SFX_OK;
}

static int
pcmout_alsa_set_option(sfx_pcm_device_t *self, char *name, char *value)
{
	return SFX_ERROR;
}

static void
pcmout_alsa_exit(sfx_pcm_device_t *self)
{
	int err;
	sfx_audbuf_free(&audio_buffer);
	if ((err = snd_pcm_drop(handle)) < 0) {
		sciprintf("[SND:ALSA] Can't stop PCM device: %s\n", snd_strerror(err));
	}
	if ((err = snd_pcm_close(handle)) < 0) {
		sciprintf("[SND:ALSA] Can't close PCM device: %s\n", snd_strerror(err));
	}
	snd_pcm_hw_params_free(hwparams);
	snd_pcm_sw_params_free(swparams);
}

static int
timer_alsa_set_option(char *name, char *value)
{
	return SFX_ERROR;
}


static int
timer_alsa_init(void (*callback)(void *data), void *data)
{
	alsa_sfx_timer_callback = callback;
	alsa_sfx_timer_data = data;

	return SFX_OK;
}

static int
timer_alsa_stop(void)
{
	alsa_sfx_timer_callback = NULL;

	return SFX_OK;
}

#define ALSA_PCM_VERSION "0.1"

sfx_timer_t pcmout_alsa_timer = {
	"alsa-pcm-timer",
	ALSA_PCM_VERSION,
	0,
	0,
	timer_alsa_set_option,
	timer_alsa_init,
	timer_alsa_stop
};

sfx_pcm_device_t sfx_pcm_driver_alsa = {
	"alsa",
	ALSA_PCM_VERSION,
	pcmout_alsa_init,
	pcmout_alsa_exit,
	pcmout_alsa_set_option,
	pcmout_alsa_output,
	pcmout_alsa_output_timestamp,
	{0, 0, 0},
	0,
	&pcmout_alsa_timer,
	NULL
};

#endif /* HAVE_ALSA */
