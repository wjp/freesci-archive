/***************************************************************************
 amiga.c  Copyright (C) 2007 Walter van Niftrik


 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public Licence as
 published by the Free Software Foundaton; either version 2 of the
 Licence, or (at your option) any later version.

 It is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 merchantibility or fitness for a particular purpose. See the
 GNU General Public Licence for more details.

 You should have received a copy of the GNU General Public Licence
 along with this program; see the file COPYING. If not, write to
 the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.


 Please contact the maintainer for any program-related bug reports or
 inquiries.

 Current Maintainer:

    Walter van Niftrik <w.f.b.w.v.niftrik@stud.tue.nl>

***************************************************************************/

#include "resource.h"
#include "sci_memory.h"
#include "sfx_softseq.h"

#define FREQUENCY 44100
#define CHANNELS_NR 10
#define HW_CHANNELS_NR 4

/* Samplerate of the instrument bank */
#define BASE_FREQ 20000

/* Instrument looping flag */
#define MODE_LOOP 1 << 0
/* Instrument pitch changes flag */
#define MODE_PITCH 1 << 1

#define PAN_LEFT 91
#define PAN_RIGHT 164

/* #define DEBUG */

typedef struct instrument {
	char name[30];
	int mode;
	int size;
	int loop_pos;
	int transpose;
	sbyte *samples;
} instrument_t;

typedef struct bank {
	char name[31];
	int size;
	instrument_t *instruments[256];
} bank_t;

typedef struct channel {
	int instrument;
	int note;
	int velocity;
	int decay;
	int hw_channel;
	frac_t offset;
	frac_t rate;
} channel_t;

typedef struct hw_channel {
	int instrument;
	int volume;
	int pan;
} hw_channel_t;

/* Instrument bank */
static bank_t bank;
/* Internal channels */
static channel_t channels[CHANNELS_NR];
/* External channels */
static hw_channel_t hw_channels[HW_CHANNELS_NR];
/* Overall volume */
static int volume = 127;

/* Frequencies for every note */
static int freq_table[] = {
	58, 62, 65, 69, 73, 78, 82, 87,
	92, 98, 104, 110, 117, 124, 131, 139,
	147, 156, 165, 175, 185, 196, 208, 220,
	234, 248, 262, 278, 294, 312, 331, 350,
	371, 393, 417, 441, 468, 496, 525, 556,
	589, 625, 662, 701, 743, 787, 834, 883,
	936, 992, 1051, 1113, 1179, 1250, 1324, 1403,
	1486, 1574, 1668, 1767, 1872, 1984, 2102, 2227,
	2359, 2500, 2648, 2806, 2973, 3149, 3337, 3535,
	3745, 3968, 4204, 4454, 4719, 5000, 5297, 5612,
	5946, 6299, 6674, 7071, 7491, 7937, 8408, 8908,
	9438, 10000, 10594, 11224, 11892, 12599, 13348, 14142,
	14983, 15874, 16817, 17817, 18877, 20000, 21189, 22449,
	23784, 25198, 26696, 28284, 29966, 31748, 33635, 35635,
	37754, 40000, 42378, 44898, 47568, 50396, 53393, 56568,
	59932, 63496, 67271, 71271, 75509, 80000, 84757, 89796
};

static inline int
interpolate(char *samples, frac_t offset)
{
	int x = frac_to_int(offset);
	int diff = (samples[x + 1] - samples[x]) << 8;

	return (samples[x] << 8) + frac_to_int(diff * (offset & FRAC_LO_MASK));
}

