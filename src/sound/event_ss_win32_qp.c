/***************************************************************************
 event_ss_win32_qp.c Copyright (C) 2001 Alexander R Angas

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

   20011123 - Begins.
                -- Alex Angas

***************************************************************************/

#include <sci_memory.h>
#include <soundserver.h>
#include <sound.h>

#ifdef _WIN32

#include <engine.h>
#include <win32/messages.h>

#define SSWIN_DEBUG 0

sound_server_t sound_server_win32e_qp;

#define MAIN_CLASS_NAME "FreeSCI Main Receiving from Event SS"
#define SOUND_CLASS_NAME "Event SS Receiving from FreeSCI Main"
#define WINDOW_SUFFIX " (Window)"

static HWND main_wnd, sound_wnd;
static HANDLE child_thread, worker_thread;
static state_t *gs;

static sci_queue_t data_queue;
static CRITICAL_SECTION dq_cs;	/* synchronisation for data queue */


LRESULT CALLBACK
MainWndProc (HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	/* does nothing */
#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "MainWndProc called for hWnd %i with msg %i (TID: %i)\n", hWnd, nMsg, GetCurrentThreadId());
#endif

	return DefWindowProc (hWnd, nMsg, wParam, lParam);
}

LRESULT CALLBACK
SoundWndProc (HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	/* does nothing */
#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "SoundWndProc called for hWnd %i with msg %i (TID: %i)\n", hWnd, nMsg, GetCurrentThreadId());
#endif

	return DefWindowProc (hWnd, nMsg, wParam, lParam);
}

DWORD WINAPI
_timing_thread(LPVOID lpP)
{
	LARGE_INTEGER tim, freq;
	unsigned int old_val, new_val;

if (SetThreadPriority(worker_thread, THREAD_PRIORITY_LOWEST) == 0)
fprintf(stderr, "stp failed\n");

	old_val = 0;

	for (;;)
	{
		if (QueryPerformanceCounter(&tim) == 0)
			fprintf(debug_stream, "_timing_thread(): QueryPerformanceCounter() failed, GetLastError() returned %u\n", GetLastError());
		if (QueryPerformanceFrequency(&freq) == 0)
			fprintf(debug_stream, "_timing_thread(): QueryPerformanceFrequency() failed, GetLastError() returned %u\n", GetLastError());

		new_val = (tim.QuadPart*1000 / freq.QuadPart) % 16667;
//fprintf(stderr, "checking if new_val %u < old_val %u\n", new_val, old_val);
		if (new_val < old_val)
//			global_sound_server->queue_command(0, SOUND_COMMAND_DO_SOUND, 0);
		old_val = new_val;
	}
}

