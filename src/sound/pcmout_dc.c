/***************************************************************************
 pcmout_dc.c Copyright (C) 2002 Walter van Niftrik

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

#include <kos/thread.h>
#include <dc/sound/stream.h>
#include <pcmout.h>

static gint16 *buffer;

int pcm_run;
int thread_exit;

static void *send_audio(int size, int * size_out) 
{
	static int callback_nr = 0;

	/* The first two callbacks we need to supply the requested amount of
	** bytes. This is done by return a pointer to a buffer of the requested
	** size containing just zeroes
	*/

	switch(callback_nr)
	{
		static uint8 *buf;
		case 0:	
			{
				int i;
				buf = sci_malloc(size);
				for (i = 0; i < size; i++) buf[i] = 0x00;
			}
		case 1:
			callback_nr++;
			*size_out = size;
			return buf;
		case 2:
			free(buf);
			callback_nr++;
		case 3:
			{
				int count = mix_sound(BUFFER_SIZE) << (pcmout_stereo? 2:1);
				if (count > size)
					*size_out = size;
				else
					*size_out = count;
				return buffer;
			}
	}

	/* Gets rid of warning */
	return 0;
}

static void pcm_thread(void *p)
{
	while (pcm_run)
	{
		snd_stream_poll();
		thd_pass();
	}
	thread_exit = 1;
}

static int pcmout_dc_open(gint16 *b, guint16 rate, guint8 stereo)
{
	buffer = b;
	pcm_run = 1;
	thread_exit = 0;
	snd_stream_init(send_audio);
	snd_stream_start(rate, stereo);
	thd_create((void *) pcm_thread, NULL);
	return 0;
}

static int pcmout_dc_close()
{
	pcm_run = 0;
	while (!thread_exit)
		thd_pass();
	snd_stream_stop();
	snd_stream_shutdown();
	return 0;
}

pcmout_driver_t pcmout_driver_dc =
{
	"dc",
	"0.1",
	NULL,
	&pcmout_dc_open,
	&pcmout_dc_close
};
