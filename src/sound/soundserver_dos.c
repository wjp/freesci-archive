/***************************************************************************
 soundserver_dos.c Copyright (C) 1999, 2000 Rink Springer


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

    Rink Springer [RS] [rink@springer.cx]

***************************************************************************/
/* Sound server for DOS without sound output- just takes care of the events */

#include <resource.h>
#include <engine.h>

#include <soundserver.h>
#include <resource.h>
#include <sound.h>

static char *sound_dos_song;
static int  sound_dos_len;

static sound_eq_t sound_dos_queue; /* The event queue */
static song_t* modsong;
static song_t *song = NULL; /* The song we're playing */

static int suspended = 0; /* Used to suspend the sound server */
static GTimeVal suspend_time; /* Time at which the sound server was suspended */
static GTimeVal last_played, wakeup_time;

static int command;

static int ticks = 0; /* Ticks to next command */
static int fadeticks = 0;
static int ccc = 127; /* cumulative cue counter */
static int sound_hack; /* non-zero if we should not pay attention to the waiting */

static song_t *_songp = NULL;
static songlib_t songlib = &_songp;   /* Song library */
static GTimeVal wait_tv;

static int cmdlen[16] = {0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 0};
/* cmdlen[] is ripped from playmidi */

void sound_dos_handlecommand(sound_event_t *ev);

int
sound_dos_init(state_t *s) {
  sound_eq_init(&sound_dos_queue);

  ticks=0; sound_hack=1;
  gettimeofday((struct timeval *)&last_played, NULL);
  gettimeofday((struct timeval *)&wakeup_time, NULL);
  
  printf("DOS Sound Server fired up\n");
  return 0;
}

int
sound_dos_configure(state_t *s, char *option, char *value) {
  return 1; /* No options apply to this driver */
}

void sound_dos_poll();

sfx_driver_t sound_dos = {
  "dos",
  &sound_dos_init,
  &sound_dos_configure,

  /* Default implementations: */
  &sound_exit,
  &sound_get_event,
  &sound_save,
  &sound_restore,
  &sound_command,
  &sound_suspend,
  &sound_resume,
  &sound_dos_poll
};

/*
 * sound_exit(state_t* s)
 *
 * This will shutdown the sound server
 *
 */
void
sound_exit(state_t *s) {
    printf("DOS Sound Server is history now\n");
}

/*
 * sound_get_event(state_t* s)
 *
 * This will cause the sound server to return an event.
 *
 */
sound_event_t*
sound_get_event(state_t* s) {
  return sound_eq_retreive_event(&sound_dos_queue);

}

/*
 * sound_save(state_t* s,char* dir)
 *
 * This will cause the sound server to save the sound status.
 *
 */
int
sound_save(state_t* s, char* dir) {
  /* just say it's ok */
  return 1;
}

/*
 * sound_restore(state_t* s,char* dir)
 *
 * This will cause the sound server to restore the sound status.
 *
 */
int
sound_restore(state_t* s, char* dir) {
  /* just say it's ok */
  return 1;
}

/*
 * int sound_command(state_t *s, int command, int handle, int parameter)
 *
 * This will cause the sound server to handle a command.
 *
 */
