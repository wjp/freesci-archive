/***************************************************************************
  Copyright (C) 2008 Christoph Reichenbach


 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantability,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Christoph Reichenbach (CR) <creichen@gmail.com>

***************************************************************************/

#include <assert.h>
#include "sci_midi.h"
#include "sci_memory.h"
#include "sfx-instrument-map.h"

sfx_instrument_map_t *
sfx_instrument_map_new(int velocity_maps_nr)
{
	sfx_instrument_map_t *map = (sfx_instrument_map_t *)sci_malloc(sizeof (sfx_instrument_map_t));
	int i;

	map->initialisation_block_size = 0;
	map->initialisation_block = NULL;

	map->velocity_maps_nr = velocity_maps_nr;
	map->percussion_velocity_map_index = SFX_NO_VELOCITY_MAP;

	if (velocity_maps_nr == 0)
		map->velocity_map = NULL; /* Yes, this complicates control flow needlessly, but it avoids some of the pointless
					  ** warnings that certain memory tools seem to find appropriate. */
	else {
		map->velocity_map = (byte **)sci_malloc(sizeof (byte *) * velocity_maps_nr);
		for (i = 0; i < velocity_maps_nr; i++)
			map->velocity_map[i] = (byte *)sci_malloc(SFX_VELOCITIES_NR);
	}
	for (i = 0; i < SFX_INSTRUMENTS_NR; i++)
		map->velocity_map_index[i] = SFX_NO_VELOCITY_MAP;

	map->percussion_volume_adjust = 0;
	for (i = 0; i < SFX_RHYTHM_NR; i++)
		     map->percussion_map[i] = i;


	for (i = 0; i < SFX_INSTRUMENTS_NR; i++) {
		map->patch_map[i] = i;
		map->patch_key_shift[i] = 0;
		map->patch_volume_adjust[i] = 0;
	}

	return map;
}

void
sfx_instrument_map_free(sfx_instrument_map_t *map)
{
	if (!map)
		return;

	if (map->velocity_map) {
		int i;
		for (i = 0; i < map->velocity_maps_nr; i++)
			sci_free(map->velocity_map[i]);
		sci_free(map->velocity_map);
		map->velocity_map = NULL;
	}

	if (map->initialisation_block) {
		sci_free(map->initialisation_block);
		map->initialisation_block = NULL;
	}

	sci_free(map);
}

#define PATCH_MAP_OFFSET		0x0000
#define PATCH_KEY_SHIFT_OFFSET		0x0080
#define PATCH_VOLUME_ADJUST_OFFSET	0x0100
#define PATCH_PERCUSSION_MAP_OFFSET	0x0180
#define PATCH_PERCUSSION_VOLUME_ADJUST	0x0200
#define PATCH_VELOCITY_MAP_INDEX	0x0201
#define PATCH_VELOCITY_MAP(i)		(0x0281 + (0x80 * i))
#define PATCH_INIT_DATA_SIZE_LE		0x0481
#define PATCH_INIT_DATA			0x0483

#define PATCH_INSTRUMENT_MAPS_NR 4

#define PATCH_MIN_SIZE PATCH_INIT_DATA




