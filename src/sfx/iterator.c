/***************************************************************************
 iterator.c Copyright (C) 2001 Christoph Reichenbach


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

    Christoph Reichenbach (CR) <jameson@linuxgames.com>

***************************************************************************/
/* Song iterators */

#include <stdio.h>
#include <sfx_iterator.h>
#include <resource.h>
#include <sci_memory.h>

#define SCI_SONG_ITERATOR_TYPE_SCI0 0
#define SCI_SONG_ITERATOR_TYPE_SCI1 1

#define SIPFX __FILE__" : "


static const int MIDI_cmdlen[16] = {0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 0};

#define PLAYMASK_NONE 0xffff

#ifndef HAVE_MEMCHR
static void *
memchr(void *_data, int c, int n)
{
	unsigned char *data = (unsigned char *) _data;

	while (n && !(*data == c)) {
		++data;
		--n;
	}

	if (n)
		return data;
	else
		return NULL;
}
#endif


static void
_common_init(base_song_iterator_t *self)
{
	memset(self->instruments, 0, sizeof(int) * MIDI_CHANNELS);
	memset(self->velocity, 0, sizeof(int) * MIDI_CHANNELS);
	memset(self->pressure, 0, sizeof(int) * MIDI_CHANNELS);
	memset(self->pitch, 0, sizeof(int) * MIDI_CHANNELS);
	memset(self->channel_map, 1, sizeof(int) * MIDI_CHANNELS);
	memset(self->reverb, 1, sizeof(int) * MIDI_CHANNELS);

	self->resetflag = 0;
	self->loops = 0;
}


/************************************/
/*-- SCI0 iterator implementation --*/
/************************************/

#define SCI0_MIDI_OFFSET 33
#define SCI0_END_OF_SONG 0xfc /* proprietary MIDI command */

#define SCI0_PCM_SAMPLE_RATE_OFFSET 0x0e
#define SCI0_PCM_SIZE_OFFSET 0x20
#define SCI0_PCM_DATA_OFFSET 0x2c

#define CHECK_FOR_END(offset_augment) \
	if (self->offset + (offset_augment) > self->size) { \
		self->state = SI_STATE_FINISHED; \
		fprintf(stderr, SIPFX "Reached end of song without terminator!\n"); \
                return SI_FINISHED; \
	}

