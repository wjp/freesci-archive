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

#ifdef WINVER

#include <windows.h>

/* #define SSWIN_DEBUG */

HANDLE child_thread;
HANDLE out_mutex;
HANDLE in_mutex;

DWORD master_thread_id;

int reverse_stereo = 0;

extern sound_server_t sound_server_win32;

/* Mutex names */
LPCTSTR out_mutex_name = "SoundServerOutMutex";
LPCTSTR in_mutex_name = "SoundServerInMutex";
LPCTSTR bulk_mutex_name = "SoundServerBulkMutex";

HANDLE bulk_mutices[2];
sci_queue_t bulk_queues[2];

const DWORD MUTEX_TIMEOUT = 5000L;

/* Used for manipulating name of bulk mutex */
int mutex_name_len;
char *mutex_name_temp;
char *dump;

sound_eq_t inqueue; /* The in-event queue */
sound_eq_t ev_queue; /* The event queue */


/* function called when sound server child thread begins */
DWORD WINAPI
win32_soundserver_init(LPVOID lpP)
{
#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u - CHILD thread ID, win32_soundserver_init()\n", GetCurrentThreadId());
#endif

	/* start the sound server */
	sci0_soundserver(reverse_stereo);
	return 0;
}

int
sound_win32_init(state_t *s, int flags)
{
	LPDWORD lpChildId = NULL;	/* child thread ID */
	int i;					/* for enumerating over bulk queues */

#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u, sound_win32_init()\n", GetCurrentThreadId());
#endif

	/* let app know what sound server we are running and yield to the scheduler */
	global_sound_server = &sound_server_win32;
	sci_sched_yield();

	/* store a copy of the master thread id so we can use it with our mutices later */
	master_thread_id = GetCurrentThreadId();

#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u - MASTER thread ID\n", GetCurrentThreadId());
#endif

	if (init_midi_device(s) < 0)
		return -1;

	if (flags & SOUNDSERVER_INIT_FLAG_REVERSE_STEREO)
		reverse_stereo = 1;

	/* create mutices and spawn thread */
	out_mutex = CreateMutex(NULL,
							FALSE,  /* no thread gets the mutex to start with */
							out_mutex_name);
	if (out_mutex == NULL)
	{
		fprintf(stderr, "sound_win32_init(): CreateMutex(%s) failed\n", out_mutex_name);
	}

	in_mutex = CreateMutex(NULL, FALSE, in_mutex_name);
	if (in_mutex == NULL)
	{
		fprintf(stderr, "sound_win32_init(): CreateMutex(%s) failed\n", in_mutex_name);
	}

	/* set up name for bulk mutices and create them */
	mutex_name_len = strlen(bulk_mutex_name) + 2; /* room for number and \0 */
	mutex_name_temp = (char *) sci_malloc(mutex_name_len);
	mutex_name_temp = strcpy(mutex_name_temp, bulk_mutex_name);
	dump = (char *) sci_malloc(2);
	for (i = 0; i < 2; i++) {
		itoa(i, dump, 10);
		strcat(mutex_name_temp, dump);

		sci_init_queue(&(bulk_queues[i]));
		bulk_mutices[i] = CreateMutex(NULL, FALSE, mutex_name_temp);
		if (bulk_mutices[i] == NULL)
		{
			fprintf(stderr, "sound_win32_init(): CreateMutex(%s) failed\n", mutex_name_temp);
		}

		mutex_name_temp[mutex_name_len - 2] = '\0'; /* overwrite number */
	}

	ds = stderr;

	sound_eq_init(&inqueue);
	sound_eq_init(&ev_queue);

	child_thread = CreateThread( NULL, 0, win32_soundserver_init, s, 0, lpChildId);
	if (child_thread == NULL)
	{
		fprintf(stderr, "sound_win32_init(): CreateThread() failed\n");
	}

	return 0;
}

int
sound_win32_configure(state_t *s, char *option, char *value)
{
#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u, sound_win32_configure()\n", GetCurrentThreadId());
#endif

	return 1; /* No options apply to this driver */
}

sound_event_t *
sound_win32_get_event(state_t *s)
{
	sound_event_t *event = NULL;

#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u, sound_win32_get_event()\n", GetCurrentThreadId());
#endif

	if (WaitForSingleObject(out_mutex, MUTEX_TIMEOUT) != WAIT_OBJECT_0)
	{
		fprintf(stderr, "sound_win32_get_event(): WaitForSingleObject(%s) failed\n", out_mutex_name);
		return NULL;
	}

	if (!sound_eq_peek_event(&ev_queue))
	{
		if (ReleaseMutex(out_mutex) == 0)
		{
			fprintf(stderr, "sound_win32_get_event(): ReleaseMutex(%s) failed\n", out_mutex_name);
		}

		return NULL;
	}

	event = sound_eq_retreive_event(&ev_queue);

	if (ReleaseMutex(out_mutex) == 0)
	{
		fprintf(stderr, "sound_win32_get_event(): ReleaseMutex(%s) failed\n", out_mutex_name);
	}

	/*
	if (event)
		printf("got %04x %d %d\n", event->handle, event->signal, event->value);
	*/

	return event;
}

void
sound_win32_queue_event(int handle, int signal, int value)
{
#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u, sound_win32_queue_event()\n", GetCurrentThreadId());
#endif

	if (WaitForSingleObject(out_mutex, MUTEX_TIMEOUT) != WAIT_OBJECT_0)
	{
		fprintf(stderr, "sound_win32_queue_event(): WaitForSingleObject(%s) failed\n", out_mutex_name);
		return;
	}

	sound_eq_queue_event(&ev_queue, handle, signal, value);

	if (ReleaseMutex(out_mutex) == 0)
	{
		fprintf(stderr, "sound_win32_queue_event(): ReleaseMutex(%s) failed\n", out_mutex_name);
	}

	/*  printf("set %04x %d %d\n", handle, signal, value); */
}