static void
play_instrument(gint16 *dest, channel_t *channel, int count)
{
	int index = 0;
	int vol = hw_channels[channel->hw_channel].volume;
	instrument_t *instrument = bank.instruments[channel->instrument];

	while (1) {
		/* Available source samples until wrapping/end point */
		frac_t lin_avail = int_to_frac(instrument->size) - channel->offset;
		int rem = count - index;
		int i;

		/* Amount of destination samples that we will compute this iteration */
		int amount = lin_avail / channel->rate;

		if (lin_avail % channel->rate)
			amount++;

		if (amount > rem)
			amount = rem;

		for (i = 0; i < amount; i++) {
			dest[index++] = interpolate(instrument->samples, channel->offset) * channel->velocity * vol / (127 * 127);
			channel->offset += channel->rate;

			/* Decay quickly */
			if (channel->decay && channel->velocity > 0)
				channel->velocity--;
		}

		if (index == count) {
			/* Stop note after velocity has dropped to 0 */
			if (channel->decay && channel->velocity == 0)
				channel->note = -1;
			break;
		}

		if (frac_to_int(channel->offset) >= instrument->size) {
			if (instrument->mode & MODE_LOOP)
				/* Loop the samples */
				channel->offset -= int_to_frac(instrument->size - instrument->loop_pos);
			else {
				/* All samples have been played */
				channel->note = -1;
				break;
			}
		}
	}
}

static void
change_instrument(int channel, int instrument)
{
#ifdef DEBUG
	if (bank.instruments[instrument])
		sciprintf("[sfx:seq:amiga] Setting channel %i to \"%s\" (%i)\n", channel, bank.instruments[instrument]->name, instrument);
	else
		sciprintf("[sfx:seq:amiga] Warning: instrument %i does not exist (channel %i)\n", instrument, channel);
#endif
	hw_channels[channel].instrument = instrument;
}

static void
stop_channel(int ch)
{
	int i;

	/* Start decay phase for note on this hw channel, if any */
	for (i = 0; i < CHANNELS_NR; i++)
		if (channels[i].note != -1 && channels[i].hw_channel == ch && !channels[i].decay) {
			channels[i].decay = 1;
			break;
		}
}

static void
stop_note(int ch, int note)
{
	int channel;

	for (channel = 0; channel < CHANNELS_NR; channel++)
		if (channels[channel].note == note && channels[channel].hw_channel == ch && !channels[channel].decay)
			break;

	if (channel == CHANNELS_NR) {
#ifdef DEBUG
		sciprintf("[sfx:seq:amiga] Warning: cannot stop note %i on channel %i\n", note, ch);
#endif
		return;
	}

	
	if (bank.instruments[channels[channel].instrument]->mode & MODE_LOOP)
		channels[channel].decay = 1;

	/* Non-looping instruments are not stopped by a note-off */
}

static void
start_note(int ch, int note, int velocity)
{
	instrument_t *instrument;
	int channel, i;

	if (hw_channels[ch].instrument < 0 || hw_channels[ch].instrument > 255) {
		sciprintf("[sfx:seq:amiga] Error: invalid instrument %i\n", hw_channels[ch].instrument);
		return;
	}

	instrument = bank.instruments[hw_channels[ch].instrument];

	if (!instrument) {
		sciprintf("[sfx:seq:amiga] Error: instrument %i does not exist\n", hw_channels[ch].instrument);
		return;
	}

	for (channel = 0; channel < CHANNELS_NR; channel++)
		if (channels[channel].note == -1)
			break;

	if (channel == CHANNELS_NR) {
		sciprintf("[sfx:seq:amiga] Warning: could not find a free channel\n");
		return;
	}

	stop_channel(ch);

	if (instrument->mode & MODE_PITCH) {
		int fnote = note + instrument->transpose;

		if (fnote < 0 || fnote > 127) {
			sciprintf("[sfx:seq:amiga] Error: illegal note %i\n", fnote);
			return;
		}

		/* Compute rate for note */
		channels[channel].rate = double_to_frac(freq_table[fnote] / (double) FREQUENCY);
	}
	else
		channels[channel].rate = double_to_frac(BASE_FREQ / (double) FREQUENCY);

	channels[channel].instrument = hw_channels[ch].instrument;
	channels[channel].note = note;
	channels[channel].velocity = velocity;
	channels[channel].offset = 0;
	channels[channel].hw_channel = ch;
	channels[channel].decay = 0;
}

static int read_int16(byte *data)
{
	int val = data[0] * 256 + data[1];

	if (val >= 32768)
		return val - 65536;

	return val;
}

