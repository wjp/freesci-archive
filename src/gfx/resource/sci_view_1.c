/***************************************************************************
 view_1.c Copyright (C) 2000 Christoph Reichenbach


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
/* SCI 1 view resource defrobnicator */

#include <sci_memory.h>
#include <gfx_system.h>
#include <gfx_resource.h>
#include <gfx_tools.h>

#define V1_LOOPS_NR_OFFSET 0
#define V1_PALETTE_OFFSET 6
#define V1_FIRST_LOOP_OFFSET 8

#define V1_RLE 0x80 /* run-length encode? */
#define V1_RLE_BG 0x40 /* background fill */

static gfx_pixmap_t *
gfxr_draw_cel1(int id, int loop, int cel, byte *resource, int size, gfxr_view_t *view)
{
	int xl = get_int_16(resource);
	int yl = get_int_16(resource + 2);
	int xhot = get_int_16(resource + 4);
	int yhot = get_int_16(resource + 6);
	int pos = 8;
	int writepos = 0;
	int pixmap_size = xl * yl;
	gfx_pixmap_t *retval = gfx_pixmap_alloc_index_data(gfx_new_pixmap(xl, yl, id, loop, cel));
	byte *dest = retval->index_data;

	retval->xoffset = xhot;
	retval->yoffset = yhot;
	retval->colors = view->colors;
	retval->colors_nr = view->colors_nr;
	retval->flags |= GFX_PIXMAP_FLAG_EXTERNAL_PALETTE;

	if (xl <= 0 || yl <= 0) {
		gfx_free_pixmap(NULL, retval);
		GFXERROR("View %02x:(%d/%d) has invalid xl=%d or yl=%d\n", id, loop, cel, xl, yl);
		return NULL;
	}

	while (writepos < pixmap_size && pos < size) {
		int op = resource[pos++];
		int bytes;
		int readbytes = 0;

		if (op & V1_RLE) {
			bytes = op & 0x3f;
			op &= (V1_RLE | V1_RLE_BG);
			readbytes = (op & V1_RLE_BG)? 0 : 1;
		} else {
			readbytes = bytes = op & 0x7f;
			op = 0;
		}

		if (pos + readbytes > size) {
			gfx_free_pixmap(NULL, retval);
			GFXERROR("View %02x:(%d/%d) requires %d bytes to be read when %d are available at pos %d\n",
				 id, loop, cel, readbytes, size - pos, pos - 1);
			return NULL;
		}

		if (bytes + writepos > pixmap_size) {
			GFXWARN("View %02x:(%d/%d) describes more bytes than needed: %d/%d bytes at rel. offset 0x%04x\n",
				id, loop, cel, bytes + writepos, pixmap_size, pos - 1);
			bytes = pixmap_size - writepos;
		}

		if (op) {
			if (op & V1_RLE_BG)
				memset(dest + writepos, GFX_COLOR_INDEX_TRANSPARENT, bytes);
			else {
				int color = resource[pos++];
				memset(dest + writepos, color, bytes);
			}
		} else {
			memcpy(dest + writepos, resource + pos, bytes);
			pos += bytes;
		}

		writepos += bytes;
	}
	return retval;
}

static int
gfxr_draw_loop1(gfxr_loop_t *dest, int id, int loop, byte *resource, int offset, int size, gfxr_view_t *view)
{
	int i;
	int cels_nr = get_int_16(resource + offset);

	if (get_uint_16(resource + offset + 2)) {
		GFXWARN("View %02x:(%d): Gray magic %04x in loop, expected white\n", id, loop, get_uint_16(resource + offset + 2));
	}

	if (cels_nr * 2 + 4 + offset > size) {
		GFXERROR("View %02x:(%d): Offset array for %d cels extends beyond resource space\n", id, loop, cels_nr);
		dest->cels_nr = 0; /* Mark as "delete no cels" */
		dest->cels = NULL;
		return 1;
	}

	dest->cels = sci_malloc(sizeof(gfx_pixmap_t *) * cels_nr);

	for (i = 0; i < cels_nr; i++) {
		int cel_offset = get_uint_16(resource + offset + 4 + (i << 1));
		gfx_pixmap_t *cel;

		if (cel_offset >= size) {
			GFXERROR("View %02x:(%d/%d) supposed to be at illegal offset 0x%04x\n", id, loop, i, cel_offset);
			cel = NULL;
		} else
			cel = gfxr_draw_cel1(id, loop, i, resource + cel_offset, size - cel_offset, view);

		if (!cel) {
			dest->cels_nr = i;
			return 1;
		}

		dest->cels[i] = cel;
	}

	dest->cels_nr = cels_nr;

	return 0;
}


#define V1_FIRST_MAGIC 1
#define V1_MAGICS_NR 5
static byte view_magics[V1_MAGICS_NR] = {0x80, 0x00, 0x00, 0x00, 0x00};

gfxr_view_t *
gfxr_draw_view1(int id, byte *resource, int size)
{
	int i;
	int palette_offset;
	gfxr_view_t *view;

	if (size < V1_FIRST_LOOP_OFFSET + 8) {
		GFXERROR("Attempt to draw empty view %04x\n", id);
		return NULL;
	}

	view = sci_malloc(sizeof(gfxr_view_t));
	view->ID = id;
	view->flags = 0;

	view->loops_nr = resource[V1_LOOPS_NR_OFFSET];
	palette_offset = get_uint_16(resource + V1_PALETTE_OFFSET);

	if (view->loops_nr * 2 + V1_FIRST_LOOP_OFFSET > size) {
		GFXERROR("View %04x: Not enough space in resource to accomodate for the claimed %d loops\n", id, view->loops_nr);
		free(view);
		return NULL;
	}

	for (i = 0; i < V1_MAGICS_NR; i++)
		if (resource[V1_FIRST_MAGIC + i] != view_magics[i]) {
			GFXWARN("View %04x: View magic #%d should be %02x but is %02x\n",
				id, i, view_magics[i], resource[V1_FIRST_MAGIC + i]);
		}

	if (palette_offset > size) {
		GFXERROR("Palette is outside of view %04x\n", id);
		free(view);
		return NULL;
	}

	if (!(view->colors = gfxr_read_pal1(id, &(view->colors_nr),
					    resource + palette_offset, size - palette_offset))) {
		GFXERROR("view %04x: Palette reading failed. Aborting...\n", id);
		free(view);
		return NULL;
	}

	view->loops = sci_malloc(sizeof (gfxr_loop_t) * view->loops_nr);

	for (i = 0; i < view->loops_nr; i++) {
		int error_token = 0;
		int loop_offset = get_uint_16(resource + V1_FIRST_LOOP_OFFSET + (i << 1));

		if (loop_offset >= size) {
			GFXERROR("View %04x:(%d) supposed to be at illegal offset 0x%04x\n", id, i);
			error_token = 1;
		}

		if (error_token || gfxr_draw_loop1(view->loops + i, id, i, resource, loop_offset, size, view)) {
			/* An error occured */
			view->loops_nr = i;
			gfxr_free_view(NULL, view);
			return NULL;
		}
	}

	return view;
}
