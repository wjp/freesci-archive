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
#ifdef HAVE_ALSA
        &pcmout_driver_alsa,
#endif
#ifdef HAVE_SDL
	&pcmout_driver_sdl,
#endif
	&pcmout_driver_null,
	NULL
};

static guint16 *snd_buffer = NULL;
guint16 pcmout_sample_rate = 44100;

int pcmout_open()
{
	snd_buffer = sci_calloc(4, BUFFER_SIZE);
	memset(snd_buffer, 0x00, BUFFER_SIZE * 4);
	return pcmout_driver->pcmout_open(snd_buffer, pcmout_sample_rate);
}

int pcmout_close(void)
{
  int retval = pcmout_driver->pcmout_close();

  if (snd_buffer)
    sci_free(snd_buffer);

  return retval;
}

int synth_mixer (void* tmp_bk, int samples);

/* returns # of frames, not bytes */
int mix_sound(int count)
{
	return synth_mixer(snd_buffer, count);
}

/* the pcmout_null sound driver */

int pcmout_null_open(guint16 *b, guint16 rate)
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