static instrument_t *read_instrument(int file, int *id)
{
	instrument_t *instrument;
	byte header[61];
	int size_rem;
	int size;
	sbyte *samples;
	int seg_size[3];

	if (read(file, header, 61) < 61) {
		sciprintf("[sfx:seq:amiga] Error: failed to read instrument header\n");
		return NULL;
	}

	instrument = (instrument_t *) sci_malloc(sizeof(instrument_t));

	seg_size[0] = read_int16(header + 35) * 2;
	seg_size[1] = read_int16(header + 41) * 2;
	seg_size[2] = read_int16(header + 47) * 2;

	instrument->mode = header[33];
	instrument->transpose = header[34];
	instrument->loop_pos = read_int16(header + 39);
	size = seg_size[0] + seg_size[1] + seg_size[2];

	*id = read_int16(header);

	strncpy(instrument->name, (char *) header + 2, 29);
	instrument->name[29] = 0;
#ifdef DEBUG
	sciprintf("[sfx:seq:amiga] Reading instrument %i: \"%s\" (%i bytes)\n",
		*id, instrument->name, size);
#endif
	/* Extra byte for interpolation code. */
	samples = (sbyte *) sci_malloc(size + 1);
	if (read(file, samples, size) < size) {
		sciprintf("[sfx:seq:amiga] Error: failed to read instrument samples\n");
		return NULL;
	}

	if (instrument->mode & MODE_LOOP) {
		if (instrument->loop_pos + seg_size[1] > size) {
#ifdef DEBUG
			sciprintf("[sfx:seq:amiga] Warning: looping samples extends %i bytes past end of sample block\n",
				instrument->loop_pos + seg_size[1] - size);
#endif
			seg_size[1] = size - instrument->loop_pos;
		}

		if (seg_size[1] < 0) {
			sciprintf("[sfx:seq:amiga] Error: invalid looping point\n");
			return NULL;
		}

		instrument->size = seg_size[0] + seg_size[1];
		instrument->samples = (sbyte *) sci_malloc(instrument->size + 1);
		memcpy(instrument->samples, samples, seg_size[0]);
		memcpy(instrument->samples + seg_size[0], samples + instrument->loop_pos, seg_size[1]);
		/* Copy first sample to make interpolation easier */
		instrument->samples[instrument->size] = instrument->samples[seg_size[0]];

		sci_free(samples);
	} else {
		instrument->size = size;
		instrument->samples = samples;
		instrument->samples[size] = 0;
	}

	return instrument;
}

static int
ami_set_option(sfx_softseq_t *self, char *name, char *value)
{
	return SFX_ERROR;
}

static int
ami_init(sfx_softseq_t *self, byte *patch, int patch_len)
{
	int file;
	byte header[40];
	int i;

	file = sci_open("bank.001", O_RDONLY);

	if (file == SCI_INVALID_FD) {
		sciprintf("[sfx:seq:amiga] Error: file bank.001 not found\n");
		return SFX_ERROR;
	}

	if (read(file, header, 40) < 40) {
		sciprintf("[sfx:seq:amiga] Error: failed to read header of file bank.001\n");
		close(file);
		return SFX_ERROR;
	}

	for (i = 0; i < 256; i++)
		bank.instruments[i] = NULL;

	for (i = 0; i < CHANNELS_NR; i++) {
		hw_channels[i].instrument = -1;
		hw_channels[i].volume = 127;
		channels[i].note = -1;
	}

	hw_channels[0].pan = PAN_LEFT;
	hw_channels[1].pan = PAN_RIGHT;
	hw_channels[2].pan = PAN_RIGHT;
	hw_channels[3].pan = PAN_LEFT;

	bank.size = read_int16(header + 38);
	strncpy(bank.name, (char *) header + 8, 30);
	bank.name[30] = 0;
#ifdef DEBUG
	sciprintf("[sfx:seq:amiga] Reading %i instruments from bank \"%s\"\n", bank.size, bank.name);
#endif

	for (i = 0; i < bank.size; i++) {
		int id;
		instrument_t *instrument = read_instrument(file, &id);

		if (!instrument) {
			sciprintf("[sfx:seq:amiga] Error: failed to read bank.001\n");
			close(file);
			return SFX_ERROR;
		}

		if (id < 0 || id > 255) {
			sciprintf("[sfx:seq:amiga] Error: instrument ID out of bounds\n");
			return SFX_ERROR;
		}

		bank.instruments[id] = instrument;
	}

	close(file);

	return SFX_OK;
}

