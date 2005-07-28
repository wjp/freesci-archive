/***************************************************************************
 polled_ss_pthread.c Copyright (C) 2005 Walter van Niftrik
 
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

#include <pthread.h>
#include <sci_memory.h>
#include <engine.h>
#include <soundserver.h>
#include <sound.h>
#include <pcmout.h>

static pthread_t child;
static pthread_mutex_t out_mutex;
static pthread_mutex_t in_mutex;

static pthread_t master;

static int reverse_stereo = 0;

extern sound_server_t sound_server_pthread;

static pthread_mutex_t bulk_mutices[2];
static sci_queue_t bulk_queues[2];
static pthread_cond_t bulk_conds[2];

static sound_eq_t inqueue;
static sound_eq_t ev_queue;

void *
pthread_soundserver_init(void *args)
{
    sound_server_state_t sss;
    memset(&sss, 0, sizeof(sound_server_state_t));

    sci0_polled_ss(reverse_stereo, &sss);
    return NULL;
}

int
sound_pthread_init(state_t *s, int flags)
{
    int i;

    global_sound_server = &sound_server_pthread;

    master = pthread_self();

    if (pcmout_open() < 0)
        return -1;

    if (init_midi_device(s) < 0)
        return -1;

    if (flags & SOUNDSERVER_INIT_FLAG_REVERSE_STEREO)
        reverse_stereo = 1;

    pthread_mutex_init(&out_mutex, NULL);
    pthread_mutex_init(&in_mutex, NULL);

    for (i = 0; i < 2; i++)
    {
        pthread_cond_init(&bulk_conds[i], NULL);
        sci_init_queue(&(bulk_queues[i]));
        pthread_mutex_init(&bulk_mutices[i], NULL);
    }

    sound_eq_init(&inqueue);
    sound_eq_init(&ev_queue);

    debug_stream = stderr;

    if (pthread_create(&child, NULL, pthread_soundserver_init, NULL))
    {
        fprintf(debug_stream, "%s, L%d: Could not create child thread.\n",
                __FILE__, __LINE__);
        return 1;
    }
    return 0;
}

int
sound_pthread_configure(state_t *s, char *option, char *value)
{
    return 1;
}

sound_event_t *
sound_pthread_get_event(state_t *s)
{
    sound_event_t *event = NULL;

    if (pthread_mutex_lock(&out_mutex) != 0)
    {
        fprintf(debug_stream, "%s, L%d: Mutex lock failed.\n", __FILE__,
                __LINE__);
        return NULL;
    }

    if (!sound_eq_peek_event(&ev_queue))
    {
        if (pthread_mutex_unlock(&out_mutex) != 0)
            fprintf(debug_stream, "%s, L%d: Mutex unlock failed.\n",
                    __FILE__, __LINE__);

        return NULL;
    }

    event = sound_eq_retreive_event(&ev_queue);

    if (pthread_mutex_unlock(&out_mutex) != 0)
        fprintf(debug_stream, "%s, L%d: Mutex unlock failed.\n", __FILE__,
                __LINE__);

    return event;
}

void
sound_pthread_queue_event(unsigned int handle, unsigned int signal, long value)
{
    if (pthread_mutex_lock(&out_mutex) != 0)
    {
        fprintf(debug_stream, "%s, L%d: Mutex lock failed.\n", __FILE__,
                __LINE__);
        return;
    }

    sound_eq_queue_event(&ev_queue, handle, signal, value);

    if (pthread_mutex_unlock(&out_mutex) != 0)
        fprintf(debug_stream, "%s, L%d: Mutex unlock failed.\n", __FILE__,
                __LINE__);
}

void
sound_pthread_queue_command(unsigned int handle, unsigned int signal,
                         long value)
{
    if (pthread_mutex_lock(&in_mutex) != 0)
    {
        fprintf(debug_stream, "%s, L%d: Mutex lock failed.\n", __FILE__,
                __LINE__);
        return;
    }

    sound_eq_queue_event(&inqueue, handle, signal, value);

    if (pthread_mutex_unlock(&in_mutex) != 0)
    {
        fprintf(debug_stream, "%s, L%d: Mutex unlock failed.\n", __FILE__,
                __LINE__);
        return;
    }
}

sound_event_t *
sound_pthread_get_command(GTimeVal *wait_tvp)
{
    sound_event_t *event	= NULL;

    if (pthread_mutex_lock(&in_mutex) != 0)
    {
        fprintf(debug_stream, "%s, L%d: Mutex lock failed.\n", __FILE__,
                __LINE__);
        return NULL;
    }

    if (!sound_eq_peek_event(&inqueue))
    {
        if (pthread_mutex_unlock(&in_mutex) != 0)
            fprintf(debug_stream, "%s, L%d: Mutex unlock failed.\n",
                    __FILE__, __LINE__);

        sched_yield();

        return NULL;
    }

    event = sound_eq_retreive_event(&inqueue);

    if (pthread_mutex_unlock(&in_mutex) != 0)
        fprintf(debug_stream, "%s, L%d: Mutex unlock failed.\n", __FILE__,
                __LINE__);

    return event;
}

int
sound_pthread_get_data(byte **data_ptr, int *size)
{
    int index = (pthread_self() == master);
    pthread_mutex_t *mutex = &bulk_mutices[index];
    pthread_cond_t *cond = &bulk_conds[index];
    sci_queue_t *queue	= &bulk_queues[index];
    byte *data = NULL;

    if (pthread_mutex_lock(mutex) != 0)
    {
        fprintf(debug_stream, "%s, L%d: Mutex lock failed.\n", __FILE__,
                __LINE__);
        return 0;
    }

    while (!(data = sci_get_from_queue(queue, size)))
        pthread_cond_wait(cond, mutex);

    if (pthread_mutex_unlock(mutex) != 0)
        fprintf(debug_stream, "%s, L%d: Mutex unlock failed.\n", __FILE__,
                __LINE__);

    *data_ptr = data;

    return *size;
}

int
sound_pthread_send_data(byte *data_ptr, int maxsend)
{
    int index = 1 - (pthread_self() == master);
    pthread_mutex_t *mutex = &bulk_mutices[index];
    pthread_cond_t *cond = &bulk_conds[index];
    sci_queue_t *queue = &(bulk_queues[index]);

    if (pthread_mutex_lock(mutex) != 0)
    {
        fprintf(debug_stream, "%s, L%d: Mutex lock failed.\n", __FILE__,
                __LINE__);
        return 0;
    }

    sci_add_to_queue(queue, sci_memdup(data_ptr, maxsend), maxsend);

    if (pthread_cond_signal(cond) != 0)
        fprintf(debug_stream, "%s, L%d: Cond signal failed.\n", __FILE__,
                __LINE__);

    if (pthread_mutex_unlock(mutex) != 0)
        fprintf(debug_stream, "%s, L%d: Mutex unlock failed.\n", __FILE__,
                __LINE__);

    return maxsend;
}

void
sound_pthread_exit(state_t *s)
{
    int i;

    global_sound_server->queue_command(0, SOUND_COMMAND_SHUTDOWN, 0);

    pthread_join(child, NULL);
    pthread_mutex_destroy(&out_mutex);
    pthread_mutex_destroy(&in_mutex);

    for (i = 0; i < 2; i++)
    {
        void *data;
        pthread_cond_destroy(&bulk_conds[i]);
        pthread_mutex_destroy(&bulk_mutices[i]);

        while ((data = sci_get_from_queue(&bulk_queues[i], NULL)))
            free(data);
    }

}


int
sound_pthread_save(state_t *s, char *dir)
{
    byte *success = NULL;
    int retval;
    int size;

    global_sound_server->queue_command(0, SOUND_COMMAND_SAVE_STATE, 2);
    global_sound_server->send_data((byte *) ".", 2);

    global_sound_server->get_data(&success, &size);
    retval = *success;

    free(success);
    return retval;
}

sound_server_t sound_server_pthread = {
                                       "pthread",
                                       "0.1",
                                       0,
                                       &sound_pthread_init,
                                       &sound_pthread_configure,
                                       &sound_pthread_exit,
                                       &sound_pthread_get_event,
                                       &sound_pthread_queue_event,
                                       &sound_pthread_get_command,
                                       &sound_pthread_queue_command,
                                       &sound_pthread_get_data,
                                       &sound_pthread_send_data,
                                       &sound_pthread_save,
                                       &sound_restore_default,
                                       &sound_command_default,
                                       &sound_suspend_default,
                                       &sound_resume_default
                                   };
