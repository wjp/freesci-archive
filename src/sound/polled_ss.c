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

static int
_sound_expect_answer(char *timeoutmessage, int def_value)
{
	int *success;
	int retval;
	int size;

	global_sound_server->get_data((byte **)&success, &size);

	retval = *success;
	free(success);
	return retval;

}

static int
_sound_transmit_text_expect_anwer(state_t *s, char *text, int command, char *timeoutmessage)
{
	int count;

	count = strlen(text) + 1;
	global_sound_server->queue_command(0, command, count);
	if (global_sound_server->send_data((unsigned char *)text, count) != count) {
		fprintf(debug_stream, "_sound_transmit_text_expect_anwer():"
			" sound_send_data returned < count\n");
	}

	return _sound_expect_answer(timeoutmessage, 1);
}

int
sound_save_default(state_t *s, char *dir)
{
	return _sound_transmit_text_expect_anwer(s, dir, SOUND_COMMAND_SAVE_STATE,
					   "Sound server timed out while saving\n");
}

int
sound_restore_default(state_t *s, char *dir)
{
	return _sound_transmit_text_expect_anwer(s, dir, SOUND_COMMAND_RESTORE_STATE,
					   "Sound server timed out while restoring\n");
}

void
sci0_polled_ss(int reverse_stereo, sound_server_state_t *ss_state)
{
	GTimeVal last_played, /* Time the last note was played at */
		wakeup_time, /* Time at which we stop waiting for events */
		ctime; /* 'Current time' (temporary local usage) */
	playing_notes_t playing_notes[16];	/* keeps track of polyphony */
	byte mute_channel[MIDI_CHANNELS];	/* which channels are muted */
	song_t *_songp = NULL;
	songlib_t songlib = &_songp;   /* Song library */
	song_t *newsong, *song = NULL; /* The song we're playing */
	int master_volume = 0;  /* the master volume.. whee */
	int ccc = 127; /* cumulative cue counter- see SCI sound specs */
	int suspended = 0; /* Used to suspend the sound server */
	GTimeVal suspend_time; /* Time at which the sound server was suspended */
	int command = 0; /* MIDI operation */

	/* initialise default values */
	memset(playing_notes, 0, 16 * sizeof(playing_notes_t));
	memset(mute_channel, MUTE_OFF, MIDI_CHANNELS * sizeof(byte));
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
		long ticks = 0; /* Ticks to next command */
		int old_songpos = 33; /* initial positiion */
		song_t *oldsong = song;	/* Keep the last song we played referenced to check
								** whether the song was changed later on */
		fflush(debug_stream); /* Flush debug stream */

		if (song && (song->fading == 0)) { /* Finished fading out the current song? */
			printf("song %04x faded out \n", song->handle);
			song->status = SOUND_STATUS_STOPPED;
			midi_allstop();
			song->pos = 33;
			song->loopmark = 33; /* Reset position */
			global_sound_server->queue_event(song->handle, SOUND_SIGNAL_LOOP, -1);
			global_sound_server->queue_event(song->handle, SOUND_SIGNAL_FINISHED, 0);
		}

		/* find the active song */
		song = song_lib_find_active(songlib, song);
		if (song == NULL)
		{
			/* if not found, then wait 60 ticks */
			sci_get_current_time((GTimeVal *)&last_played);
			ticks = 60;
		}
		else
		{
			/* have an active song */
			long tempticks;

			sci_get_current_time((GTimeVal *)&last_played);
			/* Just played a note (see bottom of the big outer loop), so we
			** reset the timer */

			ticks = 0; /* Number of ticks until the next song is played */

			old_songpos = song->pos;	/* Keep a backup of the current song position,
										** in case we are interrupted and want to resume
										** at this point  */

			/* Handle length escape sequence (see SCI sound specs) */
			while ((tempticks = song->data[(song->pos)++]) == SCI_MIDI_TIME_EXPANSION_PREFIX) {
				ticks += SCI_MIDI_TIME_EXPANSION_LENGTH;
				if (song->pos >= song->size) {
					fprintf(stderr, "Sound server: Error: Reached end of song while decoding prefix:\n");
					dump_song(song);
					song = NULL;
				}
			}
			ticks += tempticks;
		}


		/*--------------*/
		/* Handle input */
		/*--------------*/
		newsong = song;	/* Same old song; newsong is changed if the parent process
						** orders us to do so.  */

		if (ticks)
			do {	/* Delay loop: Exit if we've waited long enough, or if we were
					** ordered to suspend. */

				GTimeVal *wait_tvp; /* Delay parameter (see select(3)) */

				if (!suspended) {
					/* Calculate the time we have to sleep */
					wakeup_time = song_next_wakeup_time(&last_played, ticks);

					wait_tv = song_sleep_time(&last_played, ticks);
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

					modsong = song_lib_find(songlib, (word)event.handle);

					/* find out what the event is */
					if (event.signal == SOUND_COMMAND_INIT_HANDLE) {
						byte *data;
						int totalsize = 0;

#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Receiving song for handle %04x: ", event.handle);
#endif
						if (modsong) { /* If the song already exists in cache... */
							int lastmode = song_lib_remove(songlib, (word)event.handle); /* remove it */
							if (lastmode == SOUND_STATUS_PLAYING) {
								newsong = songlib[0];	/* If we just retreived the current song,
														** we'll have to reset the queue for
														** song_lib_find_active(). */
								/* Force song detection to start with the highest priority song */
								global_sound_server->queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);
							}
							if (modsong == song)	/* Usually the same case as above, but not always.
													** Reset the song (just been freed) so that we don't
													** try to read from it later. */
								song = NULL;
						}

						global_sound_server->get_data(&data, &totalsize);	/* Retreive song size */
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Song received.\n");
#endif
						modsong = song_new((word)event.handle, data, totalsize, event.value);	/* Create new song */

						if (midi_playrhythm)	/* Enable rhythm channel, if requested by the hardware */
							modsong->flags[RHYTHM_CHANNEL] |= midi_playflag;

						song_lib_add(songlib, modsong);	/* Add song to song library */

						ccc = 127; /* Reset ccc */

						/* set default reverb */
						/* midi_reverb(-1); */
						/* midi_allstop(); */

						global_sound_server->queue_event(event.handle, SOUND_SIGNAL_INITIALIZED, 0); /* Send back ACK */

					} else if (event.signal == SOUND_COMMAND_PLAY_HANDLE) {
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
							else fprintf(stderr, "Song has no iterator: Cannot check for PCM!\n");

							if (pcm) {
								int fd;
								int tmp;

								fprintf(stderr,"SndSrv: Handle %04x: Playing PCM at %d Hz, %d bytes (%fs)\n",
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

									fprintf(stderr, "SndSrv: Sample rate set to %d Hz (requested %d Hz)\n",
										tmp, sample_rate);

									if (write(fd, pcm, pcm_size) < pcm_size)
										perror("SndSrv: While writing to /dev/dsp\n");

									close(fd);
									fprintf(stderr, "SndSrv: Finished writing PCM to /dev/dsp\n");
								}
							} else
#endif
#endif
							{
								midi_allstop();
								modsong->status = SOUND_STATUS_PLAYING;
								global_sound_server->queue_event(event.handle, SOUND_SIGNAL_PLAYING, 0);
								newsong = modsong; /* Play this song */
#ifdef DEBUG_SOUND_SERVER
								memcpy(channel_instrument, channel_instrument_orig, sizeof(int)*16);
								/* Reset instrument mappings from song defaults */
#endif
							}
						} else {
							fprintf(debug_stream, "Attempt to play invalid handle %04x\n", event.handle);
						}

					} else if (event.signal == SOUND_COMMAND_LOOP_HANDLE) {
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Set loops to %d on handle %04x\n", event.value, event.handle);
#endif
						if (modsong)
							modsong->loops = event.value;
						else
							fprintf(debug_stream, "Attempt to set loops on invalid handle %04x\n", event.handle);

					} else if (event.signal == SOUND_COMMAND_DISPOSE_HANDLE) {
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Disposing handle %04x (value %04x)\n", event.handle, event.value);
#endif
						if (modsong) {
							int lastmode = song_lib_remove(songlib, (word)event.handle);
							if (lastmode == SOUND_STATUS_PLAYING) {
								newsong = songlib[0]; /* Force song detection to start with the highest priority song */
								global_sound_server->queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);
							}

						} else {
							fprintf(debug_stream, "Attempt to dispose invalid handle %04x\n", event.handle);
						}

						if (modsong == song)	/* If we just deleted the current song, make sure we don't
												** dereference it later on! */
							song = NULL;


					} else if (event.signal == SOUND_COMMAND_STOP_HANDLE) {
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Stopping handle %04x (value %04x)\n", event.handle, event.value);
#endif
						if (modsong) {
							midi_allstop();
							modsong->status = SOUND_STATUS_STOPPED;
							if (modsong->resetflag) { /* only reset if we are supposed to. */
								modsong->pos = 33;
								modsong->loopmark = 33; /* Reset position */
							}
							global_sound_server->queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);

						} else {
							fprintf(debug_stream, "Attempt to stop invalid handle %04x\n", event.handle);
						}

					} else if (event.signal == SOUND_COMMAND_SUSPEND_HANDLE) {
						song_t *seeker = *songlib;

						if (event.handle) { /* Only act for non-zero song IDs */
							while (seeker && (seeker->status != SOUND_STATUS_SUSPENDED))
								seeker = seeker->next;
							if (seeker) {
								seeker->status = SOUND_STATUS_WAITING;
#ifdef DEBUG_SOUND_SERVER
		fprintf(debug_stream, "Un-suspending paused song\n");
#endif
							} else {
								while (seeker && (seeker->status != SOUND_STATUS_WAITING)) {
									if (seeker->status == SOUND_STATUS_SUSPENDED)
										return;
									seeker = seeker->next;
								}
								if (seeker) {
									seeker->status = SOUND_STATUS_SUSPENDED;
									fprintf(debug_stream, "Suspending paused song\n");
								}
							}
						}
	  /*

	    if (debugging)
	    fprintf(debug_stream, "Suspending handle %04x (value %04x)\n", event.handle, event.value);

	    if (modsong) {

	    modsong->status = SOUND_STATUS_SUSPENDED;
	    global_sound_server->queue_event(event.handle, SOUND_SIGNAL_PAUSED, 0);

	    } else
	    fprintf(debug_stream, "Attempt to suspend invalid handle %04x\n", event.handle);
	  */ /* Old code. */

					} else if (event.signal == SOUND_COMMAND_RESUME_HANDLE) {
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Resuming on handle %04x (value %04x)\n", event.handle, event.value);
#endif
						if (modsong) {
							if (modsong->status == SOUND_STATUS_SUSPENDED) {
								modsong->status = SOUND_STATUS_WAITING;
								global_sound_server->queue_event(event.handle, SOUND_SIGNAL_RESUMED, 0);
							} else {
								fprintf(debug_stream, "Attempt to resume handle %04x although not suspended\n", event.handle);
							}
						} else
							fprintf(debug_stream, "Attempt to resume invalid handle %04x\n", event.handle);

					} else if (event.signal == SOUND_COMMAND_SET_VOLUME) {
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Set volume to %d\n", event.value);
#endif
						master_volume = (unsigned char)(event.value * 100 / 15); /* scale to % */
						midi_volume(master_volume);

					} else if (event.signal == SOUND_COMMAND_SET_MUTE) {
						fprintf(debug_stream, "Called mute??? should never happen\n");

					} else if (event.signal == SOUND_COMMAND_RENICE_HANDLE) {
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Renice to %d on handle %04x\n", event.value, event.handle);
#endif
						if (modsong) {
							modsong->priority = event.value;
							song_lib_resort(songlib, modsong); /* Re-sort it according to the new priority */
						} else
							fprintf(debug_stream, "Attempt to renice on invalid handle %04x\n", event.handle);

					} else if (event.signal == SOUND_COMMAND_FADE_HANDLE) {
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Fading %d on handle %04x\n", event.value, event.handle);
#endif
						if (event.handle == 0x0000) {
							if (song) {
								song->fading = event.value;
								song->maxfade = event.value;
							}
						} else if (modsong) {
							modsong->fading = event.value;
						} else
							fprintf(debug_stream, "Attempt to fade on invalid handle %04x\n", event.handle);

					} else if (event.signal == SOUND_COMMAND_TEST) {
						int i = midi_polyphony;
#ifdef DEBUG_SOUND_SERVER
						fprintf(debug_stream, "Received TEST signal. Responding...\n");
#endif

						global_sound_server->send_data((byte *) &i, sizeof(int));

					} else if (event.signal == SOUND_COMMAND_SAVE_STATE) {
						char *dirname;
						int success;
						long usecs;
						int size;
						GTimeVal currtime;

						/* Retreive target directory from command stream.
						** Since we might be in a separate process, cwd must be set
						** separately from the main process. That's why we need it here.
						*/
						global_sound_server->get_data((byte **)&dirname, &size);
						sci_get_current_time(&currtime);
						usecs = (currtime.tv_sec - last_played.tv_sec) * 1000000
						         + (currtime.tv_usec - last_played.tv_usec);

						success = soundsrv_save_state(debug_stream,
						  global_sound_server->flags & SOUNDSERVER_FLAG_SEPARATE_CWD? dirname : NULL,
						  songlib, newsong,
						  ccc, usecs, ticks, 0, master_volume);

						/* Return soundsrv_save_state()'s return value */
						global_sound_server->send_data((byte *)&success, sizeof(int));
						free(dirname);

					} else if (event.signal == SOUND_COMMAND_RESTORE_STATE) {
						char *dirname;
						int success;
						long usecs, secs;
						int len;

						long fadeticks_obsolete;
						/* FIXME: Needs to be removed from save games */

						global_sound_server->get_data((byte **)&dirname, &len); /* see SAVE_STATE */

						sci_get_current_time(&last_played); /* Get current time, store in last_played.
										    ** We effectively reset the sound delay this
										    ** way, which seems appropriate.
										    */

						success = soundsrv_restore_state(debug_stream,
										 global_sound_server->flags & SOUNDSERVER_FLAG_SEPARATE_CWD? dirname : NULL,
										 songlib,
										 &newsong,
										 &ccc,
										 &usecs,
										 &ticks,
										 &fadeticks_obsolete,
										 &master_volume);
						last_played.tv_sec -= secs = (usecs - last_played.tv_usec) / 1000000;
						last_played.tv_usec -= (usecs + secs * 1000000);

						/* Return return value */
						global_sound_server->send_data((byte *)&success, sizeof(int));
						/* REPORT_STATUS(success); */

						song = newsong;

						free(dirname);
						/* restore some MIDI device state */
						/* (see MIDI specs). Effectively sets current instruments etc. */
						if (newsong) {
							guint8 i;
							midi_allstop();
							midi_volume(master_volume);
							for (i = 0; i < MIDI_CHANNELS; i++) {
								if (newsong->instruments[i])
									midi_event2((guint8)(0xc0 | i), newsong->instruments[i]);
							}
							midi_reverb(newsong->reverb);
						}

					} else if (event.signal == SOUND_COMMAND_PRINT_SONG_INFO) {
						if (song)
							fprintf(debug_stream, "%04x", song->handle);
						else
							fprintf(debug_stream, "No song is playing.");

					} else if (event.signal == SOUND_COMMAND_PRINT_CHANNELS) {
#ifdef DEBUG_SOUND_SERVER
						print_channels_any(0, ss_state);
#endif

					} else if (event.signal == SOUND_COMMAND_PRINT_MAPPING) {
#ifdef DEBUG_SOUND_SERVER
						print_channels_any(1, ss_state);
#endif

					} else if ( (event.signal == SOUND_COMMAND_IMAP_SET_INSTRUMENT)   ||
					            (event.signal == SOUND_COMMAND_IMAP_SET_KEYSHIFT)     ||
					            (event.signal == SOUND_COMMAND_IMAP_SET_FINETUNE)     ||
					            (event.signal == SOUND_COMMAND_IMAP_SET_BENDER_RANGE) ||
					            (event.signal == SOUND_COMMAND_IMAP_SET_PERCUSSION)   ||
								(event.signal == SOUND_COMMAND_IMAP_SET_VOLUME) ) {
						imap_set(event.signal,
						  event.handle, event.value);

					} else if (event.signal == SOUND_COMMAND_MUTE_CHANNEL) {
						if (event.value >= 0 && event.value < 16)
							mute_channel[event.value] = MUTE_ON;

					} else if (event.signal == SOUND_COMMAND_UNMUTE_CHANNEL) {
						if (event.value >= 0 && event.value < 16)
							mute_channel[event.value] = MUTE_OFF;

					} else if (event.signal == SOUND_COMMAND_SUSPEND_ALL) {
						sci_get_current_time((GTimeVal *)&suspend_time); /* Track the time we suspended at */
						suspended = 1;

					} else if (event.signal == SOUND_COMMAND_RESUME_ALL) {
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

						suspended = 0;

					} else if (event.signal == SOUND_COMMAND_STOP_ALL) {
						song_t *seeker = *songlib;

	  					while (seeker) { /* FOREACH seeker IN songlib */
						    if ((seeker->status == SOUND_STATUS_WAITING)
							|| (seeker->status == SOUND_STATUS_PLAYING)) {

							    seeker->status = SOUND_STATUS_STOPPED; /* Stop song */
							    global_sound_server->queue_event(seeker->handle, SOUND_SIGNAL_FINISHED, 0);
							    /* Notify parent process that the song was stopped */
							}
							seeker = seeker->next;
						}

						newsong = NULL;
						midi_allstop();

					} else if (event.signal == SOUND_COMMAND_SHUTDOWN) {
						fprintf(stderr,"Sound server: Received shutdown signal\n");
						midi_close();
						song_lib_free(songlib);

						soundserver_dead = 1;
						return;

					} else {
						fprintf(debug_stream, "Received invalid signal: %d\n", event.signal);
					}
				}

				sci_get_current_time((GTimeVal *)&ctime); /* Get current time, store in ctime */

			} while (suspended /* Exit when suspended or when we've waited long enough */
				 || (wakeup_time.tv_sec > ctime.tv_sec)
				 || ((wakeup_time.tv_sec == ctime.tv_sec)
				 && (wakeup_time.tv_usec > ctime.tv_usec)));


		/*---------------*/
		/* Handle output */
		/*---------------*/

		if (newsong != oldsong && newsong) { /* If the song to play changed and is non-NULL */
			int i;
			for (i = 0; i < 16; i++) {
				playing_notes[i].polyphony = MAX(1, MIN(POLYPHONY(newsong, i), MAX_POLYPHONY));
				playing_notes[i].playing = 0;
				/* Lingering notes are usually intended */
			}
		}

		if (song && song->data) { /* If we have a current song */
			int newcmd;
			guint8 param, param2 = 0;

			newcmd = song->data[song->pos]; /* Retreive MIDI command */

			if (newcmd & 0x80) { /* Check for running status mode */
				++(song->pos);
				command = newcmd;
			} /* else we've got the 'running status' mode defined in the MIDI standard */

			if (command == SCI_MIDI_EOT) { /* End of Track */
				if ((--(song->loops) != 0) && song->loopmark) {
#ifdef DEBUG_SOUND_SERVER
					fprintf(debug_stream, "looping back from %d to %d on handle %04x\n",
					        song->pos, song->loopmark, song->handle);
#endif
					song->pos = song->loopmark;
					global_sound_server->queue_event(song->handle, SOUND_SIGNAL_LOOP, song->loops);

				} else { /* Finished */

					song->status = SOUND_STATUS_STOPPED; /* Song is stopped */
					song->pos = 33;
					song->loopmark = 33; /* Reset position */
					global_sound_server->queue_event(song->handle, SOUND_SIGNAL_LOOP, -1);
					global_sound_server->queue_event(song->handle, SOUND_SIGNAL_FINISHED, 0);
					ticks = 1; /* Wait one tick, then continue with next song */

					midi_allstop();
				}

			} else { /* Song running normally */
				param = song->data[song->pos];

				if (MIDI_cmdlen[command >> 4] == 2)
					param2 = song->data[song->pos + 1]; /* If the MIDI instruction
									    ** takes two parameters, read
									    ** second parameter  */
				else
					param2 = 0; /* Waste processor cycles otherwise */

				song->pos += MIDI_cmdlen[command >> 4];

				if (reverse_stereo
				    && ((command & MIDI_CONTROL_CHANGE) == MIDI_CONTROL_CHANGE)
				    && (param == MIDI_CC_PAN))
					param2 = (unsigned char)(0x7f - param2); /* Reverse stereo */
#ifdef DEBUG_SOUND_SERVER
				if ((command & 0xf0) == 0xc0) /* Change instrument */
					channel_instrument[command & 0xf] = param;
#endif

				if (!((command & 0xf0) == 0x90 && mute_channel[command & 0xf]))
					/* Unless the channel is muted */
					sci_midi_command(debug_stream, song, command, param, param2,
					                 &ccc, &(playing_notes[command & 0xf]));
			}

			if (song->fading >= 0) { /* If the song is fading out... */
				long fadeticks = ticks;
				ticks = !!ticks; /* Ticks left? */
				song->fading -= fadeticks; /* OK, decrease! */

				if (song->fading < 0)
					song->fading = 0; /* Faded out after the ticks have run out */

			}

		}
		if (oldsong != newsong) { /* Song change! */
			if (newsong) {	/* New song is for real (not a NULL song) */
				guint8 i;
				for (i = 0; i < MIDI_CHANNELS; i++) {
					if (newsong->instruments[i])
						midi_event2((guint8)(0xc0 | i), newsong->instruments[i]);
				}
			}
			if (song)	/* Muting active song: Un-play the last note/MIDI event */
				song->pos = old_songpos;
		}

		song = newsong;
	}

	/* implicitly quit */
}