static int
_sci0_read_next_command(base_song_iterator_t *self, unsigned char *buf, int *result)
{
	CHECK_FOR_END(0);

	switch (self->state) {

	case SI_STATE_UNINITIALIZED:
		fprintf(stderr, SIPFX "Attempt to read command from uninitialized iterator!\n");
		self->init((song_iterator_t *) self);
		return self->next((song_iterator_t *) self, buf, result);
		return 0;

	case SI_STATE_FINISHED:
		return SI_FINISHED;

	case SI_STATE_DELTA_TIME: {
		int ticks = 0;
		int tempticks;

		do {
			tempticks = self->data[self->offset++];
			ticks += (tempticks == SCI_MIDI_TIME_EXPANSION_PREFIX)?
				SCI_MIDI_TIME_EXPANSION_LENGTH : tempticks;
		} while (tempticks == SCI_MIDI_TIME_EXPANSION_PREFIX
			 && self->offset < self->size);

		CHECK_FOR_END(0);

		self->state = SI_STATE_COMMAND;

		if (ticks)
			return ticks;
	}

	case SI_STATE_COMMAND: {
		unsigned char cmd;
		int paramsleft;
		int midi_op;
		int midi_channel;

		self->state = SI_STATE_DELTA_TIME;

		cmd = self->data[self->offset++];

		if (!(cmd & 0x80)) {
			/* 'Running status' mode */
			cmd = self->last_cmd;
			self->offset--;
		}

		midi_op = cmd >> 4;
		midi_channel = cmd & 0xf;
		paramsleft = MIDI_cmdlen[midi_op];
#if 0
		fprintf(stderr, "[IT]: off=%d, cmd=%02x, takes %d args ",
			self->offset - 1, cmd, paramsleft);
		fprintf(stderr, "[%02x %02x <%02x> %02x %02x %02x]\n",
			self->data[self->offset-3],
			self->data[self->offset-2],
			self->data[self->offset-1],
			self->data[self->offset],
			self->data[self->offset+1],
			self->data[self->offset+2]);
#endif

		buf[0] = cmd;


		CHECK_FOR_END(paramsleft);
		memcpy(buf + 1, self->data + self->offset, paramsleft);
		*result = 1 + paramsleft;

		self->offset += paramsleft;

		self->last_cmd = cmd;

		/* Are we supposed to play this channel? */
		if (
		    /* First, exclude "global" properties-- such as cues-- from consideration */
		    (midi_op < 0xf
		     && !(cmd == SCI_MIDI_SET_SIGNAL)
		     && !(SCI_MIDI_CONTROLLER(cmd)
			  && buf[1] == SCI_MIDI_CUMULATIVE_CUE))

		    /* Next, check if the channel is allowed */
		    && (self->playmask == PLAYMASK_NONE /* Any channels at all? */

			/* Rhythm channel? Abort if it's explicitly disabled */
			|| ((midi_channel == MIDI_RHYTHM_CHANNEL) && !self->play_rhythm)
			/* Not rhythm channel? Abort if it's filtered out by the playmask */
			|| ((midi_channel != MIDI_RHYTHM_CHANNEL)
			    && !(self->data[2 + (midi_channel << 1)] & self->playmask))))
			return /* Execute next command */
				_sci0_read_next_command(self, buf,
							result);

		if (cmd == SCI_MIDI_EOT) {
			/* End of track? */
			if (self->loops) {
				*result = --self->loops;
				self->offset = self->loop_offset;
				self->last_cmd = 0;
				self->state = SI_STATE_COMMAND;
				return SI_LOOP;
			} else {
				self->state = SI_STATE_FINISHED;
				return SI_FINISHED;
			}

		} else if (cmd == SCI_MIDI_SET_SIGNAL) {
			if (buf[1] == SCI_MIDI_SET_SIGNAL_LOOP) {
				self->loop_offset = self->offset - 1 - paramsleft;
				return /* Execute next command */
					_sci0_read_next_command(self, buf,
								result);
			} else {
				/* Used to be conditional <= 127 */
				*result = buf[1]; /* Absolute cue */
				return SI_CUE;
			}
		} else if (SCI_MIDI_CONTROLLER(cmd)) {
			switch (buf[1]) {

			case SCI_MIDI_CUMULATIVE_CUE:
				self->ccc += buf[2];
				*result = self->ccc;
				return SI_CUE;

			case SCI_MIDI_RESET_ON_SUSPEND:
				self->resetflag = buf[2];
				break;

			case SCI_MIDI_SET_POLYPHONY:
				self->polyphony[midi_channel] = buf[2];
				break;

			case SCI_MIDI_SET_REVERB:
				self->reverb[midi_channel] = buf[2];
				break;

			case SCI_MIDI_SET_VELOCITY:
				self->velocity[midi_channel] = buf[2];
				break;

			case 0x04: /* UNKNOWN NYI (happens in LSL2 gameshow) */
			case 0x46: /* UNKNOWN NYI (happens in LSL3 binoculars) */
			case 0x61: /* UNKNOWN NYI (special for adlib? Iceman) */
			case 0x73: /* UNKNOWN NYI (happens in Hoyle) */
			case 0xd1: /* UNKNOWN NYI (happens in KQ4 when riding the unicorn) */

				return /* Execute next command */
					_sci0_read_next_command(self, buf,
								result);

			case 0x01: /* modulation */
			case 0x07: /* volume */
			case 0x0a: /* panpot */
			case 0x0b: /* expression */
			case 0x40: /* hold */
			case 0x79: /* reset all */
				/* No special treatment neccessary */
				break;

			}
			return 0;

		} else
			/* Process as normal MIDI operation */
			return 0;
	}

	default:
		fprintf(stderr, SIPFX "Invalid iterator state %d!\n", self->state);
		return SI_FINISHED;
	}
}


static inline int
_sci0_header_magic_p(unsigned char *data, int offset, int size)
{
	if (offset + 0x10 > size)
		return 0;
	return
		(data[offset] == 0x1a)
		&& (data[offset + 1] == 0x00)
		&& (data[offset + 2] == 0x01)
		&& (data[offset + 3] == 0x00);
}

static unsigned char *
_sci0_check_pcm(base_song_iterator_t *self, int *size, int *sample_rate, int *type)
{
	unsigned int offset;

	if (self->data[0] != 2)
		return NULL;
	/* No such luck */

	offset = (self->data[1 + 15*2] << 8) | self->data[1 + 15*2 + 1];
	/* Big-endian encoding */

	if (!offset) {
		int tries = 2;
		int found_it = 0;
		offset = SCI0_MIDI_OFFSET;

		while (tries-- && offset < self->size && !found_it) {
			/* Search through the garbage manually */
			unsigned char *fc = memchr(self->data + offset, SCI0_END_OF_SONG,
					  self->size - offset);

			if (!fc) {
				fprintf(stderr, SIPFX "Warning: Playing unterminated song!\n");
				return NULL;
			}

			offset = fc - self->data + 1;

			if (_sci0_header_magic_p(self->data, offset, self->size))
				found_it = 1;
		}

		if (!found_it) {
			fprintf(stderr, SIPFX "Warning: Song indicates presence of PCM, but"
				" none found (finally at offset %04x)\n", offset);

			return NULL;
		}
	} else {
		if (!_sci0_header_magic_p(self->data, offset, self->size)) {
			fprintf(stderr, SIPFX "Warning: Song indicated presence of a PCM"
				" but did not contain a PCM header!\n");
			return NULL;
		}
	}

	*size = getInt16(self->data + offset + SCI0_PCM_SIZE_OFFSET);
	*sample_rate = getInt16(self->data + offset + SCI0_PCM_SAMPLE_RATE_OFFSET);

	if (offset + SCI0_PCM_DATA_OFFSET + *size != self->size) {
		int d = offset + SCI0_PCM_DATA_OFFSET + *size - self->size;

		fprintf(stderr, SIPFX "Warning: PCM advertizes %d bytes of data, but %d"
			  " bytes are trailing in the resource!\n",
			  *size, self->size - (offset + SCI0_PCM_DATA_OFFSET));

		if (d > 0)
			*size -= d; /* Fix this */
	}

	return self->data + SCI0_PCM_DATA_OFFSET;
}

