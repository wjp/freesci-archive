/***************************************************************************
 thread_ss_sdl.c Copyright (C) 2001,2 Solomon Peachy

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

***************************************************************************/

#include <sci_memory.h>
#include <engine.h>
#include <soundserver.h>
#include <sound.h>
#include <pcmout.h>
#include <midi_device.h>
#include <sciresource.h>
#include <scitypes.h>

#include <stdarg.h>
#include <sys/types.h>

#ifdef HAVE_SDL

#ifndef _MSC_VER
#  include <SDL/SDL.h>
#  include <SDL/SDL_thread.h>
#  include <sys/timeb.h>
#else
#  include <SDL.h>
#  include <SDL_thread.h>
#  include <sci_win32.h>
#  include <windows.h>
#endif

extern sound_server_t sound_server_sdl;

/* global config variables */
static int sdl_reverse_stereo = 0;

SDL_Thread *child = NULL;

/* event queue */
#define MAX_EVENTS 20
sound_event_t evqueue[MAX_EVENTS];
int evqueue_head;
int evqueue_tail; 

/* local state */
static sound_server_state_t sss;

static GTimeVal suspend_time; /* Time at which we were suspended */
static GTimeVal last_played;  /* Time the last note was played at */
static unsigned long usecs_to_sleep;
static song_t *songlib = NULL;
static song_t *current = NULL;       /* The currently playing song */
static int run = 0;                  

static void
sci0_thread_ss(int reverse_stereo, sound_server_state_t *ss_state);

int sdl_soundserver_thread(void *args)
{
  // Ideally, this would be a song iterator loop.
  run = 1;
  sci0_thread_ss(sdl_reverse_stereo, &sss);
  return 0;
}

int sound_sdl_init(state_t *s, int flags) 
{
  memset(&sss, 0, sizeof(sound_server_state_t));
  memset(evqueue, 0, sizeof(evqueue));
  evqueue_head = 0;
  evqueue_tail = 0;

  if (SDL_Init(SDL_INIT_EVENTTHREAD | SDL_INIT_NOPARACHUTE) < 0) {
    fprintf(debug_stream, "sound_sdl_init(): SDL_Init() returned -1\n");
    return -1;
  }

  /* open up the output devices */
  if (pcmout_open() < 0)
    return -1;
  if (init_midi_device(s) < 0)
    return -1;

  /* set up globals */
  global_sound_server = &sound_server_sdl;
  debug_stream = stderr;
  if (flags & SOUNDSERVER_INIT_FLAG_REVERSE_STEREO)
	  sdl_reverse_stereo = 1;

  /* clean up state */
  memset(sss.mute_channel, MIDI_MUTE_OFF, MIDI_CHANNELS * sizeof(byte));
  sss.songlib = &songlib;
  sss.current_song = NULL;

  sss.resmgr = s->resmgr;

  fprintf(debug_stream, "Threaded sound server initialized\n");

  return 0;
}

int sound_sdl_configure(state_t *s, char *option, char *value)
{
	return 1; /* No options apply to this driver */
}

