/***************************************************************************
 soundserver_win32.c Copyright (C) 2001 Alexander R Angas

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

    Alexander R Angas (Alex Angas) <wgd@internode.on.net>

 History:

   20010901 - Adapted from soundserver_sdl.c by Solomon Peachy.
                -- Alex Angas

***************************************************************************/

#include <sci_memory.h>
#include <engine.h>
#include <soundserver.h>
#include <sound.h>

/* #ifdef WINVER */
#ifdef _WIN32

#include <windows.h>

/* #define SSWIN_DEBUG */

static HANDLE child_thread;
static DWORD master_thread_id;

static int reverse_stereo = 0;

extern sound_server_t sound_server_win32;

/* Deadlock prevention */
static CRITICAL_SECTION ev_cs, in_cs;
static CRITICAL_SECTION bulk_cs[2];

static sci_queue_t bulk_queues[2];

static sound_eq_t inqueue; /* The in-event queue */
static sound_eq_t ev_queue; /* The event queue */


/* function called when sound server child thread begins */
DWORD WINAPI
win32_soundserver_init(LPVOID lpP)
{
#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u - CHILD thread ID, win32_soundserver_init()\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	/* start the sound server */
	sci0_soundserver(reverse_stereo);

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, win32_soundserver_init() end\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	return 0;
}

int
sound_win32_init(state_t *s, int flags)
{
	DWORD dwChildId;	/* child thread ID */
	int i;				/* for enumerating over bulk queues */

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_init()\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	/* let app know what sound server we are running and yield to the scheduler */
	global_sound_server = &sound_server_win32;
	sci_sched_yield();

	/* store a copy of the master thread id so we can use it with our mutices later */
	master_thread_id = GetCurrentThreadId();

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u - MASTER thread ID\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	if (init_midi_device(s) < 0)
		return -1;

	if (flags & SOUNDSERVER_INIT_FLAG_REVERSE_STEREO)
		reverse_stereo = 1;

	/* set up critical section variables */
	InitializeCriticalSection(&ev_cs);
	InitializeCriticalSection(&in_cs);
	for (i = 0; i < 2; i++)
		InitializeCriticalSection(&bulk_cs[i]);

	ds = stderr;

	sound_eq_init(&inqueue);
	sound_eq_init(&ev_queue);

	child_thread = CreateThread( NULL,		/* not inheritable */
		                         0,			/* use default stack size */
								 win32_soundserver_init,	/* callback function */
								 0,			/* cb function parameter - should be s but fails on Win9x */
								 0,			/* thread runs immediately */
								 &dwChildId);	/* pointer to id of thread */
	if (child_thread == NULL)
	{
		fprintf(stderr, "sound_win32_init(): CreateThread() failed, GetLastError() returned %u\n", GetLastError());
	}

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_init() end\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	return 0;
}

int
sound_win32_configure(state_t *s, char *option, char *value)
{
#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_configure()\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	return 1; /* No options apply to this driver */
}

sound_event_t *
sound_win32_get_event(state_t *s)
{
	sound_event_t *event = NULL;

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_get_event()\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	/* Request ownership of the critical section */
	__try
	{
		EnterCriticalSection(&ev_cs);

		/* Access the shared resource */

		/* Get event from queue if there is one */
		if (sound_eq_peek_event(&ev_queue))
			event = sound_eq_retreive_event(&ev_queue);
	}
	__finally
	{
		/* Release ownership of the critical section */
		LeaveCriticalSection(&ev_cs);
	}

#ifdef SSWIN_DEBUG
	if (event)
		fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_get_event() got %04x %d %d\n", event->handle, event->signal, event->value);
	else
		fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_get_event() no event\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	/* implicitly returns NULL if there was no event */
	return event;
}

void
sound_win32_queue_event(int handle, int signal, int value)
{
#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_queue_event()\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	__try
	{
		EnterCriticalSection(&ev_cs);

		/* Queue event */
		sound_eq_queue_event(&ev_queue, handle, signal, value);
	}
	__finally
	{
		LeaveCriticalSection(&ev_cs);
	}

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_queue_event() set %04x %d %d\n", handle, signal, value);
	fflush(NULL);
#endif
}

