/***************************************************************************
 midi_mt32.c Copyright (C) 2000,2001 Rickard Lind, Solomon Peachy
 mt32.c Copyright (C) 2002 Christoph Reichenbach

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

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <sfx_sequencer.h>

#ifdef HAVE_SYS_SOUNDCARD_H

#include <string.h>
#include <sfx_engine.h>

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/soundcard.h>

static int delta = 0; /* Accumulated delta time */
static midi_writer_t *midi_writer = NULL;

static int midi_mt32_poke(guint32 address, guint8 *data, unsigned int n);
static int midi_mt32_poke_gather(guint32 address, guint8 *data1, unsigned int count1,
			  guint8 *data2, unsigned int count2);
static int midi_mt32_write_block(guint8 *data, unsigned int count);
static int midi_mt32_patch001_type(guint8 *data, unsigned int length);
static int midi_mt32_patch001_type0_length(guint8 *data, unsigned int length);
static int midi_mt32_patch001_type1_length(guint8 *data, unsigned int length);
static int midi_mt32_sysex_delay();
static int midi_mt32_volume(guint8 volume);
static int midi_mt32_reverb(int param);
static int midi_mt32_event(byte command, int argc, byte *argv);
static int midi_mt32_allstop(void);

static int type;
static guint8 sysex_buffer[266] = {0xF0, 0x41, 0x10, 0x16, 0x12};
static guint8 default_reverb;
static char shutdown_msg[20];

static gint8 patch_map[128] = {
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
	48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
	80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
	96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127};
static gint8 keyshift[128] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static gint8 volume_adjust[128] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static guint8 velocity_map_index[128] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static guint8 velocity_map[4][128] = {
	{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	 16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	 32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
	 48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	 64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
	 80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
	 96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,
	 112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127},
	{0,32,32,33,33,34,34,35,35,36,36,37,37,38,38,39,
	 39,40,40,41,41,42,42,43,43,44,44,45,45,46,46,47,
	 47,48,48,49,49,50,50,51,51,52,52,53,53,54,54,55,
	 55,56,56,57,57,58,58,59,59,60,60,61,61,62,62,63,
	 64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
	 80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
	 96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,
	 112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127},
	{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	 16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	 32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
	 48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	 64,65,66,66,67,67,68,68,69,69,70,70,71,71,72,72,
	 73,73,74,74,75,75,76,76,77,77,78,78,79,79,80,80,
	 81,81,82,82,83,83,84,84,85,85,86,86,87,87,88,88,
	 89,89,90,90,91,91,92,92,93,93,94,94,95,95,96,96},
	{0,32,32,33,33,34,34,35,35,36,36,37,37,38,38,39,
	 39,40,40,41,41,42,42,43,43,44,44,45,45,46,46,47,
	 47,48,48,49,49,50,50,51,51,52,52,53,53,54,54,55,
	 55,56,56,57,57,58,58,59,59,60,60,61,61,62,62,63,
	 64,65,66,66,67,67,68,68,69,69,70,70,71,71,72,72,
	 73,73,74,74,75,75,76,76,77,77,78,78,79,79,80,80,
	 81,81,82,82,83,83,84,84,85,85,86,86,87,87,88,88,
	 89,89,90,90,91,91,92,92,93,93,94,94,95,95,96,96}};
static gint8 rhythmkey_map[128] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,35,36,37,38,39,40,41,42,43,44,45,46,47,
	48,49,50,51,-1,-1,54,-1,56,-1,-1,-1,60,61,62,63,
	64,65,66,67,68,69,70,71,72,73,-1,75,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

