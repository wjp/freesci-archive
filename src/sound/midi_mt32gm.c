/***************************************************************************
 midi_mt32gm.c Copyright (C) 2001 Solomon Peachy

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

#include <midi_device.h>
#include <midiout.h>
#include <sound.h>
/* we reuse the mt32 open/close/etc */

int midi_mt32_event(guint8 command, guint8 param, guint8 param2, guint32 delta);
int midi_mt32_event2(guint8 command, guint8 param, guint32 delta);
int midi_mt32_allstop(void);
int midi_mt32_write_block(guint8 *data, unsigned int count);

static int mt32gm_channel_map[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define CHAN(x) MIDI_mapping[mt32gm_channel_map[x]]

/* gm mapping of mt-32 */
int midi_mt32gm_open(guint8 *data_ptr, unsigned int data_length)
{
	if (midiout_open() < 0)
	{
		fprintf(stderr, "midi_mt32gm_open(): midiout_open failed\n");
		return -1;
	}
	return midi_mt32_allstop();
}

int midi_mt32gm_close(void)
{
	return midiout_close();
}

void
midi_mt32gm_print_all_instruments(FILE *file)
{
	int i;
	for (i = 0; i < MIDI_mappings_nr; i++)
		midi_mt32gm_print_instrument(file, i);
}

void
midi_mt32gm_print_instrument(FILE *file, int index)
{
	MIDI_map_t *map = MIDI_mapping + index;

	if (NULL == file)
	{
		fprintf(stderr, "midi_mt32gm_print_instrument(): NULL passed for file\n");
		return;
	}

	if (index < 0 || index >= MIDI_mappings_nr) {
		fprintf(file, "Instr #%d: <invalid>\n", index);
		return;
	}

	fprintf(file, "Instr #%d -> ", index);

	if (map->gm_instr == NOMAP)
		fprintf(file, "[mute]\n");

	else if (map->gm_instr == RHYTHM) {
		char *perc_name = GM_Percussion_Names[map->gm_rhythmkey];

		fprintf(file, "GM percussion %d vol:%d\n",
			map->gm_rhythmkey, map->volume);

		if (map->gm_rhythmkey < 80 && map->gm_rhythmkey > 0 && perc_name)
			fprintf(file, "    (%s)\n", perc_name);
		else
			fprintf(file, "    (<invalid>)\n");

	} else {
		char *instr_name = GM_Instrument_Names[map->gm_instr];

		fprintf(file, "GM %d shift:%d ft:%d bend:%d vol:%d\n",
			map->gm_instr, map->keyshift, map->finetune,
			map->bender_range, map->volume);
		fprintf(file, "    (%s)\n", instr_name);
	}
}

int midi_mt32gm_event(guint8 command, guint8 param, guint8 param2, guint32 delta)
{
	guint8 channel;
	guint8 oper;
	unsigned long volume;
	int xparam = param;

	channel = command & 0x0f;
	oper = command & 0xf0;

	switch (oper) {
	case 0x90:
	case 0x80:  /* noteon and noteoff */

		if (CHAN(channel).gm_instr == RHYTHM) {
/*			xparam = CHAN(channel).gm_rhythmkey;*/
			channel = RHYTHM_CHANNEL;
		} else if (CHAN(channel).gm_instr == NOMAP)
			return 0;

		volume = param2;
		param2 = (volume * MIDI_mapping[param].volume) >> (7);

		if (channel == RHYTHM_CHANNEL)
			xparam = MIDI_mapping[param].gm_rhythmkey;

		break;
	case 0xd0:    /* aftertouch.  let it through */
	case 0xe0:    /* Pitch bend -- needs scaling? */
	        break;
	case 0xb0:    /* CC changes.  let 'em through... */
#if 0
	        if (param == 0x0a)  /* mt32 and gm pan reversed */
	                param2 = 0xf7 - param2;
#endif
	        break;
	default:
		printf("MT32GM: Unknown event: %02x\n", command);

	}

	if (xparam < 0)
		return 0;

	return midi_mt32_event(oper | channel, xparam, param2, delta);
}

int midi_mt32gm_event2(guint8 command, guint8 param, guint32 delta)
{
	guint8 channel;
	guint8 oper;
	int xparam = param;

	channel = command & 0x0f;
	oper = command & 0xf0;
	switch (oper) {
	case 0xc0: {  /* change instrument */
		int instr = param;

		mt32gm_channel_map[channel] = param;

		if (channel == RHYTHM_CHANNEL)
			xparam = MIDI_mapping[param].gm_rhythmkey;
		else {
			xparam = MIDI_mapping[param].gm_instr;

			if (channel != RHYTHM_CHANNEL && xparam >= 0) {
				if (midi_mt32_event(0xb0 | channel,
						    0x65, 0x00, delta) < 0) return -1;
				if (midi_mt32_event(0xb0 | channel,
						    0x64, 0x02, delta) < 0) return -1;
				if (midi_mt32_event(0xb0 | channel,
						    0x06, MIDI_mapping[instr].keyshift, delta) < 0) return -1;
				if (midi_mt32_event(0xb0 | channel,
						    0x65, 0x00, delta) < 0) return -1;
				if (midi_mt32_event(0xb0 | channel,
						    0x64, 0x00, delta) < 0) return -1;
				if (midi_mt32_event(0xb0 | channel,
						    0x06, MIDI_mapping[instr].bender_range, delta) < 0) return -1;
				if (midi_mt32_event(0xb0 | channel,
						    0x26, 0x00, delta) < 0) return -1;
			}
		}

		if (xparam < 0)
			return 0;

		return midi_mt32_event2(command, xparam, delta);
	}

	default:
		printf("MT32GM: Unknown event: %02x\n", command);
	}

	return 0;
}

int midi_mt32gm_reverb(short param)
{
  printf("MT32GM: Reverb control not supported\n");
  return 0;
}

/* device struct */

midi_device_t midi_device_mt32gm = {
  "mt32gm",
  "v0.01",
  &midi_mt32gm_open,
  &midi_mt32gm_close,
  &midi_mt32gm_event,
  &midi_mt32gm_event2,
  &midi_mt32_allstop,
  NULL,
  &midi_mt32gm_reverb,
  001,		/* patch.001 */
  0x01,		/* playflag */
  1, 		/* play channel 9 */
  32		/* Max polyphony */
};