void
sound_win32_queue_command(int handle, int signal, int value)
{
#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_queue_command()\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	__try
	{
		EnterCriticalSection(&in_cs);

		/* Queue event */
		sound_eq_queue_event(&inqueue, handle, signal, value);
	}
	__finally
	{
		LeaveCriticalSection(&in_cs);
	}

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_queue_command() end\n", GetCurrentThreadId());
	fflush(NULL);
#endif
}

sound_event_t *
sound_win32_get_command(GTimeVal *wait_tvp)
{
	sound_event_t *event = NULL;

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_get_command()\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	__try
	{
		EnterCriticalSection(&in_cs);

		/* Get event from queue if there is one */
		if (sound_eq_peek_event(&inqueue))
			event = sound_eq_retreive_event(&inqueue);
		else
			Sleep(1);
		/* note: removed Sleep(1) from else case */
	}
	__finally
	{
		LeaveCriticalSection(&in_cs);
	}

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_get_command() end\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	/* implicitly returns NULL if there was no event in the queue */
	return event;
}

int
sound_win32_get_data(byte **data_ptr, int *size, int maxlen)
{
	int index	= (GetCurrentThreadId() == master_thread_id);
	void *data = NULL;

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_get_data()\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	__try
	{
		EnterCriticalSection(&bulk_cs[index]);
#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_get_data() entered critical section\n", GetCurrentThreadId());
	fflush(NULL);
#endif

		while (!(data = sci_get_from_queue(&(bulk_queues[index]), size)))
		{
			/* no data */
			LeaveCriticalSection(&bulk_cs[index]);

			Sleep(1); /* take a nap */

			/* re-enter critical section */
			EnterCriticalSection(&bulk_cs[index]);
		}
	}
	__finally
	{
		LeaveCriticalSection(&bulk_cs[index]);
#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_get_data() left critical section\n", GetCurrentThreadId());
	fflush(NULL);
#endif
	}

	*data_ptr = (byte *) data;

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_get_data() end\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	return *size;
}

int
sound_win32_send_data(byte *data_ptr, int maxsend)
{
	int index = 1 - (GetCurrentThreadId() == master_thread_id);

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_send_data(), queue %i\n", GetCurrentThreadId(), index);
	fflush(NULL);
#endif

	__try
	{
		EnterCriticalSection(&bulk_cs[index]);
		sci_add_to_queue(&(bulk_queues[index]), sci_memdup(data_ptr, maxsend), maxsend);
	}
	__finally
	{
		LeaveCriticalSection(&bulk_cs[index]);
	}

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_send_data() end, queue %i\n", GetCurrentThreadId(), index);
	fflush(NULL);
#endif

	return maxsend;
}

void
sound_win32_exit(state_t *s)
{
	int i;

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_exit()\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	sound_command(s, SOUND_COMMAND_SHUTDOWN, 0, 0); /* Kill server */

	/* clean up */
	WaitForSingleObject(child_thread, INFINITE);
	CloseHandle(child_thread);
	DeleteCriticalSection(&in_cs);
	DeleteCriticalSection(&ev_cs);

	for (i = 0; i < 2; i++) {
		void *data = NULL;
		DeleteCriticalSection(&bulk_cs[i]);

		while (data = sci_get_from_queue(&(bulk_queues[i]), NULL))
			free(data); /* Flush queues */
	}
}

int
sound_win32_save(state_t *s, char *dir)
{
  int *success = NULL;
  int retval;
  int size;

#ifdef SSWIN_DEBUG
	fprintf(stdout, "SSWIN_DEBUG: TID%u, sound_win32_save()\n", GetCurrentThreadId());
	fflush(NULL);
#endif

	/* we ignore the dir */

	sound_command(s, SOUND_COMMAND_SAVE_STATE, 0, 2);
	sound_send_data((byte *) ".", 2);

	sound_get_data((byte **) &success, &size, sizeof(int));
	retval = *success;
	free(success);
	return retval;
}

sound_server_t sound_server_win32 = {
	"win32",
	"0.2",
	0,
	&sound_win32_init,
	&sound_win32_configure,
	&sound_win32_exit,
	&sound_win32_get_event,
	&sound_win32_queue_event,
	&sound_win32_get_command,
	&sound_win32_queue_command,
	&sound_win32_get_data,
	&sound_win32_send_data,
	&sound_win32_save,
	&sound_restore,
	&sound_command,
	&sound_suspend,
	&sound_resume
};

#endif