static song_iterator_t *
_base_handle_message(base_song_iterator_t *self, song_iterator_message_t msg)
{
	if (msg.recipient == _SIMSG_BASE) {
		switch (msg.type) {

		case _SIMSG_BASEMSG_SET_LOOPS:
			self->loops = msg.args[0];
			break;

		case _SIMSG_BASEMSG_CLONE: {
			int tsize = sizeof(base_song_iterator_t) + self->size;
			base_song_iterator_t *mem = sci_malloc(tsize);
			memcpy(mem, self, tsize);
			mem->data = sci_malloc(mem->size);
			memcpy(mem->data, self->data, self->size);
			return (struct _song_iterator *) mem; /* Assume caller has another copy of this */
		}

		case _SIMSG_BASEMSG_SET_PLAYMASK:
			self->playmask = msg.args[0];
			break;

		case _SIMSG_BASEMSG_SET_RHYTHM:
			self->play_rhythm = msg.args[0];
			break;

		default:
			return NULL;
		}

		return (song_iterator_t *)self;
	}
	return NULL;
}

static void
_sci0_init(base_song_iterator_t *self)
{
	_common_init(self);

	self->offset = self->loop_offset = SCI0_MIDI_OFFSET;
	self->state = SI_STATE_DELTA_TIME;
	self->ccc = 127; /* Reset cumulative cue counter */
	self->playmask = PLAYMASK_NONE; /* All channels */
	self->play_rhythm = 0; /* Disable rhythm channel */
}

static void
_sci0_cleanup(base_song_iterator_t *self)
{
	if (self->data)
		sci_free(self->data);
	self->data = NULL;
}

/*************************************/
/*-- General purpose functionality --*/
/*************************************/


song_iterator_t *
songit_new(unsigned char *data, unsigned int size, int type)
{
	base_song_iterator_t *it = sci_malloc(sizeof(base_song_iterator_t));
	int i;

	it->delegate = NULL;

	it->data = sci_malloc(size);
	memcpy(it->data, data, size);
	it->size = size;

	if (!data || size < 33) {
		sci_free(it);
		fprintf(stderr, SIPFX "Attempt to instantiate song iterator for null"
			  " song data\n");
		return NULL;
	}


	switch (type) {

	case SCI_SONG_ITERATOR_TYPE_SCI0:
		/**-- Playing SCI0 sound resources --**/
		for (i = 0; i < MIDI_CHANNELS; i++) {
			it->polyphony[i] = data[1 + (i << 1)];
			it->flags[i] = data[2 + (i << 1)];
		}
		it->next = (int(*)(song_iterator_t *, unsigned char *, int *))
			_sci0_read_next_command;
		it->get_pcm = (unsigned char*(*)(song_iterator_t *, int*, int *, int *))
			_sci0_check_pcm;
		it->handle_message = (song_iterator_t *(*)(song_iterator_t *, song_iterator_message_t))
				      _base_handle_message;
		it->init = (void(*)(song_iterator_t *))_sci0_init;
		it->cleanup = (void(*)(song_iterator_t *))_sci0_cleanup;
		break;

	default:
		/**-- Invalid/unsupported sound resources --**/
		sci_free(it);
		fprintf(stderr, SIPFX "Attempt to instantiate invalid/unknown"
			" song iterator type %d\n", type);
		return NULL;
	}

	it->state = SI_STATE_UNINITIALIZED;

	return (song_iterator_t *) it;
}

void
songit_free(song_iterator_t *it)
{
	if (it) {
		it->cleanup(it);
		sci_free(it);
	}
}

song_iterator_message_t
songit_make_message(int recipient, int type, int a1, int a2)
{
	song_iterator_message_t rv;
	rv.recipient = recipient;
	rv.type = type;
	rv.args[0] = a1;
	rv.args[1] = a2;

	return rv;
}


int 
songit_handle_message(song_iterator_t **it_reg_p, song_iterator_message_t msg)
{
	song_iterator_t *it = *it_reg_p;
	song_iterator_t *newit;

	if (it->delegate && songit_handle_message(&(it->delegate), msg))
		return 1; /* Handled recursively */

	/* No one else could handle it: */
	newit = it->handle_message(it, msg);

	if (!newit)
		return 0; /* Couldn't handle */

	*it_reg_p = newit; /* Might have self-morphed */
	return 1;
}

song_iterator_t *
songit_clone(song_iterator_t *it)
{
	SIMSG_SEND(it, SIMSG_CLONE);
	return it;
}