/* function called when sound server child thread begins */
DWORD WINAPI
win32e_qp_soundserver_init(LPVOID lpP)
{
	DWORD dwWorkerId;
	sound_server_state_t sss;
	WNDCLASS wc;
	HINSTANCE hi_sound;
	memset(&sss, 0, sizeof(sound_server_state_t));

#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "win32e_qp_soundserver_init() begins with TID %i\n", GetCurrentThreadId());
#endif

	/*** register window class for messages to sound server ***/
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = SoundWndProc;
	hi_sound = GetModuleHandle(NULL);
	if (hi_sound == NULL)
		fprintf(debug_stream, "win32e_qp_soundserver_init(): GetModuleHandle() failed, GetLastError() returned %u\n", GetLastError());
	wc.hInstance = hi_sound;
	wc.lpszClassName = SOUND_CLASS_NAME;

	if (RegisterClass(&wc) == 0)
		fprintf(debug_stream, "win32e_qp_soundserver_init(): RegisterClass() failed, GetLastError() returned %u\n", GetLastError());

	/* create our 'window' (not visible of course) */
	sound_wnd = CreateWindow (
		SOUND_CLASS_NAME,
		SOUND_CLASS_NAME WINDOW_SUFFIX,
		( WS_DISABLED | WS_POPUP ),
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (sound_wnd == NULL)
		fprintf(debug_stream, "win32e_qp_soundserver_init(): CreateWindow() for sound failed, GetLastError() returned %u\n", GetLastError());
	else
		fprintf(stderr, "Created sound window class\n");

#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "win32e_qp_soundserver_init() sound_wnd %i, TID %i\n", sound_wnd, GetCurrentThreadId());
#endif

	/* start the worker thread that gives the timing */
	worker_thread = CreateThread( NULL,		/* not inheritable */
		                         0,			/* use default stack size */
								 _timing_thread,	/* callback function */
								 0,			/* cb function parameter - should be s but fails on Win9x */
								 0,			/* thread runs immediately */
								 &dwWorkerId);	/* pointer to id of new thread */
	if (worker_thread == NULL)
		fprintf(debug_stream, "win32e_qp_soundserver_init(): CreateThread() failed, GetLastError() returned %u\n", GetLastError());
	else
		fprintf(stderr, "Created worker thread for sound timing\n");

	/* start the sound server */
	sci0_event_ss(&sss);

	return 0;
}

int
sound_win32e_qp_init(struct _state *s, int flags)
{
	WNDCLASS wc;
	HINSTANCE hi_main;
	DWORD dwChildId;	/* child thread ID */

	/* let app know what sound server we are running and yield to the scheduler */
	global_sound_server = &sound_server_win32e_qp;
	gs = s;	/* keep copy of pointer */
	sci_sched_yield();

	fprintf(stderr, "Sorry, this sound server is currently broken.\n");
	exit(-1);

	if (init_midi_device(s) < 0)
		return -1;
/*
TODO: fix reverse stereo
	if (flags & SOUNDSERVER_INIT_FLAG_REVERSE_STEREO)
		sss->reverse_stereo = 1;
*/
	debug_stream = stderr;

	InitializeCriticalSection(&dq_cs);
	sci_init_queue(&data_queue);

	/*** register window class for messages to main thread ***/
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = MainWndProc;
	hi_main = GetModuleHandle(NULL);
	if (hi_main == NULL)
		fprintf(debug_stream, "sound_win32e_qp_init(): GetModuleHandle() failed, GetLastError() returned %u\n", GetLastError());
	wc.hInstance = hi_main;
	wc.lpszClassName = MAIN_CLASS_NAME;

	if (RegisterClass(&wc) == 0)
		fprintf(debug_stream, "sound_win32e_qp_init(): RegisterClass() failed, GetLastError() returned %u\n", GetLastError());

	/*** create window for main thread to receive messages (not visible of course) ***/
	main_wnd = CreateWindow (
		MAIN_CLASS_NAME,
		MAIN_CLASS_NAME WINDOW_SUFFIX,
		( WS_DISABLED | WS_POPUP ),
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (main_wnd == NULL)
		fprintf(debug_stream, "sound_win32e_qp_init(): CreateWindow() for main failed, GetLastError() returned %u\n", GetLastError());
	else
		fprintf(stderr, "Created main window class\n");

#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "sound_win32e_qp_init() main_wnd %i, TID %i\n", main_wnd, GetCurrentThreadId());
#endif

	/*** create thread ***/
	child_thread = CreateThread( NULL,		/* not inheritable */
		                         0,			/* use default stack size */
								 win32e_qp_soundserver_init,	/* callback function */
								 0,			/* cb function parameter - should be s but fails on Win9x */
								 0,			/* thread runs immediately */
								 &dwChildId);	/* pointer to id of thread */
	if (child_thread == NULL)
		fprintf(debug_stream, "sound_win32e_qp_init(): CreateThread() failed, GetLastError() returned %u\n", GetLastError());
	else
		fprintf(stderr, "Created sound thread\n");

	fprintf(stderr, "Sound server win32e_qp initialised\n");

	return 0;
}

int
sound_win32e_qp_configure(struct _state *s, char *option, char *value)
{
	return 1; /* No options apply to this driver */
}

sound_event_t *
sound_win32e_qp_get_event()
{
	/* receives message for main thread queue */

	MSG msg; /* incoming message */

	sound_event_t *event_temp = NULL;

	if (PeekMessage(&msg, main_wnd, 0, 0, PM_REMOVE))
	{
#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "sound_win32e_qp_get_event() called with msg %i (time: %i) (TID: %i)\n", msg.message, msg.time, GetCurrentThreadId());
#endif
		event_temp = (sound_event_t*)malloc(sizeof(sound_event_t));
		event_temp->signal = msg.message;
		event_temp->handle = msg.wParam;
		event_temp->value = msg.lParam;

		/* pass on the message regardless */
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		return event_temp;
	}

	return NULL;
}

sound_event_t *
sound_win32e_qp_get_command(GTimeVal *wait_tvp)
{
	/* receives message from sound server queue */

	MSG msg; /* incoming message */
	BOOL bRet;

	sound_event_t *event_temp = NULL;

	/* wait for a message */
	if ( (bRet = GetMessage( &msg, sound_wnd, 0, 0 )) != 0)
	{
	    if (bRet == -1)
	    {
			fprintf(debug_stream, "sound_win32e_qp_get_command(): GetMessage() failed, GetLastError() returned %u\n", GetLastError());

		} else {
#if (SSWIN_DEBUG == 1)
			fprintf(stderr, "sound_win32e_qp_get_command() called with msg %i (time: %i) (TID: %i)\n", msg.message, msg.time, GetCurrentThreadId());
#endif
			event_temp = (sound_event_t*)malloc(sizeof(sound_event_t));
			event_temp->signal = msg.message;
			event_temp->handle = msg.wParam;
			event_temp->value = msg.lParam;

			/* pass on the message regardless */
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			return event_temp;
		}
	}

	return NULL;
}

void
sound_win32e_qp_queue_event(unsigned int handle, unsigned int signal, long value)
{
	/* posts message to main thread queue */
	if (PostMessage(main_wnd, signal, handle, value) == 0)
		fprintf(debug_stream, "sound_win32e_qp_queue_event(): PostMessage() failed, GetLastError() returned %u\n", GetLastError());
}

void
sound_win32e_qp_queue_command(unsigned int handle, unsigned int signal, long value)
{
	/* posts message to sound server queue */
	if (PostMessage(sound_wnd, signal, handle, value) == 0)
		fprintf(debug_stream, "sound_win32e_qp_queue_command(): PostMessage() failed, GetLastError() returned %u\n", GetLastError());
}

int
sound_win32e_qp_get_data(byte **data_ptr, int *size)
{
	/* returns 1 if do_sound() should be called */
	/* data_ptr and size are ignored */

	void *data = NULL;

	__try
	{
		EnterCriticalSection(&dq_cs);
		while (!(data = sci_get_from_queue(&data_queue, size)))
		{
			/* no data */
			LeaveCriticalSection(&dq_cs);

			/* yield */
			Sleep(1);

			/* re-enter critical section */
			EnterCriticalSection(&dq_cs);
		}
	}
	__finally
	{
		LeaveCriticalSection(&dq_cs);
	}

	*data_ptr = (byte *)data;
	return *size;
}

int
sound_win32e_qp_send_data(byte *data_ptr, int maxsend)
{
	__try
	{
		EnterCriticalSection(&dq_cs);
		sci_add_to_queue(&data_queue, sci_memdup(data_ptr, maxsend), maxsend);
	}
	__finally
	{
		LeaveCriticalSection(&dq_cs);
	}

	return maxsend;
}

void
sound_win32e_qp_exit(struct _state *s)
{
	/* kill server */
	global_sound_server->queue_command(0, SOUND_COMMAND_SHUTDOWN, 0);

	/* kill threads */
	WaitForSingleObject(worker_thread, INFINITE);
	CloseHandle(worker_thread);
	WaitForSingleObject(child_thread, INFINITE);
	CloseHandle(child_thread);
	DeleteCriticalSection(&dq_cs);

	/* close window class handles */
	CloseHandle(main_wnd);
	CloseHandle(sound_wnd);
}

int
sound_win32e_qp_save(struct _state *s, char *dir)
{
	return 0;
}

sound_server_t sound_server_win32e_qp = {
	"win32e_qp",
	"0.1",
	0,
	&sound_win32e_qp_init,
	&sound_win32e_qp_configure,
	&sound_win32e_qp_exit,
	&sound_win32e_qp_get_event,
	&sound_win32e_qp_queue_event,
	&sound_win32e_qp_get_command,
	&sound_win32e_qp_queue_command,
	&sound_win32e_qp_get_data,
	&sound_win32e_qp_send_data,
	&sound_win32e_qp_save,
	&sound_restore_default,
	&sound_command_default,
	&sound_suspend_default,
	&sound_resume_default
};

#endif
