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

static sfx_audio_buf_t audio_buffer;
static void (*sdl_sfx_timer_callback)(void *data);
static void *sdl_sfx_timer_data;
static int sample_size;


static void
timer_sdl_internal_callback(void *userdata, byte *dest, int len)
{
	if (sdl_sfx_timer_callback)
		sdl_sfx_timer_callback(sdl_sfx_timer_data);

	sfx_audbuf_read(&audio_buffer, dest, sample_size, len / sample_size);
}


static int
pcmout_sdl_init(sfx_pcm_device_t *self)
{
	SDL_AudioSpec a;
	sfx_audbuf_init(&audio_buffer);

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

	self->buf_size = a.samples << 1; /* Looks like they're using double size */
	self->conf.rate = a.freq;
	self->conf.stereo = a.channels > 1;
	self->conf.format = SFX_PCM_FORMAT_S16_NATIVE;

	sample_size = SFX_PCM_SAMPLE_SIZE(self->conf);

	SDL_PauseAudio (0);

	sfx_audbuf_init(&audio_buffer);

	return SFX_OK;
}

int
pcmout_sdl_output(sfx_pcm_device_t *self, byte *buf, int count)
{
	sfx_audbuf_write(&audio_buffer, buf, SFX_PCM_SAMPLE_SIZE(self->conf), count);
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
	{0, 0, 0},
	0,
	&pcmout_sdl_timer,
	NULL
};

#endif
