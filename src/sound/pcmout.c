/***************************************************************************
 pcmout.c Copyright (C) 2002 Solomon Peachy

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

pcmout_driver_t *pcmout_driver = NULL;

pcmout_driver_t *pcmout_drivers[] = {
#ifndef NO_PCMOUT
#	ifdef SUPPORT_ALSA_PCM
		&pcmout_driver_alsa,
#	endif
#	if defined(HAVE_SYS_SOUNDCARD_H) && defined(HAVE_PTHREADS)
		&pcmout_driver_oss,
#	endif
#	ifdef HAVE_SDL
		&pcmout_driver_sdl,
#	endif
#	if defined(HAVE_DMEDIA_AUDIO_H) && defined(HAVE_PTHREADS)
		&pcmout_driver_al,
#	endif
#	ifdef _DREAMCAST
		&pcmout_driver_dc,
#	endif
#	ifdef _GP32
		&pcmout_driver_gp32,
#	endif
#endif
	&pcmout_driver_null,
	NULL
};

static gint16 *snd_buffer = NULL;
guint16 pcmout_sample_rate = 44100;
guint8  pcmout_stereo = 1;
guint16 pcmout_buffer_size;
static synth_mixer_func_t synth_mixer = NULL;

int pcmout_open()
{
	snd_buffer = sci_calloc(4, pcmout_buffer_size);
	memset(snd_buffer, 0x00, pcmout_buffer_size * 4);
	return pcmout_driver->pcmout_open(snd_buffer, pcmout_buffer_size,
					  pcmout_sample_rate, pcmout_stereo);
}

int pcmout_close(void)
{
  int retval = pcmout_driver->pcmout_close();

  if (snd_buffer)
	  sci_free(snd_buffer);

  return retval;
}

/* returns # of frames, not bytes */
int mix_sound(int count)
{
	int i;

	if (count > pcmout_buffer_size)
		count = pcmout_buffer_size;

	if (synth_mixer)
		i = synth_mixer(snd_buffer, count);
	else return count;

	if (i <= 0)
		i = count;

	return i;
}

void pcmout_set_mixer(synth_mixer_func_t func)
{
	synth_mixer = func;
}

/* the pcmout_null sound driver */

int pcmout_null_open(gint16 *b, guint16 buffer_size, guint16 rate, guint8 stereo)
{
	printf("Opened null pcm device\n");
	return 0;
}

int pcmout_null_close(void)
{
	printf("Closed null pcm device\n");
	return 0;
}

pcmout_driver_t pcmout_driver_null = {
	"null",
	"v0.01",
	NULL,
	&pcmout_null_open,
	&pcmout_null_close
};

struct _pcmout_driver *pcmout_find_driver(char *name)
{
        int retval = 0;

        if (!name) { /* Find default driver */
	  return pcmout_drivers[0];
        }

        while (pcmout_drivers[retval] &&
	       strcasecmp(name, pcmout_drivers[retval]->name))
		retval++;

        return pcmout_drivers[retval];
}


void
pcmout_disable(void)
{
	pcmout_driver = &pcmout_driver_null;
}
