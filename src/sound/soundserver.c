/***************************************************************************
 soundserver.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt

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

***************************************************************************/

#include <sciresource.h>
#include <stdarg.h>
#include <engine.h>
#include <sound.h>
#include <scitypes.h>
#include <soundserver.h>
#include <midi_device.h>
#include <sys/types.h>

sound_server_t *global_sound_server = NULL;

#define DEBUG_SOUND_SERVER

int soundserver_dead = 0;

#define POLYPHONY(song, channel) song->data[(channel << 1) + 1]

static void
sound_server_print_channels_any(FILE *ds, int *channel_instrument,
								byte *mute_channel, int flag)
{
	int i;

	for (i = 0; i < 16; i++)
		if (channel_instrument[i] >= 0) {
			fprintf(ds, "#%d%s:", i+1, mute_channel[i]? " [MUTE]":"");
			if (flag)
				midi_mt32gm_print_instrument(ds, channel_instrument[i]);
			else
				fprintf(ds, " Instrument %d\n", channel_instrument[i]);
		}
}

static void
sound_server_print_channels(FILE *ds, int *channel_instrument,
			    byte *mute_channel)
{
	sound_server_print_channels_any(ds, channel_instrument, mute_channel, 0);
}

static void
sound_server_print_mapped_channels(FILE *ds, int *channel_instrument,
				   byte *mute_channel)
{
	sound_server_print_channels_any(ds, channel_instrument, mute_channel, 1);
}


#define SOUNDSERVER_INSTRMAP_CHANGE(ELEMENT, VAL_MIN, VAL_MAX, ELEMENT_NAME) \
		if (value >= (VAL_MIN) && value <= (VAL_MAX)) \
			MIDI_mapping[instr].ELEMENT = value; \
		else \
			fprintf(output, "Invalid %s: %d\n", ELEMENT_NAME, value) \

void
sound_server_change_instrmap(FILE *output, int action, int instr, int value)
{
	if (instr < 0 || instr >= MIDI_mappings_nr) {
		fprintf(output, "sound_server_change_instrmap(): Attempt to re-map"
			" invalid instrument %d!\n", instr);
		return;
	}

	switch (action) {
	case SOUND_COMMAND_INSTRMAP_SET_INSTRUMENT:
		if ((value >= (0) && value <= (MIDI_mappings_nr - 1))
		    || value == NOMAP
		    || value == RHYTHM)
			MIDI_mapping[instr].gm_instr = value;
		else
			fprintf(output, "Invalid instrument ID: %d\n", value);
		break;

	case SOUND_COMMAND_INSTRMAP_SET_KEYSHIFT:
		SOUNDSERVER_INSTRMAP_CHANGE(keyshift, -128, 127,
					    "key shift");
		break;

	case SOUND_COMMAND_INSTRMAP_SET_FINETUNE:
		SOUNDSERVER_INSTRMAP_CHANGE(finetune, -32768, 32767,
					    "finetune value");
		break;

	case SOUND_COMMAND_INSTRMAP_SET_BENDER_RANGE:
		SOUNDSERVER_INSTRMAP_CHANGE(bender_range, -128, 127,
					    "bender range");
		break;

	case SOUND_COMMAND_INSTRMAP_SET_PERCUSSION:
		SOUNDSERVER_INSTRMAP_CHANGE(gm_rhythmkey, 0, 79,
					    "percussion instrument");
		break;

	case SOUND_COMMAND_INSTRMAP_SET_VOLUME:
		SOUNDSERVER_INSTRMAP_CHANGE(volume, 0, 100,
					    "instrument volume");
		break;

	default:
		fprintf(output, "sound_server_change_instrmap(): Internal error: "
			"Invalid action %d!\n", action);
	}
}


