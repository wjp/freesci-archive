/***************************************************************************
 sound.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
#include <sys/types.h>

#ifdef HAVE_SOCKETPAIR
#include <sys/socket.h>

#ifndef AF_LOCAL
# ifdef AF_UNIX
#  define AF_LOCAL AF_UNIX
# else
#  warn Neither AF_LOCAL nor AF_UNIX are defined!
#  undef HAVE_SOCKETPAIR
# endif
#endif

#endif /* HAVE_SOCKETPAIR */

#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else /* !HAVE_UNISTD_H */
#ifdef _WIN32
#include <io.h>
#endif /* _WIN32 */
#endif /* !HAVE_UNISTD_H */

#ifdef HAVE_FORK
extern sfx_driver_t sound_null;
#endif

#ifdef _DOS
extern sfx_driver_t sound_dos;
#endif

sound_event_t sound_eq_eoq_event = {0, SOUND_SIGNAL_END_OF_QUEUE, 0};

sfx_driver_t *sfx_drivers[] = {
#ifdef HAVE_FORK
  /* Assume that sound_null works on any box that has fork() */
  &sound_null,
#endif /* HAVE_FORK */
#ifdef _DOS
  &sound_dos,
#endif
  NULL
};



#ifdef HAVE_FORK

void
_sound_server_oops_handler(int signal)
{
  if (signal == SIGCHLD)
    fprintf(stderr, "Warning: Sound server died\n");
  else if (signal == SIGPIPE)
    fprintf(stderr, "Warning: Connection to sound server was severed\n");
  else
    fprintf(stderr,"Warning: Signal handler cant' handle signal %d\n", signal);
}


int
_make_pipe(int fildes[2])
     /* Opens an IPC channel */
{
#ifdef HAVE_PIPE
  if (pipe(fildes) == 0)
    return 0; /* :-) */
#endif
#ifdef HAVE_SOCKETPAIR
  if (socketpair(AF_LOCAL, SOCK_STREAM, 0, fildes) == 0)
    return 0; /* :-) */
#endif
  return 1; /* :-( */
}

int
sound_init_pipes(state_t *s)
{
  if (_make_pipe(s->sound_pipe_in)
      || _make_pipe(s->sound_pipe_out)
      || _make_pipe(s->sound_pipe_events)
      || _make_pipe(s->sound_pipe_debug))
    {
      fprintf(stderr, "Could not create IPC connection to server\n");
      return 1;
    }

  signal(SIGCHLD, &_sound_server_oops_handler);
  signal(SIGPIPE, &_sound_server_oops_handler);

  return 0;
}

sound_event_t *
sound_get_event(state_t *s)
{
  fd_set inpfds;
  int inplen;
  GTimeVal waittime = {0, 0};
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

  FD_ZERO(&inpfds);
  FD_SET(s->sound_pipe_events[0], &inpfds);

  sound_command(s, SOUND_COMMAND_GET_NEXT_EVENT, 0, 0);
  /*  select(s->sound_pipe_events[0] + 1, &inpfds, NULL, NULL, (struct timeval *)&waittime2); */

  if (read(s->sound_pipe_events[0], event, sizeof(sound_event_t)) == sizeof(sound_event_t)) {

    if (event->signal == SOUND_SIGNAL_END_OF_QUEUE) {
      free(event);
      return NULL;
    }
      
    return event;

  } else {

    sciprintf("sound_get_event: Warning: sound event was crippled!\n");
    free(event);
    return NULL;

  }

}

void
_sound_confirm_death(int signal)
{
  printf("Sound server shut down\n");
}

void
sound_exit(state_t *s)
{
  signal(SIGPIPE, SIG_IGN); /* Ignore SIGPIPEs */
  signal(SIGCHLD, &_sound_confirm_death);

  sound_command(s, SOUND_COMMAND_SHUTDOWN, 0, 0); /* Kill server */

  close(s->sound_pipe_in[1]);
  close(s->sound_pipe_out[0]);
  close(s->sound_pipe_events[0]);
  close(s->sound_pipe_debug[0]); /* Close all pipe file descriptors */
}


void
sound_suspend(state_t *s)
{
  sound_command(s, SOUND_COMMAND_SUSPEND_SOUND, 0, 0);
}

