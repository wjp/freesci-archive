/***************************************************************************
 ksound.c Copyright (C) 1999 Christoph Reichenbach


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

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/

#include <engine.h>

#define _K_SOUND_INIT 0
#define _K_SOUND_PLAY 1
#define _K_SOUND_NOP 2
#define _K_SOUND_DISPOSE 3
#define _K_SOUND_MUTE 4
#define _K_SOUND_STOP 5
#define _K_SOUND_SUSPEND 6
#define _K_SOUND_RESUME 7
#define _K_SOUND_VOLUME 8
#define _K_SOUND_UPDATE 9
#define _K_SOUND_FADE 10
#define _K_SOUND_CHECK_DRIVER 11
#define _K_SOUND_STOP_ALL 12

void
process_sound_events(state_t *s) /* Get all sound events, apply their changes to the heap */
{
	sound_event_t *event;

	if (s->sfx_driver == NULL)
		return;

	while ((event = s->sfx_driver->get_event(s))) {
		heap_ptr obj = event->handle;

		if (is_object(s, obj))
			switch (event->signal) {
			case SOUND_SIGNAL_CUMULATIVE_CUE: {
				int signal = GET_SELECTOR(obj, signal);
				SCIkdebug(SCIkSOUND,"Received cumulative cue for %04x\n", obj);

				PUT_SELECTOR(obj, signal, signal + 1);
			}
			break;

			case SOUND_SIGNAL_LOOP:

				SCIkdebug(SCIkSOUND,"Received loop signal for %04x\n", obj);
				PUT_SELECTOR(obj, loop, event->value);
				PUT_SELECTOR(obj, signal, -1);
				break;

			case SOUND_SIGNAL_FINISHED:

				SCIkdebug(SCIkSOUND,"Received finished signal for %04x\n", obj);
				PUT_SELECTOR(obj, state, _K_SOUND_STATUS_STOPPED);
				//PUT_SELECTOR(obj, loop, -1);
				break;

			case SOUND_SIGNAL_PLAYING:

				SCIkdebug(SCIkSOUND,"Received playing signal for %04x\n", obj);
				PUT_SELECTOR(obj, state, _K_SOUND_STATUS_PLAYING);
				break;

			case SOUND_SIGNAL_PAUSED:

				SCIkdebug(SCIkSOUND,"Received pause signal for %04x\n", obj);
				PUT_SELECTOR(obj, state, _K_SOUND_STATUS_PAUSED);
				break;

			case SOUND_SIGNAL_RESUMED:

				SCIkdebug(SCIkSOUND,"Received resume signal for %04x\n", obj);
				PUT_SELECTOR(obj, state, _K_SOUND_STATUS_PAUSED);
				break;

			case SOUND_SIGNAL_INITIALIZED:

				PUT_SELECTOR(obj, state, _K_SOUND_STATUS_INITIALIZED);
				SCIkdebug(SCIkSOUND,"Received init signal for %04x\n", obj);
				break;

			case SOUND_SIGNAL_ABSOLUTE_CUE:

				SCIkdebug(SCIkSOUND,"Received absolute cue %d for %04x\n", event->value, obj);
				PUT_SELECTOR(obj, signal, event->value);
				break;

			default:
				SCIkwarn(SCIkERROR, "Unknown sound signal: %d\n", event->signal);
			}

		free(event);
	}
}


