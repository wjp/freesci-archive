/***************************************************************************
  Copyright (C) 2001 Christoph Reichenbach


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

#include <soundserver.h>

#define SCI_SONG_ITERATOR_TYPE_SCI0 0
#define SCI_SONG_ITERATOR_TYPE_SCI1 1

#define SIPFX __FILE__" : "


#ifndef HAVE_MEMCHR
static void *
memchr(void *_data, int c, int n)
{
	byte *data = (byte *) _data;

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
_common_init(song_iterator_t *self)
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
	if (self->offset + (offset_augment) >= self->size) { \
		self->state = SI_STATE_FINISHED; \
		fprintf(stderr, SIPFX "Reached end of song without terminator!\n"); \
                return SI_FINISHED; \
	}

static int
_sci0_read_next_command(song_iterator_t *self, byte *buf, int *buf_size)
{
	CHECK_FOR_END(0);

	switch (self->state) {

	case SI_STATE_UNINITIALIZED:
		fprintf(stderr, SIPFX "Attempt to read command from uninitialized iterator!\n");
		self->init(self);
		return self->read_next_command(self, buf, buf_size);
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

		if (self->offset == self->size) {
			self->state = SI_STATE_FINISHED;
			fprintf(stderr, SIPFX "Reached end of song without terminator!\n");
		}

		CHECK_FOR_END(0);

		self->state = SI_STATE_COMMAND;

		if (ticks)
			return ticks;
	}

	case SI_STATE_COMMAND: {
		byte cmd;
		int paramcount = 0;
		int paramsleft;
		int midi_op;
		int midi_channel;

		self->state = SI_STATE_DELTA_TIME;

		cmd = self->data[self->offset++];

		if (!(cmd & 0x80)) {
			/* 'Running status' mode */
			buf[paramcount++] = cmd;
			cmd = self->last_cmd;
		}

		midi_op = cmd >> 4;
		midi_channel = cmd & 0xf;
		paramsleft = MIDI_cmdlen[midi_op];

		if (paramcount)
			--paramsleft;

		CHECK_FOR_END(paramsleft);
		memcpy(buf + paramcount, self->data, paramsleft);
		self->offset += paramsleft;

		if (cmd == SCI_MIDI_EOT) {
			/* End of track? */
			if (self->loops) {
				--self->loops;
				self->offset = self->loop_offset;
				return SI_LOOP;
			} else {
				self->state = SI_STATE_FINISHED;
				return SI_FINISHED;
			}

		} else if ((cmd = SCI_MIDI_SET_SIGNAL)) {
			if (buf[1] == SCI_MIDI_SET_SIGNAL_LOOP) {
				self->loop_offset = self->offset;
				return /* Execute next command */
					_sci0_read_next_command(self, buf,
								buf_size);
			} else {
				/* Used to be conditional <= 127 */
				buf[0] = buf[1]; /* Absolute cue */
				return SI_CUE;
			}
		} else if (SCI_MIDI_CONTROLLER(cmd)) {
			switch (buf[1]) {

			case SCI_MIDI_CUMULATIVE_CUE:
				self->ccc += buf[2];
				buf[0] = self->ccc;
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
								buf_size);

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
_sci0_header_magic_p(byte *data, int offset, int size)
{
fprintf(stderr,"Determining header magic at %04x/%04x: %02x %02x %02x %02x\n",
offset, size, data[offset],
data[offset + 1], data[offset + 2], data[offset + 3]);

	if (offset + 0x10 > size)
		return 0;
	return
		(data[offset] == 0x1a)
		&& (data[offset + 1] == 0x00)
		&& (data[offset + 2] == 0x01)
		&& (data[offset + 3] == 0x00);
}

static byte *
_sci0_check_pcm(song_iterator_t *self, int *size, int *sample_rate)
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
			byte *fc = memchr(self->data + offset, SCI0_END_OF_SONG,
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
fprintf(stderr,"Found PCM at offset %04x\n", offset);
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


static void
_sci0_init(song_iterator_t *self)
{
	_common_init(self);

	self->offset = self->loop_offset = SCI0_MIDI_OFFSET;
	self->state = SI_STATE_DELTA_TIME;
	self->ccc = 127; /* Reset cumulative cue counter */
}


/*************************************/
/*-- General purpose functionality --*/
/*************************************/


song_iterator_t *
songit_new(byte *data, unsigned int size, int type)
{
	song_iterator_t *it = sci_malloc(sizeof(song_iterator_t));
	int i;

	it->data = data;
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
		it->read_next_command = _sci0_read_next_command;
		it->check_pcm = _sci0_check_pcm;
		it->init = _sci0_init;
		break;

	default:
		/**-- Invalid/unsupported sound resources --**/
		sci_free(it);
		fprintf(stderr, SIPFX "Attempt to instantiate invalid/unknown"
			" song iterator type %d\n", type);
		return NULL;
	}

	it->state = SI_STATE_UNINITIALIZED;

	return it;
}

void
songit_free(song_iterator_t *it)
{
	if (it) {
		sci_free(it->data);
		it->data = NULL;
		sci_free(it);
	}
}

void
songit_set_loops(song_iterator_t *it, int loops)
{
	it->loops = loops;
}
