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

#define _K_SCI0_SOUND_INIT_HANDLE 0
#define _K_SCI0_SOUND_PLAY_HANDLE 1
#define _K_SCI0_SOUND_NOP 2
#define _K_SCI0_SOUND_DISPOSE_HANDLE 3
#define _K_SCI0_SOUND_MUTE_SOUND 4
#define _K_SCI0_SOUND_STOP_HANDLE 5
#define _K_SCI0_SOUND_SUSPEND_HANDLE 6
#define _K_SCI0_SOUND_RESUME_HANDLE 7
#define _K_SCI0_SOUND_VOLUME 8
#define _K_SCI0_SOUND_UPDATE_VOL_PRI 9
#define _K_SCI0_SOUND_FADE_HANDLE 10
#define _K_SCI0_SOUND_GET_POLYPHONY 11
#define _K_SCI0_SOUND_STOP_ALL 12

#define _K_SCI01_SOUND_MASTER_VOLME 0 /* Set/Get */
#define _K_SCI01_SOUND_MUTE_SOUND 1
#define _K_SCI01_SOUND_UNUSED 2
#define _K_SCI01_SOUND_GET_POLYPHONY 3
#define _K_SCI01_SOUND_PLAY_HANDLE 4
#define _K_SCI01_SOUND_INIT_HANDLE 5
#define _K_SCI01_SOUND_DISPOSE_HANDLE 6
#define _K_SCI01_SOUND_UPDATE_HANDLE 7
#define _K_SCI01_SOUND_STOP_HANDLE 8
#define _K_SCI01_SOUND_SUSPEND_HANDLE 9 /* or resume */
#define _K_SCI01_SOUND_FADE_HANDLE 10
#define _K_SCI01_SOUND_UPDATE_CUES 11
#define _K_SCI01_SOUND_PITCH_WHEEL 12
#define _K_SCI01_SOUND_REVERB 13 /* Get/Set */
#define _K_SCI01_SOUND_CONTROLLER 14
 
#define _K_SCI1_SOUND_MASTER_VOLME 0 /* Set/Get */
#define _K_SCI1_SOUND_MUTE_SOUND 1
#define _K_SCI1_SOUND_UNUSED1 2
#define _K_SCI1_SOUND_GET_POLYPHONY 3
#define _K_SCI1_SOUND_CD_AUDIO 4
#define _K_SCI1_SOUND_PLAY_HANDLE 5
#define _K_SCI1_SOUND_INIT_HANDLE 6
#define _K_SCI1_SOUND_DISPOSE_HANDLE 7
#define _K_SCI1_SOUND_UPDATE_HANDLE 8
#define _K_SCI1_SOUND_STOP_HANDLE 9
#define _K_SCI1_SOUND_SUSPEND_HANDLE 10 /* or resume */
#define _K_SCI1_SOUND_FADE_HANDLE 11
#define _K_SCI1_SOUND_HOLD_HANDLE 12
#define _K_SCI1_SOUND_UNUSED2 13
#define _K_SCI1_SOUND_SET_HANDLE_VOLUME 14
#define _K_SCI1_SOUND_SET_HANDLE_PRIORITY 15
#define _K_SCI1_SOUND_SET_HANDLE_LOOP 16
#define _K_SCI1_SOUND_UPDATE_CUES 17
#define _K_SCI1_SOUND_MIDI_SEND 18 
#define _K_SCI1_SOUND_REVERB 19 /* Get/Set */
#define _K_SCI1_SOUND_UPDATE_VOL_PRI 20