/* timbre, volume, panpot, reverb.  keys 24-87 (64 keys)*/
static guint8 default_rhythm_keymap[256] = { /* MT-32 default */
	0x7f,0x64,7,1,  0x7f,0x64,7,1,  0x7f,0x64,7,1, 0x7f,0x64,7,1, /* 24-27 */
	0x7f,0x64,7,1,  0x7f,0x64,7,1,  0x7f,0x64,7,1, 0x7f,0x64,7,1,
	0x7f,0x64,7,1,  0x7f,0x64,7,1,  0x7f,0x64,7,1, 0x40,0x64,7,1,
	0x40,0x64,7,1,  0x4a,0x64,6,1,  0x41,0x64,7,1, 0x4b,0x64,8,1,
	0x45,0x64,6,1,  0x44,0x64,11,1, 0x46,0x64,6,1, 0x44,0x64,11,1,
	0x5d,0x64,6,1,  0x43,0x64,8,1,  0x47,0x64,6,1, 0x43,0x64,8,1,
	0x42,0x64,3,1,  0x48,0x64,6,1,  0x42,0x64,3,1, 0x49,0x64,8,1,
	0x7f,0x64,7,1,  0x7f,0x64,7,1,  0x56,0x64,9,1, 0x7f,0x64,7,1,
	0x4c,0x64,7,1,  0x7f,0x64,7,1,  0x7f,0x64,7,1, 0x7f,0x64,7,1,
	0x52,0x64,2,1,  0x53,0x64,4,1,  0x4d,0x64,8,1, 0x4e,0x64,9,1,
	0x4f,0x64,10,1, 0x50,0x64,7,1,  0x51,0x64,5,1, 0x54,0x64,2,1,
	0x55,0x64,2,1,  0x5b,0x64,9,1,  0x58,0x64,4,1, 0x5a,0x64,9,1,
	0x59,0x64,9,1,  0x5c,0x64,10,1, 0x7f,0x64,7,1, 0x57,0x64,12,1,
	0x7f,0x64,7,1,  0x7f,0x64,7,1,  0x7f,0x64,7,1, 0x7f,0x64,7,1,
	0x7f,0x64,7,1,  0x7f,0x64,7,1,  0x7f,0x64,7,1, 0x7f,0x64,7,1,
	0x7f,0x64,7,1,  0x7f,0x64,7,1,  0x7f,0x64,7,1, 0x7f,0x64,7,1  /* 84-87 */
};

static guint8 default_partial_reserve[9] = {  /* MT-32 DEFAULT */
	3, 10, 6, 4, 3, 0, 0, 0, 6 };

static struct {
	guint8 mode;
	guint8 time;
	guint8 level;
} mt32_reverb[11];


static int
midiout_write_block(byte *buf, int len, int delta)
{
	if (delta)
		midi_writer->delay(midi_writer, delta);
		
	return midi_writer->write(midi_writer, buf, len);
}

/* send default rhythm map and reserve */
int midi_mt32_defaults(guint8 volume, guint8 reverb) {
	printf("MT-32: Writing Default Rhythm key map\n");
	midi_mt32_poke(0x030110, default_rhythm_keymap, 256);

	printf("MT-32: Writing Default Partial Reserve\n");
	midi_mt32_poke(0x100004, default_partial_reserve, 9);

	if (reverb) {
		mt32_reverb[0].mode = 0;
		mt32_reverb[0].time = 5;
		mt32_reverb[0].level = 3;
		default_reverb = 0;

		printf("MT-32: Setting up default reverb levels\n");
		midi_mt32_reverb(default_reverb);
	}

	if (volume) {
		printf("MT-32: Setting default volume (%d)\n", volume);
		midi_mt32_volume(volume);
	}

	return SFX_OK;
}