void
kDoSound(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	word command = UPARAM(0);
	heap_ptr obj = UPARAM_OR_ALT(1, 0);

	CHECK_THIS_KERNEL_FUNCTION;

	if (SCI_VERSION_MAJOR(s->version) != 0) return; /* Can't do much else ATM */ 
  
	if (s->debug_mode & (1 << SCIkSOUNDCHK_NR)) {
		int i;

		SCIkdebug(SCIkSOUND, "Command 0x%x", command);
		switch (command) {
		case 0: sciprintf("[InitObj]"); break;
		case 1: sciprintf("[Play]"); break;
		case 2: sciprintf("[NOP]"); break;
		case 3: sciprintf("[DisposeHandle]"); break;
		case 4: sciprintf("[SetSoundOn(?)]"); break;
		case 5: sciprintf("[Stop]"); break;
		case 6: sciprintf("[Suspend]"); break;
		case 7: sciprintf("[Resume]"); break;
		case 8: sciprintf("[Get(Set?)Volume]"); break;
		case 9: sciprintf("[Signal: Obj changed]"); break;
		case 10: sciprintf("[Fade(?)]"); break;
		case 11: sciprintf("[ChkDriver]"); break;
		case 12: sciprintf("[StopAll]"); break;
		}

		sciprintf("(");
		for (i = 1; i < argc; i++) {
			sciprintf("%04x", UPARAM(i));
			if (i + 1 < argc)
				sciprintf(", ");
		}
		sciprintf(")\n");
	}


	if (s->sfx_driver)
		switch (command) {
		case _K_SOUND_INIT:
			s->sfx_driver->command(s, SOUND_COMMAND_INIT_SONG, obj, GET_SELECTOR(obj, number));
			break;

		case _K_SOUND_PLAY:

			s->sfx_driver->command(s, SOUND_COMMAND_PLAY_HANDLE, obj, 0);
			s->sfx_driver->command(s, SOUND_COMMAND_SET_LOOPS, obj, GET_SELECTOR(obj, loop));
			break;

		case _K_SOUND_NOP:

			break;

		case _K_SOUND_DISPOSE:

			s->sfx_driver->command(s, SOUND_COMMAND_DISPOSE_HANDLE, obj, 0);
			break;

		case _K_SOUND_STOP:

			s->sfx_driver->command(s, SOUND_COMMAND_STOP_HANDLE, obj, 0);
			break;

		case _K_SOUND_SUSPEND:

			s->sfx_driver->command(s, SOUND_COMMAND_SUSPEND_HANDLE, obj, 0);
			break;

		case _K_SOUND_RESUME:

			s->sfx_driver->command(s, SOUND_COMMAND_RESUME_HANDLE, obj, 0);
			break;

		case _K_SOUND_MUTE: {
		  /* if there's a parameter, we're setting it.  Otherwise,
		     we're querying it. */
		  int param = UPARAM_OR_ALT(1,-1);

		  if (param != -1)
		    s->acc = s->sfx_driver->command(s, SOUND_COMMAND_SET_MUTE, obj, param);
		  else
		    s->acc = s->sfx_driver->command(s, SOUND_COMMAND_GET_MUTE, obj, 0);

		}
		break;
		case _K_SOUND_VOLUME:

		        /* range from 0x0 to 0xf */
		        /* parameter optional.  set to -1 if just query. */
		        obj = UPARAM_OR_ALT(1, -1);
		        s->acc = s->sfx_driver->command(s, SOUND_COMMAND_SET_VOLUME, 0, obj);
		        break;

		case _K_SOUND_UPDATE:

			s->sfx_driver->command(s, SOUND_COMMAND_RENICE_HANDLE, obj, GET_SELECTOR(obj, priority));
			s->sfx_driver->command(s, SOUND_COMMAND_SET_LOOPS, obj, GET_SELECTOR(obj, loop));
			break;

		case _K_SOUND_FADE:

			s->sfx_driver->command(s, SOUND_COMMAND_FADE_HANDLE, obj, 120); /* Fade out in 2 secs */
			break;

		case _K_SOUND_CHECK_DRIVER:
		  
			s->acc = s->sfx_driver->command(s, SOUND_COMMAND_TEST, 0, 0);
			break;

		case _K_SOUND_STOP_ALL:

			s->acc = s->sfx_driver->command(s, SOUND_COMMAND_STOP_ALL, 0, 0);
			break;

		default:
			SCIkwarn(SCIkWARNING, "Unhandled DoSound command: %x\n", command);

		}
	process_sound_events(s); /* Take care of incoming events */

}
