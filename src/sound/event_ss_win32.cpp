/***************************************************************************
 event_ss_win32.cpp Copyright (C) 2001 Alexander R Angas

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

   20011117 - Begins.
                -- Alex Angas

***************************************************************************/

#ifndef __cplusplus
#error NOTE: This file MUST be compiled as C++. In Visual C++, use the /Tp command line option.
#endif

extern "C" {
#include <sci_memory.h>
#include <soundserver.h>
#include <sound.h>
};

#ifdef _WIN32

#include <engine.h>
#include <win32/messages.h>

#define SSWIN_DEBUG 0

extern "C" sound_server_t sound_server_win32e;

#define MAIN_CLASS_NAME "FreeSCI Main Receiving from Event SS"
#define SOUND_CLASS_NAME "Event SS Receiving from FreeSCI Main"
#define WINDOW_SUFFIX " (Window)"

static HWND main_wnd, sound_wnd;
static HINSTANCE hi_main, hi_sound;
static HANDLE child_thread;
static HANDLE sound_sem, sound_data_event;
static DWORD sound_timer;
static IReferenceClock *pRefClock;
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

/* function called when sound server child thread begins */
DWORD WINAPI
win32e_soundserver_init(LPVOID lpP)
{
	sound_server_state_t sss;
	WNDCLASS wc;
	memset(&sss, 0, sizeof(sound_server_state_t));

#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "win32e_soundserver_init() begins with TID %i\n", GetCurrentThreadId());
#endif

	/*** register window class for messages to sound server ***/
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = SoundWndProc;
	hi_sound = GetModuleHandle(NULL);
	if (hi_sound == NULL)
	{
		fprintf(debug_stream, "win32e_soundserver_init(): GetModuleHandle() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}
	wc.hInstance = hi_sound;
	wc.lpszClassName = SOUND_CLASS_NAME;

	if (RegisterClass(&wc) == 0)
	{
		fprintf(debug_stream, "win32e_soundserver_init(): RegisterClass() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}

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
	{
		fprintf(debug_stream, "win32e_soundserver_init(): CreateWindow() for sound failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}
	else
		fprintf(stderr, "Created sound window class\n");

#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "win32e_soundserver_init() sound_wnd %i, TID %i\n", sound_wnd, GetCurrentThreadId());
#endif

	/*** start the sound server ***/
	sci0_event_ss(&sss);

	return 0;
}

int
sound_win32e_init(struct _state *s, int flags)
{
	WNDCLASS wc;
	DWORD dwChildId;	/* child thread ID */
	REFERENCE_TIME rtNow;

	/* let app know what sound server we are running and yield to the scheduler */
	global_sound_server = &sound_server_win32e;
	gs = s;	/* keep copy of pointer */
	sci_sched_yield();

	if (init_midi_device(s) < 0)
		return -1;
/*
TODO: fix reverse stereo
	if (flags & SOUNDSERVER_INIT_FLAG_REVERSE_STEREO)
		sss->reverse_stereo = 1;
*/
	debug_stream = stderr;

	/*** set up what we need for data transfer ***/
	InitializeCriticalSection(&dq_cs);
	sci_init_queue(&data_queue);

	/* create event that will signal when data is waiting */
	sound_data_event = CreateEvent(NULL, FALSE, FALSE, NULL);

	/*** register window class for messages to main thread ***/
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = MainWndProc;
	hi_main = GetModuleHandle(NULL);
	if (hi_main == NULL)
	{
		fprintf(debug_stream, "sound_win32e_init(): GetModuleHandle() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}
	wc.hInstance = hi_main;
	wc.lpszClassName = MAIN_CLASS_NAME;

	if (RegisterClass(&wc) == 0)
		fprintf(debug_stream, "sound_win32e_init(): RegisterClass() failed, GetLastError() returned %u\n", GetLastError());

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
	{
		fprintf(debug_stream, "sound_win32e_init(): CreateWindow() for main failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}
	else
		fprintf(stderr, "Created main window class\n");

#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "sound_win32e_init() main_wnd %i, TID %i\n", main_wnd, GetCurrentThreadId());
#endif

	/*** create thread ***/
	child_thread = CreateThread( NULL,		/* not inheritable */
		                         0,			/* use default stack size */
								 win32e_soundserver_init,	/* callback function */
								 0,			/* cb function parameter - should be s but fails on Win9x */
								 0,			/* thread runs immediately */
								 &dwChildId);	/* pointer to id of thread */
	if (child_thread == NULL)
	{
		fprintf(debug_stream, "sound_win32e_init(): CreateThread() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}
	else
		fprintf(stderr, "Created sound thread\n");

	/*** start timer ***/
	sound_sem = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
	if (sound_sem == NULL)
	{
		fprintf(debug_stream, "sound_win32e_init(): CreateSemaphore() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}
	else
		fprintf(stderr, "Created sound semaphore\n");

	if (CoInitialize(NULL) != S_OK)
	{
		fprintf(debug_stream, "sound_win32e_init(): CoInitialize() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}
	else
		fprintf(stderr, "Initialised COM\n");

	if (CoCreateInstance(CLSID_SystemClock, NULL, CLSCTX_INPROC,
                         IID_IReferenceClock, (LPVOID*)&pRefClock) != S_OK)
	{
		fprintf(debug_stream, "sound_win32e_init(): CoCreateInstance() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}
	else
		fprintf(stderr, "Initialised reference clock\n");

	/* 10ms = 100000 100-nanoseconds */
	pRefClock->GetTime(&rtNow);
	if (pRefClock->AdvisePeriodic(rtNow + 166667, 166667, sound_sem, &sound_timer) != S_OK)
	{
		fprintf(debug_stream, "sound_win32e_init(): AdvisePeriodic() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}
	else
		fprintf(stderr, "Started timer\n");

	fprintf(stderr, "Sound server win32e initialised\n");

	return 0;
}

int
sound_win32e_configure(struct _state *s, char *option, char *value)
{
	return 1; /* No options apply to this driver */
}

sound_event_t *
sound_win32e_get_event()
{
	/* receives message for main thread queue */

	MSG msg; /* incoming message */

	sound_event_t *event_temp = NULL;

	if (PeekMessage(&msg, main_wnd, 0, 0, PM_REMOVE))
	{
#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "sound_win32e_get_event() got msg %i (time: %i) (TID: %i)\n", msg.message, msg.time, GetCurrentThreadId());
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
sound_win32e_get_command(GTimeVal *wait_tvp)
{
	/* receives message from sound server queue */

	MSG msg; /* incoming message */

	sound_event_t *event_temp = NULL;

	/* wait for timing semaphore */
	if (WaitForSingleObject(sound_sem, INFINITE) != WAIT_OBJECT_0)
	{
		fprintf(debug_stream, "sound_win32e_get_command(): WaitForSingleObject() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}

	if (PeekMessage( &msg, sound_wnd, 0, 0, PM_REMOVE ))
	{
#if (SSWIN_DEBUG == 1)
		fprintf(stderr, "sound_win32e_get_command() got msg %i (time: %i) (TID: %i)\n", msg.message, msg.time, GetCurrentThreadId());
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

void
sound_win32e_queue_event(unsigned int handle, unsigned int signal, long value)
{
	/* posts message to main thread queue */
	if (PostMessage(main_wnd, signal, handle, value) == 0)
	{
		fprintf(debug_stream, "sound_win32e_queue_event(): PostMessage() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}
}

void
sound_win32e_queue_command(unsigned int handle, unsigned int signal, long value)
{
	/* posts message to sound server queue */
	if (PostMessage(sound_wnd, signal, handle, value) == 0)
	{
		fprintf(debug_stream, "sound_win32e_queue_command(): PostMessage() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}
}

int
sound_win32e_get_data(byte **data_ptr, int *size)
{
	void *data = NULL;

	__try
	{
		EnterCriticalSection(&dq_cs);
		while (!(data = sci_get_from_queue(&data_queue, size)))
		{
			/* no data */
			LeaveCriticalSection(&dq_cs);

			/* yield */
#if (SSWIN_DEBUG == 0)
		fprintf(stderr, "sound_win32e_get_data(): waiting for data (TID %i)\n", GetCurrentThreadId());
#endif
			if (WaitForSingleObject(sound_data_event, INFINITE) != WAIT_OBJECT_0)
			{
				fprintf(debug_stream, "sound_win32e_get_data(): WaitForSingleObject() failed, GetLastError() returned %u\n", GetLastError());
				exit(-1);
			}

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
sound_win32e_send_data(byte *data_ptr, int maxsend)
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

	/* signal event that data is waiting */
	if (SetEvent(sound_data_event) == 0)
	{
		fprintf(debug_stream, "sound_win32e_send_data(): SetEvent() failed, GetLastError() returned %u\n", GetLastError());
		exit(-1);
	}

	return maxsend;
}

void
sound_win32e_exit(struct _state *s)
{
	/* kill server */
	global_sound_server->queue_command(0, SOUND_COMMAND_SHUTDOWN, 0);

	/* kill child thread */
	WaitForSingleObject(child_thread, INFINITE);
	CloseHandle(child_thread);
	DeleteCriticalSection(&dq_cs);

	/* kill reference clock and COM */
	pRefClock->Unadvise(sound_timer);
	pRefClock->Release();
	CoUninitialize();

	/* close handles */
	CloseHandle(sound_data_event);
	CloseHandle(sound_sem);
	DestroyWindow(sound_wnd);
	DestroyWindow(main_wnd);
	UnregisterClass(SOUND_CLASS_NAME, hi_sound);
	UnregisterClass(MAIN_CLASS_NAME, hi_main);
}

int
sound_win32e_save(struct _state *s, char *dir)
{
	return 0;
}

extern "C"
sound_server_t sound_server_win32e = {
	"win32e",
	"0.1",
	0,
	&sound_win32e_init,
	&sound_win32e_configure,
	&sound_win32e_exit,
	&sound_win32e_get_event,
	&sound_win32e_queue_event,
	&sound_win32e_get_command,
	&sound_win32e_queue_command,
	&sound_win32e_get_data,
	&sound_win32e_send_data,
	&sound_win32e_save,
	&sound_restore_default,
	&sound_command_default,
	&sound_suspend_default,
	&sound_resume_default
};

#endif