int sound_sdl_command(state_t *s, unsigned int command, unsigned int handle, 
		      long value)
{
  sound_server_state_t *ss_state = &sss;

  /* let's handle the incoming Soundserver commands here */
  switch (command) {

  case SOUND_COMMAND_FADE_HANDLE:
    fade_handle(value, handle, ss_state);
    break;
    
  case SOUND_COMMAND_TEST:
    return midi_device->polyphony;

  case SOUND_COMMAND_SET_VOLUME:
    if (s->sound_mute) {  /* if we're muted, update the mute */
      s->sound_mute = value;
    } else {
      s->sound_volume = value;
      set_master_volume(value, ss_state);
    }
    if (s->sound_mute)
      return s->sound_mute;
    else
      return s->sound_volume;

    /* intentional fallthrough */    
  case SOUND_COMMAND_GET_VOLUME:
    if (s->sound_mute)
      return s->sound_mute;
    else
      return s->sound_volume;
    
  case SOUND_COMMAND_SET_MUTE:
    switch (value) {
    case SCI_MUTE_ON:
      /* sound should not be active (but store old volume) */
      s->sound_mute = s->sound_volume;
      s->sound_volume = 0;
      break;

    case SCI_MUTE_OFF:
      /* sound should be active */
      if (s->sound_mute != 0) /* don't let volume get set to 0 */
	s->sound_volume = s->sound_mute;
      s->sound_mute = 0;
      break;
      
    default:
      sciprintf("Unknown mute value %d\n", value);
    }
    /* send volume change across the wire */
    command = SOUND_COMMAND_SET_VOLUME;
    value = s->sound_volume;
    set_master_volume(value, ss_state);
    
    /* Intentional fallthrough */
  case SOUND_COMMAND_GET_MUTE:
    if (s->sound_mute)
      return SCI_MUTE_ON;
    else 
      return SCI_MUTE_OFF;

  case SOUND_COMMAND_IMAP_SET_INSTRUMENT:
  case SOUND_COMMAND_IMAP_SET_KEYSHIFT:
  case SOUND_COMMAND_IMAP_SET_FINETUNE:
  case SOUND_COMMAND_IMAP_SET_BENDER_RANGE:
  case SOUND_COMMAND_IMAP_SET_PERCUSSION:
  case SOUND_COMMAND_IMAP_SET_VOLUME:
    imap_set(command, handle, value);
    break;

  case SOUND_COMMAND_MUTE_CHANNEL:
  case SOUND_COMMAND_UNMUTE_CHANNEL:
    set_channel_mute(value, handle, ss_state);
    break;

  case SOUND_COMMAND_LOOP_HANDLE:
    loop_handle(value, handle, ss_state);
    break;
    
  case SOUND_COMMAND_STOP_HANDLE:
    if (current && (current->handle == handle)) {
      if (run) {
	run = 0;
	SDL_WaitThread(child, NULL);
      }
    }
    stop_handle(handle, ss_state);
    break;
    
  case SOUND_COMMAND_SUSPEND_HANDLE:
    if (current && (current->handle == handle)) {
      if (run) {
	run = 0;
	SDL_WaitThread(child, NULL);
      }
    }
    suspend_handle(handle, ss_state);
    if (!run) {
      child = SDL_CreateThread( sdl_soundserver_thread, s);
      sci_sched_yield();
    }
    break;
    
  case SOUND_COMMAND_RESUME_HANDLE:
    if (current && (current->handle == handle)) {
      if (run) {
	run = 0;
	SDL_WaitThread(child, NULL);
      }
    }
    resume_handle(handle, ss_state);
    if (!run) {
      child = SDL_CreateThread( sdl_soundserver_thread, s);
      sci_sched_yield();
    }
    break;
    
  case SOUND_COMMAND_RENICE_HANDLE:
    if (current && (current->handle == handle)) {
      if (run) {
	run = 0;
	SDL_WaitThread(child, NULL);
      }
    }
    renice_handle(value, handle, ss_state);
    if (!run) {
      child = SDL_CreateThread( sdl_soundserver_thread, s);
      sci_sched_yield();
    }
    break;

  case SOUND_COMMAND_STOP_ALL:
    /* kill music loop. */
    if (run) {
      run = 0;
      SDL_WaitThread(child, NULL);
    }
    stop_all(ss_state);
    ss_state->current_song = NULL;
    break;

  case SOUND_COMMAND_SHUTDOWN: 
    fprintf(debug_stream,"Sound server: Received shutdown signal\n");

    /* kill music loop. */
    if (run) {
      run = 0;
      SDL_WaitThread(child, NULL);
    }

    /* clean up state */
    song_lib_free(ss_state->songlib);    

    break;
  
  case SOUND_COMMAND_PRINT_SONG_INFO: 
    if (ss_state->current_song)
      fprintf(debug_stream, "%04x", ss_state->current_song->handle);
    else
      fprintf(debug_stream, "No song is playing.");
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

  case SOUND_COMMAND_SUSPEND_ALL: 
    /* Track the time we suspended at */
    sci_get_current_time((GTimeVal *)&suspend_time); 

    if (run) {
      run = 0;
      SDL_WaitThread(child, NULL);
    }

    suspend_all(ss_state);
    midi_allstop();
    break;

  case SOUND_COMMAND_RESUME_ALL: {
    GTimeVal resume_time;
    
    sci_get_current_time((GTimeVal *)&resume_time);
    /* Modify last_played relative to wakeup_time - suspend_time */
    last_played.tv_sec += resume_time.tv_sec - suspend_time.tv_sec - 1;
    last_played.tv_usec += resume_time.tv_usec - suspend_time.tv_usec + 1000000;
    /* Make sure that 0 <= tv_usec <= 999999 */
    last_played.tv_sec -= last_played.tv_usec / 1000000;
    last_played.tv_usec %= 1000000;
    
    /* All these calculations ensure that, after resuming, we wait 
       for the exact (well, mostly) number of microseconds that we were 
       supposed to wait for before we were suspended.  */
    
    if (run) {
      run = 0;
      SDL_WaitThread(child, NULL);
    }

    resume_all(ss_state);
    if (!run) {
      child = SDL_CreateThread( sdl_soundserver_thread, s);
      sci_sched_yield();
    }
    break;
  }

  case SOUND_COMMAND_INIT_HANDLE: {
    resource_t *song_resource;
    song_t *modsong; 
    byte *song_copy;
    
#ifdef DEBUG_SOUND_SERVER
    fprintf(debug_stream, "Initialising song handle %04x: ", handle);
#endif
     
     song_resource = scir_find_resource(s->resmgr, sci_sound, value, 1);
     if (song_resource == NULL) {
       fprintf(debug_stream, "Attempted to init a non-existant sound resource! %04x\n", value);
       return 1;
     }

     if (s->version < SCI_VERSION_RESUME_SUSPENDED_SONG) {
       global_sound_server->command(s, SOUND_COMMAND_STOP_ALL, handle, value);
     }

     /* If we're init-ing our current handle, 
	we need to kill off the current playback of it */
     if (current && (current->handle == handle)) {
       if (run) {
	 run = 0;
	 SDL_WaitThread(child, NULL);
       }
       midi_allstop();
     }
     
    /* look up the song in our library */
    modsong = song_lib_find(ss_state->songlib, handle);

    if (modsong) { 
      if (modsong->status == SOUND_STATUS_PLAYING) {
	/* if it's the active song, reset the current song pointer */
	ss_state->current_song = ss_state->songlib[0];	
	global_sound_server->queue_event(handle, SOUND_SIGNAL_FINISHED, 0);
      }

      /* Mark it for deletion */
      song_lib_remove(ss_state->songlib, handle, ss_state->resmgr);

      /* Usually the same as above, but not always */
      if (modsong == ss_state->current_song)
	ss_state->current_song = NULL;
    }
    
    /* create a new song */
    song_copy = malloc(song_resource->size);
    memcpy(song_copy, song_resource->data, song_resource->size);
    modsong = song_new(handle, song_copy, song_resource->size, value);

    /* Now unlock the resource, since we have a copy again */
    scir_unlock_resource(s->resmgr, song_resource, sci_sound, value);

    /* If the hardware wants it, enable the rhythm channel */
    if (midi_playrhythm) 
      modsong->flags[RHYTHM_CHANNEL] |= midi_playflag;
    
    /* Add song to song library */
    song_lib_add(ss_state->songlib, modsong);
    
    /* reset ccc */
    ss_state->sound_cue = 127;

    /* if the song loop is dead, we should re-start it */
    if (!run) {
      child = SDL_CreateThread( sdl_soundserver_thread, s);
      sci_sched_yield();
    }

    /* Send back ACK */    
    global_sound_server->queue_event(handle, SOUND_SIGNAL_INITIALIZED, 0);

    return 0;
  }

  case SOUND_COMMAND_PLAY_HANDLE: 
    /* stop current song */
    if (run) {
      run = 0;
      SDL_WaitThread(child, NULL);
      midi_allstop();
    }

    play_handle(value, handle, ss_state);

    if (!run) {
      child = SDL_CreateThread( sdl_soundserver_thread, s);
      sci_sched_yield();
    }
    break;

  case SOUND_COMMAND_DISPOSE_HANDLE: 
    /* if we're to dispose our current handle, stop its playback */
    if (current && (current->handle == handle)) {
      if (run) {
	run = 0;
	SDL_WaitThread(child, NULL);
	midi_allstop();
      }
    }
    dispose_handle(handle, ss_state);
    if (!run) {
      child = SDL_CreateThread( sdl_soundserver_thread, s);
      sci_sched_yield();
    }
    break;

  default:
    fprintf(debug_stream, "Received invalid signal: %d\n", command);
    break;
  }
  return 0;
}