void
process_sound_events(state_t *s) /* Get all sound events, apply their changes to the heap */
{
	/*
	sound_event_t *event = NULL;

	if (s->sound_server == NULL)
		return;

	while ((event = s->sound_server->get_event(s))) {
		heap_ptr obj = event->handle;

		if (is_object(s, obj))
		{
			int signal = GET_SELECTOR(obj, signal);
			
			switch(event->signal)
			{
			case SOUND_SIGNAL_CUMULATIVE_CUE:
				SCIkdebug(SCIkSOUND,"Received cumulative cue for %04x\n", obj);
				PUT_SELECTOR(obj, signal, signal + 1);
				break;

			case SOUND_SIGNAL_LOOP:
				SCIkdebug(SCIkSOUND,"Received loop signal for %04x\n", obj);
				PUT_SELECTOR(obj, signal, -1);
				break;

			case SOUND_SIGNAL_FINISHED:
				SCIkdebug(SCIkSOUND,"Received finished signal for %04x\n", obj);
				PUT_SELECTOR(obj, state, _K_SOUND_STATUS_STOPPED);
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
				break;
			}
		}

		free(event);
	}
	*/
}


void
kDoSound_SCI0(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = UPARAM_OR_ALT(1, 0);
	word command = UPARAM(0);

	CHECK_THIS_KERNEL_FUNCTION;

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
		default: sciprintf("[unknown]"); break;
		}

		sciprintf("(");
		for (i = 1; i < argc; i++) {
			sciprintf("%04x", UPARAM(i));
			if (i + 1 < argc)
				sciprintf(", ");
		}
		sciprintf(")\n");
	}


	switch (command) {
	case _K_SCI0_SOUND_INIT_HANDLE:
		/*			s->sound_server->command(s, SOUND_COMMAND_INIT_HANDLE, obj, GET_SELECTOR(obj, number));*/
		break;

	case _K_SCI0_SOUND_PLAY_HANDLE:
		/*
		  s->sound_server->command(s, SOUND_COMMAND_PLAY_HANDLE, obj, 0);
		  s->sound_server->command(s, SOUND_COMMAND_LOOP_HANDLE, obj, GET_SELECTOR(obj, loop));
		*/
		break;

	case _K_SCI0_SOUND_NOP:
		break;

	case _K_SCI0_SOUND_DISPOSE_HANDLE:
		/*s->sound_server->command(s, SOUND_COMMAND_DISPOSE_HANDLE, obj, 0);*/
		break;

	case _K_SCI0_SOUND_STOP_HANDLE:
		/*s->sound_server->command(s, SOUND_COMMAND_STOP_HANDLE, obj, 0);*/
		break;

	case _K_SCI0_SOUND_SUSPEND_HANDLE:
		/*s->sound_server->command(s, SOUND_COMMAND_SUSPEND_HANDLE, obj, 0);*/
		break;

	case _K_SCI0_SOUND_RESUME_HANDLE:
		/*s->sound_server->command(s, SOUND_COMMAND_RESUME_HANDLE, obj, 0);*/
		break;

	case _K_SCI0_SOUND_MUTE_SOUND: {
		/* if there's a parameter, we're setting it.  Otherwise,
		   we're querying it. */
		/*int param = UPARAM_OR_ALT(1,-1);

		if (param != -1)
		s->acc = s->sound_server->command(s, SOUND_COMMAND_SET_MUTE, 0, param);
		else
		s->acc = s->sound_server->command(s, SOUND_COMMAND_GET_MUTE, 0, 0);*/

	}
		break;

	case _K_SCI0_SOUND_VOLUME: {
		/* range from 0x0 to 0xf */
		/* parameter optional. If present, set.*/
		/*int vol = UPARAM_OR_ALT(1, -1);

		if (vol != -1)
		s->acc = s->sound_server->command(s, SOUND_COMMAND_SET_VOLUME, 0, vol);
		else
		s->acc = s->sound_server->command(s, SOUND_COMMAND_GET_VOLUME, 0, 0);
		*/
	}
		break;

	case _K_SCI0_SOUND_UPDATE_VOL_PRI:
		/*s->sound_server->command(s, SOUND_COMMAND_RENICE_HANDLE, obj, GET_SELECTOR(obj, priority));
		  s->sound_server->command(s, SOUND_COMMAND_LOOP_HANDLE, obj, GET_SELECTOR(obj, loop));*/
		break;

	case _K_SCI0_SOUND_FADE_HANDLE:
		/*s->sound_server->command(s, SOUND_COMMAND_FADE_HANDLE, obj, 120);*/ /* Fade out in 2 secs */
		break;

	case _K_SCI0_SOUND_GET_POLYPHONY:
		/*s->acc = s->sound_server->command(s, SOUND_COMMAND_TEST, 0, 0);*/
		break;

	case _K_SCI0_SOUND_STOP_ALL:
		/*s->acc = s->sound_server->command(s, SOUND_COMMAND_STOP_ALL, 0, 0);*/
		break;

	default:
		SCIkwarn(SCIkWARNING, "Unhandled DoSound command: %x\n", command);

	}
		process_sound_events(s); /* Take care of incoming events */

}


