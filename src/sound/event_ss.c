/***************************************************************************
 event_ss.c Copyright (C) 2001,2002 Alexander R Angas

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

		Alexander R Angas (Alex) <wgd@internode.on.net>

 History:

   20010922 - based on soundserver.c
                -- Alex Angas

***************************************************************************/

#include <sciresource.h>
#include <stdarg.h>
#include <sound.h>
#include <scitypes.h>
#include <soundserver.h>
#include <midi_device.h>
#include <sys/types.h>
#include <pcmout.h>

#define STANDARD_TIMER 0
#define SCI_MIDI_TIME_TIMER 1

/*
#undef DEBUG_SOUND_SERVER
#define DEBUG_SOUND_SERVER 2
*/

void
sci0_event_ss(sound_server_state_t *ss_state)
{
	song_t *_songp = NULL;
	sound_event_t *new_event = NULL;

	/*** initialisation ***/
	ss_state->songlib = &_songp;	/* song library (the local song cache) */
	memset(ss_state->mute_channel, MIDI_MUTE_OFF, MIDI_CHANNELS * sizeof(byte));
	ss_state->master_volume = 0;
	ss_state->sound_cue = 127;

	fprintf(debug_stream, "Sound server initialized\n");
	fflush(debug_stream);	/* flush debug stream. Perhaps should move to main server loop */


	/*** sound server loop ***/
	for (;;)
	{
		/* check for new commands */
		/* this must include some sort of waiting instruction so scheduling
		** under Win32 will work correctly. it is assumed that the call to
		** get_command() takes about 16.6667 milliseconds (because do_sound()
		** needs to be called every 16.6667 milliseconds).
		*/
		new_event = global_sound_server->get_command(NULL);

		/* process any waiting sound */
		if (new_event->signal == UNRECOGNISED_SOUND_SIGNAL)
			continue;

		if (new_event->signal == SOUND_COMMAND_SHUTDOWN)
			break;		/* drop out of for loop */

		switch (new_event->signal)
		{
		case SOUND_COMMAND_DO_SOUND:
			do_sound(ss_state, 0);
			break;

		case SOUND_COMMAND_INIT_HANDLE:
			init_handle((int)new_event->value, (word)new_event->handle, ss_state);
			break;

		case SOUND_COMMAND_PLAY_HANDLE:
			play_handle((int)new_event->value, (word)new_event->handle, ss_state);
			break;

		case SOUND_COMMAND_STOP_HANDLE:
			stop_handle((word)new_event->handle, ss_state);
			break;

		case SOUND_COMMAND_SUSPEND_HANDLE:
			suspend_handle((word)new_event->handle, ss_state);
			break;

		case SOUND_COMMAND_RESUME_HANDLE:
			resume_handle((word)new_event->handle, ss_state);
			break;

		case SOUND_COMMAND_RENICE_HANDLE:
			renice_handle((int)new_event->value, (word)new_event->handle, ss_state);
			break;

		case SOUND_COMMAND_FADE_HANDLE:
			fade_handle((int)new_event->value, (word)new_event->handle, ss_state);
			break;

		case SOUND_COMMAND_LOOP_HANDLE:
			loop_handle((int)new_event->value, (word)new_event->handle, ss_state);
			break;

		case SOUND_COMMAND_DISPOSE_HANDLE:
			dispose_handle((word)new_event->handle, ss_state);
			break;

		case SOUND_COMMAND_IMAP_SET_INSTRUMENT:
		case SOUND_COMMAND_IMAP_SET_KEYSHIFT:
		case SOUND_COMMAND_IMAP_SET_FINETUNE:
		case SOUND_COMMAND_IMAP_SET_BENDER_RANGE:
		case SOUND_COMMAND_IMAP_SET_PERCUSSION:
		case SOUND_COMMAND_IMAP_SET_VOLUME:
			imap_set(new_event->signal,
					 (int)new_event->value,	/* instrument */
					 (word)new_event->handle);	/* handle */
			break;

		case SOUND_COMMAND_MUTE_CHANNEL:
		case SOUND_COMMAND_UNMUTE_CHANNEL:
			set_channel_mute((int)new_event->value, (unsigned char)new_event->handle, ss_state);
			break;

		case SOUND_COMMAND_SET_VOLUME:
			set_master_volume((unsigned char)new_event->value, ss_state);
			break;

		case SOUND_COMMAND_TEST:
			sound_check(midi_polyphony, ss_state);
			break;

		case SOUND_COMMAND_STOP_ALL:
			stop_all(ss_state);
			break;

		case SOUND_COMMAND_SUSPEND_ALL:
			suspend_all(ss_state);
			break;

		case SOUND_COMMAND_RESUME_ALL:
			resume_all(ss_state);
			break;

		case SOUND_COMMAND_SAVE_STATE:
			save_sound_state(ss_state);
			break;

		case SOUND_COMMAND_RESTORE_STATE:
			restore_sound_state(ss_state);
			break;

		case SOUND_COMMAND_PRINT_SONG_INFO:
#ifdef DEBUG_SOUND_SERVER
			if ((int)new_event->value == 0)
				print_song_info((word)ss_state->current_song->handle, ss_state);
			else
				print_song_info((word)new_event->handle, ss_state);
#endif
			break;

		case SOUND_COMMAND_PRINT_CHANNELS:
#ifdef DEBUG_SOUND_SERVER
			print_channels_any(0, ss_state);
#endif
			break;

		case SOUND_COMMAND_PRINT_MAPPING:
#ifdef DEBUG_SOUND_SERVER
			print_channels_any(1, ss_state);
#endif
			break;

		case SOUND_COMMAND_SHUTDOWN:
			fprintf(debug_stream, "sci0_event_ss(): Should never get to shutdown case!\n");
			break;

		case SOUND_COMMAND_REVERSE_STEREO:
			ss_state->reverse_stereo = (new_event->handle) ? 1 : 0;
			break;

		default:
			fprintf(debug_stream, "sci0_event_ss(): Received unknown sound signal %i\n", new_event->signal);
		}
	}

	/*** shut down server ***/
	fprintf(debug_stream, "Sound server: Received shutdown signal\n");
	pcmout_close();
	midi_close();
	song_lib_free(ss_state->songlib);
}