int sound_sdl_save(state_t *s, char *dirname)
{
    GTimeVal currtime;
    int success;

    sci_get_current_time(&currtime);
    usecs_to_sleep = (currtime.tv_sec - last_played.tv_sec) * 1000000
      + (currtime.tv_usec - last_played.tv_usec);
    
   success = soundsrv_save_state(debug_stream,
				 global_sound_server->flags & SOUNDSERVER_FLAG_SEPARATE_CWD ? dirname : NULL, &sss);    
   return success;
}

int sound_sdl_restore(state_t *s, char *dirname)
{
  int success;
  
  /* fetch the current time, store in last_played.
     We use this to reset the sound delay */
  sci_get_current_time(&last_played); 

  /* Had to be lost in transition to restoring saved games from different sound server types
     last_played.tv_sec -= secs = (usecs_to_sleep - last_played.tv_usec) / 1000000;
     last_played.tv_usec -= (usecs_to_sleep + secs * 1000000);
  */
    

  if (run) {
    run = 0;
    SDL_WaitThread(child, NULL);
    midi_allstop();
  }

    // XXXX We should make this more intelligent and pull data from the heap.
  success = soundsrv_restore_state(debug_stream,
				   global_sound_server->flags & SOUNDSERVER_FLAG_SEPARATE_CWD ? dirname : NULL, &sss);
  
  if (!run) {
    child = SDL_CreateThread( sdl_soundserver_thread, s);
    sci_sched_yield();
  }

  return success;
}

