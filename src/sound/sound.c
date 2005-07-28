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

#include <sci_memory.h>
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
#include <win32/messages.h>
#endif /* _WIN32 */
#endif /* !HAVE_UNISTD_H */

#if defined(HAVE_SYSV_IPC) && !defined(ARM_WINCE)
extern sound_server_t sound_server_unix;
#endif

#ifdef HAVE_SDL
extern sound_server_t sound_server_sdl;
#endif

#if defined(_WIN32) || defined(ARM_WINCE)
extern sound_server_t sound_server_win32p;
extern sound_server_t sound_server_win32e;
#endif

#ifdef _DOS
extern sound_server_t sound_server_dos;
#endif

#ifdef _DREAMCAST
extern sound_server_t sound_server_dc;
#endif

#ifdef _GP32
extern sound_server_t sound_server_pthread;
#endif

sound_server_t *sound_servers[] = {
#ifndef NO_SOUND
#  if defined (HAVE_SYSV_IPC) && !defined(ARM_WINCE)
	/* Assume that sound_null works on any box that has fork() */
	&sound_server_unix,
#  endif /* HAVE_FORK */

#  ifdef _WIN32
	&sound_server_win32p,
	&sound_server_win32e,
#  endif

#  ifdef ARM_WINCE
	&sound_server_win32p,
#  endif

#  ifdef HAVE_SDL
	&sound_server_sdl,
#  endif

#  ifdef _DOS
	&sound_server_dos,
#  endif

#  ifdef _DREAMCAST
	&sound_server_dc,
#  endif

#  ifdef _GP32
	&sound_server_pthread,
#  endif
#endif /* NO_SOUND */
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
sound_suspend_default(state_t *s)
{
	global_sound_server->queue_command(0, SOUND_COMMAND_SUSPEND_ALL, 0);
}

void
sound_resume_default(state_t *s)
{
	global_sound_server->queue_command(0, SOUND_COMMAND_RESUME_ALL, 0);
}

int
sound_command_default(state_t *s, unsigned int command, unsigned int handle, long value)
{
	sound_event_t event;
	event.handle = handle;
	event.signal = command;
	event.value = value;

	switch (command)
	{
	case SOUND_COMMAND_INIT_HANDLE: {
		resource_t *song = scir_find_resource(
			s->resmgr, sci_sound, event.value, 0);
		int len;

		if (!song) {
			sciprintf("Attempt to play invalid sound.%03d\n", event.value);
			return 1;
		}

		if (s->version < SCI_VERSION_RESUME_SUSPENDED_SONG) {
		  global_sound_server->queue_command(event.handle, SOUND_COMMAND_STOP_ALL, event.value);
		}

		len = song->size;

		if (!(global_sound_server->flags & SOUNDSERVER_FLAG_PIPED))
		{
			if (global_sound_server->send_data(song->data, len) != len) {
				fprintf(debug_stream, "sound_command(): sound_send_data"
					" returned < count\n");
			}
			global_sound_server->queue_command(event.handle, event.signal, event.value);
		}
		else
		{
			global_sound_server->queue_command(event.handle, event.signal, event.value);
			if (global_sound_server->send_data(song->data, len) != len) {
				fprintf(debug_stream, "sound_command(): sound_send_data"
					" returned < count\n");
			}
		}

		return 0;
	}

	case SOUND_COMMAND_PLAY_HANDLE:
	case SOUND_COMMAND_LOOP_HANDLE:
	case SOUND_COMMAND_DISPOSE_HANDLE:
	case SOUND_COMMAND_STOP_HANDLE:
	case SOUND_COMMAND_SUSPEND_HANDLE:
	case SOUND_COMMAND_RESUME_HANDLE:
	case SOUND_COMMAND_RENICE_HANDLE:
	case SOUND_COMMAND_SHUTDOWN:
	case SOUND_COMMAND_MAPPINGS:
	case SOUND_COMMAND_SAVE_STATE:    /* These two commands are only used from special wrapper */
	case SOUND_COMMAND_RESTORE_STATE: /* functions that provide additional data. */
	case SOUND_COMMAND_SUSPEND_ALL:
	case SOUND_COMMAND_RESUME_ALL:
	case SOUND_COMMAND_STOP_ALL:
	case SOUND_COMMAND_FADE_HANDLE:
	case SOUND_COMMAND_PRINT_SONG_INFO:
	case SOUND_COMMAND_PRINT_CHANNELS:
	case SOUND_COMMAND_PRINT_MAPPING:
	case SOUND_COMMAND_IMAP_SET_INSTRUMENT:
	case SOUND_COMMAND_IMAP_SET_KEYSHIFT:
	case SOUND_COMMAND_IMAP_SET_FINETUNE:
	case SOUND_COMMAND_IMAP_SET_BENDER_RANGE:
	case SOUND_COMMAND_IMAP_SET_PERCUSSION:
	case SOUND_COMMAND_IMAP_SET_VOLUME:
	case SOUND_COMMAND_MUTE_CHANNEL:
	case SOUND_COMMAND_UNMUTE_CHANNEL:
		global_sound_server->queue_command(event.handle, event.signal, event.value);
		return 0;

	case SOUND_COMMAND_SET_VOLUME: {
		if (s->sound_mute) {  /* if we're muted, update the mute */
			s->sound_mute = event.value;
		} else {
			s->sound_volume = event.value;
			global_sound_server->queue_command(event.handle, event.signal, event.value);
		}

		if (s->sound_mute)
			return s->sound_mute;
		else
			return s->sound_volume;
	}

	case SOUND_COMMAND_GET_VOLUME: {
		if (s->sound_mute)
			return s->sound_mute;
		else
			return s->sound_volume;
	}

	case SOUND_COMMAND_SET_MUTE: {
		switch (event.value)
		{
		case SCI_MUTE_ON:
			/* sound should not be active (but store old volume) */
			s->sound_mute = s->sound_volume;
			s->sound_volume = 0;
			break;

		case SCI_MUTE_OFF:
			/* sound should be active */
			if (s->sound_mute != 0)	/* don't let volume get set to 0 */
				s->sound_volume = s->sound_mute;
			s->sound_mute = 0;
			break;

		default:
			sciprintf("Unknown mute value %d\n", event.value);
		}

		/* send volume change across the wire */
		event.signal = SOUND_COMMAND_SET_VOLUME;
		event.value = s->sound_volume;
		global_sound_server->queue_command(event.handle, event.signal, event.value);

		/* return the mute status */
	}

	/* Intentional fallthrough */
	case SOUND_COMMAND_GET_MUTE: {
	  if (s->sound_mute) {
		return SCI_MUTE_ON;
	  } else {
	      return SCI_MUTE_OFF;
	  }
	}
	break;
	case SOUND_COMMAND_TEST: {
		int *retval = NULL;
		int len = 0;
		global_sound_server->queue_command(event.handle, event.signal, event.value);
		global_sound_server->get_data((byte **)&retval, &len);
                if(retval) { /* or check if get_data has failed */
		  len = *retval;
		  free(retval);
                }
		return len; /* should be the polyphony */
	}

	default:
		sciprintf("Unknown sound command %d\n", command);
		return 1;
	}
}

GTimeVal
song_sleep_time(GTimeVal *lastslept, long ticks)
{
	GTimeVal tv;
	long timetosleep;
	long timeslept; /* Time already slept */
	timetosleep = ticks * SOUND_TICK; /* Time to sleep in us */

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
song_next_wakeup_time(GTimeVal *lastslept, long ticks)
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
        unsigned int i, j;
	song_t *retval;
	retval = (song_t*)sci_malloc(sizeof(song_t));

#ifdef SATISFY_PURIFY
	memset(retval, 0, sizeof(song_t));
#endif

	retval->data = data;
	retval->handle = handle;
	retval->priority = priority;
	retval->size = size;
	retval->next = NULL;

	retval->shared = 0;

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

	switch (data[0]) {
	case 2:
	  j = MIDI_CHANNELS -1;
	  retval->polyphony[j] = 0;
	  retval->flags[j] = 0;
	  break;
	case 0:
	  j = MIDI_CHANNELS;
	  break;
	default:
	  j = MIDI_CHANNELS;
	  printf("Unrecognized number of channels in song resource!\n");
	}

	for (i = 0; i < j; i++) {
		retval->polyphony[i] = data[1 + (i << 1)];
		retval->flags[i] = data[2 + (i << 1)];
	}

	return retval;
}


void
song_lib_add(songlib_t songlib, song_t *song)
{
	song_t *seeker = NULL;
	int pri	= song->priority;

	if (NULL == song)
	{
		sciprintf("song_lib_add(): NULL passed for song\n");
	return;
	}

	if (*songlib == NULL)
	{
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
		free(song->data);
		free(song);
	}
}


void
song_lib_init(songlib_t *songlib)
{
	*songlib = NULL;
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

	while (seeker) {
		if (seeker->handle == handle)
			break;
		seeker = seeker->next;
	}

	return seeker;
}


song_t *
song_lib_find_active(songlib_t songlib, song_t *last_played_song)
{
	song_t *seeker = *songlib;

	if (last_played_song)
		if (last_played_song->status == SOUND_STATUS_PLAYING)
			return last_played_song; /* That one was easy... */

	while (seeker) {
		if ((seeker->status == SOUND_STATUS_WAITING) ||
		    (seeker->status == SOUND_STATUS_PLAYING))
			break;
		seeker = seeker->next;
	}

	return seeker;
}

int
song_lib_remove(songlib_t songlib, word handle, resource_mgr_t *resmgr)
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

		if (goner->next) { /* Found him? */
			song_t *oldnext = goner->next;

			goner->next = goner->next->next;
			goner = oldnext;
		} else return -1; /* No. */
	}

	retval = goner->status;

	/* Don't free if we are sharing this resource. */
	if (goner->shared == 0)
		free(goner->data);
	else if (resmgr)
		scir_unlock_resource(resmgr, 
				     scir_find_resource(resmgr, sci_sound,
							goner->shared, 0),
				     sci_sound, goner->shared);

	goner->data = NULL;
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

