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

#include <stdarg.h>
#include <sound.h>
#include <glib.h>
#include <soundserver.h>
#include <sys/types.h>
#include <engine.h>

#ifdef HAVE_SOCKETPAIR
#include <sys/socket.h>
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


sfx_driver_t *sfx_drivers[] = {
#ifdef HAVE_FORK
  /* Assume that sound_null works on any box that has fork() */
  &sound_null,
#endif /* HAVE_FORK */
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
  GTimeVal waittime2 = {0, 0};
  char debug_buf[65];
  sound_event_t *event = xalloc(sizeof(sound_event_t));

  FD_ZERO(&inpfds);
  FD_SET(s->sound_pipe_debug[0], &inpfds);
  while ((select(s->sound_pipe_debug[0] + 1, &inpfds, NULL, NULL, &waittime)
	  && (inplen = read(s->sound_pipe_debug[0], debug_buf, 64)) > 0)) {

    debug_buf[inplen] = 0; /* Terminate string */
    fprintf(stderr, debug_buf); /* Transfer debug output */
    waittime.tv_sec = 0;
    waittime.tv_usec = 0;

    FD_ZERO(&inpfds);
    FD_SET(s->sound_pipe_debug[0], &inpfds);

  }

  FD_ZERO(&inpfds);
  FD_SET(s->sound_pipe_events[0], &inpfds);

  if (select(s->sound_pipe_events[0] + 1, &inpfds, NULL, NULL, &waittime2)) {

      if (read(s->sound_pipe_events[0], event, sizeof(sound_event_t)) == sizeof(sound_event_t))
	return event;

      else free(event);
    }

  return NULL;
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


int
sound_command(state_t *s, int command, int handle, int parameter)
{
  sound_event_t event = {handle, command, parameter};

  switch (command) {
  case SOUND_COMMAND_INIT_SONG: {
    resource_t *song = findResource(sci_sound, parameter);
    int len;

    if (!song) {
      sciprintf("Attempt to play invalid sound.%03d\n", parameter);
      return 1;
    }

    len = song->length;

    write(s->sound_pipe_in[1], &event, sizeof(sound_event_t));
    write(s->sound_pipe_in[1], &len, sizeof(int)); /* Write song length */
    write(s->sound_pipe_in[1], song->data, len); /* Transfer song */


    return 0;
  }

  case SOUND_COMMAND_PLAY_HANDLE:
  case SOUND_COMMAND_SET_LOOPS:
  case SOUND_COMMAND_DISPOSE_HANDLE:
  case SOUND_COMMAND_SET_MUTE:
  case SOUND_COMMAND_STOP_HANDLE:
  case SOUND_COMMAND_SUSPEND_HANDLE:
  case SOUND_COMMAND_RESUME_HANDLE:
  case SOUND_COMMAND_SET_VOLUME:
  case SOUND_COMMAND_RENICE_HANDLE:
  case SOUND_COMMAND_SHUTDOWN:
  case SOUND_COMMAND_MAPPINGS:
    write(s->sound_pipe_in[1], &event, sizeof(sound_event_t));
    return 0;

  case SOUND_COMMAND_TEST: {
    fd_set fds;
    GTimeVal timeout = {0, SOUND_SERVER_TIMEOUT};
    int dummy, success;

    write(s->sound_pipe_in[1], &event, sizeof(sound_event_t));

    FD_ZERO(&fds);
    FD_SET(s->sound_pipe_out[0], &fds);
    success = select(s->sound_pipe_out[0]+1, &fds, NULL, NULL, &timeout);

    if (success) {

      read(s->sound_pipe_out[0], &dummy, sizeof(int)); /* Empty pipe */
      return 0;

    } else {

      fprintf(stderr,"Sound server timed out\n");
      return 1;

    }
  }    

  default: sciprintf("Unknown sound command %d\n", command);
  }

}

#endif /* HAVE_FORK */


GTimeVal
song_sleep_time(GTimeVal *lastslept, int ticks)
{
  GTimeVal tv;
  long timetosleep = ticks * SOUND_TICK; /* Time to sleep in us */
  long timeslept; /* Time already slept */

  g_get_current_time(&tv);
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
  GTimeVal retval = {lastslept->tv_sec, lastslept->tv_usec};

  retval.tv_sec += ticks / 60;
  retval.tv_usec += (ticks % 60) * SOUND_TICK;

  if (retval.tv_usec >= 1000000) {
    retval.tv_usec -= 1000000;
    ++retval.tv_sec;
  }

  return retval;
}


song_t *
song_new(word handle, byte *data, int size, int priority)
{
  song_t *retval = malloc(sizeof(song_t));

  retval->data = data;
  retval->handle = handle;
  retval->priority = priority;
  retval->size = size;

  retval->pos = retval->loopmark = 33; /* The first 33 bytes are header data */
  retval->fading = -1; /* Not fading */
  retval->loops = 0; /* No fancy additional loops */
  retval->status = SOUND_STATUS_STOPPED;

  return retval;
}


void
song_lib_add(songlib_t songlib, song_t *song)
{
  song_t *seeker;
  int pri = song->priority;

  if (songlib[0] == NULL) {
    songlib[0] = song;
    song->next = NULL;
    return;
  }

  seeker = songlib[0];

  while (seeker->next && (seeker->next->priority > pri))
    seeker = seeker->next;

  song->next = seeker->next;
  seeker->next = song;

}

song_t *
song_lib_find(songlib_t songlib, word handle)
{
  song_t *seeker = songlib[0];

  while (seeker && (seeker->handle != handle))
    seeker = seeker->next;

  return seeker;
}


song_t *
song_lib_find_active(songlib_t songlib, song_t *last_played_song)
{
  song_t *seeker = songlib[0];

  if (last_played_song)
    if (last_played_song->status == SOUND_STATUS_PLAYING)
      return last_played_song; /* This one was easy... */

  while (seeker && (seeker->status != SOUND_STATUS_WAITING)
	 && (seeker->status != SOUND_STATUS_PLAYING))
    seeker = seeker->next;

  return seeker;
}



int
song_lib_remove(songlib_t songlib, word handle)
{
  int retval;
  song_t *goner = songlib[0];

  if (goner->handle == handle)
    songlib[0] = goner->next;

  else while ((goner->next) && (goner->next->handle != handle))
    goner = goner->next;

  if (goner->next) {/* Found him? */
    song_t * oldnext = goner->next;

    goner->next = goner->next->next;
    goner = oldnext;
  } else return -1; /* No. */

  retval = goner->status;
  free(goner->data);
  free(goner);

  return retval;
}


void
song_lib_resort(songlib_t songlib, song_t *song)
{
  if (songlib[0] == song)
    songlib[0] = song->next;
  else {
    song_t *seeker = songlib[0];

    while (seeker->next && (seeker->next != song))
      seeker = seeker->next;

    if (seeker->next)
      seeker->next = seeker->next->next;
  }

  song_lib_add(songlib, song);
}