void sound_sdl_exit(state_t *s)
{
  /* tell the soundserver to die */
  global_sound_server->command(s, SOUND_COMMAND_SHUTDOWN, 0, 0);

  /* clean up */

  pcmout_close();
  midi_close();

  return;
}

void sound_sdl_enqueue(unsigned int handle, unsigned int signal, long value)
{
  evqueue[evqueue_head].handle = handle;
  evqueue[evqueue_head].signal = signal;
  evqueue[evqueue_head].value = value;

  evqueue_head = (evqueue_head + 1) % MAX_EVENTS;

  if (evqueue_head == evqueue_tail)
    fprintf(debug_stream, "Sound Event Queue filled up!  CRAP!\n");

  return;
}

sound_event_t *sound_sdl_dequeue(void)
{
  sound_event_t *event;

  /* if head == tail, we're empty */
  if (evqueue_head == evqueue_tail)
    return NULL;

  event = sci_malloc(sizeof(sound_event_t));

  event->handle = evqueue[evqueue_tail].handle;
  event->signal = evqueue[evqueue_tail].signal;
  event->value = evqueue[evqueue_tail].value;

  evqueue_tail = (evqueue_tail + 1) % MAX_EVENTS;

  return event;
}

void
sound_sdl_suspend(state_t *s)
{
	global_sound_server->command(s, SOUND_COMMAND_SUSPEND_ALL, 0, 0);
}

void
sound_sdl_resume(state_t *s)
{
	global_sound_server->command(s, SOUND_COMMAND_RESUME_ALL, 0, 0);
}

sound_server_t sound_server_sdl = {
	"sdl",
	"0.1",
	SOUNDSERVER_FLAG_SHAREDMEM,
	&sound_sdl_init,
	&sound_sdl_configure,
	&sound_sdl_exit,
	&sound_sdl_dequeue,
	&sound_sdl_enqueue,
	NULL,   // get_command
	NULL,   // queue_command
	NULL,   // get_data
	NULL,   // send_data
	&sound_sdl_save,
	&sound_sdl_restore,
	&sound_sdl_command,
	&sound_sdl_suspend,
	&sound_sdl_resume,
	NULL // poll
};

/* #define OUTPUT_SONG_CHANGES */

