/***************************************************************************
 soft-pcspeaker.c  Copyright (C) 2002 Christoph Reichenbach


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

    Christoph Reichenbach (CR) <jameson@linuxgames.com>

***************************************************************************/
/* PC speaker software sequencer for FreeSCI */

#include <sfx_sw_sequencer.h>
#include <sfx_sequencer.h>

#define FREQUENCY 94020
#define SAMPLE_BLOCK_SIZE (FREQUENCY / 60) /* Size for a block of samples comprising all needed for one tick */

static gint16 sample_block[SAMPLE_BLOCK_SIZE];

static int volume = 0x0600;
static int note = 0; /* Current halftone, or 0 if off */
static int freq_count = 0; 

static sfx_pcm_config_t pcm_conf = {
	FREQUENCY,
	SFX_PCM_MONO,
	SFX_PCM_FORMAT_S16_NATIVE
};

extern sfx_sequencer_t sfx_sequencer_sw_pcspeaker;
/* Forward-declare the sequencer we are defining here */


static int
sps_set_option(char *name, char *value)
{
	return SFX_ERROR;
}

static int
sps_open(int patch_len, byte *patch, void *device)
{
	sfx_init_sw_sequencer(&sfx_sequencer_sw_pcspeaker, 1, pcm_conf);
	fprintf(stderr, "init-Chanlast = %p\n", PCM_SW_CHANNEL(&sfx_sequencer_sw_pcspeaker, 0)->last);

	return SFX_OK;
}

static int
sps_close(void)
{
	sfx_exit_sw_sequencer(&sfx_sequencer_sw_pcspeaker);
	return SFX_OK;
}

static int
sps_event(byte command, int argc, byte *argv)
{
#if 0
	fprintf(stderr, "Note [%02x : %02x %02x]\n", command,  argc?argv[0] : 0, (argc > 1)? argv[1] : 0);
#endif

	switch (command & 0xf0) {

	case 0x80:
		if (argv[0] == note)
			note = 0;
		break;

	case 0x90:
		if (!argv[1]) {
			if (argv[0] == note)
				note = 0;
		} else
			note = argv[0];
		/* Ignore velocity otherwise; we just use the global one */
		break;

	default:
		fprintf(stderr, "[SFX:PCM-PC] Unused MIDI command %02x %02x %02x\n", command, argc?argv[0] : 0, (argc > 1)? argv[1] : 0);
		break; /* ignore */
	}

	return SFX_OK;
}

#define BASE_NOTE 129	/* A10 */
#define BASE_OCTAVE 10	/* A10, as I said */

static int
freq_table[12] = { /* A4 is 440Hz, halftone map is x |-> ** 2^(x/12) */
	28160, /* A10 */
	29834,
	31608,
	33488,
	35479,
	37589,
	39824,
	42192,
	44701,
	47359,
	50175,
	53159
};


int
sps_delay(int ticks)
{
	int halftone_delta = note - BASE_NOTE;
	int oct_diff = ((halftone_delta + BASE_OCTAVE * 12) / 12) - BASE_OCTAVE;
	int halftone_index = (halftone_delta + (12*100)) % 12 ;
	int freq = (!note)? 0 : freq_table[halftone_index] / (1 << (-oct_diff));

	while (ticks--) {
		int i;
		for (i = 0; i < SAMPLE_BLOCK_SIZE; i++) {
			if (note) {
				freq_count += freq;
				while (freq_count >= (FREQUENCY << 1))
					freq_count -= (FREQUENCY << 1);

				if (freq_count - freq < 0) {
					/* Unclean rising edge */
					int l = volume << 1;
					sample_block[i] = -volume + (l*freq_count)/freq;
				} else if (freq_count >= FREQUENCY
					   && freq_count - freq < FREQUENCY) {
					/* Unclean falling edge */
					int l = volume << 1;
					sample_block[i] = volume - (l*(freq_count - FREQUENCY))/freq;
				} else {
					if (freq_count < FREQUENCY)
						sample_block[i] = volume;
					else
						sample_block[i] = -volume;
				}
			} else
				sample_block[i] = 0;
		}

		sfx_audbuf_write(PCM_SW_CHANNEL(&sfx_sequencer_sw_pcspeaker, 0),
				 (byte *) &(sample_block[0]), SAMPLE_BLOCK_SIZE);
	}
	return SFX_OK;
}

int
sps_volume(guint8 new_volume)
{
	volume = new_volume << 7;
	return SFX_OK;
}

int
sps_reset_timer(GTimeVal ts)
{
	long secs, usecs;
	int i;

	sci_gettime(&secs, &usecs);

	sfx_sw_sequencer_start_recording(&sfx_sequencer_sw_pcspeaker,
					 ts);
	/*n	sfx_audbuf_write_timestamp(PCM_SW_CHANNEL(&sfx_sequencer_sw_pcspeaker, 0),
	  sfx_new_timestamp(secs, usecs, FREQUENCY));*/
	return SFX_OK;
}

int
sps_allstop(void)
{
	note = 0;
	return SFX_OK;
}

sfx_sequencer_t sfx_sequencer_sw_pcspeaker = {
	"sw-pcspeaker",
	"0.2",
	SFX_DEVICE_NONE,
	sps_set_option,
	sps_open,
	sps_close,
	sps_event,
	sps_delay,
	sps_reset_timer,
	sps_allstop,
	sps_volume,
	NULL,
	SFX_SEQ_PATCHFILE_NONE,
	0x20,  /* PC speaker channel only */
	0,
	1,
	0,
	NULL /* Initialised later */
};