void
sound_win32_queue_command(int handle, int signal, int value)
{
#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u, sound_win32_queue_command()\n", GetCurrentThreadId());
#endif

	if (WaitForSingleObject(in_mutex, MUTEX_TIMEOUT) != WAIT_OBJECT_0)
	{
		fprintf(stderr, "sound_win32_queue_command(): WaitForSingleObject(%s) failed\n", in_mutex_name);
		return;
	}

	sound_eq_queue_event(&inqueue, handle, signal, value);

	if (ReleaseMutex(in_mutex) == 0)
	{
		fprintf(stderr, "sound_win32_queue_command(): ReleaseMutex(%s) failed\n", in_mutex_name);
	}
}

sound_event_t *
sound_win32_get_command(GTimeVal *wait_tvp)
{
	sound_event_t *event = NULL;

#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u, sound_win32_get_command()\n", GetCurrentThreadId());
#endif

	if (WaitForSingleObject(in_mutex, MUTEX_TIMEOUT) != WAIT_OBJECT_0)
	{
		fprintf(stderr, "sound_win32_get_command(): WaitForSingleObject(%s) failed\n", in_mutex_name);
		return NULL;
	}

	if (!sound_eq_peek_event(&inqueue)) {
		Sleep(1);

		if (ReleaseMutex(in_mutex) == 0)
		{
			fprintf(stderr, "sound_win32_get_command(): ReleaseMutex(%s) failed\n", in_mutex_name);
		}

		return NULL;
	}

	event = sound_eq_retreive_event(&inqueue);

	if (ReleaseMutex(in_mutex) == 0)
	{
		fprintf(stderr, "sound_win32_get_command(): ReleaseMutex(%s) failed\n", in_mutex_name);
	}

	return event;
}

int
sound_win32_get_data(byte **data_ptr, int *size, int maxlen)
{
	int index	= (GetCurrentThreadId() == master_thread_id);
	void *data = NULL;

#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u, sound_win32_get_data()\n", GetCurrentThreadId());
#endif

	/* set up bulk mutex name */
	itoa(index, dump, 10);
	mutex_name_temp[mutex_name_len - 2] = '\0'; /* overwrite number there */
	strcat(mutex_name_temp, dump);

	if (WaitForSingleObject(bulk_mutices[index], MUTEX_TIMEOUT) != WAIT_OBJECT_0)
	{
		fprintf(stderr, "sound_win32_get_data(): WaitForSingleObject(%s) failed\n", mutex_name_temp);
		return -1;
	}

	while (!(data = sci_get_from_queue(&(bulk_queues[index]), size)))
	{
		/* no data, so release the mutex */
		if (ReleaseMutex(bulk_mutices[index]) == 0)
		{
			fprintf(stderr, "sound_win32_get_data(): ReleaseMutex(%s) failed\n", mutex_name_temp);
		}

		Sleep(1); /* take a nap */

		/* then get it back again */
		if (WaitForSingleObject(bulk_mutices[index], MUTEX_TIMEOUT) != WAIT_OBJECT_0)
		{
			fprintf(stderr, "sound_win32_get_data(): WaitForSingleObject(%s) failed\n", mutex_name_temp);
			return -1;
		}
	}

	if (ReleaseMutex(bulk_mutices[index]) == 0)
	{
		fprintf(stderr, "sound_win32_get_data(): ReleaseMutex(%s) failed\n", mutex_name_temp);
	}

	*data_ptr = (byte *) data;

	return *size;
}

int
sound_win32_send_data(byte *data_ptr, int maxsend)
{
	int index = 1 - (GetCurrentThreadId() == master_thread_id);

#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u, sound_win32_send_data()\n", GetCurrentThreadId());
#endif

	/* set up bulk mutex name */
	itoa(index, dump, 10);
	mutex_name_temp[mutex_name_len - 2] = '\0'; /* overwrite number there */
	strcat(mutex_name_temp, dump);

	if (WaitForSingleObject(bulk_mutices[index], MUTEX_TIMEOUT) != WAIT_OBJECT_0)
	{
		fprintf(stderr, "sound_win32_send_data(): WaitForSingleObject(%s) failed\n", mutex_name_temp);
		return -1;
	}

	sci_add_to_queue(&(bulk_queues[index]), sci_memdup(data_ptr, maxsend), maxsend);

	if (ReleaseMutex(bulk_mutices[index]) == 0)
	{
		fprintf(stderr, "sound_win32_send_data(): ReleaseMutex(%s) failed\n", mutex_name_temp);
	}

	return maxsend;
}

void
sound_win32_exit(state_t *s)
{
	int i;

#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u, sound_win32_exit()\n", GetCurrentThreadId());
#endif

	sound_command(s, SOUND_COMMAND_SHUTDOWN, 0, 0); /* Kill server */

	/* clean up */
	WaitForSingleObject(child_thread, INFINITE);
	CloseHandle(child_thread);
	CloseHandle(out_mutex);
	CloseHandle(in_mutex);

	for (i = 0; i < 2; i++) {
		void *data = NULL;
		CloseHandle(bulk_mutices[i]);

		while (data = sci_get_from_queue(&(bulk_queues[i]), NULL))
			free(data); /* Flush queues */
	}

	free(mutex_name_temp);
	free(dump);
}

int
sound_win32_save(state_t *s, char *dir)
{
  int *success = NULL;
  int retval;
  int size;

#ifdef SSWIN_DEBUG
	fprintf(stderr, "SSWIN_DEBUG: TID%u, sound_win32_save()\n", GetCurrentThreadId());
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
	"0.1",
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