void
sci0_soundserver(int reverse_stereo)
{
	GTimeVal last_played, /* Time the last note was played at */
		wakeup_time, /* Time at which we stop waiting for events */
		ctime; /* 'Current time' (temporary local usage */
	byte mute_channel[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
	playing_notes_t playing_notes[16];
	int channel_instrument_orig[16] = {-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1};
	int channel_instrument[16];
	song_t *_songp = NULL;
	songlib_t songlib = &_songp;   /* Song library */
	song_t *newsong, *song = NULL; /* The song we're playing */
	int master_volume = 0;  /* the master volume.. whee */
	int ccc = 127; /* cumulative cue counter- see SCI sound specs */
	int suspended = 0; /* Used to suspend the sound server */
	GTimeVal suspend_time; /* Time at which the sound server was suspended */
	int command = 0; /* MIDI operation */
	int i;

	/* initialise default values */
	for (i = 0; i < 16; i++)
		playing_notes[i].playing = 0;

#ifdef SATISFY_PURIFY
	memset(playing_notes, 0, 16 * sizeof(playing_notes_t));
#endif

	memset(&suspend_time, 0, sizeof(GTimeVal));
	memset(&wakeup_time, 0, sizeof(GTimeVal));
	memset(&ctime, 0, sizeof(GTimeVal));
	sci_get_current_time(&last_played);

	fprintf(ds, "Sound server initialized\n");

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
					** whether the song was changed later on  */
		byte last_command = 0; /* Used for 'running' mode (see MIDI specs) */
		fflush(ds); /* Flush debug stream */

		if (song && (song->fading == 0)) { /* Finished fading out the current song? */
			printf("song %04x faded out \n", song->handle);
			song->status = SOUND_STATUS_STOPPED;
			midi_allstop();
			song->pos = 33;
			song->loopmark = 33; /* Reset position */
			sound_queue_event(song->handle, SOUND_SIGNAL_LOOP, -1);
			sound_queue_event(song->handle, SOUND_SIGNAL_FINISHED, 0);

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
			** reset the timer  */

			ticks = 0; /* Number of ticks until the next song is played */

			old_songpos = song->pos; /* Keep a backup of the current song position,
						 ** in case we are interrupted and want to resume
						 ** at this point  */

			/* Handle length escape sequence (see SCI sound specs) */
			while ((tempticks = song->data[(song->pos)++]) == SCI_MIDI_TIME_EXPANSION_PREFIX)
				ticks += SCI_MIDI_TIME_EXPANSION_LENGTH;
			ticks += tempticks;
		}


		/*--------------*/
		/* Handle input */
		/*--------------*/
		newsong = song;	/* Same old song; newsong is changed if the parent process
				** orders us to do so.  */

		if (ticks)
			do {	/* Delay loop: Exit if we've waited long enough, or if we were
				** ordered to suspend.  */

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
				event_temp = sound_get_command(wait_tvp);
				if (event_temp) { /* We've got mail! */
					sound_event_t event = *event_temp;
					/* Copy event we got, so we can resume without risking a memory leak
					** (OK, this was old code I didn't want to meddle with...  */
					song_t *modsong; /* Pointer to the song the current instruction
							 ** intends to operate on  */
					free(event_temp);

					modsong = song_lib_find(songlib, event.handle);

					/* find out what the event is */
					switch (event.signal) {

					case SOUND_COMMAND_INIT_SONG: {
						byte *data;
						int totalsize = 0;

#ifdef DEBUG_SOUND_SERVER
						fprintf(ds, "Receiving song for handle %04x: ", event.handle);
#endif
						if (modsong) { /* If the song already exists in cache... */
							int lastmode = song_lib_remove(songlib, event.handle); /* remove it */
							if (lastmode == SOUND_STATUS_PLAYING) {
								newsong = songlib[0]; /* If we just retreived the current song,
										      ** we'll have to reset the queue for
										      ** song_lib_find_active().  */
								/* Force song detection to start with the highest priority song */
								sound_queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);
							}
							if (modsong == song) /* Usually the same case as above, but not always.
									     ** Reset the song (just been freed) so that we don't
									     ** try to read from it later.  */
								song = NULL;
						}

						sound_get_data(&data,&totalsize,sizeof(int));	/* Retreive song size */
#ifdef DEBUG_SOUND_SERVER
						fprintf(ds, "Song received.\n");
#endif
						modsong = song_new(event.handle, data, totalsize, event.value);	/* Create new song */

						if (midi_playrhythm)	/* Enable rhythm channel, if requested by the hardware */
							modsong->flags[RHYTHM_CHANNEL] |= midi_playflag;

						song_lib_add(songlib, modsong);	/* Add song to song library */

						ccc = 127; /* Reset ccc */

						/* set default reverb */
						/* midi_reverb(-1); */
						/* midi_allstop(); */

						sound_queue_event(event.handle, SOUND_SIGNAL_INITIALIZED, 0); /* Send back ACK */

					}
					break;

					case SOUND_COMMAND_PLAY_HANDLE:
#ifdef DEBUG_SOUND_SERVER
						fprintf(ds, "Playing handle %04x\n", event.handle);
#endif
						if (modsong) {
							midi_allstop();
							modsong->status = SOUND_STATUS_PLAYING;
							sound_queue_event(event.handle, SOUND_SIGNAL_PLAYING, 0);
							newsong = modsong; /* Play this song */
							memcpy(channel_instrument, channel_instrument_orig, sizeof(int)*16);
							/* Reset instrument mappings from song defaults */

						} else {
							fprintf(ds, "Attempt to play invalid handle %04x\n", event.handle);
						}
					break;

					case SOUND_COMMAND_SET_LOOPS:
#ifdef DEBUG_SOUND_SERVER
						fprintf(ds, "Set loops to %d on handle %04x\n", event.value, event.handle);
#endif
						if (modsong)
							modsong->loops = event.value;
						else
							fprintf(ds, "Attempt to set loops on invalid handle %04x\n", event.handle);
					break;

					case SOUND_COMMAND_DISPOSE_HANDLE:
#ifdef DEBUG_SOUND_SERVER
						fprintf(ds, "Disposing handle %04x (value %04x)\n", event.handle, event.value);
#endif
						if (modsong) {
							int lastmode = song_lib_remove(songlib, event.handle);
							if (lastmode == SOUND_STATUS_PLAYING) {
								newsong = songlib[0]; /* Force song detection to start with the highest priority song */
								sound_queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);
							}

						} else {
							fprintf(ds, "Attempt to dispose invalid handle %04x\n", event.handle);
						}

						if (modsong == song) /* If we just deleted the current song, make sure we don't
								     ** dereference it later on!  */
							song = NULL;

					break;

					case SOUND_COMMAND_STOP_HANDLE:
#ifdef DEBUG_SOUND_SERVER
						fprintf(ds, "Stopping handle %04x (value %04x)\n", event.handle, event.value);
#endif
						if (modsong) {
							midi_allstop();
							modsong->status = SOUND_STATUS_STOPPED;
							if (modsong->resetflag) { /* only reset if we are supposed to. */
								modsong->pos = 33;
								modsong->loopmark = 33; /* Reset position */
							}
							sound_queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);

						} else {
							fprintf(ds, "Attempt to stop invalid handle %04x\n", event.handle);
						}

					break;

					case SOUND_COMMAND_SUSPEND_HANDLE: {
						song_t *seeker = *songlib;

						if (event.handle) { /* Only act for non-zero song IDs */
							while (seeker && (seeker->status != SOUND_STATUS_SUSPENDED))
								seeker = seeker->next;
							if (seeker) {
								seeker->status = SOUND_STATUS_WAITING;
#ifdef DEBUG_SOUND_SERVER
		fprintf(ds, "Un-suspending paused song\n");
#endif
							} else {
								while (seeker && (seeker->status != SOUND_STATUS_WAITING)) {
									if (seeker->status == SOUND_STATUS_SUSPENDED)
										return;
									seeker = seeker->next;
								}
								if (seeker) {
									seeker->status = SOUND_STATUS_SUSPENDED;
									fprintf(ds, "Suspending paused song\n");
								}
							}
						}
	  /*

	    if (debugging)
	    fprintf(ds, "Suspending handle %04x (value %04x)\n", event.handle, event.value);

	    if (modsong) {

	    modsong->status = SOUND_STATUS_SUSPENDED;
	    sound_queue_event(event.handle, SOUND_SIGNAL_PAUSED, 0);

	    } else
	    fprintf(ds, "Attempt to suspend invalid handle %04x\n", event.handle);
	  */ /* Old code. */

					}
					break;

					case SOUND_COMMAND_RESUME_HANDLE:
#ifdef DEBUG_SOUND_SERVER
						fprintf(ds, "Resuming on handle %04x (value %04x)\n", event.handle, event.value);
#endif
						if (modsong) {
							if (modsong->status == SOUND_STATUS_SUSPENDED) {
								modsong->status = SOUND_STATUS_WAITING;
								sound_queue_event(event.handle, SOUND_SIGNAL_RESUMED, 0);
							} else {
								fprintf(ds, "Attempt to resume handle %04x although not suspended\n", event.handle);
							}
						} else
							fprintf(ds, "Attempt to resume invalid handle %04x\n", event.handle);
					break;

					case SOUND_COMMAND_SET_VOLUME:
#ifdef DEBUG_SOUND_SERVER
						fprintf(ds, "Set volume to %d\n", event.value);
#endif
						master_volume = event.value * 100 / 15; /* scale to % */
						midi_volume(master_volume);
					break;

					case SOUND_COMMAND_SET_MUTE:
						fprintf(ds, "Called mute??? should never happen\n");
					break;

					case SOUND_COMMAND_RENICE_HANDLE:
#ifdef DEBUG_SOUND_SERVER
						fprintf(ds, "Renice to %d on handle %04x\n", event.value, event.handle);
#endif
						if (modsong) {
							modsong->priority = event.value;
							song_lib_resort(songlib, modsong); /* Re-sort it according to the new priority */
						} else
							fprintf(ds, "Attempt to renice on invalid handle %04x\n", event.handle);
					break;

					case SOUND_COMMAND_FADE_HANDLE:
#ifdef DEBUG_SOUND_SERVER
						fprintf(ds, "Fading %d on handle %04x\n", event.value, event.handle);
#endif
						if (event.handle == 0x0000) {
							if (song) {
								song->fading = event.value;
								song->maxfade = event.value;
							}
						} else if (modsong) {
							modsong->fading = event.value;
						} else
							fprintf(ds, "Attempt to fade on invalid handle %04x\n", event.handle);
					break;

					case SOUND_COMMAND_TEST: {
						int i = midi_polyphony;
#ifdef DEBUG_SOUND_SERVER
						fprintf(ds, "Received TEST signal. Responding...\n");
#endif

						sound_send_data((byte *) &i, sizeof(int));
					}
					break;

					case SOUND_COMMAND_SAVE_STATE: {
						char *dirname;
						int success;
						long usecs;
						int size;
						GTimeVal currtime;

						/* Retreive target directory from command stream.
						** Since we might be in a separate process, cwd must be set
						** separately from the main process. That's why we need it here.
						*/
						sound_get_data((byte **)&dirname,&size,event.value);
						sci_get_current_time(&currtime);
						usecs = (currtime.tv_sec - last_played.tv_sec) * 1000000
						         + (currtime.tv_usec - last_played.tv_usec);

						success = soundsrv_save_state(ds,
						  global_sound_server->flags & SOUNDSERVER_FLAG_SEPARATE_CWD? dirname : NULL,
						  songlib, newsong,
						  ccc, usecs, ticks, 0, master_volume);

						/* Return soundsrv_save_state()'s return value */
						sound_send_data((byte *)&success, sizeof(int));
						free(dirname);
					}
					break;

					case SOUND_COMMAND_RESTORE_STATE: {
						char *dirname;
						int success;
						long usecs, secs;
						int len;
						long fadeticks_obsolete; /* FIXME: Keeps API compatibility */

						sound_get_data((byte **)&dirname, &len, event.value); /* see SAVE_STATE */

						sci_get_current_time(&last_played); /* Get current time, store in last_played.
										    ** We effectively reset the sound delay this
										    ** way, which seems appropriate.
										    */

						success = soundsrv_restore_state(ds,
						  global_sound_server->flags & SOUNDSERVER_FLAG_SEPARATE_CWD? dirname : NULL,
						  songlib, &newsong,
						  &ccc, &usecs, &ticks, &fadeticks_obsolete, &master_volume);
						last_played.tv_sec -= secs = (usecs - last_played.tv_usec) / 1000000;
						last_played.tv_usec -= (usecs + secs * 1000000);

						/* Return return value */
						sound_send_data((byte *)&success, sizeof(int));
						/* REPORT_STATUS(success); */

						song = newsong;

						free(dirname);
						/* restore some MIDI device state */
						/* (see MIDI specs). Effectively sets current instruments etc. */
						if (newsong) {
							int i;
							midi_allstop();
							midi_volume(master_volume);
							for (i = 0; i < MIDI_CHANNELS; i++) {
								if (newsong->instruments[i])
									midi_event2(0xc0 | i, newsong->instruments[i]);
							}
							midi_reverb(newsong->reverb);
						}
					}
					break;

					case SOUND_COMMAND_PRINT_SONGID: {
						if (song)
							fprintf(ds, "%04x", song->handle);
						else
							fprintf(ds, "No song is playing.");
					}
					break;

					case SOUND_COMMAND_PRINT_CHANNELS:
						sound_server_print_channels(ds, &(channel_instrument[0]),
						  &(mute_channel[0]));
						break;

					case SOUND_COMMAND_PRINT_MAPPING:
						sound_server_print_mapped_channels(ds, &(channel_instrument[0]),
						  &(mute_channel[0]));
						break;

					case SOUND_COMMAND_INSTRMAP_SET_INSTRUMENT:
					case SOUND_COMMAND_INSTRMAP_SET_KEYSHIFT:
					case SOUND_COMMAND_INSTRMAP_SET_FINETUNE:
					case SOUND_COMMAND_INSTRMAP_SET_BENDER_RANGE:
					case SOUND_COMMAND_INSTRMAP_SET_PERCUSSION:
					case SOUND_COMMAND_INSTRMAP_SET_VOLUME:
						sound_server_change_instrmap(ds, event.signal,
						  event.handle, event.value);
						break;

					case SOUND_COMMAND_MUTE_CHANNEL:
						if (event.value >= 0 && event.value < 16)
							mute_channel[event.value] = 1;
						break;

					case SOUND_COMMAND_UNMUTE_CHANNEL:
						if (event.value >= 0 && event.value < 16)
							mute_channel[event.value] = 0;
						break;

					case SOUND_COMMAND_SUSPEND_SOUND: {
						sci_get_current_time((GTimeVal *)&suspend_time); /* Track the time we suspended at */
						suspended = 1;
					}
					break;

					case SOUND_COMMAND_RESUME_SOUND: {
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
					}
					break;

					case SOUND_COMMAND_STOP_ALL: {
						song_t *seeker = *songlib;

	  					while (seeker) { /* FOREACH seeker IN songlib */
						    if ((seeker->status == SOUND_STATUS_WAITING)
							|| (seeker->status == SOUND_STATUS_PLAYING)) {

							    seeker->status = SOUND_STATUS_STOPPED; /* Stop song */
							    sound_queue_event(seeker->handle, SOUND_SIGNAL_FINISHED, 0);
							    /* Notify parent process that the song was stopped */
							}
							seeker = seeker->next;
						}

						newsong = NULL;
						midi_allstop();
					}
					break;

					case SOUND_COMMAND_SHUTDOWN:
						fprintf(stderr,"Sound server: Received shutdown signal\n");
						midi_close();
						song_lib_free(songlib);

						soundserver_dead = 1;
						return;

					default:
						fprintf(ds, "Received invalid signal: %d\n", event.signal);

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
			int param, param2 = -1;

			newcmd = song->data[song->pos]; /* Retreive MIDI command */

			if (newcmd & 0x80) { /* Running status mode: see MIDI specs */
				++(song->pos);
				command = newcmd;
			} /* else we've got the 'running status' mode defined in the MIDI standard */

			if (command == SCI_MIDI_EOT) { /* End of Track */
				if ((--(song->loops) != 0) && song->loopmark) {
#ifdef DEBUG_SOUND_SERVER
					fprintf(ds, "looping back from %d to %d on handle %04x\n",
					        song->pos, song->loopmark, song->handle);
#endif
					song->pos = song->loopmark;
					sound_queue_event(song->handle, SOUND_SIGNAL_LOOP, song->loops);

				} else { /* Finished */

					song->status = SOUND_STATUS_STOPPED; /* Song is stopped */
					song->pos = 33;
					song->loopmark = 33; /* Reset position */
					sound_queue_event(song->handle, SOUND_SIGNAL_LOOP, -1);
					sound_queue_event(song->handle, SOUND_SIGNAL_FINISHED, 0);
					ticks = 1; /* Wait one tick, then continue with next song */

					midi_allstop();
				}

			} else { /* Song running normally */
				param = song->data[song->pos];

				if (cmdlen[command >> 4] == 2)
					param2 = song->data[song->pos + 1]; /* If the MIDI instruction
									    ** takes two parameters, read
									    ** second parameter  */
				else
					param2 = -1; /* Waste processor cycles otherwise */

				song->pos += cmdlen[command >> 4];

				if (reverse_stereo
				    && ((command & MIDI_CONTROL_CHANGE) == MIDI_CONTROL_CHANGE)
				    && (param == MIDI_CC_PAN))
					param2 = 0x7f - param2; /* Reverse stereo */

				if ((command & 0xf0) == 0xc0) /* Change instrument */
					channel_instrument[command & 0xf] = param;

				if (!((command & 0xf0) == 0x90 && mute_channel[command & 0xf]))
					/* Unless the channel is muted */
					sci_midi_command(song, command, param, param2,
					                 &ccc, ds, &(playing_notes[command & 0xf]));
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
				int i;
				for (i = 0; i < MIDI_CHANNELS; i++) {
					if (newsong->instruments[i])
						midi_event2(0xc0 | i, newsong->instruments[i]);
				}
			}
			if (song)	/* Muting active song: Un-play the last note/MIDI event */
				song->pos = old_songpos;
		}

		song = newsong;
	}

	/* implicitly quit */
}

void
sound_queue_event(int handle, int signal, int value) {
	global_sound_server->queue_event(handle, signal, value);
}

void
sound_queue_command(int handle, int signal, int value) {
	global_sound_server->queue_command(handle, signal, value);
}

sound_event_t *
sound_get_command(GTimeVal *wait_tvp) {
	return global_sound_server->get_command(wait_tvp);
}

int sound_send_data(byte *data_ptr, int maxsend) {
	return global_sound_server->send_data(data_ptr, maxsend);
}

int sound_get_data(byte **data_ptr, int *size, int maxlen){
	return global_sound_server->get_data(data_ptr, size, maxlen);
}
