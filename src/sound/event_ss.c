/***************************************************************************
 event_ss.c Copyright (C) 2001 Alexander R Angas

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

#define STANDARD_TIMER 0
#define SCI_MIDI_TIME_TIMER 1

static void do_sound(sound_server_state_t *sss)
{
	static midi_op_t cached_cmd;
	/* cached MIDI command stored while waiting for ticks to == 0 */

	/* if sound suspended do nothing */
	if (sss->suspended)
		return;

	/* find the active song */
	sss->current_song = song_lib_find_active(sss->songlib, sss->current_song);

	if (sss->current_song)	/* have an active song */
	{
		midi_op_t this_cmd;

		/* check if song has faded out */
		if (sss->current_song->fading == 0)
		{
#ifdef DEBUG_SOUND_SERVER
		fprintf(debug_stream, "Song %04x faded out\n", sss->current_song->handle);
#endif
			/* set to reset song position */
			sss->current_song->resetflag = 1;

			/* stop this song */
			global_sound_server->queue_command(
				sss->current_song->handle, SOUND_COMMAND_STOP_HANDLE, 0);

		} else if (sss->current_song->fading > 0) {
			/* fade by one more tick */
			sss->current_song->fading = sss->current_song->fading - 1;
		}

		/* see if there is a MIDI command waiting to be played */
		if (cached_cmd.midi_cmd != 0)
		{
			if (cached_cmd.delta_time > 0)
			{
				cached_cmd.delta_time--;	/* maybe next time around! */
				return;

			} else {
				/* the tick is now so build midi cmd from cache */
				this_cmd.pos_in_song = cached_cmd.pos_in_song;
				this_cmd.delta_time = 0;
				this_cmd.midi_cmd = cached_cmd.midi_cmd;
				this_cmd.param1 = cached_cmd.param1;
				this_cmd.param2 = cached_cmd.param2;
				cached_cmd.midi_cmd = 0;
			}

		} else if (sss->current_song->data)	{
			/* no waiting MIDI command so if this song has data, process it */

			static unsigned char last_cmd;
			unsigned char msg_type;

			/* retrieve MIDI command */
			this_cmd.delta_time = 0;

			if (sss->current_song->pos >= sss->current_song->size) {
				fprintf(debug_stream, "Sound server: Error: Reached end of song while decoding prefix:\n");
				dump_song(sss->current_song);
				sss->current_song = NULL;
				return;
			}

			while (sss->current_song->data[(sss->current_song->pos)] == SCI_MIDI_TIME_EXPANSION_PREFIX)
			{
				this_cmd.delta_time += SCI_MIDI_TIME_EXPANSION_LENGTH;
				(sss->current_song->pos)++;
			}
			this_cmd.delta_time += sss->current_song->data[sss->current_song->pos];
			(sss->current_song->pos)++;
			this_cmd.pos_in_song = sss->current_song->pos;
			this_cmd.midi_cmd = sss->current_song->data[sss->current_song->pos];

			if ((this_cmd.midi_cmd >> 4) >= '\x8')	/* this is a command */
			{
				last_cmd = this_cmd.midi_cmd;	/* cache this command */
				(sss->current_song->pos)++;

			} else {	/* this is data */
				/* running status mode - use the last command cached */
				this_cmd.midi_cmd = last_cmd;
			}

			/* work out the parameters */
			msg_type = (unsigned char)(this_cmd.midi_cmd >> 4);
			if (MIDI_PARAMETERS_TWO(msg_type))
			{
				/* two parameters */
				this_cmd.param1 = sss->current_song->data[sss->current_song->pos];
				this_cmd.param2 = sss->current_song->data[(sss->current_song->pos) + 1];
				(sss->current_song->pos) += 2;	/* lined up for next time around */

				if (sss->reverse_stereo &&
					(msg_type == MIDI_MSG_TYPE_CONTROLLER_CHANGE) &&
					(this_cmd.param1 == MIDI_CONTROLLER_PAN_COARSE))
					this_cmd.param2 = 0x7f - this_cmd.param2;	/* reverse stereo */

			} else if (MIDI_PARAMETERS_ONE(msg_type)) {
				/* one parameter */
				this_cmd.param1 = sss->current_song->data[sss->current_song->pos];
				this_cmd.param2 = 0;
				(sss->current_song->pos)++;

			} else if (msg_type == MIDI_MSG_TYPE_SYSTEM) {
				if (this_cmd.midi_cmd <= 0xF6)
					/* all system common messages cancel running status */
					last_cmd = 0;

				/* varying parameters - yay! */

				/* this command is ignored */
				if (this_cmd.midi_cmd == MIDI_SYSTEM_SYSEX)	/* 0xF0 */
				{
					while ((sss->current_song->data[(sss->current_song->pos) + 1] >= 0x80) &&
						   (sss->current_song->data[(sss->current_song->pos) + 1] <= MIDI_SYSTEM_SYSEX_END))
						  (sss->current_song->pos)++;
					return;

				} else if (this_cmd.midi_cmd == MIDI_SYSTEM_SYSEX_END) {	/* 0xF7 */
					/* ignore this if it just appears from nowhere */
					return;

				} else if (this_cmd.midi_cmd == SCI_MIDI_END_OF_TRACK) {	/* 0xFC */
					if ((--(sss->current_song->loops) != 0) && sss->current_song->loopmark)
					{
						sss->current_song->pos = sss->current_song->loopmark;
						global_sound_server->queue_event(
							sss->current_song->handle, SOUND_SIGNAL_LOOP, sss->current_song->loops);

					} else {
						sss->current_song->resetflag = 1;	/* reset song position */
						global_sound_server->queue_command(
							sss->current_song->handle, SOUND_COMMAND_STOP_HANDLE, 0);

					}
					return;	/* no more commands to send if track has ended */
				}

			} else {
				fprintf(debug_stream, "ERROR: do_sound() MIDI command 0x%02X is not recognised\n", this_cmd.midi_cmd);
				exit(-1);
			}

			/* now we have a MIDI command, but is it time for it to be played? */
			if (this_cmd.delta_time > 0)
			{
				/* not yet, make cache */
				cached_cmd.pos_in_song = this_cmd.pos_in_song;
				cached_cmd.delta_time = this_cmd.delta_time;
				cached_cmd.midi_cmd = this_cmd.midi_cmd;
				cached_cmd.param1 = this_cmd.param1;
				cached_cmd.param2 = this_cmd.param2;
				return;
			}

		} else {
			fprintf(debug_stream, "WARNING: do_sound() has handle with no data!\n");
		}

		/* unless the channel is muted, send MIDI command */
		if (!( ((this_cmd.midi_cmd & 0xf0) == MIDI_NOTE_ON) &&
			   sss->mute_channel[this_cmd.midi_cmd & 0xf] ))
		{
#ifdef DEBUG_SOUND_SERVER
			/* fprintf(stdout, "MIDI (ess): %04x:\t0x%02x\t%u\t%u\n", this_cmd.pos_in_song, this_cmd.midi_cmd, this_cmd.param1, this_cmd.param2); */
#endif

			sci_midi_command(debug_stream,
				sss->current_song,
				this_cmd.midi_cmd,
				(unsigned char)this_cmd.param1,
				(unsigned char)this_cmd.param2,
				&(sss->sound_cue),
				&(sss->playing_notes[this_cmd.midi_cmd & 0xf]));
		}
	}
}

