/***************************************************************************
 pal_1.c Copyright (C) 2000 Christoph Reichenbach


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
/* SCI1 palette resource defrobnicator */

#include <gfx_system.h>
#include <gfx_resource.h>

#define MAX_COLORS 256
#define PALETTE_START 260
#define COLOR_OK 0x01

gfx_pixmap_color_t *
gfxr_read_pal1(int id, int *colors_nr, byte *resource, int size)
{
	int counter = 0;
	int pos;
	unsigned int colors[MAX_COLORS];
	gfx_pixmap_color_t *retval;

	if (size < PALETTE_START + 4) {
		GFXERROR("Palette resource too small in %04x\n", id);
		return NULL;
	}


	pos = PALETTE_START;

	while (pos < size && resource[pos] == COLOR_OK && counter < MAX_COLORS) {
		int i;
		int color = resource[pos]
			| (resource[pos + 1] << 8)
			| (resource[pos + 2] << 16)
			| (resource[pos + 3] << 24);

		pos += 4;

		colors[counter++] = color;
	}

	if (counter < MAX_COLORS && resource[pos] != COLOR_OK) {
		GFXERROR("Palette %04x uses unknown palette color prefix 0x%02x at offset 0x%04x\n", id, resource[pos], pos);
		return NULL;
	}

	if (counter < MAX_COLORS) {
		GFXERROR("Palette %04x ends prematurely\n", id);
		return NULL;
	}

	retval = malloc(sizeof(gfx_pixmap_color_t) * counter);

	*colors_nr = counter;
	fprintf(stderr,"c=%d\n", counter);
	for (pos = 0; pos < counter; pos++) {
		unsigned int color = colors[pos];

		retval[pos].global_index = GFX_COLOR_INDEX_UNMAPPED;
		retval[pos].r = (color >> 8) & 0xff;
		retval[pos].g = (color >> 16) & 0xff;
		retval[pos].b = (color >> 24) & 0xff;
	}

	return retval;
}