int midi_mt32_open(int length, byte *data, void *dev)
{
	guint8 unknown_sysex[6] = {0x16, 0x16, 0x16, 0x16, 0x16, 0x16};
	guint8 i, memtimbres;
	unsigned int block2, block3;

	if (!dev) {
		fprintf(stderr, "Attempt to use MT-32 sequencer without device\n");
		return SFX_ERROR;
	}

	midi_writer = (midi_writer_t *) dev;

	midi_mt32_allstop();

	type = midi_mt32_patch001_type(data, length);
	printf("MT-32: Programming Roland MT-32 with patch.001 (v%i) %d bytes\n", type, length);

	if (type == 0) {
		/* Display MT-32 initialization message */
		printf("MT-32: Displaying Text: \"%.20s\"\n", data + 20);
		midi_mt32_poke(0x200000, data + 20, 20);

		/* Cache a copy of the shutdown message */
		memcpy(shutdown_msg, data + 40, 20);

		/* Write Patches (48 or 96) */
		memtimbres = data[491];
		block2 = (memtimbres * 246) + 492;
		printf("MT-32: Writing Patches #01 - #32\n");
		midi_mt32_poke(0x050000, data + 107, 256);
		if ((length > block2) &&
		    data[block2] == 0xAB &&
		    data[block2 + 1] == 0xCD) {
			printf("MT-32: Writing Patches #33 - #64\n");
			midi_mt32_poke_gather(0x050200, data + 363, 128, data + block2 + 2, 128);
			printf("MT-32: Writing Patches #65 - #96\n");
			midi_mt32_poke(0x050400, data + block2 + 130, 256);
			block3 = block2 + 386;
		} else {
			printf("MT-32: Writing Patches #33 - #48\n");
			midi_mt32_poke(0x050200, data + 363, 128);
			block3 = block2;
		}
		/* Write Memory Timbres */
		for (i = 0; i < memtimbres; i++) {
			printf("MT-32: Writing Memory Timbre #%02d: \"%.10s\"\n",
			       i + 1, data + 492 + i * 246);
			midi_mt32_poke(0x080000 + (i << 9), data + 492 + i * 246, 246);
		}
		/* Write Rhythm key map and Partial Reserve */
		if ((length > block3) &&
		    data[block3] == 0xDC &&
		    data[block3 + 1] == 0xBA) {
			printf("MT-32: Writing Rhythm key map\n");
			midi_mt32_poke(0x030110, data + block3 + 2, 256);
			printf("MT-32: Writing Partial Reserve\n");
			midi_mt32_poke(0x100004, data + block3 + 258, 9);
		} else {
			midi_mt32_defaults(0,0);  /* send default keymap/reserve */
		}
		/* Display MT-32 initialization done message */
		printf("MT-32: Displaying Text: \"%.20s\"\n", data);
		midi_mt32_poke(0x200000, data, 20);
		/* Write undocumented MT-32(?) SysEx */
		printf("MT-32: Writing {F0 41 10 16 12 52 00 0A 16 16 16 16 16 16 20 F7}\n");
		midi_mt32_poke(0x52000A, unknown_sysex, 6);
		printf("MT-32: Setting up reverb levels\n");
		default_reverb = data[0x3e];
		memcpy(mt32_reverb,data+ 0x4a, 3 * 11);
		midi_mt32_reverb(default_reverb);
		printf("MT-32: Setting default volume (%d)\n", data[0x3c]);
		midi_mt32_volume(data[0x3c]);
		return 0;
	} else if (type == 1) {
		printf("MT-32: Displaying Text: \"%.20s\"\n", data + 20);
		midi_mt32_poke(0x200000, data + 20, 20);
		printf("MT-32: Loading SyEx bank\n");
		midi_mt32_write_block(data + 1155, (data[1154] << 8) + data[1153]);
		memcpy(patch_map, data, 128);
		memcpy(keyshift, data + 128, 128);
		memcpy(volume_adjust, data + 256, 128);
		memcpy(velocity_map_index, data + 513, 128);
		for (i = 0; i < 4; i++)
			memcpy(velocity_map[i], data + 641 + i * 128, 128);
		memcpy(rhythmkey_map, data + 384, 128);

		printf("MT-32: Setting up reverb levels\n");
		default_reverb = data[0x3e];
		memcpy(mt32_reverb,data+ 0x4a, 3 * 11);
		midi_mt32_reverb(default_reverb);

		printf("MT-32: Setting default volume (%d)\n", data[0x3c]);
		midi_mt32_volume(data[0x3c]);

		printf("MT-32: Displaying Text: \"%.20s\"\n", data);
		midi_mt32_poke(0x200000, data, 20);
		return 0;
	} else if (type == 2) {
		midi_mt32_poke(0x200000, (guint8 *)"   FreeSCI Rocks!  ", 20);
		return midi_mt32_defaults(0x0c,1);  /* send defaults in absence of patch data */
	}
	return -1;
}

