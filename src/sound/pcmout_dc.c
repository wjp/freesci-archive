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
#include <stream.h>
#include <pcmout.h>

static gint16 *buffer;
static kthread_t *thread;

int pcm_run;
static guint16 pcm_rate;
static guint8 pcm_stereo;
static guint16 pcm_buffer_size;

/* Sndstream buffer time in seconds. */
#define DC_SNDSTREAM_BUF_TIME 0.3f

static void
*send_audio(int size, int * size_out) 
{
	int count;
	count = mix_sound(pcm_buffer_size) << (pcm_stereo? 2:1);
	if (count > size)
		*size_out = size;
	else
		*size_out = count;
	return buffer;
}

static void
pcm_thread(void *p)
{
	while (pcm_run)
	{
		snd_stream_poll();
		thd_pass();
	}
}

static int
pcmout_dc_open(gint16 *b, guint16 buffer_size, guint16 rate, guint8 stereo)
{
	int callback_chunk = buffer_size << (stereo? 2 : 1);
	buffer = b;
	pcm_run = 1;
	pcm_rate = rate;
	pcm_stereo = stereo;
	pcm_buffer_size = buffer_size;
	snd_stream_init(send_audio, (int)((rate << 1) *
		DC_SNDSTREAM_BUF_TIME) / callback_chunk * callback_chunk,
		buffer_size);
	snd_stream_start(rate, stereo);
	thread = thd_create((void *) pcm_thread, NULL);
	return 0;
}

static int
pcmout_dc_close()
{
	pcm_run = 0;
	if (thread)
		thd_wait(thread);
	snd_stream_stop();
	snd_stream_shutdown();

	return 0;
}

int
pcmout_dc_suspend()
{
	pcm_run = 0;
	thd_wait(thread);
	thread = NULL;
	snd_stream_stop();
	return 0;
}

int
pcmout_dc_resume()
{
	pcm_run = 1;
	snd_stream_start(pcm_rate, pcm_stereo);
	thread = thd_create((void *) pcm_thread, NULL);
	return 0;
}

pcmout_driver_t pcmout_driver_dc =
{
	"dc",
	"0.2",
	NULL,
	&pcmout_dc_open,
	&pcmout_dc_close
};