int
song_lib_count(songlib_t songlib)
{
	song_t *seeker = *songlib;
	int retval = 0;

	while (seeker) {
		retval++;
		seeker = seeker->next;
	}

	return retval;
}


void
sound_eq_init(sound_eq_t *queue)
{
	queue->first = queue->last = NULL;
}

void
sound_eq_queue_event(sound_eq_t *queue, unsigned int handle, unsigned int signal, long value)
{
	sound_eq_node_t *node;
	sound_event_t *evt;

	evt = sci_malloc(sizeof(sound_event_t));
	node = sci_malloc(sizeof(sound_eq_node_t));

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


/* process the actual midi events */

const int MIDI_cmdlen[16] = {0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 0};
/* Taken from playmidi */

void sci_midi_command(FILE *debugstream, song_t *song, guint8 command, guint8 param,
		guint8 param2, guint32 delta, int *ccc, int master_volume)
{
	if (SCI_MIDI_CONTROLLER(command)) {
		switch (param) {

		case SCI_MIDI_CUMULATIVE_CUE:
			*ccc += param2;
			global_sound_server->queue_event(song->handle, SOUND_SIGNAL_ABSOLUTE_CUE, *ccc);
			break;
		case SCI_MIDI_RESET_ON_SUSPEND:
			song->resetflag = 1;
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
		case 0x00: /* UNKNOWN NYI (happens in SQ3 on phleebut */
		case 0x04: /* UNKNOWN NYI (happens in LSL2 gameshow) */
		case 0x46: /* UNKNOWN NYI (happens in LSL3 binoculars) */
		case 0x52: /* UNKNOWN NYI (PQ2 1.001.000 scuba dive */
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
		case 0x7b: /* all notes off */
			midi_event(command, param, param2, delta);
			break;
		default:
			fprintf(debug_stream, "Unrecognised MIDI event %02x %02x %02x for handle %04x\n", command, param, param2, song->handle);
			break;
		}

	} else if (command == SCI_MIDI_SET_SIGNAL) {
		if (param == SCI_MIDI_SET_SIGNAL_LOOP) {
			song->loopmark = song->pos;
		} else if (param <= 127) {
			global_sound_server->queue_event(song->handle, SOUND_SIGNAL_ABSOLUTE_CUE, param);
		}

	} else {
		/* just your regular midi event.. */
		if (song->flags[command & 0x0f] & midi_playflag) {
			switch (command & 0xf0) {

			case MIDI_INSTRUMENT_CHANGE:  /* program change */
				song->instruments[command & 0xf] = param;
				midi_event2(command, param, delta);
				break;

			case MIDI_NOTE_OFF:
			case MIDI_NOTE_ON:
				if (song->velocity[command & 0x0f])  /* do we ignore velocities? */
					param2 = (unsigned char)song->velocity[command & 0x0f];
				/* scale according to master volume */
				if (! midi_device->volume)
				  param2 = (guint8) ((param2 * master_volume) / 100);
				if (song->fading != -1) {           /* is the song fading? */
					if (song->maxfade == 0)
						song->maxfade = 1;

					param2 = (guint8) ((param2 * (song->fading)) / (song->maxfade));  /* scale the velocity */
					/*	  printf("fading %d %d\n", song->fading, song->maxfade);*/
				}

				/* intentional fall through */

			case MIDI_CONTROL_CHANGE:
			case MIDI_CHANNEL_PRESSURE:
			case MIDI_PITCH_BEND:
				midi_event(command, param, param2, delta);
				break;
			default:
				fprintf(debug_stream, "Unrecognised MIDI event %02x %02x %02x for handle %04x\n", command, param, param2, song->handle);
			}
		}
	}
}

void
song_lib_dump(songlib_t songlib, int line)
{
	song_t *seeker = *songlib;

	fprintf(debug_stream,"L%d:", line);
	do {
		fprintf(debug_stream,"    %p", seeker);

		if (seeker) {
			fprintf(debug_stream,"[%04x]->", seeker->handle);
			seeker = seeker->next;
		}
		fprintf(debug_stream,"\n");
	} while (seeker);
	fprintf(debug_stream,"\n");

}

extern midi_device_t midi_device_null;

int init_midi_device (state_t *s)
{
	resource_t *midi_patch;
	int success = 1;

	midi_patch = scir_find_resource(s->resmgr, sci_patch, midi_patchfile, 0);

	if (midi_patch == NULL) {
		sciprintf("[SND] Patch (%03d) could not be loaded. Initializing with defaults...\n", midi_patchfile);

		if (midi_open(NULL, 0) < 0) {
			sciprintf("[SND] The MIDI device failed to open cleanly.\n");
			success = 0;
		}

	} else if (midi_open(midi_patch->data, midi_patch->size) < 0) {
		sciprintf("[SND] The MIDI device failed to open cleanly.\n");
		success = 0;
	}

	if (!success) {
		midi_device = &midi_device_null;
		if (midi_open(NULL, 0) < 0) {
			sciprintf("[SND] MIDI NULL driver failed to initialize, aborting...\n");
			return -1;
		}
		sciprintf("[SND] Disabling sound output.\n");
	}

	s->sound_volume = 0xc;
	s->sound_mute = 0;

	return 0;
}

unsigned int get_msg_value(char *msg)
{
	if (!(strcasecmp(msg, "SOUND_COMMAND_TEST")))
		return SOUND_COMMAND_TEST;

	if (!(strcasecmp(msg, "SOUND_COMMAND_SET_VOLUME")))
		return SOUND_COMMAND_SET_VOLUME;


	fprintf(debug_stream, "ERROR: %s does not have an entry in get_msg_value(). "
		"Please add one!\n", msg);
	return 0;

}