void
kDoSound_SCI01(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	word command = UPARAM(0);
	int obj = UPARAM_OR_ALT(1, 0);
	
	switch (command)
	{
	case _K_SCI01_SOUND_MASTER_VOLME :
	{
		/*int vol = UPARAM_OR_ALT (1, -1);

		if (vol != -1)
		        s->acc = s->sound_server->command(s, SOUND_COMMAND_SET_VOLUME, 0, vol);
		else
		s->acc = s->sound_server->command(s, SOUND_COMMAND_GET_VOLUME, 0, 0);*/
		break;
	}
	case _K_SCI01_SOUND_MUTE_SOUND :
	{
		/* if there's a parameter, we're setting it.  Otherwise,
		   we're querying it. */
		/*int param = UPARAM_OR_ALT(1,-1);
		
		if (param != -1)
			s->acc = s->sound_server->command(s, SOUND_COMMAND_SET_MUTE, 0, param);
		else
		s->acc = s->sound_server->command(s, SOUND_COMMAND_GET_MUTE, 0, 0);*/

		break;
	}
	case _K_SCI01_SOUND_UNUSED :
	{
		break;
	}
	case _K_SCI01_SOUND_GET_POLYPHONY :
	{
		/*s->acc = s->sound_server->command(s, SOUND_COMMAND_TEST, 0, 0);*/
		break;
	}
	case _K_SCI01_SOUND_PLAY_HANDLE :
	{

		int looping = GET_SELECTOR(obj, loop);
		int vol = GET_SELECTOR(obj, vol);
		int pri = GET_SELECTOR(obj, pri);
		
		break;
	}
	case _K_SCI01_SOUND_INIT_HANDLE :
	{
		int number = GET_SELECTOR(obj, number);

		int looping = GET_SELECTOR(obj, loop);
		int vol = GET_SELECTOR(obj, vol);
		int pri = GET_SELECTOR(obj, pri);
		
		break;
	}
	case _K_SCI01_SOUND_DISPOSE_HANDLE :
	{
		break;
	}
	case _K_SCI01_SOUND_UPDATE_HANDLE :
	{
		/* FIXME: Get these from the sound server */
		int signal = 0;
		int min = 0;
		int sec = 0;
		int frame = 0;

		/* FIXME: Update the sound server state with these */
		int looping = GET_SELECTOR(obj, loop);
		int vol = GET_SELECTOR(obj, vol);
		int pri = GET_SELECTOR(obj, pri);

		PUT_SELECTOR(obj, signal, signal);
		PUT_SELECTOR(obj, min, min);
		PUT_SELECTOR(obj, sec, sec);
		PUT_SELECTOR(obj, frame, frame);
		
		break;
	}
	case _K_SCI01_SOUND_STOP_HANDLE :
	{
		PUT_SELECTOR(obj, signal, -1);
		break;
	}
	case _K_SCI01_SOUND_SUSPEND_HANDLE :
	{
		int state = UPARAM(2);
		/*
		if (state)
			s->sound_server->command(s, SOUND_COMMAND_SUSPEND_HANDLE, obj, 0);
		else
			s->sound_server->command(s, SOUND_COMMAND_RESUME_HANDLE, obj, 0);
		*/
		break;
	}
	case _K_SCI01_SOUND_FADE_HANDLE :
	{
		/* There are four parameters that control the fade here.
		 * TODO: Figure out the exact semantics */
		break;
	}
	case _K_SCI01_SOUND_UPDATE_CUES :
	{
		/* FIXME: Fetch these from the sound server */

		int dataInc = 0;
		int signal = 1; /* This allows QfG2 to continue past the ogre */
		int min = 0;
		int sec = 0;
		int frame = 0;

		if (signal==0xff)
			/* Stop the handle */
		;	
		
		if (dataInc!=GET_SELECTOR(obj, dataInc))
		{
			PUT_SELECTOR(obj, dataInc, dataInc);
			PUT_SELECTOR(obj, signal, dataInc+0x7f);
		} else
		{
			PUT_SELECTOR(obj, signal, signal);
		}

		PUT_SELECTOR(obj, min, min);
		PUT_SELECTOR(obj, sec, sec);
		PUT_SELECTOR(obj, frame, frame);
		break;
	}
	case _K_SCI01_SOUND_PITCH_WHEEL :
	{
		break;
	}
	case _K_SCI01_SOUND_REVERB :
	{
		break;
	}
	case _K_SCI01_SOUND_CONTROLLER :
	{
		break;
	}
	}
}