void
sci0_thread_ss(int reverse_stereo, sound_server_state_t *ss_state)
{
  GTimeVal   wakeup_time; /* Time at which we stop waiting for events */
  GTimeVal   ctime; /* 'Current time' (temporary local usage) */

  unsigned int ticks_to_wait;	/* before next midi operation */
  int command = 0;              /* MIDI operation */
  long tempticks;

  /* initialise default values */
  memset(&suspend_time, 0, sizeof(GTimeVal));
  memset(&wakeup_time, 0, sizeof(GTimeVal));
  memset(&ctime, 0, sizeof(GTimeVal));
  sci_get_current_time(&last_played);
  
  /* sound server loop */
  while (run) {
      GTimeVal wait_tv;	    /* Number of seconds/usecs to sleep */
      ticks_to_wait = 0;    /* Ticks to next command */
      fflush(debug_stream); /* Flush debug stream */
      
      /* if we have a song, let's check it out. */
      if (current) {
	/* Have we finished fading out the current song? */
	if (current->fading == 0) { 
#ifdef DEBUG_SOUND_SERVER
	  fprintf(debug_stream, "Song %04x faded out\n", current->handle);
#endif
	  current->resetflag = 1; /* reset song position */
	  stop_handle((word)current->handle, ss_state);
	  global_sound_server->queue_event(current->handle, 
					   SOUND_SIGNAL_LOOP, -1);
	}
      }

      // XXX fade out race of sorts?
      
      /* Is this a song change? */
      if (ss_state->current_song != current) {	

	/* restore the instruments */
	if (ss_state->current_song) {
	  guint8 i;
	  for (i = 0; i < MIDI_CHANNELS; i++) {
	    if (ss_state->current_song->instruments[i])
	      midi_event2((guint8)(MIDI_INSTRUMENT_CHANGE | i),
			  (guint8)ss_state->current_song->instruments[i], 0);
	  }
	}
      }

      /* find the active song */
      current = song_lib_find_active(ss_state->songlib, 
				     ss_state->current_song);

      if (current == NULL) {
	run = 0;
	continue;
      }

      /* we found an active song! */
      
      sci_get_current_time((GTimeVal *)&last_played);
      /* Just played a note (see bottom of the big outer loop), so we
	 reset the timer */
      
      ticks_to_wait = 0; /* Number of ticks until the next note is played */
      
      /* Handle length escape sequence (see SCI sound specs) */
      while ((tempticks = current->data[(current->pos)++]) == SCI_MIDI_TIME_EXPANSION_PREFIX) {
	ticks_to_wait += SCI_MIDI_TIME_EXPANSION_LENGTH;
	if (current->pos >= current->size) {
	  fprintf(debug_stream, "Sound server: Error: Reached end of song while decoding prefix:\n");
	  dump_song(current);
	  fprintf(debug_stream, "Suspending sound server\n");
	  suspend_all(ss_state);
	}
      }
      ticks_to_wait += tempticks;
      
      /* the delay loop portion */
      
      if (ticks_to_wait)
	do {

	  /* Delay loop: Exit if we've waited long enough, or if we were
	     ordered to suspend. */

	  GTimeVal *wait_tvp; /* Delay parameter (see select(3)) */
	  
	  /* Calculate the time we have to sleep */
	  wakeup_time = song_next_wakeup_time(&last_played, ticks_to_wait);
	  
	  wait_tv = song_sleep_time(&last_played, ticks_to_wait);
	  wait_tvp = &wait_tv;
	  	  
	  sci_get_current_time((GTimeVal *)&ctime); /* Get current time, store in ctime */

	  /* we have to deal with latency of sleep().. so only sleep if
	     it'll be more than XXXX us. */
	  /* 5ms is arbitrary. may need to be tweaked. */
	  if ((wakeup_time.tv_usec - ctime.tv_usec) > 10000)
	    usleep(0);
	  
	  /* Exit when we've waited long enough */
	} while ((wakeup_time.tv_sec > ctime.tv_sec)
		 || ((wakeup_time.tv_sec == ctime.tv_sec)
		     && (wakeup_time.tv_usec > ctime.tv_usec)));
      
      /* The note-playing portion of the soundserver! */
      if (current && current->data) { 
	/* If we have a current song */
	int newcmd;
	guint8 param, param2 = 0;
	
#ifdef OUTPUT_SONG_CHANGES
	fprintf(stderr, "--[Handle %04x ---- pos = %04x]\n", current->handle,
		current->pos);
#endif
	newcmd = current->data[current->pos]; /* Retreive MIDI command */
	
	/* Check for running status mode */
	if (newcmd & 0x80) { 
	  ++(current->pos);
	  command = newcmd;
	} 
	
	if (command == SCI_MIDI_EOT) { /* End of Track */
#ifdef OUTPUT_SONG_CHANGES
	  fprintf(stderr,"==EOT: loops=%d, loopmark=%d\n",
		  current->loops,
		  current->loopmark);
#endif
	  /* Are we supposed to loop? */
	  if ((--(current->loops) != 0) && current->loopmark) {
#ifdef DEBUG_SOUND_SERVER
	    fprintf(debug_stream, "Looping back from %d to %d on handle %04x\n",
		    current->pos, current->loopmark, current->handle);
#endif
	    current->pos = current->loopmark;
	    global_sound_server->queue_event(current->handle, 
					     SOUND_SIGNAL_LOOP, 
					     current->loops);
	    
	  } else {
	    /* Handle playback finished */
#ifdef DEBUG_SOUND_SERVER
	    fprintf(debug_stream, "Finishing handle %04d\n", current->handle);
#endif
	    current->resetflag = 1;	/* reset song position */
	    stop_handle((word)current->handle, ss_state);
	    global_sound_server->queue_event(current->handle, 
					     SOUND_SIGNAL_LOOP, -1);
	    ticks_to_wait = 1; /* Wait one tick, then continue with next song */
#ifdef OUTPUT_SONG_CHANGES
	    {
	      song_t *debug_song = song_lib_find_active(ss_state->songlib, NULL);
					  
	      if (debug_song)
		fprintf(stderr, "NEW song: %04x-%04x-%04x\n",
			debug_song->handle,
			debug_song->loops,
			debug_song->status);
	      else
		fprintf(stderr, "NO new song!\n");
	    }
#endif
	    /* find a new song to play now that the old one ran out */
	    ss_state->current_song = ss_state->songlib[0];
	    ss_state->current_song = song_lib_find_active(ss_state->songlib, ss_state->current_song);

	  } /* playback done; no looping. */
	  
	} else {
	  /* Song running normally */
	  param = current->data[current->pos];

	  if (MIDI_cmdlen[command >> 4] == 2)
	    param2 = current->data[current->pos + 1]; /* If the MIDI instruction
										    ** takes two parameters, read
										    ** second parameter  */
	  else
	    param2 = 0; /* Waste processor cycles otherwise */

	  current->pos += MIDI_cmdlen[command >> 4];

	  /* reverse the pan if necessary */
	  if (reverse_stereo
	      && ((command & MIDI_CONTROL_CHANGE) == MIDI_CONTROL_CHANGE)
	      && (param == MIDI_CONTROLLER_PAN_COARSE))
	    param2 = (unsigned char)(0x7f - param2);

#ifdef DEBUG_SOUND_SERVER
	  if ((command & 0xf0) == MIDI_INSTRUMENT_CHANGE) /* Change instrument */
	    channel_instrument[command & 0xf] = param;
#endif

	  /* Play the event unless it's a NOTEON and the channel is muted. */
	  if (!((command & 0xf0) == MIDI_NOTE_ON && ss_state->mute_channel[command & 0xf]))
	    {
#ifdef DEBUG_SOUND_SERVER
	      /* fprintf(stdout, "MIDI (pss): 0x%02x\t%u\t%u\n", command, param, param2); */
#endif
	      sci_midi_command(debug_stream,
			       current,
			       (unsigned char)command,
			       param,
			       param2,
			       ticks_to_wait,
			       &ss_state->sound_cue,
			       ss_state->master_volume);
	    }
	}

	/* Are we fading out? */
	if (current && (current->fading >= 0)) {
	  long fadeticks = ticks_to_wait;
	  ticks_to_wait = !!ticks_to_wait; /* Ticks left? */
	  current->fading -= fadeticks; /* OK, decrease! */

	  if (current->fading < 0)
	    current->fading = 0; /* Faded out after the ticks have run out */
	}

      } /* If current && current->data */

    } /* while we're still alive */

  child = NULL;
  current = NULL;
  /* implicitly quit */
}

#endif  /* HAVE_SDL */