int midi_mt32_close(void)
{
	midi_mt32_allstop();
	if (type == 0) {
		printf("MT-32: Displaying Text: \"%.20s\"\n", shutdown_msg);
		midi_mt32_poke(0x200000, shutdown_msg, 20);
	}
	midi_writer->close(midi_writer);
	return SFX_OK;
}

int midi_mt32_volume(guint8 volume)
{
	volume &= 0x7f; /* (make sure it's not over 127) */
	if (midi_mt32_poke(0x100016, &volume, 1) < 0)
		return -1;

	return 0;
}

int midi_mt32_allstop(void)
{
	byte buf[4];
	int i;

	buf[0] = 0x7b;
	buf[1] = 0;
	buf[2] = 0;

	for (i = 0; i < 16; i++) {
		midi_mt32_event((guint8)(0xb0 | i), 3, buf);
	}

	return 0;
}

int midi_mt32_reverb(int param)
{
	guint8 buffer[3];

	if (param == -1)
		param = default_reverb;

	printf("MT-32: Sending reverb # %d (%d, %d, %d)\n",param, mt32_reverb[param].mode,
	       mt32_reverb[param].time,
	       mt32_reverb[param].level);

	buffer[0] = mt32_reverb[param].mode;
	buffer[1] = mt32_reverb[param].time;
	buffer[2] = mt32_reverb[param].level;
	midi_mt32_poke(0x100001, buffer, 3);

	return 0;
}


static int
midi_mt32_poke(guint32 address, guint8 *data, unsigned int count)
{
	guint8 checksum = 0;
	unsigned int i;

	if (count > 256) return -1;

	checksum -= (sysex_buffer[5] = (char)((address >> 16) & 0x7F));
	checksum -= (sysex_buffer[6] = (char)((address >> 8) & 0x7F));
	checksum -= (sysex_buffer[7] = (char)(address & 0x7F));

	for (i = 0; i < count; i++)
		checksum -= (sysex_buffer[i + 8] = data[i]);

	sysex_buffer[count + 8] = checksum & 0x7F;
	sysex_buffer[count + 9] = 0xF7;

	midiout_write_block(sysex_buffer, count + 10, 0);
	midi_writer->flush(midi_writer);
	midi_mt32_sysex_delay();

	return count + 10;

}

static int
midi_mt32_poke_gather(guint32 address, guint8 *data1, unsigned int count1,
		      guint8 *data2, unsigned int count2)
{
	guint8 checksum = 0;
	unsigned int i;

	if ((count1 + count2) > 256) return -1;

	checksum -= (sysex_buffer[5] = (char)((address >> 16) & 0x7F));
	checksum -= (sysex_buffer[6] = (char)((address >> 8) & 0x7F));
	checksum -= (sysex_buffer[7] = (char)(address & 0x7F));

	for (i = 0; i < count1; i++)
		checksum -= (sysex_buffer[i + 8] = data1[i]);
	for (i = 0; i < count2; i++)
		checksum -= (sysex_buffer[i + 8 + count1] = data2[i]);

	sysex_buffer[count1 + count2 + 8] = checksum & 0x7F;
	sysex_buffer[count1 + count2 + 9] = 0xF7;

	midiout_write_block(sysex_buffer, count1 + count2 + 10, 0);
	midi_writer->flush(midi_writer);
	midi_mt32_sysex_delay();
	return count1 + count2 + 10;
}