void
kDoSound_SCI1(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	word command = UPARAM(0);

	switch (command)
	{
	case _K_SCI1_SOUND_MASTER_VOLME :
	{
	   /*int vol = UPARAM_OR_ALT (1, -1);

		if (vol != -1)
		        s->acc = s->sound_server->command(s, SOUND_COMMAND_SET_VOLUME, 0, vol);
		else
		        s->acc = s->sound_server->command(s, SOUND_COMMAND_GET_VOLUME, 0, 0);
			break;*/
	}
	case _K_SCI1_SOUND_MUTE_SOUND :
	{
		/* if there's a parameter, we're setting it.  Otherwise,
		   we're querying it. */
		/*int param = UPARAM_OR_ALT(1,-1);
		
		if (param != -1)
			s->acc = s->sound_server->command(s, SOUND_COMMAND_SET_MUTE, 0, param);
		else
			s->acc = s->sound_server->command(s, SOUND_COMMAND_GET_MUTE, 0, 0);
			break;*/
	}
	case _K_SCI1_SOUND_UNUSED1 :
	{
		break;
	}
	case _K_SCI1_SOUND_GET_POLYPHONY :
	{
		/*s->acc = s->sound_server->command(s, SOUND_COMMAND_TEST, 0, 0);*/
		break;
	}
	case _K_SCI1_SOUND_CD_AUDIO : 
	{
		break;
	}
	case _K_SCI1_SOUND_PLAY_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_INIT_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_DISPOSE_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_UPDATE_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_STOP_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_SUSPEND_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_FADE_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_HOLD_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_UNUSED2 :
	{
		break;
	}
	case _K_SCI1_SOUND_SET_HANDLE_VOLUME :
	{
		break;
	}
	case _K_SCI1_SOUND_SET_HANDLE_PRIORITY :
	{
		break;
	}
	case _K_SCI1_SOUND_SET_HANDLE_LOOP :
	{
		break;
	}
	case _K_SCI1_SOUND_UPDATE_CUES :
	{
		break;
	}
	case _K_SCI1_SOUND_MIDI_SEND :
	{
		break;
	}
	case _K_SCI1_SOUND_REVERB :
	{
		break;
	}
	case _K_SCI1_SOUND_UPDATE_VOL_PRI :
	{
		break;
	}
	}
}

void
kDoSound(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	if (s->version>SCI_VERSION_FTU_DOSOUND_VARIANT_2)
		kDoSound_SCI1(s, funct_nr, argc, argp);
	else if (s->version>SCI_VERSION_FTU_DOSOUND_VARIANT_1)
		kDoSound_SCI01(s, funct_nr, argc, argp);
	else
		kDoSound_SCI0(s, funct_nr, argc, argp);
}

	
