/***************************************************************************
 polled_ss.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt

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

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

 History:

   20011105 - extracted polled SCI0 sound server from soundserver.c
                -- Alex Angas

***************************************************************************/

#include <sciresource.h>
#include <stdarg.h>
#include <engine.h>
#include <sound.h>
#include <scitypes.h>
#include <soundserver.h>
#include <midi_device.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOUNDCARD_H
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <sys/ioctl.h>
#  include <sys/soundcard.h>
#endif


#ifdef DEBUG_SOUND_SERVER
	int channel_instrument_orig[16] = {-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1};
	int channel_instrument[16];
#endif

void
sci0_polled_ss(int reverse_stereo, sound_server_state_t *ss_state)
{
	GTimeVal last_played, /* Time the last note was played at */
		wakeup_time, /* Time at which we stop waiting for events */
		ctime; /* 'Current time' (temporary local usage) */
	song_t *newsong = NULL;
	song_t *_songp = NULL;
	GTimeVal suspend_time; /* Time at which the sound server was suspended */
	unsigned int ticks_to_wait;	/* before next midi operation */
	unsigned long usecs_to_sleep;
	int command = 0; /* MIDI operation */

	ss_state->songlib = &_songp;   /* Song library */

	/* initialise default values */
	memset(ss_state->playing_notes, 0, MIDI_CHANNELS * sizeof(playing_notes_t));
	memset(ss_state->mute_channel, MUTE_OFF, MIDI_CHANNELS * sizeof(byte));
	memset(&suspend_time, 0, sizeof(GTimeVal));
	memset(&wakeup_time, 0, sizeof(GTimeVal));
	memset(&ctime, 0, sizeof(GTimeVal));
	sci_get_current_time(&last_played);

	fprintf(debug_stream, "Sound server initialized\n");

	/* sound server loop */
	while (!soundserver_dead)
	{
		sound_event_t *event_temp; /* Temporary pointer to the latest
					   ** instruction we received from the
					   ** main process/thread  */
		GTimeVal wait_tv;	/* Number of seconds/usecs to wait (see select(3)) */
		int old_songpos = 33; /* initial positiion */
		song_t *oldsong = ss_state->current_song;	/* Keep the last song we played referenced to check
								** whether the song was changed later on */
		ticks_to_wait = 0;	/* Ticks to next command */
		fflush(debug_stream); /* Flush debug stream */

		if (ss_state->current_song)
			if (ss_state->current_song->fading == 0) { /* Finished fading out the current song? */
#ifdef DEBUG_SOUND_SERVER
				printf("Song %04x faded out\n", ss_state->current_song->handle);
#endif
				ss_state->current_song->status = SOUND_STATUS_STOPPED;
				global_sound_server->queue_command(ss_state->current_song->handle, SOUND_COMMAND_STOP_HANDLE, 0);
				global_sound_server->queue_event(ss_state->current_song->handle, SOUND_SIGNAL_LOOP, -1);
			}

		/* find the active song */
		ss_state->current_song = song_lib_find_active(ss_state->songlib, ss_state->current_song);
		if (ss_state->current_song == NULL)
		{
			/* if not found, then wait 60 ticks */
			sci_get_current_time((GTimeVal *)&last_played);
			ticks_to_wait = 60;
		}
		else
		{
			/* have an active song */
			long tempticks;

			sci_get_current_time((GTimeVal *)&last_played);
			/* Just played a note (see bottom of the big outer loop), so we
			** reset the timer */

			ticks_to_wait = 0; /* Number of ticks until the next song is played */

			old_songpos = ss_state->current_song->pos;	/* Keep a backup of the current song position,
										** in case we are interrupted and want to resume
										** at this point  */

			/* Handle length escape sequence (see SCI sound specs) */
			while ((tempticks = ss_state->current_song->data[(ss_state->current_song->pos)++]) == SCI_MIDI_TIME_EXPANSION_PREFIX) {
				ticks_to_wait += SCI_MIDI_TIME_EXPANSION_LENGTH;
				if (ss_state->current_song->pos >= ss_state->current_song->size) {
					fprintf(debug_stream, "Sound server: Error: Reached end of song while decoding prefix:\n");
					dump_song(ss_state->current_song);
					ss_state->current_song = NULL;
				}
			}
			ticks_to_wait += tempticks;
		}


		/*--------------*/
		/* Handle input */
		/*--------------*/
		newsong = ss_state->current_song;	/* Same old song; newsong is changed if the parent process
						** orders us to do so.  */

		if (ticks_to_wait)
			do {	/* Delay loop: Exit if we've waited long enough, or if we were
					** ordered to suspend. */

				GTimeVal *wait_tvp; /* Delay parameter (see select(3)) */

				if (!ss_state->suspended) {
					/* Calculate the time we have to sleep */
					wakeup_time = song_next_wakeup_time(&last_played, ticks_to_wait);

					wait_tv = song_sleep_time(&last_played, ticks_to_wait);
					wait_tvp = &wait_tv;

				} else {
					/* Sound server is suspended */
					wait_tvp = NULL; /* select()s infinitely long */
				}

				/* check for command */
				event_temp = global_sound_server->get_command(wait_tvp);
				if (event_temp) { /* We've got mail! */
					sound_event_t event = *event_temp;
					/* Copy event we got, so we can resume without risking a memory leak
					** (OK, this was old code I didn't want to meddle with...)  */
					song_t *modsong; /* Pointer to the song the current instruction
									 ** intends to operate on */
					free(event_temp);

					modsong = song_lib_find(ss_state->songlib, (word)event.handle);

					/* find out what the event is */
					switch (event.signal)
					{
					case SOUND_COMMAND_INIT_HANDLE: {
						byte *data;
						int totalsize = 0;

#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Receiving song for handle %04x: ", event.handle);
#endif
						if (modsong) { /* If the song already exists in cache... */
							int lastmode = song_lib_remove(ss_state->songlib, (word)event.handle); /* remove it */
							if (lastmode == SOUND_STATUS_PLAYING) {
								newsong = ss_state->songlib[0];	/* If we just retreived the current song,
														** we'll have to reset the queue for
														** song_lib_find_active(). */
								/* Force song detection to start with the highest priority song */
								global_sound_server->queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);
							}
							if (modsong == ss_state->current_song)	/* Usually the same case as above, but not always.
													** Reset the song (just been freed) so that we don't
													** try to read from it later. */
								ss_state->current_song = NULL;
						}

						global_sound_server->get_data(&data, &totalsize);	/* Retreive song size */
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Song received\n");
#endif
						modsong = song_new((word)event.handle, data, totalsize, event.value);	/* Create new song */

						if (midi_playrhythm)	/* Enable rhythm channel, if requested by the hardware */
							modsong->flags[RHYTHM_CHANNEL] |= midi_playflag;

						song_lib_add(ss_state->songlib, modsong);	/* Add song to song library */

						ss_state->sound_cue = 127; /* Reset ccc */

						/* set default reverb */
						/* midi_reverb(-1); */
						/* midi_allstop(); */

						global_sound_server->queue_event(event.handle, SOUND_SIGNAL_INITIALIZED, 0); /* Send back ACK */
						break;
					}

					case SOUND_COMMAND_PLAY_HANDLE: {
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Playing handle %04x\n", event.handle);
#endif
						if (modsong) {
#if 0
#ifdef HAVE_SYS_SOUNDCARD_H
							/* Temporary PCM support for testing */
							byte *pcm = NULL;
							int pcm_size, sample_rate;

							if (modsong->it)
								pcm = modsong->it->check_pcm(modsong->it, &pcm_size, &sample_rate);
							else fprintf(debug_stream, "Song has no iterator: Cannot check for PCM!\n");

							if (pcm) {
								int fd;
								int tmp;

								fprintf(debug_stream,"SndSrv: Handle %04x: Playing PCM at %d Hz, %d bytes (%fs)\n",
									event.handle, sample_rate, pcm_size,
									(sample_rate)? ((pcm_size * 1.0) / (sample_rate * 1.0)) : 0);
								fd = open("/dev/dsp", O_WRONLY);
								if (!fd) {
									perror("SndSrv: While opening /dev/dsp");
								} else {
									tmp = AFMT_U8;
									if (ioctl(fd, SOUND_PCM_SETFMT, &tmp))
										perror("SndSrv: While setting the PCM format on /dev/dsp\n");

									tmp = sample_rate;
									if (ioctl(fd, SOUND_PCM_WRITE_RATE, &tmp))
										perror("SndServ: While setting the sample rate on /dev/dsp\n");

									fprintf(debug_stream, "SndSrv: Sample rate set to %d Hz (requested %d Hz)\n",
										tmp, sample_rate);

									if (write(fd, pcm, pcm_size) < pcm_size)
										perror("SndSrv: While writing to /dev/dsp\n");

									close(fd);
									fprintf(debug_stream, "SndSrv: Finished writing PCM to /dev/dsp\n");
								}
							} else {
#endif
#endif
								midi_allstop();
								modsong->status = SOUND_STATUS_PLAYING;
								global_sound_server->queue_event(event.handle, SOUND_SIGNAL_PLAYING, 0);
								newsong = modsong; /* Play this song */
#ifdef DEBUG_SOUND_SERVER
								memcpy(channel_instrument, channel_instrument_orig, sizeof(int)*16);
								/* Reset instrument mappings from song defaults */
#endif
#if 0
#ifdef HAVE_SYS_SOUNDCARD_H
							}
#endif
#endif
						} else {
							fprintf(debug_stream, "Attempt to play invalid handle %04x\n", event.handle);
						}
						break;
					}

					case SOUND_COMMAND_LOOP_HANDLE:
						loop_handle(event.value, (word)event.handle, ss_state);
						break;

					case SOUND_COMMAND_DISPOSE_HANDLE: {
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Disposing handle %04x (value %04x)\n", event.handle, event.value);
#endif
						if (modsong) {
							int lastmode = song_lib_remove(ss_state->songlib, (word)event.handle);
							if (lastmode == SOUND_STATUS_PLAYING) {
								newsong = ss_state->songlib[0]; /* Force song detection to start with the highest priority song */
								global_sound_server->queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);
							}

						} else {
							fprintf(debug_stream, "Attempt to dispose invalid handle %04x\n", event.handle);
						}

						if (modsong == ss_state->current_song)	/* If we just deleted the current song, make sure we don't
												** dereference it later on! */
							ss_state->current_song = NULL;

						break;
					}

					case SOUND_COMMAND_STOP_HANDLE:
						stop_handle((word)event.handle, ss_state);
						break;

					case SOUND_COMMAND_SUSPEND_HANDLE:
						suspend_handle((word)event.handle, ss_state);
						break;

					case SOUND_COMMAND_RESUME_HANDLE:
						resume_handle((word)event.handle, ss_state);
						break;

					case SOUND_COMMAND_SET_VOLUME:
						set_master_volume((unsigned char)event.value, ss_state);
						break;

					case SOUND_COMMAND_SET_MUTE:
						fprintf(debug_stream, "Called mute??? should never happen\n");
						break;

					case SOUND_COMMAND_RENICE_HANDLE:
						renice_handle(event.value, (word)event.handle, ss_state);
						break;

					case SOUND_COMMAND_FADE_HANDLE:
						fade_handle(event.value, (word)event.handle, ss_state);
						break;

					case SOUND_COMMAND_TEST:
						sound_check(midi_polyphony, ss_state);
						break;

					case SOUND_COMMAND_SAVE_STATE: {
						char *dirname;
						int success;
						int size;
						GTimeVal currtime;

						/* Retreive target directory from command stream.
						** Since we might be in a separate process, cwd must be set
						** separately from the main process. That's why we need it here.
						*/
						global_sound_server->get_data((byte **)&dirname, &size);
						sci_get_current_time(&currtime);
						usecs_to_sleep = (currtime.tv_sec - last_played.tv_sec) * 1000000
						         + (currtime.tv_usec - last_played.tv_usec);

						success = soundsrv_save_state(debug_stream,
							global_sound_server->flags & SOUNDSERVER_FLAG_SEPARATE_CWD? dirname : NULL,
							ss_state);

						/* Return soundsrv_save_state()'s return value */
						global_sound_server->send_data((byte *)&success, sizeof(int));
						free(dirname);

						break;
					}

					case SOUND_COMMAND_RESTORE_STATE: {
						char *dirname;
						int success;
						int len;

						global_sound_server->get_data((byte **)&dirname, &len); /* see SAVE_STATE */

						sci_get_current_time(&last_played); /* Get current time, store in last_played.
										    ** We effectively reset the sound delay this
										    ** way, which seems appropriate.
										    */

						success = soundsrv_restore_state(debug_stream,
										 global_sound_server->flags & SOUNDSERVER_FLAG_SEPARATE_CWD? dirname : NULL,
										 ss_state);
						/* Had to be lost in transition to restoring saved games from different sound server types
						last_played.tv_sec -= secs = (usecs_to_sleep - last_played.tv_usec) / 1000000;
						last_played.tv_usec -= (usecs_to_sleep + secs * 1000000);
						*/

						/* Return return value */
						global_sound_server->send_data((byte *)&success, sizeof(int));

						free(dirname);
						/* restore some MIDI device state */
						/* (see MIDI specs). Effectively sets current instruments etc. */
						midi_allstop();
						_restore_midi_state(ss_state);
						change_song(ss_state->current_song, ss_state);
						newsong = ss_state->current_song;

						break;
					}

					case SOUND_COMMAND_PRINT_SONG_INFO: {
						if (ss_state->current_song)
							fprintf(debug_stream, "%04x", ss_state->current_song->handle);
						else
							fprintf(debug_stream, "No song is playing.");

						break;
					}

					case SOUND_COMMAND_PRINT_CHANNELS: {
#ifdef DEBUG_SOUND_SERVER
						print_channels_any(0, ss_state);
#endif
						break;
					}

					case SOUND_COMMAND_PRINT_MAPPING: {
#ifdef DEBUG_SOUND_SERVER
						print_channels_any(1, ss_state);
#endif
						break;
					}

					case SOUND_COMMAND_IMAP_SET_INSTRUMENT:
					case SOUND_COMMAND_IMAP_SET_KEYSHIFT:
					case SOUND_COMMAND_IMAP_SET_FINETUNE:
					case SOUND_COMMAND_IMAP_SET_BENDER_RANGE:
					case SOUND_COMMAND_IMAP_SET_PERCUSSION:
					case SOUND_COMMAND_IMAP_SET_VOLUME:
						imap_set(event.signal, event.handle, event.value);
						break;

					case SOUND_COMMAND_MUTE_CHANNEL:
					case SOUND_COMMAND_UNMUTE_CHANNEL:
						set_channel_mute(event.value, (unsigned char)event.handle, ss_state);
						break;

					case SOUND_COMMAND_SUSPEND_ALL: {
						sci_get_current_time((GTimeVal *)&suspend_time); /* Track the time we suspended at */
						suspend_all(ss_state);
						break;
					}

					case SOUND_COMMAND_RESUME_ALL: {
						GTimeVal resume_time;

						sci_get_current_time((GTimeVal *)&resume_time);
						/* Modify last_played relative to wakeup_time - suspend_time */
						last_played.tv_sec += resume_time.tv_sec - suspend_time.tv_sec - 1;
						last_played.tv_usec += resume_time.tv_usec - suspend_time.tv_usec + 1000000;

						/* Make sure that 0 <= tv_usec <= 999999 */
						last_played.tv_sec -= last_played.tv_usec / 1000000;
						last_played.tv_usec %= 1000000;

						/* All these calculations ensure that, after resuming, we wait for the exact
						** (well, mostly) number of microseconds that we were supposed to wait for before
						** we were suspended.  */

						resume_all(ss_state);
						break;
					}

					case SOUND_COMMAND_STOP_ALL:
						stop_all(ss_state);
						newsong = NULL;
						break;

					case SOUND_COMMAND_SHUTDOWN: {
						fprintf(debug_stream,"Sound server: Received shutdown signal\n");
						midi_close();
						song_lib_free(ss_state->songlib);

						soundserver_dead = 1;
						return;
					}

					default:
						fprintf(debug_stream, "Received invalid signal: %d\n", event.signal);
						break;
					}
				}

				sci_get_current_time((GTimeVal *)&ctime); /* Get current time, store in ctime */

			} while (ss_state->suspended /* Exit when suspended or when we've waited long enough */
				 || (wakeup_time.tv_sec > ctime.tv_sec)
				 || ((wakeup_time.tv_sec == ctime.tv_sec)
				 && (wakeup_time.tv_usec > ctime.tv_usec)));


		/*---------------*/
		/* Handle output */
		/*---------------*/

		if (ss_state->current_song && ss_state->current_song->data) { /* If we have a current song */
			int newcmd;
			guint8 param, param2 = 0;

			newcmd = ss_state->current_song->data[ss_state->current_song->pos]; /* Retreive MIDI command */

			if (newcmd & 0x80) { /* Check for running status mode */
				++(ss_state->current_song->pos);
				command = newcmd;
			} /* else we've got the 'running status' mode defined in the MIDI standard */

			if (command == SCI_MIDI_EOT) { /* End of Track */
				if ((--(ss_state->current_song->loops) != 0) && ss_state->current_song->loopmark) {
#ifdef DEBUG_SOUND_SERVER
					fprintf(debug_stream, "looping back from %d to %d on handle %04x\n",
					        ss_state->current_song->pos, ss_state->current_song->loopmark, ss_state->current_song->handle);
#endif
					ss_state->current_song->pos = ss_state->current_song->loopmark;
					global_sound_server->queue_event(ss_state->current_song->handle, SOUND_SIGNAL_LOOP, ss_state->current_song->loops);

				} else { /* Finished */

					global_sound_server->queue_command(ss_state->current_song->handle, SOUND_COMMAND_STOP_HANDLE, 0);
					global_sound_server->queue_event(ss_state->current_song->handle, SOUND_SIGNAL_LOOP, -1);
					ticks_to_wait = 1; /* Wait one tick, then continue with next song */

					midi_allstop();
				}

			} else { /* Song running normally */
				param = ss_state->current_song->data[ss_state->current_song->pos];

				if (MIDI_cmdlen[command >> 4] == 2)
					param2 = ss_state->current_song->data[ss_state->current_song->pos + 1]; /* If the MIDI instruction
									    ** takes two parameters, read
									    ** second parameter  */
				else
					param2 = 0; /* Waste processor cycles otherwise */

				ss_state->current_song->pos += MIDI_cmdlen[command >> 4];

				if (reverse_stereo
				    && ((command & MIDI_CONTROL_CHANGE) == MIDI_CONTROL_CHANGE)
				    && (param == MIDI_CONTROLLER_PAN_COARSE))
					param2 = (unsigned char)(0x7f - param2); /* Reverse stereo */
#ifdef DEBUG_SOUND_SERVER
				if ((command & 0xf0) == MIDI_INSTRUMENT_CHANGE) /* Change instrument */
					channel_instrument[command & 0xf] = param;
#endif

				if (!((command & 0xf0) == MIDI_NOTE_ON && ss_state->mute_channel[command & 0xf]))
				{
#ifdef DEBUG_SOUND_SERVER
					/* fprintf(stdout, "MIDI (pss): 0x%02x\t%u\t%u\n", command, param, param2); */
#endif

					/* Unless the channel is muted */
					sci_midi_command(debug_stream,
						ss_state->current_song,
						(unsigned char)command,
						param,
						param2,
						&ss_state->sound_cue,
						&(ss_state->playing_notes[command & 0xf]));
				}
			}

			if (ss_state->current_song->fading >= 0) { /* If the song is fading out... */
				long fadeticks = ticks_to_wait;
				ticks_to_wait = !!ticks_to_wait; /* Ticks left? */
				ss_state->current_song->fading -= fadeticks; /* OK, decrease! */

				if (ss_state->current_song->fading < 0)
					ss_state->current_song->fading = 0; /* Faded out after the ticks have run out */
			}

		}

		if (oldsong != newsong) { /* Song change! */
			if (newsong) {	/* New song is for real (not a NULL song) */
				guint8 i;
				for (i = 0; i < MIDI_CHANNELS; i++) {
					if (newsong->instruments[i])
						midi_event2((guint8)(MIDI_INSTRUMENT_CHANGE | i),
							(unsigned char)newsong->instruments[i]);
				}
			}
			if (ss_state->current_song)	/* Muting active song: Un-play the last note/MIDI event */
				ss_state->current_song->pos = old_songpos;
		}

		ss_state->current_song = newsong;
	}

	/* implicitly quit */
}