int
sound_command(state_t *s, int command, int handle, int parameter) {
  sound_event_t event = { handle, command, parameter };

  /* handle the command */
  switch (command) {
      case SOUND_COMMAND_INIT_SONG: { /* initialize song */
                                    resource_t* song = findResource(sci_sound, parameter);
                                    int len;

                                    if (!song) {
                                      sciprintf("DOS_SOUND: Attempt to play invalid sound.%03d\n", parameter);
                                      return 1;
                                    }
                                    len = song->length;

                                    /* set the pointer (uugh!) */
                                    sound_dos_song=song->data;
                                    sound_dos_len=len;

                                    sound_dos_handlecommand(&event);
                                    break;
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
      case  SOUND_COMMAND_MAPPINGS:
     case SOUND_COMMAND_SAVE_STATE: /* Those two commands are only used from special wrapper */
  case SOUND_COMMAND_RESTORE_STATE: /* functions that provide additional data. */
  case SOUND_COMMAND_SUSPEND_SOUND:
   case SOUND_COMMAND_RESUME_SOUND:
       case SOUND_COMMAND_STOP_ALL:
 case SOUND_COMMAND_GET_NEXT_EVENT: sound_dos_handlecommand(&event);
                                    break;

  }

  /* it's ok */
  return 0;
}

/*
 * void sound_suspend(state_t* s)
 *
 * This will cause the sound server to suspend itself.
 *
 */
void
sound_suspend(state_t *s) {
  sound_command(s, SOUND_COMMAND_SUSPEND_SOUND, 0, 0);
}

/*
 * void sound_resume(state_t* s)
 *
 * This will cause the sound server to resume.
 *
 */
void
sound_resume(state_t *s) {
  sound_command(s, SOUND_COMMAND_RESUME_SOUND, 0, 0);
}

/*
 * void sound_dos_handlecommand(sound_event_t* ev)
 *
 * This will handle the commands! Yeah!
 *
 */
void
sound_dos_handlecommand(sound_event_t *ev) {
  modsong = song_lib_find(songlib, ev->handle);

  /* figure out the command */
  switch(ev->signal) {
    case SOUND_COMMAND_INIT_SONG: /* we inited a song. handle it */
                                  if(modsong) {
                                    int lastmode = song_lib_remove(songlib, ev->handle);
                                    if(modsong) {
                                      int lastmode = song_lib_remove(songlib, ev->handle);
                                      if (lastmode == SOUND_STATUS_PLAYING) {
                                       song = songlib[0]; /* Force song detection to start with the highest priority song */
                                       sound_eq_queue_event(&sound_dos_queue, ev->handle, SOUND_SIGNAL_FINISHED, 0);
                                      }
                                    }

                                    if (modsong == song) song = NULL;

                                    if (lastmode == SOUND_STATUS_PLAYING) {
                                      song = songlib[0]; /* Force song detection to start with the highest priority song */
                                      sound_eq_queue_event(&sound_dos_queue, ev->handle, SOUND_SIGNAL_FINISHED, 0);
                                    }
                                  }
                                  modsong = song_new(ev->handle, sound_dos_song, sound_dos_len, ev->value);
                                  song_lib_add(songlib, modsong);

                                  ccc = 127; /* Reset ccc */

                                  sound_eq_queue_event(&sound_dos_queue, ev->handle, SOUND_SIGNAL_INITIALIZED, 0);
                                  break;
   case SOUND_COMMAND_SET_VOLUME: /* set volume */
     case SOUND_COMMAND_SET_MUTE: /* set mute */
                                  /* [RS] I wish everything was so easy to
                                     implement ;-) */
                                  break;
  case SOUND_COMMAND_PLAY_HANDLE: /* play handle */
                                  if (modsong) {
                                    modsong->status = SOUND_STATUS_PLAYING;
                                    sound_eq_queue_event(&sound_dos_queue, ev->handle, SOUND_SIGNAL_PLAYING, 0);
                                    song = modsong; /* Play this song */
                                  } else {
                                    fprintf(stderr, "DOS_SOUND: Attempt to play invalid handle %04x\n", ev->handle);
                                  }
                                  break;
case SOUND_COMMAND_DISPOSE_HANDLE: /* dispose handle */
                                  if (modsong) {
                                    if (song_lib_remove(songlib, ev->handle) == SOUND_STATUS_PLAYING) {
                                      song = songlib[0]; /* Force song detection to start with the highest priority song */
                                      sound_eq_queue_event(&sound_dos_queue, ev->handle, SOUND_SIGNAL_FINISHED, 0);
                                    }
                                  } else {
                                    fprintf(stderr, "DOS_SOUND: Attempt to dispose invalid handle %04x\n", ev->handle);
                                  }
                                  break;
    case SOUND_COMMAND_SET_LOOPS: /* set loops */
                                  if (modsong) {
                                    modsong->loops = ev->value;
                                  } else {
                                    fprintf(stderr, "DOS_SOUND: Attempt to set loops on invalid handle %04x\n", ev->handle);
                                  }
                                  break;
  case SOUND_COMMAND_STOP_HANDLE: /* stop handle */
                                  if (modsong) {
                                    modsong->status = SOUND_STATUS_STOPPED;
                                    modsong->pos = modsong->loopmark = 33; /* Reset position */
                                    sound_eq_queue_event(&sound_dos_queue, ev->handle, SOUND_SIGNAL_FINISHED, 0);
                                  } else {
                                    fprintf(stderr,"DOS_SOUND: Attempt to stop invalid handle %04x\n", ev->handle);
                                  }
                                  break;
case SOUND_COMMAND_SUSPEND_HANDLE: /* suspend handle */
                                  if (modsong) {
                                    modsong->status = SOUND_STATUS_SUSPENDED;
                                    sound_eq_queue_event(&sound_dos_queue, ev->handle, SOUND_SIGNAL_PAUSED, 0);
                                  } else {
                                    fprintf(stderr,"DOS_SOUND: Attempt to suspend invalid handle %04x\n", ev->handle);
                                  }
                                  break;
case SOUND_COMMAND_RESUME_HANDLE: /* resume handle */
                                  if (modsong) {
                                    if (modsong->status == SOUND_STATUS_SUSPENDED) {
                                      modsong->status = SOUND_STATUS_WAITING;
                                      sound_eq_queue_event(&sound_dos_queue, ev->handle, SOUND_SIGNAL_RESUMED, 0);
                                    } else {
                                      fprintf(stderr, "DOS_SOUND: Attempt to resume handle %04x although not suspended\n", ev->handle);
                                    }
                                  } else {
                                    fprintf(stderr, "DOS_SOUND: Attempt to resume invalid handle %04x\n", ev->handle);
                                  }
                                  break;
case SOUND_COMMAND_RENICE_HANDLE: /* renice handle */
                                  if (modsong) {
                                    modsong->priority = ev->value;
                                    song_lib_resort(songlib, modsong); /* Re-sort it according to the new priority */
                                  } else {
                                    fprintf(stderr, "DOS_SOUND: Attempt to renice on invalid handle %04x\n", ev->handle);
                                  }
                                  break;
  case SOUND_COMMAND_FADE_HANDLE: /* fade handle */
                                  if (modsong) {
                                    modsong->fading = ev->value;
                                  } else {
                                    fprintf(stderr, "DOS_SOUND: Attempt to fade on invalid handle %04x\n", ev->handle);
                                  }
                                  break;
case SOUND_COMMAND_SUSPEND_SOUND: /* suspend sound server */
                                  gettimeofday((struct timeval *)&suspend_time, NULL);
                                  suspended = 1;
                                  break;
 case SOUND_COMMAND_RESUME_SOUND: /* resume from suspension */ {
                                  GTimeVal resume_time;

                                  gettimeofday((struct timeval *)&resume_time, NULL);
                                  /* Modify last_played relative to wakeup_time - suspend_time */
                                  last_played.tv_sec += resume_time.tv_sec - suspend_time.tv_sec - 1;
                                  last_played.tv_usec += resume_time.tv_usec - suspend_time.tv_usec + 1000000;

                                  /* Make sure that 0 <= tv_usec <= 999999 */
                                  last_played.tv_sec -= last_played.tv_usec / 1000000;
                                  last_played.tv_usec %= 1000000;

                                  suspended = 0;
                                  break;
                                  }
  }
}

/*
 * sound_calc_wakeup_time(GTimeVal* lastslept,int ticks)
 *
 * This will return when we should awake.
 *
 */
GTimeVal
sound_calc_wakeup_time(GTimeVal* lastslept,int ticks) {
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

/*
 * sound_check_wait()
 *
 * This will check whether the sound server should awake from the suspension.
 * It will return zero if not and non-zero if it should awake.
 *
 */
int
sound_check_wait() {
  GTimeVal ctime;

  /* if we are suspended, we should not awake *
  if(suspended) return 0;

  /* get current time */
  gettimeofday((struct timeval *)&ctime, NULL);

  /* check for the arrived time */
  if (wakeup_time.tv_sec > ctime.tv_sec) return 0;
  if ((wakeup_time.tv_sec == ctime.tv_sec)&&
      (wakeup_time.tv_usec > ctime.tv_usec)) return 0;

  /* keep sleeping */
  return 1;
}

/*
 * sound_dos_poll()
 *
 * This will poll the DOS sound server, allowing it to handle commands etc.
 *
 */
void
sound_dos_poll() {
  song_t* the_song;

  ticks=0;

  if(!sound_hack) {
    if(!sound_check_wait()) return;
  } else {
    sound_hack=0;
  }

  /* fading? */
  if(fadeticks) {
      /* yup. cut a tick off */
      fadeticks--;
      ticks = 1; /* Just fade for this tick */
  } else if (song && (song->fading == 0)) { /* Finished fading? */
      /* yup. sound is done! */
      song->status = SOUND_STATUS_STOPPED;
      song->pos = song->loopmark = 33; /* Reset position */
      sound_eq_queue_event(&sound_dos_queue, song->handle, SOUND_SIGNAL_FINISHED, 0);
  }

  the_song = song_lib_find_active(songlib, song);
  if (the_song == NULL) {
      gettimeofday((struct timeval *)&last_played, NULL);
      ticks = 60; /* Wait a second for new commands, then collect your new ticks here. */
  }
  song = the_song;

  while (ticks==0) {
      int newcmd;
      int param, param2;
      int tempticks;

      gettimeofday((struct timeval *)&last_played, NULL);

      /* Handle length escape sequence */
      while ((tempticks = song->data[(song->pos)++]) == SCI_MIDI_TIME_EXPANSION_PREFIX)
	ticks += SCI_MIDI_TIME_EXPANSION_LENGTH;

      ticks += tempticks;

      newcmd = song->data[song->pos];

      if (newcmd & 0x80) {
        ++(song->pos);
        command = newcmd;
      } /* else we've got the 'running status' mode defined in the MIDI standard */


      if (command == SCI_MIDI_EOT) {
        if (song->loops) {
          --(song->loops);
          song->pos = song->loopmark;
          sound_eq_queue_event(&sound_dos_queue, song->handle, SOUND_SIGNAL_LOOP, song->loops);
        } else { /* Finished */
          song->status = SOUND_STATUS_STOPPED;
          song->pos = song->loopmark = 33; /* Reset position */
          sound_eq_queue_event(&sound_dos_queue, song->handle, SOUND_SIGNAL_FINISHED, 0);
        }
      } else { /* not end of MIDI track */

	param = song->data[song->pos];
	if (SCI_MIDI_CONTROLLER(command))
	  param2 = song->data[song->pos + 1];
	song->pos += cmdlen[command >> 4];

	if (SCI_MIDI_CONTROLLER(command) && (param == SCI_MIDI_CUMULATIVE_CUE))
	  sound_eq_queue_event(&sound_dos_queue, song->handle, SOUND_SIGNAL_ABSOLUTE_CUE, ccc += param2);
        } else if (command == SCI_MIDI_SET_SIGNAL) {
          if (param == SCI_MIDI_SET_SIGNAL_LOOP) {
            song->loopmark = song->pos;
          } else {
            sound_eq_queue_event(&sound_dos_queue, song->handle, SOUND_SIGNAL_ABSOLUTE_CUE, param);
          }
        }
  }

  /* are we currently fading? */
  if (song->fading >= 0) {
    /* yup */
    fadeticks = ticks;
    ticks = !!ticks;
    song->fading -= fadeticks;

    if (song->fading < 0)
      song->fading = 0; /* Faded out after the ticks have run out */
    }
  }

  /* calculate new sleep time */
  wakeup_time = sound_calc_wakeup_time(&last_played,ticks);
}
