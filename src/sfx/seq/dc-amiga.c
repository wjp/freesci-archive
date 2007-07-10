/***************************************************************************
 dc-amiga.c  Copyright (C) 2007 Walter van Niftrik


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

#include <dc/sound/sound.h>
#include <dc/spu.h>
#include <dc/sound/sfxmgr.h>
#include "aica_cmd_iface.h"

#include "resource.h"
#include "sci_memory.h"
#include "sfx_sequencer.h"
#include "list.h"

#define FREQUENCY 20000
#define CHANNELS_NR 4

#define MODE_SUSTAIN 1 << 0
#define MODE_MODULATE 1 << 1

#define DEBUG

#define BASE_NOTE 101
#define BASE_FREQ 20000
#define SEMITONE 1.059463094359

/* AICA timer is running at 4410 ticks per second. */
static unsigned int delta = 0;

typedef struct sample {
	char name[30];
	int mode;
	int size;
	int attack_size;
	int sustain_pos;
	int sustain_size;
	int pitch;
	uint32 data; /* Sample data */
} sample_t;

typedef struct bank {
	char name[31];
	int size;
	sample_t *samples[256];
} bank_t;

typedef struct channel_state {
	int instrument;
	int note;
} channel_state_t;

typedef struct channel_settings {
	int instrument;
	int volume;
} channel_settings_t;

char vol_table[] = {
0, 18, 28, 36, 42, 46, 50, 54, 57, 60, 62, 65, 67, 69, 70, 72,
74, 75, 77, 78, 79, 80, 82, 83, 84, 85, 86, 87, 88, 89, 89, 90,
91, 92, 93, 93, 94, 95, 95, 96, 97, 97, 98, 99, 99, 100, 100, 101,
101, 102, 102, 103, 103, 104, 104, 105, 105, 106, 106, 107, 107, 108, 108, 108,
109, 109, 110, 110, 110, 111, 111, 111, 112, 112, 113, 113, 113, 114, 114, 114,
115, 115, 115, 115, 116, 116, 116, 117, 117, 117, 118, 118, 118, 118, 119, 119,
119, 120, 120, 120, 120, 121, 121, 121, 121, 122, 122, 122, 122, 123, 123, 123,
123, 123, 124, 124, 124, 124, 125, 125, 125, 125, 125, 126, 126, 126, 126, 127
};

static bank_t bank;
static channel_state_t ch_state[CHANNELS_NR];
static channel_settings_t ch_settings[CHANNELS_NR];
static int aica_channels[CHANNELS_NR];

static int compute_frequency(int note)
{
	double freq = BASE_FREQ;
	int diff;

	if (note == BASE_NOTE)
		return freq;

	if (note > BASE_NOTE) {
		for (diff = note - BASE_NOTE; diff > 0; diff--)
			freq *= SEMITONE;

		return freq;
	}

	for (diff = BASE_NOTE - note; diff > 0; diff--)
		freq /= SEMITONE;

	return freq;
}

static int change_instrument(int channel, int instrument)
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
	AICA_CMDSTR_CHANNEL(tmp, cmd, chan);

	if (ch_settings[channel].instrument < 0 || ch_settings[channel].instrument > 255) {
		sciprintf("[sfx:seq:amiga] Error: invalid instrument %i\n", ch_settings[channel].instrument);
		return;
	}

	sample = bank.samples[ch_settings[channel].instrument];

	if (!sample) {
		sciprintf("[sfx:seq:amiga] Error: instrument %i does not exist\n", ch_settings[channel].instrument);
		return;
	}

	cmd->cmd = AICA_CMD_CHAN;
	cmd->timestamp = delta * 147 / 2;
	cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
	cmd->cmd_id = aica_channels[channel];
	chan->cmd = AICA_CH_CMD_START;
	chan->base = sample->data;
	chan->type = AICA_SM_8BIT;
	chan->length = sample->size;

	if (sample->mode & MODE_SUSTAIN) {
		chan->loop = 1;
		chan->loopstart = sample->sustain_pos;
		chan->loopend = sample->sustain_pos + sample->sustain_size;
		if (chan->loopend > sample->size)
			chan->loopend = sample->size;
	} else {
		chan->loop = 0;
		chan->loopstart = 0;
		chan->loopend = sample->size;
	}

	if (sample->mode & MODE_MODULATE)
		chan->freq = compute_frequency(note + sample->pitch);
	else
		chan->freq = 20000;

	chan->vol = 2 * vol_table[velocity * ch_settings[channel].volume / 127];

	if (channel == 0 || channel == 3)
		chan->pan = 0;
	else
		chan->pan = 255;

	snd_sh4_to_aica(tmp, cmd->size);

	ch_state[channel].note = note;
	ch_state[channel].instrument = ch_settings[channel].instrument;
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

	AICA_CMDSTR_CHANNEL(tmp, cmd, chan);
	cmd->cmd = AICA_CMD_CHAN;
	cmd->timestamp = delta * 147 / 2;
	cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
	cmd->cmd_id = aica_channels[channel];
	chan->cmd = AICA_CH_CMD_STOP;
	chan->base = 0;
	chan->type = 0;
	chan->length = 0;
	chan->loop = 0;
	chan->loopstart = 0;
	chan->loopend = 0;
	chan->freq = 44100;
	chan->vol = 0;
	chan->pan = 0;
	snd_sh4_to_aica(tmp, cmd->size);

	ch_state[channel].note = -1;
}