static void
ami_exit(sfx_softseq_t *self)
{
	int i;

	for (i = 0; i < bank.size; i++) {
		if (bank.instruments[i]) {
			sci_free(bank.instruments[i]->samples);
			sci_free(bank.instruments[i]);
		}
	}
}

static void
ami_event(sfx_softseq_t *self, byte command, int argc, byte *argv)
{
	int channel, oper;

	channel = command & 0x0f;
	oper = command & 0xf0;

	if (channel >= CHANNELS_NR) {
#ifdef DEBUG
		sciprintf("[sfx:seq:amiga] Warning: received event for non-existing channel %i\n", channel);
#endif
		return;
	}

	switch(oper) {
	case 0x90:
		if (argv[1] > 0)
			start_note(channel, argv[0], argv[1]);
		else
			stop_note(channel, argv[0]);
		break;
	case 0xb0:
		switch (argv[0]) {
		case 0x07:
			hw_channels[channel].volume = argv[1];
			break;
		case 0x0a:
#ifdef DEBUG
			sciprintf("[sfx:seq:amiga] Warning: ignoring pan 0x%02x event for channel %i\n", argv[1], channel);
#endif
			break;
		case 0x7b:
			stop_channel(channel);
			break;
		default:
			sciprintf("[sfx:seq:amiga] Warning: unknown control event 0x%02x\n", argv[0]);
		}
		break;
	case 0xc0:
		change_instrument(channel, argv[0]);
		break;
	default:
		sciprintf("[sfx:seq:amiga] Warning: unknown event %02x\n", command);
	}
}

void
ami_poll(sfx_softseq_t *self, byte *dest, int len)
{
	int i, j;
	gint16 *buf = (gint16 *) dest;
	gint16 *buffers = malloc(len * 2 * CHANNELS_NR);

	memset(buffers, 0, len * 2 * CHANNELS_NR);
	memset(dest, 0, len * 4);

	/* Generate samples for all notes */
	for (i = 0; i < CHANNELS_NR; i++)
		if (channels[i].note >= 0)
			play_instrument(buffers + i * len, &channels[i], len);

	for (j = 0; j < len; j++) {
		int mixedl = 0, mixedr = 0;

		/* Mix and pan */
		for (i = 0; i < CHANNELS_NR; i++) {
			mixedl += buffers[i * len + j] * (256 - hw_channels[channels[i].hw_channel].pan);
			mixedr += buffers[i * len + j] * hw_channels[channels[i].hw_channel].pan;
		}

		/* Adjust volume */
		buf[2 * j] = mixedl * volume >> 16;
		buf[2 * j + 1] = mixedr *volume >> 16;
	}
}

void
ami_volume(sfx_softseq_t *self, int new_volume)
{
	volume = new_volume;
}

void
ami_allstop(sfx_softseq_t *self)
{
	int i;
	for (i = 0; i < HW_CHANNELS_NR; i++)
		stop_channel(i);
}

sfx_softseq_t sfx_softseq_amiga = {
	"amiga",
	"0.1",
	ami_set_option,
	ami_init,
	ami_exit,
	ami_volume,
	ami_event,
	ami_poll,
	ami_allstop,
	NULL,
	SFX_SEQ_PATCHFILE_NONE,
	0x40,
	0, /* No rhythm channel */
	HW_CHANNELS_NR, /* # of voices */
	{FREQUENCY, SFX_PCM_STEREO_LR, SFX_PCM_FORMAT_S16_NATIVE}
};