sfx_instrument_map_t *
sfx_instrument_map_load_sci(byte *data, size_t size)
{
	sfx_instrument_map_t * map;
	int i, m;

	if (data == NULL)
		return NULL;

	if (size < PATCH_MIN_SIZE) {
		fprintf(stderr, "[instrument-map] Instrument map too small:  %d of %d\n", (int) size, PATCH_MIN_SIZE);
		return NULL;
	}

	map = sfx_instrument_map_new(PATCH_INSTRUMENT_MAPS_NR);

	/* Set up MIDI intialisation data */
	map->initialisation_block_size = getInt16(data + PATCH_INIT_DATA_SIZE_LE);
	if (map->initialisation_block_size) {
		if (size < PATCH_MIN_SIZE + map->initialisation_block_size) {
			fprintf(stderr, "[instrument-map] Instrument map too small for initialisation block:  %d of %d\n", (int) size, PATCH_MIN_SIZE);
			return NULL;
		}

		if (size > PATCH_MIN_SIZE + map->initialisation_block_size)
			fprintf(stderr, "[instrument-map] Instrument larger than required by initialisation block:  %d of %d\n", (int) size, PATCH_MIN_SIZE);

		if (map->initialisation_block != 0) {
			map->initialisation_block = (byte *)sci_malloc(map->initialisation_block_size);
			memcpy(map->initialisation_block, data + PATCH_INIT_DATA, map->initialisation_block_size);
		}
	}

	/* Set up basic instrument info */
	for (i = 0; i < SFX_INSTRUMENTS_NR; i++) {
		map->patch_map[i] = data[PATCH_MAP_OFFSET + i];
		map->patch_key_shift[i] = (char)data[PATCH_KEY_SHIFT_OFFSET + i];
		map->patch_volume_adjust[i] = (char)data[PATCH_VOLUME_ADJUST_OFFSET + i];
		map->velocity_map_index[i] = data[PATCH_VELOCITY_MAP_INDEX + i];
	}

	/* Set up percussion maps */
	map->percussion_volume_adjust = data[PATCH_PERCUSSION_VOLUME_ADJUST];
	for (i = 0; i < SFX_RHYTHM_NR; i++)
		map->percussion_map[i] = data[PATCH_PERCUSSION_MAP_OFFSET + i];

	/* Set up velocity maps */
	for (m = 0; m < PATCH_INSTRUMENT_MAPS_NR; m++) {
		byte *velocity_map = map->velocity_map[m];
		for (i = 0; i < SFX_VELOCITIES_NR; i++)
			velocity_map[i] = data[PATCH_VELOCITY_MAP(m) + i];
	}

	map->percussion_velocity_map_index = 0;

	return map;
}


/* Output with the instrument map */
#define MIDI_CHANNELS_NR 0x10

typedef struct decorated_midi_writer {
	MIDI_WRITER_BODY

	midi_writer_t *writer;
	byte patches[MIDI_CHANNELS_NR];
	sfx_instrument_map_t *map;
} decorated_midi_writer_t;


static void
init_decorated(struct _midi_writer *self_)
{
	decorated_midi_writer_t *self = (decorated_midi_writer_t *) self_;
	self->writer->init(self->writer);
}

static void
set_option_decorated(struct _midi_writer *self_, char *name, char *value)
{
	decorated_midi_writer_t *self = (decorated_midi_writer_t *) self_;
	self->writer->set_option(self->writer, name, value);
}

static void
delay_decorated(struct _midi_writer *self_, int ticks)
{
	decorated_midi_writer_t *self = (decorated_midi_writer_t *) self_;
	self->writer->delay(self->writer, ticks);
}

static void
flush_decorated(struct _midi_writer *self_)
{
	decorated_midi_writer_t *self = (decorated_midi_writer_t *) self_;
	self->writer->flush(self->writer);
}

static void
reset_timer_decorated(struct _midi_writer *self_)
{
	decorated_midi_writer_t *self = (decorated_midi_writer_t *) self_;
	self->writer->reset_timer(self->writer);
}


static void
close_decorated(decorated_midi_writer_t *self)
{
	sfx_instrument_map_free(self->map);
	self->map = NULL;
	self->writer->close(self->writer);
	sci_free(self->name);
	self->name = NULL;
	sci_free(self);
}

#define BOUND_127(x) (((x) < 0)? 0 : (((x) > 0x7f)? 0x7f : (x)))

static int
bound_hard_127(int i, char *descr)
{
	int r = BOUND_127(i);
	if (r != 1)
		fprintf(stderr, "[instrument-map] Hard-clipping %02x to %02x in %s\n", i, r, descr);
	return r;
}