void
sci0_event_ss(sound_server_state_t *ss_state)
{
	song_t *_songp = NULL;
	sound_event_t *new_event = NULL;

	/*** initialisation ***/
	ss_state->songlib = &_songp;	/* song library (the local song cache) */
	memset(ss_state->playing_notes, 0, MIDI_CHANNELS * sizeof(playing_notes_t));
	memset(ss_state->mute_channel, MUTE_OFF, MIDI_CHANNELS * sizeof(byte));
	ss_state->master_volume = 0;
	ss_state->sound_cue = 127;

	fprintf(debug_stream, "Sound server initialized\n");
	fflush(debug_stream);	/* flush debug stream. Perhaps should move to main server loop */


	/*** sound server loop ***/
	for (;;)
	{
		/* process any waiting sound */
		do_sound(ss_state);

		/* check for new commands */
		/* this must include some sort of waiting instruction so scheduling
		** under Win32 will work correctly. it is assumed that the call to
		** get_command() takes about 16.6667 milliseconds (because do_sound()
		** needs to be called every 16.6667 milliseconds).
		*/
		new_event = global_sound_server->get_command(NULL);
		if (!new_event)
		{
			continue;	/* no new commands */

		} else if (new_event->signal == UNRECOGNISED_SOUND_SIGNAL) {
			sci_free(new_event);	/* no new commands */
			continue;

		} else if (new_event->signal == SOUND_COMMAND_SHUTDOWN) {
			sci_free(new_event);
			break;		/* drop out of for loop */
		}

		switch (new_event->signal)
		{
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

		case SOUND_COMMAND_SAVE_STATE: {
				char *dirname;
				int size;
				int success;

				global_sound_server->get_data((byte **)&dirname, &size);
				success = soundsrv_save_state(debug_stream,
					global_sound_server->flags & SOUNDSERVER_FLAG_SEPARATE_CWD ? dirname : NULL,
					ss_state);

				/* Return soundsrv_save_state()'s return value */
				global_sound_server->send_data((byte *)&success, sizeof(int));
				free(dirname);
			}
			break;

		case SOUND_COMMAND_RESTORE_STATE: {
				char *dirname;
				int len;
				int success;

				global_sound_server->get_data((byte **)&dirname, &len); /* see SAVE_STATE */

				success = soundsrv_restore_state(debug_stream,
					global_sound_server->flags & SOUNDSERVER_FLAG_SEPARATE_CWD ? dirname : NULL,
					ss_state);

				/* Return return value */
				global_sound_server->send_data((byte *)&success, sizeof(int));
				free(dirname);

				_restore_midi_state(ss_state);
				change_song(ss_state->current_song, ss_state);
			}
			break;

		case SOUND_COMMAND_PRINT_SONG_INFO:
#ifdef DEBUG_SOUND_SERVER
			if ((int)new_event->value == 0)
				print_song_info(ss_state->current_song->handle, ss_state);
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

		default:
			fprintf(debug_stream, "sci0_event_ss(): Received unknown sound signal %i\n", new_event->signal);
		}
		sci_free(new_event);
	}

	/*** shut down server ***/
	fprintf(debug_stream, "Sound server: Received shutdown signal\n");
	midi_close();
	song_lib_free(ss_state->songlib);
}