static int read_int16(unsigned char *data)
{
	int val = data[0] * 256 + data[1];

	if (val >= 32768)
		return val - 65536;

	return val;
}

static int read_int8(unsigned char *data)
{
	int val = data[0];

	if (val >= 128)
		return val - 256;

	return val;
}

static sample_t *read_sample(int file, int *id)
{
	sample_t *sample;
	unsigned char header[61];
	int size_rem;
	unsigned char *data;

	if (read(file, header, 61) < 61) {
		sciprintf("[sfx:seq:amiga] Error: failed to read sample header\n");
		return NULL;
	}

	sample = sci_malloc(sizeof(sample_t));

	sample->attack_size = read_int16(header + 35) * 2;
	sample->sustain_size = read_int16(header + 41) * 2;
	size_rem = read_int16(header + 47) * 2;

	sample->mode = header[33];
	sample->pitch = read_int8(header + 34);
	sample->sustain_pos = read_int16(header + 39);
	sample->size = sample->attack_size + sample->sustain_size + size_rem;

	*id = read_int16(header);

	strncpy(sample->name, (unsigned char *) header + 2, 29);
	sample->name[29] = 0;
	sciprintf("[sfx:seq:amiga] Reading sample %i: \"%s\" (%i bytes)\n",
		*id, sample->name, sample->size);

	data = sci_malloc(sample->size);
	if (read(file, data, sample->size) < sample->size) {
		sciprintf("[sfx:seq:amiga] Error: failed to read sample data\n");
		return NULL;
	}

	sample->data = snd_mem_malloc(sample->size);

	if (!sample->data) {
		sciprintf("[sfx:seq:amiga] Error: failed to allocate sound memory\n");
		return NULL;
	}

	spu_memload(sample->data, data, sample->size);
	sci_free(data);

	if (sample->sustain_pos + sample->sustain_size > sample->size) {
		sciprintf("[sfx:seq:amiga] Warning: looping data extends %i bytes past end of sample block\n",
			*id, sample->sustain_size + sample->sustain_pos - sample->size);
	}

	return sample;
}

static int
ami_set_option(char *name, char *value)
{
	return SFX_ERROR;
}

static int
ami_init(int data_length, byte *data_ptr, void *seq)
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
	strncpy(bank.name, (unsigned char *) header + 8, 30);
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

	for (i = 0; i < CHANNELS_NR; i++) {
		aica_channels[i] = snd_sfx_chn_alloc();
		if (aica_channels[i] == -1) {
			sciprintf("[sfx:seq:amiga] Error: failed to allocate AICA channel\n");
			return SFX_ERROR;
		}
	}

	return SFX_OK;
}

static int
ami_exit()
{
	return SFX_OK;
}

static int
ami_event(byte command, int argc, byte *argv)
{
	int channel, oper;

	channel = command & 0x0f;
	oper = command & 0xf0;

	if (channel >= CHANNELS_NR) {
		sciprintf("[sfx:seq:amiga] Warning: received event for non-existing channel %i\n", channel);
		return 0;
	}

	switch (oper) {
	case 0x90:
if (channel == 0) printf("Note volume: %i\n", argv[1]);
		if (argv[1] > 0)
			start_note(channel, argv[0], argv[1]);
		else
			stop_note(channel, argv[0]);
		break;
	case 0xb0:
		switch (argv[0]) {
		case 0x07:
if (channel == 0) printf("Channel volume: %i\n", argv[1]);
			ch_settings[channel].volume = argv[1];
			break;
		case 0x0a:
			sciprintf("[sfx:seq:amiga] Warning: unsupported pan 0x%02x event received for channel %i\n", argv[1], channel);
			break;
		case 0x7b:
			stop_note(channel, ch_state[channel].note);
			break;
		default:
			sciprintf("[sfx:seq:amiga] Warning: unknown control event 0x%02x\n", argv[0]);
		}
		break;
	case 0xc0:
		change_instrument(channel, argv[0]);
		break;
	default:
		sciprintf("[sfx:seq:amiga] Warning: unknown event 0x%02x\n", command);
	}

	return 0;
}

static int
ami_delay(int ticks)
{
	printf("Delay: %i\n", ticks);
	delta += ticks;
	return SFX_OK;
}

static int
ami_reset_timer(GTimeVal ts)
{
	AICA_CMDSTR_CHANNEL(tmp, cmd, chan);
	cmd->cmd = AICA_CMD_SYNC_CLOCK;
	cmd->timestamp = 0;
	cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
	chan->cmd = AICA_CH_CMD_NONE;
	snd_sh4_to_aica(tmp, cmd->size);

	delta = 0;
	return SFX_OK;
}

static int
ami_allstop(void)
{
	int i;
	for (i = 0; i < CHANNELS_NR; i++)
		if (ch_state[i].note != -1)
			stop_note(i, ch_state[i].note);

	return SFX_OK;
}

static int
ami_reverb(int param)
{
	printf("reverb NYI %04x \n", param);
	return SFX_OK;
}

sfx_sequencer_t sfx_sequencer_dc_amiga = {
	"dc-amiga",
	"0.1",
	SFX_DEVICE_NONE,
	ami_set_option,
	ami_init,
	ami_exit,
	ami_event,
	ami_delay,
	ami_reset_timer,
	ami_allstop,
	NULL,
	ami_reverb,
	SFX_SEQ_PATCHFILE_NONE,
	0x40,
	0, /* No rhythm channel */
	CHANNELS_NR, /* # of voices */
	0
};
