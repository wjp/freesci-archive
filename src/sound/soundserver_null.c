/***************************************************************************
 sounderver_null.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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


void
sound_null_server(int fd_in, int fd_out, int fd_events, int fd_debug);

int
sound_null_init(state_t *s)
{
  int child_pid;
  int fd_in[2], fd_out[2], fd_events[2], fd_debug[2];

  sound_init_pipes(s);

  memcpy(&fd_in, &(s->sound_pipe_in), sizeof(int)*2);
  memcpy(&fd_out, &(s->sound_pipe_out), sizeof(int)*2);
  memcpy(&fd_events, &(s->sound_pipe_events), sizeof(int)*2);
  memcpy(&fd_debug, &(s->sound_pipe_debug), sizeof(int)*2);

  child_pid = fork();

  if (child_pid < 0) {
    fprintf(stderr,"NULL Sound server init failed: fork() failed\n");
    /* If you get this message twice, something funny has happened :-> */

    return 1; /* Forking failed */
  }

  if (child_pid) { /* Parent process */

    char target[1000];

    close(s->sound_pipe_in[0]);
    close(s->sound_pipe_out[1]);
    close(s->sound_pipe_events[1]);
    close(s->sound_pipe_debug[1]); /* Close pipes */

  } else {/* Sound server */

    close(fd_in[1]);
    close(fd_out[0]);
    close(fd_events[0]);
    close(fd_debug[0]); /* Close pipes at the other end */

    signal(SIGPIPE, SIG_DFL); /* Die on sigpipe */

    sound_null_server(fd_in[0], fd_out[1], fd_events[1], fd_debug[1]); /* Run the sound server */
  }

  return 0;
}

int
sound_null_configure(state_t *s, char *option, char *value)
{
  return 1; /* No options apply to this driver */
}



sfx_driver_t sound_null = {
  "null",
  &sound_null_init,
  &sound_null_configure,

  /* Default implementations: */

  &sound_exit,
  &sound_get_event,
  &sound_command
};

/************************/
/***** SOUND SERVER *****/
/************************/

inline
_send_event(int fd, int handle, int signal, int value)
{
  sound_event_t event = {handle, signal, value};
  write(fd, &event, sizeof(sound_event_t));
}


static int cmdlen[16] =
{0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 0};
/* Those two lines were taken from playmidi */


void
song_lib_dump(songlib_t songlib, int line)
{
  song_t *seeker = *songlib;

  fprintf(stderr,"L%d:", line);
  do {
    fprintf(stderr,"    %p", seeker);

    if (seeker) {
      fprintf(stderr,"[%04x]->", seeker->handle);
      seeker = seeker->next;
    }
    fprintf(stderr,"\n");
  } while (seeker);
  fprintf(stderr,"\n");
	    
}

#define CHECK song_lib_dump(songlib, __LINE__)

