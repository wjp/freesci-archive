/***************************************************************************
 sounderver_null.c Copyright (C) 1999 Christoph Reichenbach


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
/* Sound server without sound output- just takes care of the events */


#include <engine.h>
#include <soundserver.h>
#include <signal.h>
#include <sound.h>
#include <sys/types.h>
#include <dirent.h>
#ifdef HAVE_SYS_UIO_H
#  ifdef HAVE_LIMITS_H
#    include <limits.h>
#  endif
#  include <sys/uio.h>
#endif

#include <sciresource.h>
#include <midi_device.h>

sfx_driver_t sound_null;

int x_fd_in, x_fd_out, x_fd_events, x_fd_debug;

void
sound_null_server(int fd_in, int fd_out, int fd_events, int fd_debug);

void
_sound_server_sigpipe_handler(int signal)
{
	fprintf(stderr,"Sound server process: Parent process is dead, terminating.\n");
	_exit(0);
}

int
sound_null_init(state_t *s)
{
	int child_pid;
	pid_t ppid;
	int fd_in[2], fd_out[2], fd_events[2], fd_debug[2];

	soundserver = &sound_null;

	sound_init_pipes(s);

	memcpy(&fd_in, &(s->sound_pipe_in), sizeof(int)*2);
	memcpy(&fd_out, &(s->sound_pipe_out), sizeof(int)*2);
	memcpy(&fd_events, &(s->sound_pipe_events), sizeof(int)*2);
	memcpy(&fd_debug, &(s->sound_pipe_debug), sizeof(int)*2);

	ppid = getpid(); /* Get process ID */

	if (init_midi_device(s) < 0)
	  return -1;
	
	child_pid = fork();

	if (child_pid < 0) {
		fprintf(stderr,"NULL Sound server init failed: fork() failed\n");
		/* If you get this message twice, something funny has happened :-> */

		return 1; /* Forking failed */
	}

	if (child_pid) { /* Parent process */

		close(s->sound_pipe_in[0]);
		close(s->sound_pipe_out[1]);
		close(s->sound_pipe_events[1]);
		close(s->sound_pipe_debug[1]); /* Close pipes */

	} else {/* Sound server */

		close(fd_in[1]);
		close(fd_out[0]);
		close(fd_events[0]);
		close(fd_debug[0]); /* Close pipes at the other end */

		signal(SIGPIPE, &_sound_server_sigpipe_handler); /* Die on sigpipe */
		x_fd_in = fd_in[0];
		x_fd_out = fd_out[1];
		x_fd_events = fd_events[1];
		x_fd_debug = fd_debug[1];

		sound_null_server(fd_in[0], fd_out[1], fd_events[1], fd_debug[1]); /* Run the sound server */
	}

	return 0;
}

int
sound_null_configure(state_t *s, char *option, char *value)
{
	return 1; /* No options apply to this driver */
}

void 
sound_null_queue_event(int handle, int signal, int value)
{
  sound_eq_queue_event(&queue, handle, signal, value);
}

static int get_event_error_counter = 0;