void
sound_resume(state_t *s)
{
  sound_command(s, SOUND_COMMAND_RESUME_SOUND, 0, 0);
}

static int
_sound_expect_answer(state_t *s, char *timeoutmessage, int def_value)
{
	GTimeVal timeout = {0, SOUND_SERVER_TIMEOUT};

	int success;
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(s->sound_pipe_out[0], &fds);
	success = select(s->sound_pipe_out[0]+1, &fds, NULL, NULL, (struct timeval *)&timeout);

	if (!success) {
		sciprintf(timeoutmessage);
		return def_value;
	}
	else read(s->sound_pipe_out[0], &success, sizeof(int));

	return success;
}

static int
_sound_transmit_text_expect_anwer(state_t *s, char *text, int command, char *timeoutmessage)
{
	sound_command(s, command, 0, strlen(text) + 1);
	write(s->sound_pipe_in[1], text, strlen(text) + 1);

	return _sound_expect_answer(s, timeoutmessage, 1);
}

int
sound_save(state_t *s, char *dir)
{
  return _sound_transmit_text_expect_anwer(s, dir, SOUND_COMMAND_SAVE_STATE,
					   "Sound server timed out while saving\n");
}

int
sound_restore(state_t *s, char *dir)
{
  return _sound_transmit_text_expect_anwer(s, dir, SOUND_COMMAND_RESTORE_STATE,
					   "Sound server timed out while restoring\n");
}

int
sound_command(state_t *s, int command, int handle, int parameter)
{
  sound_event_t event;
  event.handle = handle;
  event.signal = command;
  event.value = parameter;

  switch (command) {
  case SOUND_COMMAND_INIT_SONG: {
	  byte *xfer_buf;
	  resource_t *song = findResource(sci_sound, parameter);
	  int len;
	  int finished = 0;

	  if (!song) {
		  sciprintf("Attempt to play invalid sound.%03d\n", parameter);
		  return 1;
	  }

	  len = song->length;
	  write(s->sound_pipe_in[1], &event, sizeof(sound_event_t));
	  write(s->sound_pipe_in[1], &len, sizeof(int)); /* Write song length */

	  xfer_buf = song->data;
	  while (len || !finished) {
		  int status = _sound_expect_answer(s, "Sound/InitSong: Timeout while"
						    " waiting for sound server during transfer\n",
						    SOUND_SERVER_XFER_TIMEOUT);
		  int xfer_bytes = MIN(len, SOUND_SERVER_XFER_SIZE);

		  switch(status) {
		  case SOUND_SERVER_XFER_ABORT:
			  sciprintf("Sound/InitSong: Sound server aborted transfer\n");
			  len = xfer_bytes = 0;
			  break;

		  case SOUND_SERVER_XFER_OK:
			  finished = 1;
			  if (len) {
				  sciprintf("Sound/InitSong: Sound server reported OK with"
					    " %d/%d bytes missing!\n", len, song->length);
				  len = xfer_bytes = 0;
			  }
			  break;

		  case SOUND_SERVER_XFER_WAITING:
			  if (!len)
				  sciprintf("Sound/InitSong: Sound server waiting, but nothing"
					    " left to be sent!");
			  break;

		  case SOUND_SERVER_XFER_TIMEOUT:
			  len = xfer_bytes = 0;
			  finished = 1;
			  break;

		  default:
			  sciprintf("Sound/InitSong: Sound server in invalid state!\n");
			  break;
		  }

		  if (xfer_bytes) {
			  write(s->sound_pipe_in[1], xfer_buf, xfer_bytes); /* Transfer song */
			  xfer_buf += xfer_bytes;
			  len -= xfer_bytes;
		  }
	  }
	  return 0;
  }

  case SOUND_COMMAND_PLAY_HANDLE:
  case SOUND_COMMAND_SET_LOOPS:
  case SOUND_COMMAND_DISPOSE_HANDLE:
  case SOUND_COMMAND_STOP_HANDLE:
  case SOUND_COMMAND_SUSPEND_HANDLE:
  case SOUND_COMMAND_RESUME_HANDLE:
  case SOUND_COMMAND_RENICE_HANDLE:
  case SOUND_COMMAND_SHUTDOWN:
  case SOUND_COMMAND_MAPPINGS:
  case SOUND_COMMAND_SAVE_STATE: /* Those two commands are only used from special wrapper */
  case SOUND_COMMAND_RESTORE_STATE: /* functions that provide additional data. */
  case SOUND_COMMAND_SUSPEND_SOUND:
  case SOUND_COMMAND_RESUME_SOUND:
  case SOUND_COMMAND_STOP_ALL:
  case SOUND_COMMAND_GET_NEXT_EVENT:
  case SOUND_COMMAND_FADE_HANDLE:
    write(s->sound_pipe_in[1], &event, sizeof(sound_event_t));
    return 0;

    /* set the sound volume. */
  case SOUND_COMMAND_SET_VOLUME:
    if (parameter != 0xffff) { /* only set if != -1 */
      
      if (s->sound_mute != 0) {  /* if we're muted, update the mute */
	s->sound_mute = parameter;
      } else {
	s->sound_volume = parameter;
	write(s->sound_pipe_in[1], &event, sizeof(sound_event_t));
      }
      return parameter;
    }
    if (s->sound_mute)
      return s->sound_mute;
    else 
      return s->sound_volume;
    
    /* set the mute status */
  case SOUND_COMMAND_SET_MUTE:
    if (parameter == 0) {  // ie mute
      s->sound_mute = s->sound_volume;
      s->sound_volume = 0;
    } else {  // restore sound
      s->sound_volume = s->sound_mute;
      s->sound_mute = 0;
    }
    /* let's send a volume change across the wire */
    event.signal = SOUND_COMMAND_SET_VOLUME;
    event.value = s->sound_volume;
    write(s->sound_pipe_in[1], &event, sizeof(sound_event_t));
    /* deliberate fallthrough */
    /* return the mute status */
  case SOUND_COMMAND_GET_MUTE:
    if (s->sound_mute) 
      return 0;
    else
      return 1;

  case SOUND_COMMAND_TEST: {
    fd_set fds;
    GTimeVal timeout = {0, SOUND_SERVER_TIMEOUT};
    int dummy, success;

    write(s->sound_pipe_in[1], &event, sizeof(sound_event_t));

    FD_ZERO(&fds);
    FD_SET(s->sound_pipe_out[0], &fds);
    success = select(s->sound_pipe_out[0]+1, &fds, NULL, NULL, (struct timeval *)&timeout);

    if (success) {

      read(s->sound_pipe_out[0], &dummy, sizeof(int)); /* Empty pipe */
      return dummy; /* should be the polyphony */

    } else {

      fprintf(stderr,"Sound server timed out\n");
      return 0;

    }
  }    

  default: sciprintf("Unknown sound command %d\n", command);
    return 1;
  }

}

