/***************************************************************************
 event_ss_win32.c Copyright (C) 2001 Alexander R Angas

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

#define MAIN_CLASS_NAME "FreeSCI Main Receiving from Event SS"
#define SOUND_CLASS_NAME "Event SS Receiving from FreeSCI Main"
#define WINDOW_SUFFIX " (Window)"

#define SSWIN_DEBUG 0

extern "C" sound_server_t sound_server_win32e;

static HWND main_wnd, sound_wnd;
static HANDLE child_thread;
static HANDLE sound_sem;
static DWORD sound_timer;
static IReferenceClock *pRefClock;
static state_t *gs;
static sci_queue_t data_queue;


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
	HINSTANCE hi_sound;
	memset(&sss, 0, sizeof(sound_server_state_t));

#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "win32e_soundserver_init() begins with TID %i\n", GetCurrentThreadId());
#endif

	/*** register window class for messages to sound server ***/
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = SoundWndProc;
	hi_sound = GetModuleHandle(NULL);
	wc.hInstance = hi_sound;
	wc.lpszClassName = SOUND_CLASS_NAME;

	if (!RegisterClass(&wc))
		fprintf(stderr, "Can't register window class for sound thread\n");

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
		fprintf(stderr, "Couldn't create sound window class\n");

#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "win32e_soundserver_init() sound_wnd %i, TID %i\n", sound_wnd, GetCurrentThreadId());
#endif

	/* start the sound server */
	sci0_event_ss(&sss);

	return 0;
}

int
sound_win32e_init(struct _state *s, int flags)
{
	WNDCLASS wc;
	HINSTANCE hi_main;
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
	sci_init_queue(&data_queue);

	/*** register window class for messages to main thread ***/
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = MainWndProc;
	hi_main = GetModuleHandle(NULL);
	wc.hInstance = hi_main;
	wc.lpszClassName = MAIN_CLASS_NAME;

	if (!RegisterClass(&wc))
		fprintf(stderr, "Can't register window class for main thread\n");

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
		fprintf(stderr, "Couldn't create sound window class\n");

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
	}

	/*** start timer ***/
	sound_sem = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
	if (sound_sem == NULL)
	{
		fprintf(stderr, "Error creating semaphore\n");
		exit(-1);
	}

	if (CoInitialize(NULL) != S_OK)
	{
		fprintf(stderr, "Error initialising COM\n");
		exit(-1);
	}

	if (CoCreateInstance(CLSID_SystemClock, NULL, CLSCTX_INPROC,
                         IID_IReferenceClock, (LPVOID*)&pRefClock) != S_OK)
	{
		fprintf(stderr, "Error initialising reference clock\n");
		exit(-1);
	}

	/* 10ms = 100000 100-nanoseconds */
	pRefClock->GetTime(&rtNow);
	if (pRefClock->AdvisePeriodic(rtNow + 166667, 166667, sound_sem, &sound_timer) != S_OK)
		fprintf(stderr, "Error creating timer\n");

	fprintf(stderr, "Win32 event sound server initialised\n");

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
	event_temp = (sound_event_t*)malloc(sizeof(sound_event_t));

	if (PeekMessage(&msg, main_wnd, 0, 0, PM_REMOVE))
	{
#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "sound_win32e_get_event() called with msg %i (time: %i) (TID: %i)\n", msg.message, msg.time, GetCurrentThreadId());
#endif
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
//	/* if wait_tvp is NULL, messages are checked, otherwise passed on */

	MSG msg; /* incoming message */

	sound_event_t *event_temp = NULL;
	event_temp = (sound_event_t*)malloc(sizeof(sound_event_t));

	if (PeekMessage(&msg, sound_wnd, 0, 0, PM_REMOVE))
	{
#if (SSWIN_DEBUG == 1)
	fprintf(stderr, "sound_win32e_get_command() called with msg %i (time: %i) (TID: %i)\n", msg.message, msg.time, GetCurrentThreadId());
#endif
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
		fprintf(stderr, "Post of sound event to main thread failed\n");
}

void
sound_win32e_queue_command(unsigned int handle, unsigned int signal, long value)
{
	/* posts message to sound server queue */
	if (PostMessage(sound_wnd, signal, handle, value) == 0)
		fprintf(stderr, "Post of sound command to sound thread failed\n");
}

int
sound_win32e_get_data(byte **data_ptr, int *size)
{
	/* returns 1 if do_sound() should be called */
	/* data_ptr and size are ignored */
	
	HRESULT hr;

	if (size == NULL)
	{
		hr = WaitForSingleObject(sound_sem, 0);

		if (hr == WAIT_OBJECT_0)
			return 1;
		else
			return 0;

	} else {
		/* note: no need for race condition protection as only main thread calls this */
		void *data = NULL;
		while (!(data = sci_get_from_queue(&data_queue, size)))
			;
		if (!data)
			fprintf(stderr, "Got no sound data from queue!\n");
		*data_ptr = (byte *)data;
		return *size;
	}
}

int
sound_win32e_send_data(byte *data_ptr, int maxsend)
{
	/* note: no need for race condition protection */
	sci_add_to_queue(&data_queue, sci_memdup(data_ptr, maxsend), maxsend);
	return maxsend;
}

void
sound_win32e_exit(struct _state *s)
{
	global_sound_server->queue_command(0, SOUND_COMMAND_SHUTDOWN, 0); /* Kill server */

	/* clean up */
	if (CloseHandle(sound_wnd) == 0)
		fprintf(stderr, "Error closing handle of sound thread\n");

	WaitForSingleObject(child_thread, INFINITE);
	CloseHandle(child_thread);

	/* kill reference clock and COM */
	if (pRefClock->Unadvise(sound_timer) != S_OK)
		fprintf(stderr, "Error deleting timer\n");
	CoUninitialize();
	if (CloseHandle(sound_sem) == 0)
		fprintf(stderr, "Error closing semaphore handle\n");
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
