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

#if defined HAVE_SYSV_IPC
extern sound_server_t sound_server_unix;
#endif

#ifdef HAVE_SDL
extern sound_server_t sound_server_sdl;
#endif

#ifdef _WIN32
extern sound_server_t sound_server_win32p;
extern sound_server_t sound_server_win32e_qp;
extern sound_server_t sound_server_win32e;
#endif

#ifdef _DOS
extern sound_server_t sound_server_dos;
#endif

sound_server_t *sound_servers[] = {
#ifndef NO_SOUND
#  ifdef HAVE_SYSV_IPC
	/* Assume that sound_null works on any box that has fork() */
	&sound_server_unix,
#  endif /* HAVE_FORK */

#  ifdef _WIN32
	&sound_server_win32p,
	&sound_server_win32e_qp,
	&sound_server_win32e,
#  endif

#  ifdef HAVE_SDL
	&sound_server_sdl,
#  endif

#  ifdef _DOS
	&sound_server_dos,
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
sound_command_default(state_t *s, unsigned int command, unsigned int handle, long parameter)
{
	sound_event_t event;
	event.handle = handle;
	event.signal = command;
	event.value = parameter;

	if (command == 0) {
		fprintf(stderr, "WARNING: A sound command has not been defined\n");
		return 1;

	} else if (command == SOUND_COMMAND_INIT_HANDLE) {
		resource_t *song = scir_find_resource(
			s->resmgr, sci_sound, parameter, 0);
		int len;

		if (!song) {
			sciprintf("Attempt to play invalid sound.%03d\n", parameter);
			return 1;
		}

		len = song->size;
		if (global_sound_server->send_data(song->data, len) != len) {
			fprintf(debug_stream, "sound_command(): sound_send_data"
				" returned < count\n");
		}
		global_sound_server->queue_command(event.handle, event.signal, event.value);

		return 0;

	} else if ( (command == SOUND_COMMAND_PLAY_HANDLE)    ||
	            (command == SOUND_COMMAND_LOOP_HANDLE)    ||
	            (command == SOUND_COMMAND_DISPOSE_HANDLE) ||
	            (command == SOUND_COMMAND_STOP_HANDLE)    ||
				(command == SOUND_COMMAND_SUSPEND_HANDLE) ||
	            (command == SOUND_COMMAND_RESUME_HANDLE)  ||
	            (command == SOUND_COMMAND_RENICE_HANDLE)  ||
	            (command == SOUND_COMMAND_SHUTDOWN)       ||
	            (command == SOUND_COMMAND_MAPPINGS)       ||
	            (command == SOUND_COMMAND_SAVE_STATE)     || /* These two commands are only used from special wrapper */
	            (command == SOUND_COMMAND_RESTORE_STATE)  || /* functions that provide additional data. */
	            (command == SOUND_COMMAND_SUSPEND_ALL)    ||
	            (command == SOUND_COMMAND_RESUME_ALL)     ||
	            (command == SOUND_COMMAND_STOP_ALL)       ||
	            (command == SOUND_COMMAND_FADE_HANDLE)    ||
	            (command == SOUND_COMMAND_PRINT_SONG_INFO)||
	            (command == SOUND_COMMAND_PRINT_CHANNELS) ||
	            (command == SOUND_COMMAND_PRINT_MAPPING)  ||
	            (command == SOUND_COMMAND_IMAP_SET_INSTRUMENT)   ||
	            (command == SOUND_COMMAND_IMAP_SET_KEYSHIFT)     ||
	            (command == SOUND_COMMAND_IMAP_SET_FINETUNE)     ||
	            (command == SOUND_COMMAND_IMAP_SET_BENDER_RANGE) ||
	            (command == SOUND_COMMAND_IMAP_SET_PERCUSSION)   ||
	            (command == SOUND_COMMAND_IMAP_SET_VOLUME)       ||
	            (command == SOUND_COMMAND_MUTE_CHANNEL)   ||
				(command == SOUND_COMMAND_UNMUTE_CHANNEL) ) {
		global_sound_server->queue_command(event.handle, event.signal, event.value);
		return 0;

	} else if (command == SOUND_COMMAND_SET_VOLUME) {
		if (s->sound_mute) {  /* if we're muted, update the mute */
			s->sound_mute = parameter;
		} else {
			s->sound_volume = parameter;
			global_sound_server->queue_command(event.handle, event.signal, event.value);
		}

		if (s->sound_mute)
			return s->sound_mute;
		else
			return s->sound_volume;

	} else if (command == SOUND_COMMAND_GET_VOLUME) {
		if (s->sound_mute)
			return s->sound_mute;
		else
			return s->sound_volume;

	} else if (command == SOUND_COMMAND_SET_MUTE) {
		if (parameter == 0) {
			s->sound_mute = s->sound_volume;
			s->sound_volume = 0;
		} else {
			if (s->sound_mute > 0)
				s->sound_volume = s->sound_mute;
			s->sound_mute = MUTE_OFF;
		}

		/* let's send a volume change across the wire */
		event.signal = SOUND_COMMAND_SET_VOLUME;
		event.value = s->sound_volume;
		global_sound_server->queue_command(event.handle, event.signal, event.value);

		/* return the mute status */
		if (s->sound_mute)
			return MUTE_ON;
		else
			return MUTE_OFF;

	} else if (command == SOUND_COMMAND_GET_MUTE) {
		if (s->sound_mute)
			return MUTE_ON;
		else
			return MUTE_OFF;

	} else if (command == SOUND_COMMAND_TEST) {
		int *retval = NULL;
		int len = 0;
		global_sound_server->queue_command(event.handle, event.signal, event.value);
		global_sound_server->get_data((byte **)&retval, &len);
		len = *retval;
		free(retval);
		return len; /* should be the polyphony */

	} else {
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
	song_t *retval = sci_malloc(sizeof(song_t));
	unsigned int i;

#ifdef SATISFY_PURIFY
	memset(retval, 0, sizeof(song_t));
#endif

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
	song_t *seeker	= NULL;
	int pri			= song->priority;

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

	playing->notes[playing->playing-1] = (unsigned char)note;

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

int MIDI_cmdlen[16] = {0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 0};
/* Taken from playmidi */

void sci_midi_command(FILE *debugstream, song_t *song, guint8 command, guint8 param,
		guint8 param2, int *ccc, playing_notes_t *playing)
{
	if (SCI_MIDI_CONTROLLER(command)) {
		switch (param) {

		case SCI_MIDI_CUMULATIVE_CUE:
			*ccc += param2;
			global_sound_server->queue_event(song->handle, SOUND_SIGNAL_ABSOLUTE_CUE, *ccc);
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
		case 0x04: /* UNKNOWN NYI (happens in LSL2 gameshow) */
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
							midi_event((guint8)(command | 0x10), (guint8)retval, 0);
						}
					} else {
						/* Unregister notes when muting: */
						midi_event((guint8)(command & 0x80), param, param2);
						remove_note_playing(playing, param);
					}
				}

				if (song->velocity[command & 0x0f])  /* do we ignore velocities? */
					param2 = (unsigned char)song->velocity[command & 0x0f];

				if (song->fading != -1) {           /* is the song fading? */
					if (song->maxfade == 0)
						song->maxfade = 1;

					param2 = (guint8) ((param2 * (song->fading)) / (song->maxfade));  /* scale the velocity */
					/*	  printf("fading %d %d\n", song->fading, song->maxfade);*/

				}
				/* intentional fall through */

			case 0xb0:  /* program control */
			case 0xd0:  /* channel pressure */
			case 0xe0:  /* pitch bend */
				midi_event(command, param, param2);
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

void print_message_map() {
	/* used to debug what sound commands and signals are being issued */
	printf("Command                                Value\n");
	printf("SOUND_DATA                             %u\n", SOUND_DATA);
	printf("SOUND_COMMAND_INIT_HANDLE              %u\n", SOUND_COMMAND_INIT_HANDLE);
	printf("SOUND_COMMAND_PLAY_HANDLE              %u\n", SOUND_COMMAND_PLAY_HANDLE);
	printf("SOUND_COMMAND_LOOP_HANDLE              %u\n", SOUND_COMMAND_LOOP_HANDLE);
	printf("SOUND_COMMAND_DISPOSE_HANDLE           %u\n", SOUND_COMMAND_DISPOSE_HANDLE);
	printf("SOUND_COMMAND_SET_MUTE                 %u\n", SOUND_COMMAND_SET_MUTE);
	printf("SOUND_COMMAND_STOP_HANDLE              %u\n", SOUND_COMMAND_STOP_HANDLE);
	printf("SOUND_COMMAND_SUSPEND_HANDLE           %u\n", SOUND_COMMAND_SUSPEND_HANDLE);
	printf("SOUND_COMMAND_RESUME_HANDLE            %u\n", SOUND_COMMAND_RESUME_HANDLE);
	printf("SOUND_COMMAND_SET_VOLUME               %u\n", SOUND_COMMAND_SET_VOLUME);
	printf("SOUND_COMMAND_RENICE_HANDLE            %u\n", SOUND_COMMAND_RENICE_HANDLE);
	printf("SOUND_COMMAND_FADE_HANDLE              %u\n", SOUND_COMMAND_FADE_HANDLE);
	printf("SOUND_COMMAND_TEST                     %u\n", SOUND_COMMAND_TEST);
	printf("SOUND_COMMAND_STOP_ALL                 %u\n", SOUND_COMMAND_STOP_ALL);
	printf("SOUND_COMMAND_SAVE_STATE               %u\n", SOUND_COMMAND_SAVE_STATE);
	printf("SOUND_COMMAND_RESTORE_STATE            %u\n", SOUND_COMMAND_RESTORE_STATE);
	printf("SOUND_COMMAND_SUSPEND_ALL              %u\n", SOUND_COMMAND_SUSPEND_ALL);
	printf("SOUND_COMMAND_RESUME_ALL               %u\n", SOUND_COMMAND_RESUME_ALL);
	printf("SOUND_COMMAND_GET_MUTE                 %u\n", SOUND_COMMAND_GET_MUTE);
	printf("SOUND_COMMAND_GET_VOLUME               %u\n", SOUND_COMMAND_GET_VOLUME);
	printf("SOUND_COMMAND_PRINT_SONG_INFO          %u\n", SOUND_COMMAND_PRINT_SONG_INFO);
	printf("SOUND_COMMAND_PRINT_CHANNELS           %u\n", SOUND_COMMAND_PRINT_CHANNELS);
	printf("SOUND_COMMAND_PRINT_MAPPING            %u\n", SOUND_COMMAND_PRINT_MAPPING);
	printf("SOUND_COMMAND_IMAP_SET_INSTRUMENT      %u\n", SOUND_COMMAND_IMAP_SET_INSTRUMENT);
	printf("SOUND_COMMAND_IMAP_SET_KEYSHIFT        %u\n", SOUND_COMMAND_IMAP_SET_KEYSHIFT);
	printf("SOUND_COMMAND_IMAP_SET_FINETUNE        %u\n", SOUND_COMMAND_IMAP_SET_FINETUNE);
	printf("SOUND_COMMAND_IMAP_SET_BENDER_RANGE    %u\n", SOUND_COMMAND_IMAP_SET_BENDER_RANGE);
	printf("SOUND_COMMAND_IMAP_SET_PERCUSSION      %u\n", SOUND_COMMAND_IMAP_SET_PERCUSSION);
	printf("SOUND_COMMAND_IMAP_SET_VOLUME          %u\n", SOUND_COMMAND_IMAP_SET_VOLUME);
	printf("SOUND_COMMAND_MUTE_CHANNEL             %u\n", SOUND_COMMAND_MUTE_CHANNEL);
	printf("SOUND_COMMAND_UNMUTE_CHANNEL           %u\n", SOUND_COMMAND_UNMUTE_CHANNEL);
	printf("SOUND_COMMAND_SHUTDOWN                 %u\n", SOUND_COMMAND_SHUTDOWN);
	printf("SOUND_SIGNAL_CUMULATIVE_CUE            %u\n", SOUND_SIGNAL_CUMULATIVE_CUE);
	printf("SOUND_SIGNAL_LOOP                      %u\n", SOUND_SIGNAL_LOOP);
	printf("SOUND_SIGNAL_FINISHED                  %u\n", SOUND_SIGNAL_FINISHED);
	printf("SOUND_SIGNAL_PLAYING                   %u\n", SOUND_SIGNAL_PLAYING);
	printf("SOUND_SIGNAL_PAUSED                    %u\n", SOUND_SIGNAL_PAUSED);
	printf("SOUND_SIGNAL_RESUMED                   %u\n", SOUND_SIGNAL_RESUMED);
	printf("SOUND_SIGNAL_INITIALIZED               %u\n", SOUND_SIGNAL_INITIALIZED);
	printf("SOUND_SIGNAL_ABSOLUTE_CUE              %u\n", SOUND_SIGNAL_ABSOLUTE_CUE);
}

void register_sound_messages() {
	/* NOTE: should all be defined > 0 for error checking purposes */
#ifndef _WIN32
	SOUND_DATA                          = 1;
	SOUND_COMMAND_INIT_HANDLE           = 2;
	SOUND_COMMAND_PLAY_HANDLE           = 3;
	SOUND_COMMAND_LOOP_HANDLE           = 4;
	SOUND_COMMAND_DISPOSE_HANDLE        = 5;
	SOUND_COMMAND_SET_MUTE              = 6;
	SOUND_COMMAND_GET_MUTE              = 32;
	SOUND_COMMAND_STOP_HANDLE           = 7;
	SOUND_COMMAND_SUSPEND_HANDLE        = 8;
	SOUND_COMMAND_RESUME_HANDLE         = 9;
	SOUND_COMMAND_SET_VOLUME            = 10;
	SOUND_COMMAND_RENICE_HANDLE         = 11;
	SOUND_COMMAND_FADE_HANDLE           = 12;
	SOUND_COMMAND_TEST                  = 13;
	SOUND_COMMAND_STOP_ALL              = 14;
	SOUND_COMMAND_SHUTDOWN              = 15;
	SOUND_COMMAND_SAVE_STATE            = 16;
	SOUND_COMMAND_RESTORE_STATE         = 17;
	SOUND_COMMAND_SUSPEND_ALL           = 18;
	SOUND_COMMAND_RESUME_ALL            = 19;
	SOUND_COMMAND_GET_VOLUME            = 20;
	SOUND_COMMAND_PRINT_SONG_INFO       = 21;
	SOUND_COMMAND_PRINT_CHANNELS        = 22;
	SOUND_COMMAND_PRINT_MAPPING         = 23;
	SOUND_COMMAND_IMAP_SET_INSTRUMENT   = 24;
	SOUND_COMMAND_IMAP_SET_KEYSHIFT     = 25;
	SOUND_COMMAND_IMAP_SET_FINETUNE     = 26;
	SOUND_COMMAND_IMAP_SET_BENDER_RANGE = 27;
	SOUND_COMMAND_IMAP_SET_PERCUSSION   = 28;
	SOUND_COMMAND_IMAP_SET_VOLUME       = 29;
	SOUND_COMMAND_MUTE_CHANNEL          = 30;
	SOUND_COMMAND_UNMUTE_CHANNEL        = 31;
	SOUND_SIGNAL_CUMULATIVE_CUE         = 1001;
	SOUND_SIGNAL_LOOP                   = 1002;
	SOUND_SIGNAL_FINISHED               = 1003;
	SOUND_SIGNAL_PLAYING                = 1004;
	SOUND_SIGNAL_PAUSED                 = 1005;
	SOUND_SIGNAL_RESUMED                = 1006;
	SOUND_SIGNAL_INITIALIZED            = 1007;
	SOUND_SIGNAL_ABSOLUTE_CUE           = 1008;
#else
	DECLARE_MESSAGES();
#endif

#ifdef _DEBUG
	/* print_message_map(); */
#endif
}

int init_midi_device (state_t *s) {
	resource_t *midi_patch;

	midi_patch = scir_find_resource(s->resmgr, sci_patch, midi_patchfile, 0);

	if (midi_patch == NULL) {
		sciprintf(" Patch (%03d) could not be loaded. Initializing with defaults...\n", midi_patchfile);

		if (midi_open(NULL, (unsigned int)-1) < 0) {
			sciprintf(" The MIDI device failed to open cleanly.\n");
			return -1;
		}

	} else if (midi_open(midi_patch->data, midi_patch->size) < 0) {
		sciprintf(" The MIDI device failed to open cleanly.\n");
		return -1;
	}

	s->sound_volume = 0xc;
	s->sound_mute = MUTE_OFF;

	return 0;
}

unsigned int get_msg_value(char *msg)
{
	if (!(strcasecmp(msg, "SOUND_COMMAND_TEST"))) {
		return SOUND_COMMAND_TEST;

	} else {
		fprintf(stderr, "ERROR: %s does not have an entry in get_msg_value(). "
			"Please add one!\n", msg);
		return 0;
	}
}