static int
midi_mt32_write_block(guint8 *data, unsigned int count)
{
	unsigned int block_start = 0;
	unsigned int i = 0;

	while (i < count) {
		if ((data[i] == 0xF0) && (i != block_start)) {
			midiout_write_block(data + block_start, i - block_start, 0);
			block_start = i;
		}
		if (data[i] == 0xF7) {
			midiout_write_block(data + block_start, i - block_start + 1, 0);
			midi_mt32_sysex_delay();
			block_start = i + 1;
		}
		i++;
	}
	if (count >= block_start) {
		if (midiout_write_block(data + block_start, count - block_start, 0) != (count - block_start)) {
			fprintf(stderr, "midi_mt32_write_block(): midiout_write_block failed!\n");
			return 1;
		}
	}

	return 0;
}


static int
midi_mt32_patch001_type(guint8 *data, unsigned int length)
{
	if (data == NULL)  /* kq4 doesn't have a patch */
		return 2;

	/* length test */
	if (length < 1155)
		return 0;
	if (length > 16889)
		return 1;
	if (midi_mt32_patch001_type0_length(data, length) &&
	    !midi_mt32_patch001_type1_length(data, length))
		return 0;
	if (midi_mt32_patch001_type1_length(data, length) &&
	    !midi_mt32_patch001_type0_length(data, length))
		return 1;
	return -1;
}


static int
midi_mt32_patch001_type0_length(guint8 *data, unsigned int length)
{
	unsigned int pos = 492 + 246 * data[491];

	if (NULL == data) {
		fprintf(stderr, "midi_mt32_patch001_type0_length(): NULL passed for data\n");
		return 1;
	}

/*  printf("timbres %d (post = %04x)\n",data[491], pos);*/

	if ((length >= (pos + 386)) && (data[pos] == 0xAB) && (data[pos + 1] == 0xCD))
		pos += 386;

/*  printf("pos = %04x (%02x %02x)\n", pos, data[pos], data[pos + 1]); */

	if ((length >= (pos + 267)) && (data[pos] == 0xDC) && (data[pos + 1] == 0xBA))
		pos += 267;

/*  printf("pos = %04x %04x (%d)\n", pos, length, pos-length); */


	if (pos == length)
		return 1;
	return 0;
}

static int
midi_mt32_patch001_type1_length(guint8 *data, unsigned int length)
{
	if ((length >= 1155) && (((data[1154] << 8) + data[1153] + 1155) == length))
		return 1;
	return 0;
}

static int
midi_mt32_sysex_delay()
{
  /* Under Win32, we won't get any sound, in any case... */
#ifdef HAVE_USLEEP
	usleep(320 * 63); /* One MIDI byte is 320us, 320us * 63 > 20ms */
#  else
#    ifdef _WIN32
	Sleep(((320 * 63) / 1000) + 1);
#    else
	sleep(1);
#  endif
#endif
	return 0;
}

static int
midi_mt32_event(byte command, int argc, byte *argv)
{
	byte buf[8];

	buf[0] = command;
	memcpy(buf + 1, argv, argc);

	midiout_write_block(buf, argc + 1, delta);
}


static int
midi_mt32_delay(int ticks)
{
	delta += ticks; /* Accumulate, write before next command */
	return SFX_OK;
}

static int
midi_mt32_set_option(char *name, char *value)
{
	return SFX_ERROR; /* No options are supported at this time */
}

/* the driver struct */

sfx_sequencer_t sfx_sequencer_mt32 = {
	"MT32",
	"0.1",
	SFX_DEVICE_MIDI, /* No device dependancy-- fixme, this might becomde ossseq */
	&midi_mt32_set_option,
	&midi_mt32_open,
	&midi_mt32_close,
	&midi_mt32_event,
	&midi_mt32_delay,
	&midi_mt32_allstop,
	&midi_mt32_volume,
	&midi_mt32_reverb,
	001,		/* patch.001 */
	0x01,		/* playflag */
	1, 		/* do play channel 9 */
	32,  /* Max polyphony */
	0 /* Does not require any write-ahead by its own */
};

#endif /* HAVE_SYS_SOUNDCARD_H */