static int
write_decorated(decorated_midi_writer_t *self, byte *buf, int len)
{
	sfx_instrument_map_t *map = self->map;
	int op = *buf & 0xf0;
	int chan = *buf & 0x0f;
	int patch = self->patches[chan];

	assert (len >= 1);

	if (op == 0xC0) { /* Program change */
		int patch = bound_hard_127(buf[1], "program change");
		int instrument = bound_hard_127(map->patch_map[patch], "patch lookup");
		assert (len >= 2);
		buf[1] = instrument;
		self->patches[chan] = patch;
	}


	if (chan == MIDI_RHYTHM_CHANNEL) {
		/* Rhythm channel handling */
		switch (op) {
		case 0x80:
		case 0x90: { /* Note off / note on */
			int instrument_index = bound_hard_127(buf[1], "rhythm instrument index");
			int velocity = bound_hard_127(buf[2], "rhythm velocity");
			int instrument = bound_hard_127(map->percussion_map[instrument_index], "rhythm instrument");
			int velocity_map_index = map->percussion_velocity_map_index;
			assert (len >= 3);

			if (velocity_map_index != SFX_NO_VELOCITY_MAP)
				velocity += BOUND_127(map->velocity_map[map->velocity_map_index[patch]][velocity]);

			buf[1] = instrument;
			buf[2] = velocity;
			break;
		}

		case 0xB0: { /* Controller change */
			assert (len >= 3);
			if (buf[1] == 0x7) /* Volume change */
				buf[2] = BOUND_127(buf[2] + map->percussion_volume_adjust);
			break;
		}

		default: break;
		}

	} else {
		/* Instrument channel handling */

		switch (op) {
		case 0x80:
		case 0x90: { /* Note off / note on */
			int note = bound_hard_127(buf[1], "note");
			int velocity = bound_hard_127(buf[2], "velocity");
			int velocity_map_index = map->velocity_map_index[patch];
			assert (len >= 3);

			note += map->patch_key_shift[patch];
			/* Not the most efficient solutions, but the least error-prone */
			while (note < 0)
				note += 12;
			while (note > 0x7f)
				note -= 12;

			if (velocity_map_index != SFX_NO_VELOCITY_MAP)
				velocity += BOUND_127(map->velocity_map[map->velocity_map_index[patch]][velocity]);

			buf[1] = note;
			buf[2] = velocity;
			break;
		}

		case 0xB0: /* Controller change */
			assert (len >= 3);
			if (buf[1] == 0x7) /* Volume change */
				buf[2] = BOUND_127(buf[2] + map->patch_volume_adjust[patch]);
			break;

		default: break;
		}
	}

	return self->writer->write(self->writer, buf, len);
}

#define MIDI_BYTES_PER_SECOND 3250 /* This seems to be the minimum guarantee by the standard */
#define MAX_PER_TICK (MIDI_BYTES_PER_SECOND / 60) /* After this, we ought to issue one tick of pause */

static void
init(midi_writer_t *writer, byte *data, size_t len)
{
	int offset = 0;

	while (offset < len) {
		int left = len - offset;
		int to_write = MIN(MAX_PER_TICK, left);

		writer->write(writer, data + offset, to_write);
		writer->flush(writer);
		writer->delay(writer, 1);

		offset += to_write;
	}
}

#define NAME_SUFFIX "+instruments"

midi_writer_t *
sfx_mapped_writer(midi_writer_t *writer, sfx_instrument_map_t *map)
{
	decorated_midi_writer_t *retval;

	if (map == NULL)
		return writer;

	retval = (decorated_midi_writer_t *)sci_malloc(sizeof(decorated_midi_writer_t));
	retval->writer = writer;
	retval->name = (char *)sci_malloc(strlen(writer->name) + strlen(NAME_SUFFIX) + 1);
	strcpy(retval->name, writer->name);
	strcat(retval->name, NAME_SUFFIX);

	retval->init = (int (*)(midi_writer_t *)) init_decorated;
	retval->set_option = (int (*)(midi_writer_t *, char *, char *)) set_option_decorated;
	retval->write = (int (*)(midi_writer_t *, byte *, int)) write_decorated;
	retval->delay = (void (*)(midi_writer_t *, int)) delay_decorated;
	retval->flush = (void (*)(midi_writer_t *)) flush_decorated;
	retval->reset_timer = (void (*)(midi_writer_t *)) reset_timer_decorated;
	retval->close = (void (*)(midi_writer_t *)) close_decorated;

	init(writer, map->initialisation_block, map->initialisation_block_size);
	memset(retval->patches, 0, MIDI_CHANNELS_NR);


	return (midi_writer_t *) retval;
}