void
sound_null_server(int fd_in, int fd_out, int fd_events, int fd_debug)
{
  FILE *ds = fdopen(fd_debug, "w"); /* We want to output text to it */
  fd_set input_fds;
  GTimeVal last_played, wakeup_time, ctime;
  song_t *_songp;
  songlib_t songlib = &_songp;   /* Song library */
  song_t *song = NULL; /* The song we're playing */
  int debugging = 0;   /* Debugging enabled? */
  int command = 0;
  int ccc = 127; /* cumulative cue counter */

  songlib[0] = NULL;

  gettimeofday(&last_played, NULL);

  fprintf(ds, "NULL Sound server initialized\n");

  while (1) {
    GTimeVal wait_tv;
    int ticks = 0; /* Ticks to next command */
    int fadeticks = 0;

    fflush(ds);
    if (fadeticks) {

      fadeticks--;
      ticks = 1; /* Just fade for this tick */

    } else if (song && (song->fading == 0)) { /* Finished fading? */

      song->status = SOUND_STATUS_STOPPED;
      song->pos = song->loopmark = 33; /* Reset position */
      _send_event(fd_events, song->handle, SOUND_SIGNAL_FINISHED, 0);

    }
    song = song_lib_find_active(songlib, song);
    if (song == NULL) {
      gettimeofday(&last_played, NULL);
      ticks = 60; /* Wait a second for new commands, then collect your new ticks here. */
    }
    while (ticks == 0) {

      int newcmd;
      int param;

      gettimeofday(&last_played, NULL);

      ticks = song->data[(song->pos)++];
      newcmd = song->data[song->pos];

      if (newcmd & 0x80) {
	++(song->pos);
	command = newcmd;
      } /* else we've got the 'running status' mode defined in the MIDI standard */

      param = song->data[song->pos];

      song->pos += cmdlen[command >> 4];

      if (command == SCI_MIDI_EOT) {

	if (song->loops) {

	  --(song->loops);
	  song->pos = song->loopmark;
	  _send_event(fd_events, song->handle, SOUND_SIGNAL_LOOP, song->loops);

	} else { /* Finished */

	  song->status = SOUND_STATUS_STOPPED;
	  song->pos = song->loopmark = 33; /* Reset position */
	  _send_event(fd_events, song->handle, SOUND_SIGNAL_FINISHED, 0);

	}

      } else if (command == SCI_MIDI_CUMULATIVE_CUE)
	_send_event(fd_events, song->handle, SOUND_SIGNAL_ABSOLUTE_CUE, param + ++ccc);
      else if (command == SCI_MIDI_SET_SIGNAL) {

	if (param == SCI_MIDI_SET_SIGNAL_LOOP)
	  song->loopmark = song->pos;
	else
	  _send_event(fd_events, song->handle, SOUND_SIGNAL_ABSOLUTE_CUE, param);
	  
      }

      if (song->fading >= 0) {
	fadeticks = ticks;
	ticks = !!ticks;
	song->fading -= fadeticks;

	if (song->fading < 0)
	  song->fading = 0; /* Faded out after the ticks have run out */
      }

    }
    do {
      int got_input;

      wakeup_time = song_next_wakeup_time(&last_played, ticks);

      wait_tv = song_sleep_time(&last_played, ticks);

      FD_ZERO(&input_fds);
      FD_SET(fd_in, &input_fds);

      got_input = select(fd_in + 1, &input_fds, NULL, NULL, &wait_tv); /* Wait for input */

      if (got_input) { /* We've got mail! */
	sound_event_t event;

	if (read(fd_in, &event, sizeof(sound_event_t)) == sizeof(sound_event_t)) {
	  song_t *modsong = song_lib_find(songlib, event.handle);


	  switch (event.signal) {

	  case SOUND_COMMAND_INIT_SONG: {

	    byte *data, *datptr;
	    int size, totalsize;
	    int received;


	    if (debugging)
	      fprintf(ds, "Receiving song for handle %04x: ", event.handle);
	    if (modsong) {
	      int lastmode = song_lib_remove(songlib, event.handle);

	      if (modsong) {

		int lastmode = song_lib_remove(songlib, event.handle);
		if (lastmode == SOUND_STATUS_PLAYING) {
		  song = songlib[0]; /* Force song detection to start with the highest priority song */
		  _send_event(fd_events, event.handle, SOUND_SIGNAL_FINISHED, 0);
		}

	      }

	      if (modsong == song)
		song = NULL;

	      if (lastmode == SOUND_STATUS_PLAYING) {
		song = songlib[0]; /* Force song detection to start with the highest priority song */
		_send_event(fd_events, event.handle, SOUND_SIGNAL_FINISHED, 0);
	      }

	    }

	    if (read(fd_in, &size, sizeof(int)) != sizeof(int)) {
	      fprintf(ds, "Critical: Receiving song failed\n");
	      while (read(fd_in, &size, 1)); /* Empty queue */
	      break;
	    }

	    if (debugging)
	      fprintf(ds, "%d bytes: ", size);

	    datptr = data = malloc(totalsize = size);

	    while (size > 0) {
	      received = read(fd_in, datptr, MIN(size, SSIZE_MAX));

	      if (received < 0) {
		fprintf(ds, "Critical: Receiving song failed\n");
		break;
	      }

	      datptr += received;
	      size -= received;
	    }

	    if (debugging)
	      fprintf(ds, "OK\n");
	    modsong = song_new(event.handle, data, totalsize, event.value);
	    song_lib_add(songlib, modsong);

	    ccc = 127; /* Reset ccc */

	    _send_event(fd_events, event.handle, SOUND_SIGNAL_INITIALIZED, 0);

	  }
	  break;

	  case SOUND_COMMAND_PLAY_HANDLE:

	    if (debugging)
	      fprintf(ds, "Playing handle %04x\n", event.handle);
	    if (modsong) {

	      modsong->status = SOUND_STATUS_PLAYING;
	      _send_event(fd_events, event.handle, SOUND_SIGNAL_PLAYING, 0);
	      song = modsong; /* Play this song */

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
	      fprintf(ds, "Disposing handle %04x\n", event.value, event.handle);
	    if (modsong) {
	      int lastmode = song_lib_remove(songlib, event.handle);
	      if (lastmode == SOUND_STATUS_PLAYING) {
		song = songlib[0]; /* Force song detection to start with the highest priority song */
		_send_event(fd_events, event.handle, SOUND_SIGNAL_FINISHED, 0);
	      }

	    } else
	      fprintf(ds, "Attempt to dispose invalid handle %04x\n", event.handle);
	    break;

	  case SOUND_COMMAND_STOP_HANDLE:

	    if (debugging)
	      fprintf(ds, "Stopping handle %04x\n", event.value, event.handle);
	    if (modsong) {

	      modsong->status = SOUND_STATUS_STOPPED;
	      modsong->pos = modsong->loopmark = 33; /* Reset position */
	      _send_event(fd_events, event.handle, SOUND_SIGNAL_FINISHED, 0);

	    } else
	      fprintf(ds, "Attempt to stop invalid handle %04x\n", event.handle);
	    break;

	  case SOUND_COMMAND_SUSPEND_HANDLE:

	    if (debugging)
	      fprintf(ds, "Suspending handle %04x\n", event.value, event.handle);
	    if (modsong) {

	      modsong->status = SOUND_STATUS_SUSPENDED;
	      _send_event(fd_events, event.handle, SOUND_SIGNAL_PAUSED, 0);

	    } else
	      fprintf(ds, "Attempt to suspend invalid handle %04x\n", event.handle);
	    break;

	  case SOUND_COMMAND_RESUME_HANDLE:

	    if (debugging)
	      fprintf(ds, "Resuming on handle %04x\n", event.value, event.handle);
	    if (modsong) {

	      if (modsong->status == SOUND_STATUS_SUSPENDED) {

		modsong->status = SOUND_STATUS_WAITING;
		_send_event(fd_events, event.handle, SOUND_SIGNAL_RESUMED, 0);

	      } else
		fprintf(ds, "Attempt to resume handle %04x although not suspended\n", event.handle);

	    } else
	      fprintf(ds, "Attempt to resume invalid handle %04x\n", event.handle);
	    break;

	  case SOUND_COMMAND_SET_VOLUME:
	  case SOUND_COMMAND_SET_MUTE:
	    break; /* Muahahaha! Not much to do here ;-) */

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
	    if (modsong)
	      modsong->fading = event.value;
	    else
	      fprintf(ds, "Attempt to fade on invalid handle %04x\n", event.handle);
	    break;

	  case SOUND_COMMAND_TEST: {
	    int i = 42;

	    if (debugging)
	      fprintf(ds, "Received TEST signal. Responding...\n");

	    write(fd_out, &i, sizeof(int)); /* Confirm test request */

	  } break;

	  case SOUND_COMMAND_STOP_ALL: {

	    song_t *seeker = *songlib;

	    while (seeker) {

	      if ((seeker->status == SOUND_STATUS_WAITING)
		  || (seeker->status == SOUND_STATUS_PLAYING)) {

		seeker->status = SOUND_STATUS_STOPPED;
		_send_event(fd_events, seeker->handle, SOUND_SIGNAL_FINISHED, 0);

	      }

	      seeker = seeker->next;
	    }

	    song = NULL;
	  }
	  break;

	  case SOUND_COMMAND_SHUTDOWN:
	    fprintf(stderr,"Sound server: Received shutdown signal\n");
	    exit(0);

	  default:
	    fprintf(ds, "Received invalid signal: %d\n", event.signal);

	  }
	}
      }

      gettimeofday(&ctime, NULL);

    } while ((wakeup_time.tv_sec > ctime.tv_sec)
	     || ((wakeup_time.tv_sec == ctime.tv_sec)
		 && (wakeup_time.tv_usec > ctime.tv_usec)));
  }
}