#endif /* HAVE_FORK */


GTimeVal
song_sleep_time(GTimeVal *lastslept, int ticks)
{
  GTimeVal tv;
  long timetosleep = ticks * SOUND_TICK; /* Time to sleep in us */
  long timeslept; /* Time already slept */

  sci_get_current_time(&tv);
  timeslept = 1000000 * (tv.tv_sec - lastslept->tv_sec) +
    tv.tv_usec - lastslept->tv_usec;

  timetosleep -= timeslept;

  if (timetosleep < 0)
    timetosleep = 0;

  tv.tv_sec = timetosleep / 1000000;
  tv.tv_usec = timetosleep % 1000000;

  return tv;
}


GTimeVal
song_next_wakeup_time(GTimeVal *lastslept, int ticks)
{
  GTimeVal retval;

  retval.tv_sec = lastslept->tv_sec + (ticks / 60);
  retval.tv_usec = lastslept->tv_usec + ((ticks % 60) * SOUND_TICK);

  if (retval.tv_usec >= 1000000) {
    retval.tv_usec -= 1000000;
    ++retval.tv_sec;
  }

  return retval;
}


song_t *
song_new(word handle, byte *data, int size, int priority)
{
  song_t *retval = xalloc(sizeof(song_t));
  int i;

  retval->data = data;
  retval->handle = handle;
  retval->priority = priority;
  retval->size = size;

  retval->pos = retval->loopmark = 33; /* The first 33 bytes are header data */
  retval->fading = -1; /* Not fading */
  retval->loops = 0; /* No fancy additional loops */
  retval->status = SOUND_STATUS_STOPPED;

  retval->resetflag = 0; /* don't reset position on SoundStop */
  
  memset(retval->instruments, 0, sizeof(int) * MIDI_CHANNELS);
  memset(retval->velocity, 0, sizeof(int) * MIDI_CHANNELS);
  memset(retval->pressure, 0, sizeof(int) * MIDI_CHANNELS);
  memset(retval->pitch, 0, sizeof(int) * MIDI_CHANNELS);
  memset(retval->channel_map, 1, sizeof(int) * MIDI_CHANNELS);

  for (i = 0; i < MIDI_CHANNELS; i++) {
    retval->polyphony[i] = data[1 + (i << 1)]; 
    retval->flags[i] = data[2 + (i << 1)];
  }
  return retval;
}