sound_event_t *
sound_null_get_event(state_t *s)
{
	fd_set inpfds;
	int inplen;
	int success;
	GTimeVal waittime = {0, 0};
	GTimeVal waittime2 = {0, SOUND_SERVER_TIMEOUT};
	char debug_buf[65];
	sound_event_t *event = xalloc(sizeof(sound_event_t));


	FD_ZERO(&inpfds);
	FD_SET(s->sound_pipe_debug[0], &inpfds);
	while ((select(s->sound_pipe_debug[0] + 1, &inpfds, NULL, NULL, (struct timeval *)&waittime)
		&& (inplen = read(s->sound_pipe_debug[0], debug_buf, 64)) > 0)) {

		debug_buf[inplen] = 0; /* Terminate string */
		sciprintf(debug_buf); /* Transfer debug output */
		waittime.tv_sec = 0;
		waittime.tv_usec = 0;

		FD_ZERO(&inpfds);
		FD_SET(s->sound_pipe_debug[0], &inpfds);

	}

	sound_command(s, SOUND_COMMAND_GET_NEXT_EVENT, 0, 0);

	FD_ZERO(&inpfds);
	FD_SET(s->sound_pipe_events[0], &inpfds);
	success = select(s->sound_pipe_events[0] + 1, &inpfds, NULL, NULL, (struct timeval *)&waittime2);

	if (success && read(s->sound_pipe_events[0], event, sizeof(sound_event_t)) == sizeof(sound_event_t)) {

		if (event->signal == SOUND_SIGNAL_END_OF_QUEUE) {
			free(event);
			return NULL;
		}
      
		return event;

	} else {

		if (!get_event_error_counter) {
			if (success)
				sciprintf("sound_get_event: Warning: sound event was crippled!\n");
			else
				sciprintf("sound_get_event: Warning: Sound server did not respond to get_event request\n");
		}

		if (get_event_error_counter == 100) {
			sciprintf("sound_get_event: Sound server keeps on failing to respond. Looks like he's dead.\n");
			get_event_error_counter = 101;
		} else
			get_event_error_counter++;


		free(event);
		return NULL;

	}

}
void 
sound_null_queue_command(state_t *s, int handle, int signal, int value)
{
  sound_event_t event;

  event.handle = handle;
  event.signal = signal;
  event.value = value;
  /*
  printf("writing %04x %d %d\n", event.handle, event.signal, event.value);
  */
  write(s->sound_pipe_in[1], &event, sizeof(sound_event_t));

}

sound_event_t *
sound_null_get_command(struct timeval *wait_tvp)
{
  fd_set input_fds;
  sound_event_t *event = NULL;

  FD_ZERO(&input_fds);
  FD_SET(x_fd_in, &input_fds);
  
  if(select(x_fd_in + 1, &input_fds, NULL, NULL, wait_tvp)) {
    event = xalloc(sizeof(sound_event_t));
    if (read(x_fd_in, event, sizeof(sound_event_t)) != sizeof(sound_event_t)) {
      free(event);
      event = NULL;
    }
  }
  /*  if (event)
      printf("reading %04x %d %d\n", event->handle, event->signal, event->value); */
  return event;
}

sfx_driver_t sound_null = {
	"null",
	&sound_null_init,
	&sound_null_configure,
	&sound_exit,
	&sound_null_get_event,
	&sound_null_queue_event,
	&sound_null_get_command,
	&sound_null_queue_command,
	&sound_save,
	&sound_restore,
	&sound_command,
	&sound_suspend,
	&sound_resume
};

/************************/
/***** SOUND SERVER *****/
/************************/



static void
_transmit_event(int fd, sound_eq_t *queue)
{
	sound_event_t *event = sound_eq_retreive_event(queue);

	if (event) {
		write(fd, event, sizeof(sound_event_t));
		free(event);
	} else
		write(fd, &sound_eq_eoq_event, sizeof(sound_event_t));
}

#define REPORT_STATUS(status) {int _repstat = status; write(fd_out, &_repstat, sizeof(int)); }

static int
verify_pid(int pid) /* Checks if the specified PID is in use */
{
	return kill(pid, 0);
}

#define CHECK song_lib_dump(songlib, __LINE__)

