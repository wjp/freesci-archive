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
#include "sci_midi.h"
#include "list.h"

#define FREQUENCY 44100
#define CHANNELS_NR 4

#define BASE_FREQ 20000
#define MODE_SUSTAIN 1 << 0
#define MODE_MODULATE 1 << 1

#define PAN_LEFT 91
#define PAN_RIGHT 164

#define DEBUG

static int db;

typedef struct sample {
	int type;
	char name[30];
	int mode;
	int size;
	int attack_size;
	int sustain_pos;
	int sustain_size;
	int pitch;
	signed char *data; /* Sample data */
} sample_t;

typedef struct bank {
	char name[31];
	int size;
	sample_t *samples[256];
} bank_t;

typedef struct channel_state {
	int instrument;
	int note;
	int velocity;
	int looping;
	int pan;
	frac_t offset;
	frac_t rate;
} channel_state_t;

typedef struct channel_settings {
	int instrument;
	int volume;
} channel_settings_t;

static bank_t bank;
static channel_state_t ch_state[CHANNELS_NR];
static channel_settings_t ch_settings[CHANNELS_NR];

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

static void
play_sample(gint16 *dest, channel_state_t *channel, int count)
{
	int index = 0;
	sample_t *sample = bank.samples[channel->instrument];

	while (1) {
		/* Available source samples until looping/end point */
		frac_t lin_avail;
		int rem = count - index / 2;
		int i;

		/* Amount of destination samples that we will compute this iteration */
		int amount;

		if (sample->mode & MODE_SUSTAIN) {
			if (channel->looping)
				lin_avail = int_to_frac(sample->sustain_pos + sample->sustain_size) - channel->offset;
			else
				lin_avail = int_to_frac(sample->attack_size) - channel->offset;
		} else
			lin_avail = int_to_frac(sample->size) - channel->offset;

		amount = lin_avail / channel->rate;

		if (lin_avail % channel->rate)
			amount++;

		if (amount > rem)
			amount = rem;

		for (i = 0; i < amount; i++) {
			int new_sample = sample->data[frac_to_int(channel->offset)] * channel->velocity / 127;
			dest[index++] += new_sample * (255 - channel->pan) >> 8;
			dest[index++] += new_sample * channel->pan >> 8;
			channel->offset += channel->rate;
		}

		if (index / 2 == count)
			break;

		if (sample->mode & MODE_SUSTAIN) {
			if (channel->looping && frac_to_int(channel->offset) >= sample->sustain_pos + sample->sustain_size)
				channel->offset -= int_to_frac(sample->sustain_size);
			else if (!channel->looping && frac_to_int(channel->offset) >= sample->attack_size) {
				channel->looping = 1;
				channel->offset -= int_to_frac(sample->attack_size);
				channel->offset += int_to_frac(sample->sustain_pos);
			}
		} else if (frac_to_int(channel->offset) >= sample->size)
			break;
	}
}

static void
change_instrument(int channel, int instrument)
{
#ifdef DEBUG
	if (bank.samples[instrument])
		sciprintf("[sfx:seq:amiga] Setting channel %i to \"%s\" (%i)\n", channel, bank.samples[instrument]->name, instrument);
	else
		sciprintf("[sfx:seq:amiga] Warning: instrument %i does not exist (channel %i)\n", instrument, channel);
#endif
	ch_settings[channel].instrument = instrument;
}

static void
start_note(int channel, int note, int velocity)
{
	sample_t *sample;

	if (ch_settings[channel].instrument < 0 || ch_settings[channel].instrument > 255) {
		sciprintf("[sfx:seq:amiga] Error: invalid instrument %i\n", ch_settings[channel].instrument);
		return;
	}

	sample = bank.samples[ch_settings[channel].instrument];

	if (!sample) {
		sciprintf("[sfx:seq:amiga] Error: instrument %i does not exist\n", ch_settings[channel].instrument);
		return;
	}

	if (sample->mode & MODE_MODULATE) {
		int fnote = note + sample->pitch;

		if (fnote < 0 || fnote > 127) {
			sciprintf("[sfx:seq:amiga] Error: illegal note %i\n", fnote);
			return;
		}

		ch_state[channel].rate = double_to_frac(freq_table[fnote] / (double) FREQUENCY);
	}
	else
		ch_state[channel].rate = double_to_frac(BASE_FREQ / (double) FREQUENCY);

	ch_state[channel].instrument = ch_settings[channel].instrument;
	ch_state[channel].note = note;
	ch_state[channel].velocity = velocity;
	ch_state[channel].offset = 0;
	ch_state[channel].looping = 0;

	if (channel == 0 || channel == 3)
		ch_state[channel].pan = PAN_LEFT;
	else
		ch_state[channel].pan = PAN_RIGHT;
}

