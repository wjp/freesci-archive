/***************************************************************************
 polled_ss_dc.c Copyright (C) 2002 Walter van Niftrik

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

 Adapted from polled_ss_sdl.c by Solomon Peachy.

***************************************************************************/

#include <sci_memory.h>
#include <engine.h>
#include <soundserver.h>
#include <sound.h>
#include <pcmout.h>
#include <kos/thread.h>
#include <kos/sem.h>
#include <kos/cond.h>

static kthread_t *child;
static semaphore_t *out_mutex;
static semaphore_t *in_mutex;
static condvar_t *in_cond;

static kthread_t *master;

static int dc_reverse_stereo = 0;

extern sound_server_t sound_server_dc;

static semaphore_t *bulk_mutices[2];
static sci_queue_t bulk_queues[2];
static condvar_t *bulk_conds[2];

static sound_eq_t inqueue; /* The in-event queue */
static sound_eq_t ev_queue; /* The event queue */

int
dc_soundserver_init(void *args)
{
	sound_server_state_t sss;
	memset(&sss, 0, sizeof(sound_server_state_t));
	sci0_polled_ss(dc_reverse_stereo, &sss);
	return 0;
}

int
sound_dc_init(state_t *s, int flags)
{
	int i;
	global_sound_server = &sound_server_dc;
	sci_sched_yield();
	master = thd_get_current();
	if (pcmout_open() < 0)
		return -1;
	if (init_midi_device(s) < 0)
		return -1;

	/* spawn thread */

	out_mutex = sem_create(1);
	in_mutex = sem_create(1);
	in_cond = cond_create();

	for (i = 0; i < 2; i++) {
		bulk_conds[i] = cond_create();
		sci_init_queue(&(bulk_queues[i]));
		bulk_mutices[i] = sem_create(1);
	}

	debug_stream = stderr;

	sound_eq_init(&inqueue);
	sound_eq_init(&ev_queue);

	child = thd_create((void *) dc_soundserver_init, s);

	return 0;
}

int
sound_dc_configure(state_t *s, char *option, char *value)
{
	return 1; /* No options apply to this driver */
}

sound_event_t *
sound_dc_get_event(state_t *s)
{
	sound_event_t *event = NULL;
	sem_wait(out_mutex);

	if (!sound_eq_peek_event(&ev_queue))
	{
		sem_signal(out_mutex);
		return NULL;
	}

	event = sound_eq_retreive_event(&ev_queue);
	sem_signal(out_mutex);
	return event;
}

void
sound_dc_queue_event(unsigned int handle, unsigned int signal, long value)
{
	sem_wait(out_mutex);
	sound_eq_queue_event(&ev_queue, handle, signal, value);
	sem_signal(out_mutex);
}

void
sound_dc_queue_command(unsigned int handle, unsigned int signal, long value)
{
	sem_wait(in_mutex);
	sound_eq_queue_event(&inqueue, handle, signal, value);
	sem_signal(in_mutex);
	cond_signal(in_cond);
}

sound_event_t *
sound_dc_get_command(GTimeVal *wait_tvp)
{
	sound_event_t *event	= NULL;
	sem_wait(in_mutex);

	if (!sound_eq_peek_event(&inqueue)) {
		thd_sleep(0);
		sem_signal(in_mutex);
		return NULL;
	}

	event = sound_eq_retreive_event(&inqueue);
	sem_signal(in_mutex);
	return event;
}

int
sound_dc_get_data(byte **data_ptr, int *size)
{
	int index = (thd_get_current() == master);
	semaphore_t *mutex = bulk_mutices[index];
	condvar_t *cond = bulk_conds[index];
	sci_queue_t *queue = &(bulk_queues[index]);
	void *data = NULL;

	sem_wait(mutex);
	while (!(data = sci_get_from_queue(queue, size))) {
		sem_signal(mutex);
		cond_wait(cond);
		sem_wait(mutex);
	}

	sem_signal(mutex);
	*data_ptr = (byte *) data;
	return *size;
}

int
sound_dc_send_data(byte *data_ptr, int maxsend)
{
	int index = 1 - (thd_get_current() == master);
	semaphore_t *mutex = bulk_mutices[index];
	condvar_t *cond = bulk_conds[index];
	sci_queue_t *queue = &(bulk_queues[index]);

	sem_wait(mutex);
	sci_add_to_queue(queue, sci_memdup(data_ptr, maxsend), maxsend);
	sem_signal(mutex);
	cond_signal(cond);
	return maxsend;
}

void
sound_dc_exit(state_t *s)
{
	int i;

	global_sound_server->queue_command(0, SOUND_COMMAND_SHUTDOWN, 0); /* Kill server */

	/* clean up */
	sciprintf("Waiting for soundserver thread to exit...\n");
	thd_wait(child);
	sciprintf("Soundserver thread exit ok\n");
	sem_destroy(out_mutex);
	sem_destroy(in_mutex);
	cond_destroy(in_cond);

	for (i = 0; i < 2; i++) {
		void *data = NULL;
		cond_destroy(bulk_conds[i]);
		sem_destroy(bulk_mutices[i]);

		while ((data = sci_get_from_queue(&(bulk_queues[i]), NULL)))
			free(data); /* Flush queues */
	}
}

int
sound_dc_save(state_t *s, char *dir)
{
	int *success = NULL;
	int retval;
	int size;
	/* we ignore the dir */

	global_sound_server->queue_command(0, SOUND_COMMAND_SAVE_STATE, 2);
	global_sound_server->send_data((byte *) "/ram", 5);

	global_sound_server->get_data((byte **) &success, &size);
	retval = *success;
	free(success);
	return retval;
}

sound_server_t sound_server_dc = {
	"dc",
	"0.1",
	0,
	&sound_dc_init,
	&sound_dc_configure,
	&sound_dc_exit,
	&sound_dc_get_event,
	&sound_dc_queue_event,
	&sound_dc_get_command,
	&sound_dc_queue_command,
	&sound_dc_get_data,
	&sound_dc_send_data,
	&sound_dc_save,
	&sound_restore_default,
	&sound_command_default,
	&sound_suspend_default,
	&sound_resume_default
};