void
sound_null_server(int fd_in, int fd_out, int fd_events, int fd_debug)
{
	FILE *ds = fdopen(fd_debug, "w"); /* We want to output text to it */
	fd_set input_fds;
	GTimeVal last_played, wakeup_time, ctime;
	song_t *_songp = NULL;
	songlib_t songlib = &_songp;   /* Song library */
	song_t *newsong, *song = NULL; /* The song we're playing */
	int master_volume = 0;  /* the master volume.. whee */
	int debugging = 1;   /* Debugging enabled? */
	int command = 0;
	int ccc = 127; /* cumulative cue counter */
	int suspended = 0; /* Used to suspend the sound server */
	GTimeVal suspend_time; /* Time at which the sound server was suspended */
	int ppid = getppid(); /* Get parent PID */

	sound_eq_init(&queue);
	sound_eq_init(&inqueue);

	gettimeofday((struct timeval *)&last_played, NULL);

	fprintf(ds, "NULL Sound server initialized\n");

	while (1) {
	  sound_event_t *event_temp;
	  GTimeVal wait_tv;
		int ticks = 0; /* Ticks to next command */
		int fadeticks = 0;		
		int old_songpos;
		song_t *oldsong = song;

		fflush(ds);
		if (fadeticks) {

			fadeticks--;
			ticks = 1; /* Just fade for this tick */

		} else if (song && (song->fading == 0)) { /* Finished fading? */
		  printf("song %04x faded out \n", song->handle);
			song->status = SOUND_STATUS_STOPPED;
			midi_allstop();
			song->pos = 33;
			song->loopmark = 33; /* Reset position */
			sound_queue_event(song->handle, SOUND_SIGNAL_LOOP, -1);
			sound_queue_event(song->handle, SOUND_SIGNAL_FINISHED, 0);

		}
		song = song_lib_find_active(songlib, song);
		if (song == NULL) {
			gettimeofday((struct timeval *)&last_played, NULL);
			ticks = 60; /* Wait a second for new commands, then collect your new ticks here. */
		}
		if (ticks == 0) {
			int tempticks;

			gettimeofday((struct timeval *)&last_played, NULL);

			ticks = 0;

			old_songpos = song->pos;

			/* Handle length escape sequence */
			while ((tempticks = song->data[(song->pos)++]) == SCI_MIDI_TIME_EXPANSION_PREFIX)
				ticks += SCI_MIDI_TIME_EXPANSION_LENGHT;

			ticks += tempticks;
		}

		/*--------------*/
		/* Handle input */
		/*--------------*/
		newsong = song;

		if (ticks) do {
			struct timeval *wait_tvp;

			if (!suspended) {
				wakeup_time = song_next_wakeup_time(&last_played, ticks);

				wait_tv = song_sleep_time(&last_played, ticks);
				wait_tvp = &wait_tv;

			} else {
				/* Sound server is suspended */

				wait_tvp = NULL; /* select()s infinitely long */

			}

			event_temp = sound_get_command(wait_tvp);
			/* Wait for input: */

			if (event_temp) { /* We've got mail! */
				sound_event_t event = *event_temp;
				song_t *modsong;
				free(event_temp);

					modsong = song_lib_find(songlib, event.handle);


					switch (event.signal) {

					case SOUND_COMMAND_INIT_SONG: {

						byte *data, *datptr;
						int size, totalsize;
						int received, left_of_block;

						if (debugging)
							fprintf(ds, "Receiving song for handle %04x: ", event.handle);

						if (modsong) {
							int lastmode = song_lib_remove(songlib, event.handle);

							if (modsong) {

								int lastmode = song_lib_remove(songlib, event.handle);
								if (lastmode == SOUND_STATUS_PLAYING) {
									newsong = songlib[0]; /* Force song detection to start with the highest priority song */
									sound_queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);
								}
							}

							if (modsong == song)
								song = NULL;

							if (lastmode == SOUND_STATUS_PLAYING) {
								newsong = songlib[0]; /* Force song detection to start with the highest priority song */
								sound_queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);
							}

						}

						if (read(fd_in, &size, sizeof(int)) != sizeof(int)) {
							fprintf(ds, "Critical: Receiving song failed\n");
							REPORT_STATUS(SOUND_SERVER_XFER_ABORT);
							while (read(fd_in, &size, 1)); /* Empty queue */
							break;
						}

						if (debugging)
							fprintf(ds, "%d bytes: ", size);

						datptr = data = xalloc(totalsize = size);

						left_of_block = 0;
						while (size > 0) {
							if (left_of_block == 0 && size) {
								left_of_block = SOUND_SERVER_XFER_SIZE;
								REPORT_STATUS(SOUND_SERVER_XFER_WAITING);
							}

							received = read(fd_in, datptr, MIN(size, SOUND_SERVER_XFER_SIZE));

							if (received < 0) {
								fprintf(ds, "Critical: Receiving song failed\n");
								break;
							}
							datptr += received;
							size -= received;
							left_of_block -= received;

						}
						REPORT_STATUS(SOUND_SERVER_XFER_OK);

						if (debugging)
							fprintf(ds, "OK\n");
						modsong = song_new(event.handle, data, totalsize, event.value);
						if (midi_playrhythm)
							modsong->flags[RHYTHM_CHANNEL] |= midi_playflag;
						song_lib_add(songlib, modsong);

						ccc = 127; /* Reset ccc */

						/* set default reverb */
						/* midi_reverb(-1); */
						/* midi_allstop(); */


						sound_queue_event(event.handle, SOUND_SIGNAL_INITIALIZED, 0);

					}
					break;

					case SOUND_COMMAND_PLAY_HANDLE:

						if (debugging)
							fprintf(ds, "Playing handle %04x\n", event.handle);
						if (modsong) {

						  midi_allstop();
							modsong->status = SOUND_STATUS_PLAYING;
							sound_queue_event(event.handle, SOUND_SIGNAL_PLAYING, 0);
							newsong = modsong; /* Play this song */

						} else
							fprintf(ds, "Attempt to play invalid handle %04x\n", event.handle);
						break;

					case SOUND_COMMAND_SET_LOOPS:

						if (debugging)
							fprintf(ds, "Set loops to %d on handle %04x\n", event.value, event.handle);
						if (modsong)
							modsong->loops = event.value;
						else
							fprintf(ds, "Attempt to set loops on invalid handle %04x\n", event.handle);
						break;

					case SOUND_COMMAND_DISPOSE_HANDLE:

						if (debugging)
							fprintf(ds, "Disposing handle %04x (value %04x)\n", event.handle, event.value);
						if (modsong) {
							int lastmode = song_lib_remove(songlib, event.handle);
							if (lastmode == SOUND_STATUS_PLAYING) {
								newsong = songlib[0]; /* Force song detection to start with the highest priority song */
								sound_queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);
							}

						} else
							fprintf(ds, "Attempt to dispose invalid handle %04x\n", event.handle);

						if (modsong == song)
							song = NULL;

						break;

					case SOUND_COMMAND_STOP_HANDLE:

						if (debugging)
							fprintf(ds, "Stopping handle %04x (value %04x)\n", event.handle, event.value);
						if (modsong) {

							midi_allstop();
							modsong->status = SOUND_STATUS_STOPPED;
							if (modsong->resetflag) { /* only reset if we are supposed to. */
							  modsong->pos = 33;
							  modsong->loopmark = 33; /* Reset position */
							}
							sound_queue_event(event.handle, SOUND_SIGNAL_FINISHED, 0);

						} else
							fprintf(ds, "Attempt to stop invalid handle %04x\n", event.handle);

						break;

					case SOUND_COMMAND_SUSPEND_HANDLE: {

					  song_t *seeker = *songlib;

					  if (event.handle) {
					    while (seeker && (seeker->status != SOUND_STATUS_SUSPENDED))
					      seeker = seeker->next;
					    if (seeker) {
					      seeker->status = SOUND_STATUS_WAITING;
					      if (debugging)
						fprintf(ds, "Un-suspending paused song\n");
					    }
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

						if (debugging)
							fprintf(ds, "Resuming on handle %04x (value %04x)\n", event.handle, event.value);
						if (modsong) {

							if (modsong->status == SOUND_STATUS_SUSPENDED) {

								modsong->status = SOUND_STATUS_WAITING;
								sound_queue_event(event.handle, SOUND_SIGNAL_RESUMED, 0);

							} else
								fprintf(ds, "Attempt to resume handle %04x although not suspended\n", event.handle);

						} else
							fprintf(ds, "Attempt to resume invalid handle %04x\n", event.handle);
						break;

					case SOUND_COMMAND_SET_VOLUME:
						if (debugging)
						  fprintf(ds, "Set volume to %d\n", event.value);
						master_volume = event.value * 100 / 15; /* scale to % */
						midi_volume(master_volume);

						break;
					case SOUND_COMMAND_SET_MUTE:
						fprintf(ds, "Called mute??? should never happen\n");
						break;

					case SOUND_COMMAND_RENICE_HANDLE:

						if (debugging)
							fprintf(ds, "Renice to %d on handle %04x\n", event.value, event.handle);
						if (modsong) {
							modsong->priority = event.value;
							song_lib_resort(songlib, modsong); /* Re-sort it according to the new priority */
						} else
							fprintf(ds, "Attempt to renice on invalid handle %04x\n", event.handle);
						break;

					case SOUND_COMMAND_FADE_HANDLE:

						if (debugging)
						  fprintf(ds, "Fading %d on handle %04x\n", event.value, event.handle);

						if (event.handle == 0x0000) {
						  if (song) {
						    song->fading = event.value;
						    song->maxfade = event.value;
						  }
						} else if (modsong) {
						    modsong->fading = event.value;
						} else {
						  fprintf(ds, "Attempt to fade on invalid handle %04x\n", event.handle);
						}
						
						break;

					case SOUND_COMMAND_TEST: {
						int i = midi_polyphony;

						if (debugging)
							fprintf(ds, "Received TEST signal. Responding...\n");

						write(fd_out, &i, sizeof(int)); /* Confirm test request */

					} break;


					case SOUND_COMMAND_SAVE_STATE: {
						char *dirname = (char *) xalloc(event.value);
						char *dirptr = dirname;
						int size = event.value;
						int success = 1;
						int usecs;
						GTimeVal currtime;

						while (size > 0) {
							int received = read(fd_in, dirptr, MIN(size, SOUND_SERVER_XFER_SIZE));

							if (received < 0) {
								fprintf(ds, "Critical: Failed to receive save dir name\n");
								write(fd_out, &success, sizeof(int));
								break;
							}

							dirptr += received;
							size -= received;
						}

						sci_get_current_time(&currtime);
						usecs = (currtime.tv_sec - last_played.tv_sec) * 1000000
							+ (currtime.tv_usec - last_played.tv_usec);


						success = soundsrv_save_state(ds, dirname, songlib, newsong,
									      ccc, usecs, ticks, fadeticks, master_volume);

						/* Return soundsrv_save_state()'s return value */
						write(fd_out, &success, sizeof(int));
						free(dirname);
					}
					break;

					case SOUND_COMMAND_RESTORE_STATE: {
						char *dirname = (char *) xalloc(event.value);
						char *dirptr = dirname;
						int size = event.value;
						int success = 1;
						int usecs, secs;

						while (size > 0) {
							int received = read(fd_in, dirptr, MIN(size, SOUND_SERVER_XFER_SIZE));

							if (received < 0) {
								fprintf(ds, "Critical: Failed to receive restore dir name\n");
								write(fd_out, &success, sizeof(int));
								break;
							}

							dirptr += received;
							size -= received;
						}

						sci_get_current_time(&last_played);

						success = soundsrv_restore_state(ds, dirname, songlib, &newsong,
										 &ccc, &usecs, &ticks, &fadeticks, &master_volume);
						last_played.tv_sec -= secs = (usecs - last_played.tv_usec) / 1000000;
						last_played.tv_usec -= (usecs + secs * 1000000);

						/* Return return value */
						REPORT_STATUS(success);

						free(dirname);
						/* restore some device state */
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

					case SOUND_COMMAND_SUSPEND_SOUND: {

						gettimeofday((struct timeval *)&suspend_time, NULL);
						suspended = 1;

					}
					break;

					case SOUND_COMMAND_RESUME_SOUND: {

						GTimeVal resume_time;

						gettimeofday((struct timeval *)&resume_time, NULL);
						/* Modify last_played relative to wakeup_time - suspend_time */
						last_played.tv_sec += resume_time.tv_sec - suspend_time.tv_sec - 1; 
						last_played.tv_usec += resume_time.tv_usec - suspend_time.tv_usec + 1000000;

						/* Make sure that 0 <= tv_usec <= 999999 */
						last_played.tv_sec -= last_played.tv_usec / 1000000;
						last_played.tv_usec %= 1000000;

						suspended = 0;

					}
					break;

					case SOUND_COMMAND_GET_NEXT_EVENT:
						_transmit_event(fd_events, &queue);
						break;

					case SOUND_COMMAND_STOP_ALL: {

						song_t *seeker = *songlib;

						while (seeker) {

							if ((seeker->status == SOUND_STATUS_WAITING)
							    || (seeker->status == SOUND_STATUS_PLAYING)) {

								seeker->status = SOUND_STATUS_STOPPED;
								sound_queue_event(seeker->handle, SOUND_SIGNAL_FINISHED, 0);

							}

							seeker = seeker->next;
						}

						newsong = NULL;
					}
					midi_allstop();
					break;

					case SOUND_COMMAND_SHUTDOWN:
						fprintf(stderr,"Sound server: Received shutdown signal\n");
						midi_close();
						_exit(0);

					default:
						fprintf(ds, "Received invalid signal: %d\n", event.signal);

					}
			}

			if (verify_pid(ppid)) {
				fprintf(stderr,"FreeSCI Sound server: Parent process is dead, terminating\n");
				_exit(1); /* Die semi-ungracefully */
			}

			gettimeofday((struct timeval *)&ctime, NULL);

		} while (suspended
			 || (wakeup_time.tv_sec > ctime.tv_sec)
			 || ((wakeup_time.tv_sec == ctime.tv_sec)
			     && (wakeup_time.tv_usec > ctime.tv_usec)));


		/*---------------*/
		/* Handle output */
		/*---------------*/

		if (song && song->data) {
			int newcmd;
			int param, param2 = -1;

			newcmd = song->data[song->pos];

			if (newcmd & 0x80) {
				++(song->pos);
				command = newcmd;
			} /* else we've got the 'running status' mode defined in the MIDI standard */


			if (command == SCI_MIDI_EOT) {
				if ((--(song->loops) != 0) && song->loopmark) {
				  if (debugging)
				        fprintf(ds, "looping back from %d to %d on handle %04x\n",
						song->pos, song->loopmark, song->handle);
				  song->pos = song->loopmark;
				  sound_queue_event(song->handle, SOUND_SIGNAL_LOOP, song->loops);

				} else { /* Finished */

					song->status = SOUND_STATUS_STOPPED;
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
				  param2 = song->data[song->pos + 1];
				else
				  param2 = -1;

				song->pos += cmdlen[command >> 4];

				sci_midi_command(song, command, param, param2, 
						 &ccc, ds);

			}

			if (song->fading >= 0) {

				fadeticks = ticks;
				ticks = !!ticks;
				song->fading -= fadeticks;

				if (song->fading < 0) 
					song->fading = 0; /* Faded out after the ticks have run out */

			}

		}
		if (oldsong != newsong) {
		  if (newsong) {
		    int i;
		    for (i = 0; i < MIDI_CHANNELS; i++) {
		      if (newsong->instruments[i]) 
			midi_event2(0xc0 | i, newsong->instruments[i]);
		    }
		  }
		  if (song)
		    song->pos = old_songpos;
		}

	
		song = newsong;
	}
	_exit(0); /* Exit gracefully without cleanup */
}