static void
stop_note(int channel, int note)
{
	if (ch_state[channel].note != note) {
#ifdef DEBUG
		sciprintf("[sfx:seq:amiga] Warning: cannot stop note %i on channel %i\n", note, channel);
#endif
		return;
	}

	if (!(bank.samples[ch_state[channel].instrument]->mode & MODE_SUSTAIN))
		return;

	ch_state[channel].note = -1;
}

static int read_int16(unsigned char *data)
{
	int val = data[0] * 256 + data[1];

	if (val >= 32768)
		return val - 65536;

	return val;
}

static sample_t *read_sample(int file, int *id)
{
	sample_t *sample;
	unsigned char header[61];
	int size_rem;

	if (read(file, header, 61) < 61) {
		sciprintf("[sfx:seq:amiga] Error: failed to read sample header\n");
		return NULL;
	}

	sample = sci_malloc(sizeof(sample_t));

	sample->attack_size = read_int16(header + 35) * 2;
	sample->sustain_size = read_int16(header + 41) * 2;
	size_rem = read_int16(header + 47) * 2;

	sample->mode = header[33];
	sample->pitch = header[34];
	sample->sustain_pos = read_int16(header + 39);
	sample->size = sample->attack_size + sample->sustain_size + size_rem;

	*id = read_int16(header);

	strncpy(sample->name, (char *) header + 2, 29);
	sample->name[29] = 0;
	sciprintf("[sfx:seq:amiga] Reading sample %i: \"%s\" (%i bytes)\n",
		*id, sample->name, sample->size);

	sample->data = sci_malloc(sample->size);
	if (read(file, sample->data, sample->size) < sample->size) {
		sciprintf("[sfx:seq:amiga] Error: failed to read sample data\n");
		return NULL;
	}

	if (sample->sustain_pos + sample->sustain_size > sample->size) {
		sciprintf("[sfx:seq:amiga] Warning: looping data extends %i bytes past end of sample block\n",
			*id, sample->sustain_size + sample->sustain_pos - sample->size);
		
	}

	return sample;
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
	unsigned char header[40];
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
		bank.samples[i] = NULL;

	for (i = 0; i < CHANNELS_NR; i++) {
		ch_settings[i].instrument = -1;
		ch_settings[i].volume = 127;
		ch_state[i].note = -1;
	}

	bank.size = read_int16(header + 38);
	strncpy(bank.name, (char *) header + 8, 30);
	bank.name[30] = 0;
	sciprintf("[sfx:seq:amiga] Reading %i samples from bank \"%s\"\n", bank.size, bank.name);

	for (i = 0; i < bank.size; i++) {
		int id;
		sample_t *sample = read_sample(file, &id);

		if (!sample) {
			sciprintf("[sfx:seq:amiga] Error: failed to read bank.001\n");
			close(file);
			return SFX_ERROR;
		}

		if (id < 0 || id > 255) {
			sciprintf("[sfx:seq:amiga] Error: instrument ID out of bounds\n");
			return SFX_ERROR;
		}

		bank.samples[id] = sample;
	}

	close(file);

	return SFX_OK;
}

static void
ami_exit(sfx_softseq_t *self)
{
}

static void
ami_event(sfx_softseq_t *self, byte command, int argc, byte *argv)
{
	int channel, oper;

	channel = command & 0x0f;
	oper = command & 0xf0;

	if (channel >= CHANNELS_NR) {
		sciprintf("[sfx:seq:amiga] Warning: received event for non-existing channel %i\n", channel);
		return;
	}

	switch(oper) {
	case 0x90:
		if (argv[1] > 0)
			start_note(channel, argv[0], argv[1]);
		else
			stop_note(channel, argv[0]);
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
	int i;
	guint16 *buf = (guint16 *) dest;

	memset(dest, 0, len * 4);

	for (i = 0; i < CHANNELS_NR; i++)
		if (ch_state[i].note >= 0)
			play_sample(buf, &ch_state[i], len);

	for (i = 0; i < len * 2; i++)
		buf[i] <<= 7;
}

void
ami_volume(sfx_softseq_t *self, int new_volume)
{
}

void
ami_allstop(sfx_softseq_t *self)
{
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
	CHANNELS_NR, /* # of voices */
	{FREQUENCY, SFX_PCM_STEREO_LR, SFX_PCM_FORMAT_S16_NATIVE}
};
