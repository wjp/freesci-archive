/***************************************************************************
 sdl.c Copyright (C) 2002 Solomon Peachy, Claudio Matsuoka,
 			  Christoph Reichenbach

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

#include <sfx_pcm.h>
#include <sfx_audiobuf.h>

#ifdef HAVE_SDL

#ifndef _MSC_VER
#  include <sys/time.h>
#  include <SDL/SDL.h>
#else
#  include <SDL.h>
#endif

#define DELTA_TIME_LIMIT 10000 /* Report errors above this delta time */

static sfx_audio_buf_t audio_buffer;
static void (*sdl_sfx_timer_callback)(void *data);
static void *sdl_sfx_timer_data;
static int sample_size;
static int buf_size;
static int rate;
static long last_callback_secs, last_callback_usecs;
static int last_callback_len;

static sfx_timestamp_t
pcmout_sdl_output_timestamp(sfx_pcm_device_t *self)
{
	/* Number of samples enqueued in the output device: */
	int delta = (buf_size - last_callback_len) / sample_size
		/* Number of samples enqueued in the internal audio buffer: */
		+ audio_buffer.samples_nr;

	return sfx_timestamp_add(sfx_new_timestamp(last_callback_secs,
						   last_callback_usecs,
						   rate),
				 delta);
}

static void
timer_sdl_internal_callback(void *userdata, byte *dest, int len)
{
	sci_gettime(&last_callback_secs, &last_callback_usecs);
	last_callback_len = len;

	if (sdl_sfx_timer_callback)
		sdl_sfx_timer_callback(sdl_sfx_timer_data);

#if 0
	if (!sfx_audbuf_read_timestamp(&audio_buffer, &ts)) {
		int delta = (buf_size - len) / sample_size;
		sfx_timestamp_t real_ts;
		long deltatime;
		long sec2, usec2;
		sci_gettime(&sec2, &usec2);

		real_ts = sfx_timestamp_add(sfx_new_timestamp(sec, usec, rate), delta);

		deltatime = sfx_timestamp_usecs_diff(ts, real_ts);

		fprintf(stderr, "[SDL] Samples requested: %d  Playing %ld too late. Needed %ldus for computations.\n",
				len / sample_size, deltatime,
			(usec2-usec) + (sec2-sec)*1000000);

		if (abs(deltatime) > DELTA_TIME_LIMIT)
			sciprintf("[SND:SDL] Very high delta time for PCM playback: %ld too late (%d samples in the future)\n",
				  deltatime, sfx_timestamp_sample_diff(sfx_new_timestamp(sec, usec, rate), ts));

#if 0
		if (deltatime < 0) {
			/* Read and discard samples, explicitly underrunning */
			int max_read = len / sample_size;
			int samples_to_kill = sfx_timestamp_sample_diff(real_ts, ts);

			while (samples_to_kill) {
				int d = samples_to_kill > max_read? max_read : samples_to_kill;
				sfx_audbuf_read(&audio_buffer, dest, d);
				samples_to_kill -= d;
			}
		}
#endif
	}
#endif
	sfx_audbuf_read(&audio_buffer, dest, len / sample_size);

}


static int
pcmout_sdl_init(sfx_pcm_device_t *self)
{
	SDL_AudioSpec a;

	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_NOPARACHUTE) != 0) {
		fprintf (stderr, "[SND:SDL] Error while initialising: %s\n", SDL_GetError());
		return -1;
	}

	a.freq = 44100; /* FIXME */
#ifdef WORDS_BIGENDIAN
	a.format = AUDIO_S16MSB; /* FIXME */
#else
	a.format = AUDIO_S16LSB; /* FIXME */
#endif
	a.channels = 2; /* FIXME */
	a.samples = 2048; /* FIXME */
	a.callback = timer_sdl_internal_callback;
	a.userdata = NULL;

	if (SDL_OpenAudio (&a, NULL) < 0) {
		fprintf (stderr, "[SND:SDL] Error while opening audio: %s\n", SDL_GetError());
		return SFX_ERROR;
	}

	buf_size = a.samples;
	rate = a.freq;

	self->buf_size = a.samples << 1; /* Looks like they're using double size */
	self->conf.rate = a.freq;
	self->conf.stereo = a.channels > 1;
	self->conf.format = SFX_PCM_FORMAT_S16_NATIVE;

	sample_size = SFX_PCM_SAMPLE_SIZE(self->conf);

	sfx_audbuf_init(&audio_buffer, self->conf); 
	SDL_PauseAudio (0);

	return SFX_OK;
}

int
pcmout_sdl_output(sfx_pcm_device_t *self, byte *buf,
		  int count, sfx_timestamp_t *ts)
{
	if (ts)
		sfx_audbuf_write_timestamp(&audio_buffer, *ts);
	sfx_audbuf_write(&audio_buffer, buf, count);
	return SFX_OK;
}


static int
pcmout_sdl_set_option(sfx_pcm_device_t *self, char *name, char *value)
{
	return SFX_ERROR; /* Option not supported */
}

static void
pcmout_sdl_exit(sfx_pcm_device_t *self)
{
	sfx_audbuf_free(&audio_buffer);
	SDL_PauseAudio (1);
	SDL_CloseAudio();
}


static int
timer_sdl_set_option(char *name, char *value)
{
	return SFX_ERROR;
}


static int
timer_sdl_init(void (*callback)(void *data), void *data)
{
	sdl_sfx_timer_callback = callback;
	sdl_sfx_timer_data = data;


	return SFX_OK;
}

static int
timer_sdl_stop(void)
{
	sdl_sfx_timer_callback = NULL;

	return SFX_OK;
}

#define SDL_PCM_VERSION "0.1"

sfx_timer_t pcmout_sdl_timer = {
	"sdl-pcm-timer",
	SDL_PCM_VERSION,
	0,
	0,
	timer_sdl_set_option,
	timer_sdl_init,
	timer_sdl_stop
};

sfx_pcm_device_t sfx_pcm_driver_sdl = {
	"sdl",
	SDL_PCM_VERSION,
	pcmout_sdl_init,
	pcmout_sdl_exit,
	pcmout_sdl_set_option,
	pcmout_sdl_output,
	pcmout_sdl_output_timestamp,
	{0, 0, 0},
	0,
	&pcmout_sdl_timer,
	NULL
};

#endif
