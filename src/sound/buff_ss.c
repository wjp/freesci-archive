/***************************************************************************
 buff_ss.c Copyright (C) 2002 Alexander R Angas

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

   20020103 - based on event_ss.c
            - largely temporary until new song iterator system is in place
                -- Alex Angas

***************************************************************************/

#include <engine.h>
#include <sciresource.h>
#include <stdarg.h>
#include <sound.h>
#include <scitypes.h>
#include <soundserver.h>
#include <midi_device.h>
#include <midiout.h>
#include <sys/types.h>

static sound_server_state_t *sss;	/* used to support hacks */

void
do_end_of_track()
{
	/* HACK that will not be required for song iterator based sound system:
	** directly copied from do_sound() in soundserver.c */
			if ((--(sss->current_song->loops) != 0) && sss->current_song->loopmark)
			{
#ifdef DEBUG_SOUND_SERVER
					fprintf(debug_stream, "Looping back from %d to %d on handle %04x\n",
					        sss->current_song->pos, sss->current_song->loopmark, sss->current_song->handle);
#endif
				sss->current_song->pos = sss->current_song->loopmark;
				global_sound_server->queue_event(
					sss->current_song->handle, SOUND_SIGNAL_LOOP, sss->current_song->loops);

#if 0
				sss->current_song = song_lib_find(sss->songlib, sss->current_song->handle);
				if (sss->current_song && sss->current_song->data)
				{
					int i = 1;
					sss->current_song->status = SOUND_STATUS_PLAYING;

					/* call sci_midi_command() continuously until entire song is
					** dumped. */
					while (i)
					{
						sleep(1);
						i = do_sound(sss, 1);
					}
					midiout_win32mci_stream_write_event(NULL, 0, (guint32)-1);

				} else {
					fprintf(stderr, "No data to play\n");
				}
#endif
			} else {
#ifdef DEBUG_SOUND_SERVER
					fprintf(debug_stream, "Reached end of track for handle %04x\n",
					        sss->current_song->handle);
#endif
				sss->current_song->resetflag = 1;	/* reset song position */
				stop_handle((word)sss->current_song->handle, sss);
				global_sound_server->queue_event(
					sss->current_song->handle, SOUND_SIGNAL_LOOP, -1);
			}
}

void
sci0_buff_ss(sound_server_state_t *ss_state)
{
	song_t *_songp = NULL;
	sound_event_t *new_event = NULL;
	sss = ss_state;

	/*** initialisation ***/
	ss_state->songlib = &_songp;	/* song library (the local song cache) */
	memset(ss_state->mute_channel, MIDI_MUTE_OFF, MIDI_CHANNELS * sizeof(byte));
	ss_state->master_volume = 0;
	ss_state->sound_cue = 127;

	fprintf(debug_stream, "Sound server initialized\n");
	fprintf(stderr, "\nThis sound server and MCI stream driver is ALPHA code with these limitations:\n");
	fprintf(stderr, " - current song must stop playing before you can move to a new scene or quit\n");
	fprintf(stderr, " - songs can 'bank up' with their notes playing all at once\n");
	fprintf(stderr, " - looping, fading, saving and restoring is not supported\n");
	fprintf(stderr, " - doesn't work well or at all in some older SCI0 games (KQ4, HQ1)\n");
	fprintf(stderr, "Please use the -Swin32b -Owin32mci_stream command line switches to invoke!\n");
	fprintf(stderr, "\n");
	fflush(debug_stream);


	/*** sound server loop ***/
	for (;;)
	{
		/* check for new commands */
		/* this must include some sort of waiting instruction so scheduling
		** under Win32 will work correctly. it is assumed that the call to
		** get_command() takes about 16.6667 milliseconds.
		*/
		new_event = global_sound_server->get_command(NULL);

		/* process any waiting sound */
		if (new_event->signal == UNRECOGNISED_SOUND_SIGNAL)
			continue;

		if (new_event->signal == SOUND_COMMAND_SHUTDOWN)
			break;		/* drop out of for loop */

		switch (new_event->signal)
		{
		case SOUND_COMMAND_INIT_HANDLE:
			init_handle((int)new_event->value, (word)new_event->handle, ss_state);
			break;

		case SOUND_COMMAND_PLAY_HANDLE: {
			ss_state->current_song = song_lib_find(ss_state->songlib, (word)new_event->handle);
			if (ss_state->current_song && ss_state->current_song->data)
			{
				int i = 1;
				ss_state->current_song->status = SOUND_STATUS_PLAYING;

				/* call sci_midi_command() continuously until entire song is
				** dumped. */
				while (i)
				{
					sleep(1);
					i = do_sound(ss_state, 1);
				}
				midiout_win32mci_stream_write_event(NULL, 0, (guint32)-1);

			} else {
				fprintf(stderr, "No data to play\n");
			}

			global_sound_server->queue_event((word)new_event->handle, SOUND_SIGNAL_PLAYING, 0);
			break;
		}

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
			fprintf(stderr, "Fade is unsupported\n");
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
			fprintf(stderr, "imap_set needs fixing!\n");
			break;

		case SOUND_COMMAND_MUTE_CHANNEL:
		case SOUND_COMMAND_UNMUTE_CHANNEL:
			set_channel_mute((int)new_event->value, (unsigned char)new_event->handle, ss_state);
			fprintf(stderr, "set_channel_mute needs fixing!\n");
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
			fprintf(stderr, "suspend_all needs fixing!\n");
			break;

		case SOUND_COMMAND_RESUME_ALL:
			resume_all(ss_state);
			fprintf(stderr, "resume_all needs fixing!\n");
			break;

		case SOUND_COMMAND_SAVE_STATE:
			save_sound_state(ss_state);
			fprintf(stderr, "save_state needs fixing!\n");
			break;

		case SOUND_COMMAND_RESTORE_STATE:
			restore_sound_state(ss_state);
			fprintf(stderr, "restore_state needs fixing!\n");
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

		case SOUND_COMMAND_REVERSE_STEREO:
			ss_state->reverse_stereo = (new_event->handle) ? 1 : 0;

		default:
			fprintf(debug_stream, "sci0_buff_ss(): Received unknown sound signal %i\n", new_event->signal);
		}
	}

	/*** shut down server ***/
	fprintf(debug_stream, "Sound server: Received shutdown signal\n");
	midi_close();
	song_lib_free(ss_state->songlib);
}
