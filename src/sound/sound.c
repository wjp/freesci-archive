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
#include <midi_device.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else /* !HAVE_UNISTD_H */
#ifdef _WIN32
#include <io.h>
#endif /* _WIN32 */
#endif /* !HAVE_UNISTD_H */

#ifdef HAVE_FORK
extern sound_server_t sound_server_unix;
#endif

#ifdef HAVE_SDL
extern sound_server_t sound_server_sdl;
#endif

#ifdef _DOS
extern sound_server_t sound_server_dos;
#endif

sound_server_t *sound_servers[] = {
#ifdef HAVE_FORK
  /* Assume that sound_null works on any box that has fork() */
  &sound_server_unix,
#endif /* HAVE_FORK */

#ifdef HAVE_SDL
  &sound_server_sdl,
#endif

#ifdef _DOS
  &sound_server_dos,
#endif
  NULL
};

sound_server_t *
sound_server_find_driver(char *name)
{
	sound_server_t **drv = sound_servers;

	if (!name)
		return sound_servers[0];

	while (*drv) {
		if (!strcmp((*drv)->name, name))
			return *drv;
		drv++;
	}

	return NULL;
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
_sound_expect_answer(char *timeoutmessage, int def_value)
{
  int *success;
  int retval;
  int size;

  sound_get_data((byte **)&success,&size,sizeof(int));
  
  retval = *success;
  free(success);
  return retval;

}

static int
_sound_transmit_text_expect_anwer(state_t *s, char *text, int command, char *timeoutmessage)
{
	sound_command(s, command, 0, strlen(text) + 1);
	sound_send_data(text, strlen(text) +1);

	return _sound_expect_answer(timeoutmessage, 1);
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
	int errcount = 0;
	event.handle = handle;
	event.signal = command;
	event.value = parameter;

	switch (command) {
	case SOUND_COMMAND_INIT_SONG: {
		resource_t *song = findResource(sci_sound, parameter);
		int len;

		if (!song) {
			sciprintf("Attempt to play invalid sound.%03d\n", parameter);
			return 1;
		}

		len = song->length;
		sound_queue_command(event.handle, event.signal, event.value);
		sound_send_data(song->data, len);
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
	case SOUND_COMMAND_PRINT_SONGID:
	case SOUND_COMMAND_PRINT_CHANNELS:
	case SOUND_COMMAND_PRINT_MAPPING:
	case SOUND_COMMAND_INSTRMAP_SET_INSTRUMENT:
	case SOUND_COMMAND_INSTRMAP_SET_KEYSHIFT:
	case SOUND_COMMAND_INSTRMAP_SET_FINETUNE:
	case SOUND_COMMAND_INSTRMAP_SET_BENDER_RANGE:
	case SOUND_COMMAND_INSTRMAP_SET_PERCUSSION:
	case SOUND_COMMAND_INSTRMAP_SET_VOLUME:
	case SOUND_COMMAND_MUTE_CHANNEL:
	case SOUND_COMMAND_UNMUTE_CHANNEL:
		sound_queue_command(event.handle, event.signal, event.value);
		return 0;

		/* set the sound volume. */
	case SOUND_COMMAND_SET_VOLUME:
		if (s->sound_mute) {  /* if we're muted, update the mute */
			s->sound_mute = parameter;
		} else {
			s->sound_volume = parameter;
			sound_queue_command(event.handle, event.signal, event.value);
		}
		/* deliberate fallthrough */
	case SOUND_COMMAND_GET_VOLUME:
		if (s->sound_mute)
			return s->sound_mute;
		else 
			return s->sound_volume;
    
		/* set the mute status */
	case SOUND_COMMAND_SET_MUTE:
		if (parameter == 0) { 
			s->sound_mute = s->sound_volume;
			s->sound_volume = 0;
		} else { 
			if (s->sound_mute > 0)
				s->sound_volume = s->sound_mute;
			s->sound_mute = 0;
		}
		/* let's send a volume change across the wire */
		event.signal = SOUND_COMMAND_SET_VOLUME;
		event.value = s->sound_volume;
		sound_queue_command(event.handle, event.signal, event.value);
		/* deliberate fallthrough */
		/* return the mute status */
	case SOUND_COMMAND_GET_MUTE:
		if (s->sound_mute) 
			return 0;
		else
			return 1;

	case SOUND_COMMAND_TEST: {
	  int *retval = NULL;
	  int len = 0;
	  
	  sound_queue_command(event.handle, event.signal, event.value);
	  sound_get_data((byte **)&retval,&len,sizeof(int));
	  len = *retval ;
	  free(retval);
	  return len; /* should be the polyphony */
	}    

	default: sciprintf("Unknown sound command %d\n", command);
		return 1;
	}

}


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

  retval->pos = 33;
  retval->loopmark = 33; /* The first 33 bytes are header data */
  retval->fading = -1; /* Not fading */
  retval->maxfade = 1; /* placeholder */
  retval->loops = 0; /* No fancy additional loops */
  retval->status = SOUND_STATUS_STOPPED;

  retval->reverb = 0;  /* what reverb setting to use */
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
sound_eq_peek_event(sound_eq_t *queue)
{
  if (queue->last)
    return queue->last->event;

  return NULL;
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




/** Two utility functions to modify playing note lists */

static inline int
add_note_playing(playing_notes_t *playing, int note)
     /* Returns 0 or a note to suspend */
{
	int retval = 0;

	if (playing->playing >= playing->polyphony) {
		retval = playing->notes[0];
		if (playing->polyphony > 1)
			memcpy(playing->notes, &(playing->notes[1]), sizeof(byte) * (playing->polyphony - 1)); /* Yes, this sucks. */
	} else playing->playing++;

	playing->notes[playing->playing-1] = note;

	return retval;
}

static inline void
remove_note_playing(playing_notes_t *playing, int note)
{
	int i;

	for (i = 0; i < playing->playing; i++)
		if (playing->notes[i] == note) {
			if (i < playing->polyphony)
				memcpy(&(playing->notes[i]), &(playing->notes[i + 1]), (playing->polyphony - i));
			playing->playing--;
		}
}


/* process the actual midi events */

int cmdlen[16] = {0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 0};
/* Taken from playmidi */

void sci_midi_command(song_t *song, guint8 command, 
		      guint8 param, guint8 param2, 
		      int *ccc,
		      FILE *ds, playing_notes_t *playing)
{
  
  if (SCI_MIDI_CONTROLLER(command)) {
    switch (param) {

    case SCI_MIDI_CUMULATIVE_CUE:
      *ccc += param2;
      sound_queue_event(song->handle, SOUND_SIGNAL_ABSOLUTE_CUE, *ccc);
      break;
    case SCI_MIDI_RESET_ON_STOP:
      song->resetflag = param2;
      break;
    case SCI_MIDI_SET_POLYPHONY:
      song->polyphony[command & 0x0f] = param2;
      break;
    case SCI_MIDI_SET_REVERB:
      song->reverb = param2;
      midi_reverb(param2);
      break;
    case SCI_MIDI_SET_VELOCITY:
      if (!param)
	song->velocity[command & 0x0f] = 127;
      break;
    case 0x46: /* UNKNOWN NYI (happens in LSL3 binoculars) */
    case 0x61: /* UNKNOWN NYI (special for adlib? Iceman) */
    case 0x73: /* UNKNOWN NYI (happens in Hoyle) */
    case 0xd1: /* UNKNOWN NYI (happens in KQ4 when riding the unicorn) */
      break;
    case 0x01: /* modulation */
    case 0x07: /* volume */
    case 0x0a: /* panpot */
    case 0x0b: /* expression */
    case 0x40: /* hold */
    case 0x79: /* reset all */
      midi_event(command, param, param2);
      break; 
    default:
      fprintf(ds, "Unrecognised MIDI event %02x %02x %02x for handle %04x\n", command, param, param2, song->handle);
      break;
    }

  } else if (command == SCI_MIDI_SET_SIGNAL) {

    if (param == SCI_MIDI_SET_SIGNAL_LOOP) {
      song->loopmark = song->pos;
    } else if (param <= 127) {
      sound_queue_event(song->handle, SOUND_SIGNAL_ABSOLUTE_CUE, param);
    }

  } else {  
    /* just your regular midi event.. */

    if (song->flags[command & 0x0f] & midi_playflag) { 
      switch (command & 0xf0) {

      case 0xc0:  /* program change */
	song->instruments[command & 0xf] = param;
	midi_event2(command, param);
	break;
      case 0x80:  /* note on */
      case 0x90:  /* note off */

	      if (1 || (command & 0xf != RHYTHM_CHANNEL)) {
		      if ((command & 0x90) == 0x80) {
			      int retval;
			      /* Register notes when playing: */

			      retval = add_note_playing(playing, param);

			      if (retval) { /* If we exceeded our polyphony */
				      midi_event(command | 0x10, retval, 0);
			      }
		      } else {
			      /* Unregister notes when muting: */
			      midi_event(command & 0x80, param, param2);
			      remove_note_playing(playing, param);
		      }
	      }

	if (song->velocity[command & 0x0f])  /* do we ignore velocities? */
	  param2 = song->velocity[command & 0x0f];

	if (song->fading != -1) {           /* is the song fading? */
		if (song->maxfade == 0)
			song->maxfade = 1;
			
		param2 = (param2 * (song->fading)) / (song->maxfade);  /* scale the velocity */
		/*	  printf("fading %d %d\n", song->fading, song->maxfade);*/
		
	}

      case 0xb0:  /* program control */
      case 0xd0:  /* channel pressure */
      case 0xe0:  /* pitch bend */
	midi_event(command, param, param2);
	break;
      default:
	fprintf(ds, "Unrecognised MIDI event %02x %02x %02x for handle %04x\n", command, param, param2, song->handle);
      }
    }
  }  
}

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

int init_midi_device (state_t *s) {
	resource_t *midi_patch;

	midi_patch = findResource(9,midi_patchfile);
  
	if (midi_patch == NULL) {
		sciprintf(" Patch (%03d) could not be loaded. Initializing with defaults...\n", midi_patchfile);
    
		if (midi_open(NULL, -1) < 0) {
			sciprintf(" The MIDI device failed to open cleanly.\n");
			return -1;
		}
    
	} else if (midi_open(midi_patch->data, midi_patch->length) < 0) {
		sciprintf(" The MIDI device failed to open cleanly.\n");
		return -1;
	}
  
	s->sound_volume = 0xc;
	s->sound_mute = 0;

	return 0;
}