void
song_lib_add(songlib_t songlib, song_t *song)
{
  song_t *seeker;
  int pri = song->priority;

  if (*songlib == NULL) {
    *songlib = song;
    song->next = NULL;

    return;
  }

  seeker = *songlib;
  while (seeker->next && (seeker->next->priority > pri))
    seeker = seeker->next;

  song->next = seeker->next;
  seeker->next = song;
}

void /* Recursively free a chain of songs */
_sonfree_chain(song_t *song)
{
  if (song) {
    _sonfree_chain(song->next);
    free(song);
  }
}


void
song_lib_free(songlib_t songlib)
{
  _sonfree_chain(*songlib);
}


song_t *
song_lib_find(songlib_t songlib, word handle)
{
  song_t *seeker = *songlib;

  while (seeker && (seeker->handle != handle))
    seeker = seeker->next;

  return seeker;
}


song_t *
song_lib_find_active(songlib_t songlib, song_t *last_played_song)
{
  song_t *seeker = *songlib;

  if (last_played_song)
    if (last_played_song->status == SOUND_STATUS_PLAYING) {
      return last_played_song; /* That one was easy... */
    }

  while (seeker && (seeker->status != SOUND_STATUS_WAITING)
     && (seeker->status != SOUND_STATUS_PLAYING))
    seeker = seeker->next;

  return seeker;
}

int
song_lib_remove(songlib_t songlib, word handle)
{
  int retval;
  song_t *goner = *songlib;

  if (!goner)
    return -1;

  if (goner->handle == handle)
    *songlib = goner->next;

  else {
      while ((goner->next) && (goner->next->handle != handle))
	goner = goner->next;

      if (goner->next) {/* Found him? */
	song_t *oldnext = goner->next;

	goner->next = goner->next->next;
	goner = oldnext;
      } else return -1; /* No. */
    }

  retval = goner->status;

  free(goner->data);
  free(goner);

  return retval;
}


void
song_lib_resort(songlib_t songlib, song_t *song)
{ 
  if (*songlib == song)
    *songlib = song->next;
  else {
    song_t *seeker = *songlib;

    while (seeker->next && (seeker->next != song))
      seeker = seeker->next;

    if (seeker->next)
      seeker->next = seeker->next->next;
  }

  song_lib_add(songlib, song);
}


void
sound_eq_init(sound_eq_t *queue)
{
  queue->first = queue->last = NULL;
}

void
sound_eq_queue_event(sound_eq_t *queue, int handle, int signal, int value)
{
  sound_eq_node_t *node;
  sound_event_t *evt;

  evt = xalloc(sizeof(sound_event_t));
  node = xalloc(sizeof(sound_eq_node_t));

  evt->handle = handle;
  evt->signal = signal;
  evt->value = value;

  node->event = evt;

  if (queue->first)
    queue->first->prev = node;
  else
    queue->last = node; /* !(queue->first) implies !(queue->last) */

  node->next = queue->first;
  node->prev = NULL;
  queue->first = node;
}

sound_event_t *
sound_eq_retreive_event(sound_eq_t *queue)
{
  if (queue->last) {
    sound_event_t *retval = queue->last->event;
    sound_eq_node_t *ntf = queue->last;

    if (ntf->prev)
      ntf->prev->next = NULL;

    queue->last = ntf->prev;
    free(ntf);

    if (!queue->last)
      queue->first = NULL;
    return retval;
  }
  else return NULL;
}
