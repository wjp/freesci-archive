/***************************************************************************
 sci_pic_0.c Copyright (C) 2000 Christoph Reichenbach


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

#include <sci_memory.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <gfx_resource.h>
#include <gfx_tools.h>

#undef GFXR_DEBUG_PIC0 /* Enable to debug pic0 messages */

#define GFXR_PIC0_PALETTE_SIZE 40
#define GFXR_PIC0_NUM_PALETTES 4

#undef FILL_RECURSIVE_DEBUG /* Enable for verbose fill debugging */

#define INTERCOL(a, b) ((int) sqrt((((3.3 * (a))*(a)) + ((1.7 * (b))*(b))) / 5.0))
/* Macro for color interpolation */

#define SCI_PIC0_MAX_FILL 30 /* Number of times to fill before yielding to scheduler */

#define SCI0_MAX_PALETTE 2

int sci0_palette = 0;


/* Default color maps */
gfx_pixmap_color_t gfx_sci0_image_colors[SCI0_MAX_PALETTE+1][GFX_SCI0_IMAGE_COLORS_NR] = {
	{{GFX_COLOR_SYSTEM, 0x00, 0x00, 0x00}, {GFX_COLOR_SYSTEM, 0x00, 0x00, 0xaa},
	 {GFX_COLOR_SYSTEM, 0x00, 0xaa, 0x00}, {GFX_COLOR_SYSTEM, 0x00, 0xaa, 0xaa},
	 {GFX_COLOR_SYSTEM, 0xaa, 0x00, 0x00}, {GFX_COLOR_SYSTEM, 0xaa, 0x00, 0xaa},
	 {GFX_COLOR_SYSTEM, 0xaa, 0x55, 0x00}, {GFX_COLOR_SYSTEM, 0xaa, 0xaa, 0xaa},
	 {GFX_COLOR_SYSTEM, 0x55, 0x55, 0x55}, {GFX_COLOR_SYSTEM, 0x55, 0x55, 0xff},
	 {GFX_COLOR_SYSTEM, 0x55, 0xff, 0x55}, {GFX_COLOR_SYSTEM, 0x55, 0xff, 0xff},
	 {GFX_COLOR_SYSTEM, 0xff, 0x55, 0x55}, {GFX_COLOR_SYSTEM, 0xff, 0x55, 0xff},
	 {GFX_COLOR_SYSTEM, 0xff, 0xff, 0x55}, {GFX_COLOR_SYSTEM, 0xff, 0xff, 0xff}}, /* "Normal" EGA */


	{{GFX_COLOR_SYSTEM, 0x00, 0x00, 0x00}, {GFX_COLOR_SYSTEM, 0x00, 0x00, 0xff},
	 {GFX_COLOR_SYSTEM, 0x00, 0xaa, 0x00}, {GFX_COLOR_SYSTEM, 0x00, 0xaa, 0xaa},
	 {GFX_COLOR_SYSTEM, 0xce, 0x00, 0x00}, {GFX_COLOR_SYSTEM, 0xbe, 0x71, 0xde},
	 {GFX_COLOR_SYSTEM, 0x8d, 0x50, 0x00}, {GFX_COLOR_SYSTEM, 0xbe, 0xbe, 0xbe},
	 {GFX_COLOR_SYSTEM, 0x55, 0x55, 0x55}, {GFX_COLOR_SYSTEM, 0x00, 0xbe, 0xff},
	 {GFX_COLOR_SYSTEM, 0x00, 0xce, 0x55}, {GFX_COLOR_SYSTEM, 0x55, 0xff, 0xff},
	 {GFX_COLOR_SYSTEM, 0xff, 0x9d, 0x8d}, {GFX_COLOR_SYSTEM, 0xff, 0x55, 0xff},
	 {GFX_COLOR_SYSTEM, 0xff, 0xff, 0x00}, {GFX_COLOR_SYSTEM, 0xff, 0xff, 0xff}}, /* AGI Amiga-ish */

/* RGB and I intensities (former taken from the GIMP) */
#define GR 30
#define GG 59
#define GB 11
#define GI 15

#define FULL (GR+GG+GB+GI)

#define CC(x) (((x)*255)/FULL),(((x)*255)/FULL),(((x)*255)/FULL)         /* Combines color intensities */

	{{GFX_COLOR_SYSTEM, CC(0)           }, {GFX_COLOR_SYSTEM, CC(GB)          },
	 {GFX_COLOR_SYSTEM, CC(GG)          }, {GFX_COLOR_SYSTEM, CC(GB+GG)       },
	 {GFX_COLOR_SYSTEM, CC(GR)          }, {GFX_COLOR_SYSTEM, CC(GB+GR)       },
	 {GFX_COLOR_SYSTEM, CC(GG+GR)       }, {GFX_COLOR_SYSTEM, CC(GB+GG+GR)    },
	 {GFX_COLOR_SYSTEM, CC(GI)          }, {GFX_COLOR_SYSTEM, CC(GB+GI)       },
	 {GFX_COLOR_SYSTEM, CC(GG+GI)       }, {GFX_COLOR_SYSTEM, CC(GB+GG+GI)    },
	 {GFX_COLOR_SYSTEM, CC(GR+GI)       }, {GFX_COLOR_SYSTEM, CC(GB+GR+GI)    },
	 {GFX_COLOR_SYSTEM, CC(GG+GR+GI)    }, {GFX_COLOR_SYSTEM, CC(GB+GG+GR+GI) }}}; /* Grayscale */

#undef GR
#undef GG
#undef GB
#undef GI

#undef FULL

#undef C2
#undef C3
#undef C4

gfx_pixmap_color_t gfx_sci0_pic_colors[GFX_SCI0_PIC_COLORS_NR]; /* Initialized during initialization */

static int _gfxr_pic0_colors_initialized = 0;

/************************************/
/************************************/
/************************************/
int fillc = 100000000;
int fillrc = 3000000;
int fillmagc = 30000000;
/************************************/
/************************************/
/************************************/

void
gfxr_init_static_palette()
{
	int i;

	if (!_gfxr_pic0_colors_initialized) {
		for (i = 0; i < 256; i++) {
			gfx_sci0_pic_colors[i].global_index = GFX_COLOR_INDEX_UNMAPPED;
			gfx_sci0_pic_colors[i].r = INTERCOL(gfx_sci0_image_colors[sci0_palette][i & 0xf].r,
							    gfx_sci0_image_colors[sci0_palette][i >> 4].r);
			gfx_sci0_pic_colors[i].g = INTERCOL(gfx_sci0_image_colors[sci0_palette][i & 0xf].g,
							    gfx_sci0_image_colors[sci0_palette][i >> 4].g);
			gfx_sci0_pic_colors[i].b = INTERCOL(gfx_sci0_image_colors[sci0_palette][i & 0xf].b,
							    gfx_sci0_image_colors[sci0_palette][i >> 4].b);
		}
WARNING("Uncomment me after fixing sci0_palette changes to reset me");
                /*  _gfxr_pic0_colors_initialized = 1; */
	}
}


gfxr_pic_t *
gfxr_init_pic(gfx_mode_t *mode, int ID)
{
	gfxr_pic_t *pic = sci_malloc(sizeof(gfxr_pic_t));

	pic->mode = mode;

	pic->control_map = gfx_pixmap_alloc_index_data(gfx_new_pixmap(320, 200, ID, 2, 0));

	pic->priority_map = gfx_pixmap_alloc_index_data(gfx_new_pixmap(mode->xfact * 320, mode->yfact * 200,
								       ID, 1, 0));


	pic->visual_map = gfx_pixmap_alloc_index_data(gfx_new_pixmap(320 * mode->xfact,
								     200 * mode->yfact, ID, 0, 0));
	pic->visual_map->colors = gfx_sci0_pic_colors;
	pic->visual_map->colors_nr = GFX_SCI0_PIC_COLORS_NR;

	pic->visual_map->flags = GFX_PIXMAP_FLAG_EXTERNAL_PALETTE;
	pic->priority_map->flags = GFX_PIXMAP_FLAG_EXTERNAL_PALETTE;
	pic->control_map->flags = GFX_PIXMAP_FLAG_EXTERNAL_PALETTE;
	if (mode->xfact > 1 || mode->yfact > 1) {
		pic->visual_map->flags |= GFX_PIXMAP_FLAG_SCALED_INDEX;
		pic->priority_map->flags |= GFX_PIXMAP_FLAG_SCALED_INDEX;
	}

	pic->priority_map->colors = gfx_sci0_image_colors[sci0_palette];
	pic->priority_map->colors_nr = GFX_SCI0_IMAGE_COLORS_NR;
	pic->control_map->colors = gfx_sci0_image_colors[sci0_palette];
	pic->control_map->colors_nr = GFX_SCI0_IMAGE_COLORS_NR;

	/* Initialize colors */
	gfxr_init_static_palette();

	pic->undithered_buffer_size = pic->visual_map->index_xl * pic->visual_map->index_yl;
	pic->undithered_buffer = NULL;
	pic->internal = NULL;

	return pic;
}


/****************************/
/* Pic rendering operations */
/****************************/

void
gfxr_clear_pic0(gfxr_pic_t *pic)
{
	memset(pic->visual_map->index_data, 0x00, (320 * pic->mode->xfact * SCI_TITLEBAR_SIZE * pic->mode->yfact));
	memset(pic->visual_map->index_data + (320 * pic->mode->xfact * SCI_TITLEBAR_SIZE * pic->mode->yfact),
	       0xff, pic->mode->xfact * 320 * pic->mode->yfact * (200 - SCI_TITLEBAR_SIZE)); /* white */
	memset(pic->priority_map->index_data + (320 * pic->mode->xfact * SCI_TITLEBAR_SIZE * pic->mode->yfact),
	       0x0, pic->mode->xfact * 320 * pic->mode->yfact * (200 - SCI_TITLEBAR_SIZE));
	memset(pic->priority_map->index_data, 0x0a, SCI_TITLEBAR_SIZE * (pic->mode->yfact * 320 * pic->mode->xfact));
	memset(pic->control_map->index_data, 0, GFXR_AUX_MAP_SIZE);
	memset(pic->aux_map, 0, GFXR_AUX_MAP_SIZE);
}


/*** Basic operations on the auxiliary buffer ***/

#define FRESH_PAINT 0x40
/* freshly filled or near to something that is */

#define LINEMACRO(startx, starty, deltalinear, deltanonlinear, linearvar, nonlinearvar, \
                  linearend, nonlinearstart, linearmod, nonlinearmod, operation) \
   x = (startx); y = (starty); \
   incrNE = ((deltalinear) > 0)? (deltalinear) : -(deltalinear); \
   incrNE <<= 1; \
   deltanonlinear <<= 1; \
   incrE = ((deltanonlinear) > 0) ? -(deltanonlinear) : (deltanonlinear);  \
   d = nonlinearstart-1;  \
   while (linearvar != (linearend)) { \
     buffer[linewidth * y + x] operation color; \
     color ^= color2; color2 ^= color; color ^= color2; /* Swap colors */ \
     linearvar += linearmod; \
     if ((d+=incrE) < 0) { \
       d += incrNE; \
       nonlinearvar += nonlinearmod; \
     }; \
   }; \
  buffer[linewidth * y + x] operation color;

static void
_gfxr_auxbuf_line_draw(gfxr_pic_t *pic, rect_t line, int color, int color2)
{
	int dx, dy, incrE, incrNE, d, finalx, finaly;
	int x = line.x;
	int y = line.y + SCI_TITLEBAR_SIZE;
	unsigned char *buffer = pic->aux_map;
	int linewidth = 320;

	dx = line.xl;
	dy = line.yl;
	finalx = x + dx;
	finaly = y + dy;

	dx = abs(dx);
	dy = abs(dy);

	if (dx > dy) {
		if (finalx < x) {
			if (finaly < y) { /* llu == left-left-up */
				LINEMACRO(x, y, dx, dy, x, y, finalx, dx, -1, -1, |=);
			} else {         /* lld */
				LINEMACRO(x, y, dx, dy, x, y, finalx, dx, -1, 1, |=);
			}
		} else { /* x1 >= x */
			if (finaly < y) { /* rru */
				LINEMACRO(x, y, dx, dy, x, y, finalx, dx, 1, -1, |=);
			} else {         /* rrd */
				LINEMACRO(x, y, dx, dy, x, y, finalx, dx, 1, 1, |=);
			}
		}
	} else { /* dx <= dy */
		if (finaly < y) {
			if (finalx < x) { /* luu */
				LINEMACRO(x, y, dy, dx, y, x, finaly, dy, -1, -1, |=);
			} else {         /* ruu */
				LINEMACRO(x, y, dy, dx, y, x, finaly, dy, -1, 1, |=);
			}
		} else { /* y1 >= y */
			if (finalx < x) { /* ldd */
				LINEMACRO(x, y, dy, dx, y, x, finaly, dy, 1, -1, |=);
			} else {         /* rdd */
				LINEMACRO(x, y, dy, dx, y, x, finaly, dy, 1, 1, |=);
			}
		}
	}
}

static void
_gfxr_auxbuf_line_clear(gfxr_pic_t *pic, rect_t line, int color)
{
	int dx, dy, incrE, incrNE, d, finalx, finaly;
	int x = line.x;
	int y = line.y + SCI_TITLEBAR_SIZE;
	unsigned char *buffer = pic->aux_map;
	int linewidth = 320;
	int color2 = color;

	dx = line.xl;
	dy = line.yl;
	finalx = x + dx;
	finaly = y + dy;

	dx = abs(dx);
	dy = abs(dy);

	if (dx > dy) {
		if (finalx < x) {
			if (finaly < y) { /* llu == left-left-up */
				LINEMACRO(x, y, dx, dy, x, y, finalx, dx, -1, -1, &=);
			} else {         /* lld */
				LINEMACRO(x, y, dx, dy, x, y, finalx, dx, -1, 1, &=);
			}
		} else { /* x1 >= x */
			if (finaly < y) { /* rru */
				LINEMACRO(x, y, dx, dy, x, y, finalx, dx, 1, -1, &=);
			} else {         /* rrd */
				LINEMACRO(x, y, dx, dy, x, y, finalx, dx, 1, 1, &=);
			}
		}
	} else { /* dx <= dy */
		if (finaly < y) {
			if (finalx < x) { /* luu */
				LINEMACRO(x, y, dy, dx, y, x, finaly, dy, -1, -1, &=);
			} else {         /* ruu */
				LINEMACRO(x, y, dy, dx, y, x, finaly, dy, -1, 1, &=);
			}
		} else { /* y1 >= y */
			if (finalx < x) { /* ldd */
				LINEMACRO(x, y, dy, dx, y, x, finaly, dy, 1, -1, &=);
			} else {         /* rdd */
				LINEMACRO(x, y, dy, dx, y, x, finaly, dy, 1, 1, &=);
			}
		}
	}
}

#undef LINEMACRO


static void
_gfxr_auxbuf_propagate_changes(gfxr_pic_t *pic, int bitmask)
{
	/* Propagates all filled bits into the planes described by bitmask */
	unsigned long *data = (unsigned long *) pic->aux_map;
	unsigned long clearmask = 0x07070707;
	unsigned long andmask =
		(bitmask << 3)
		| (bitmask << (3+8))
		| (bitmask << (3+16))
		| (bitmask << (3+24));
	int i;

	if (sizeof(unsigned long) == 8) { /* UltraSparc, Alpha, newer MIPSens, etc */
		andmask |= (andmask << 32);
		clearmask |= (clearmask << 32);
	}

	for (i = 0; i < GFXR_AUX_MAP_SIZE / sizeof(unsigned long); i++) {
		unsigned long temp = *data & andmask;
		temp >>= 3;
		*data = (temp | *data) & clearmask;
		++data;
	}
}


static void
_gfxr_auxbuf_fill_helper(gfxr_pic_t *pic, int old_xl, int old_xr, int y, int dy,
			 int clipmask, int control)
{
	int xl, xr;
	int oldytotal = y * 320;
	unsigned char fillmask = 0x78;

	do {
		int ytotal = oldytotal + (320 * dy);
		int xcont;
		int state;

		y += dy;

		if (y < SCI_TITLEBAR_SIZE || y > 199)
			return;

		xl = old_xl;
		if (!(pic->aux_map[ytotal + xl] & clipmask)) { /* go left */
			while (xl && !(pic->aux_map[ytotal + xl - 1] & clipmask))
				--xl;
		} else /* go right and look for the first valid spot */
			while ((xl <= old_xr) && (pic->aux_map[ytotal + xl] & clipmask))
				++xl;

		if (xl > old_xr) /* No fillable strip above the last one */
			return;

		if ((ytotal + xl) < 0) { fprintf(stderr,"AARGH-%d\n", __LINE__); BREAKPOINT(); }

		xr = xl;
		while (xr < 320 && !(pic->aux_map[ytotal + xr] & clipmask)) {
			pic->aux_map[ytotal + xr] |= fillmask;
			++xr;
		}

		if ((ytotal + xr) > 64000) { fprintf(stderr,"AARGH-%d\n", __LINE__);
		BREAKPOINT();
		}

		--xr;

		if (xr < xl)
			return;

		/* Check whether we need to recurse on branches in the same direction */
		if ((y > SCI_TITLEBAR_SIZE && dy < 0)
		    || (y < 199 && dy > 0)) {

			state = 0;
			xcont = xr + 1;
			while (xcont <= old_xr) {
				if (pic->aux_map[ytotal + xcont] & clipmask)
					state = 0;
				else if (!state) { /* recurse */
					state = 1;
					_gfxr_auxbuf_fill_helper(pic, xcont, old_xr,
								 y - dy, dy, clipmask, control);
				}
				++xcont;
			}
		}

		/* Check whether we need to recurse on backward branches: */
		/* left */
		if (xl < old_xl - 1) {
			state = 0;
			for (xcont = old_xl - 1; xcont >= xl; xcont--) {
				if (pic->aux_map[oldytotal + xcont] & clipmask)
					state = xcont;
				else if (state) { /* recurse */
					_gfxr_auxbuf_fill_helper(pic, xcont, state,
								 y, -dy, clipmask, control);
					state = 0;
				}
			}
		}

		/* right */
		if (xr > old_xr + 1) {
			state = 0;
			for (xcont = old_xr + 1; xcont <= xr; xcont++) {
				if (pic->aux_map[oldytotal + xcont] & clipmask)
					state = xcont;
				else if (state) { /* recurse */
					_gfxr_auxbuf_fill_helper(pic, state, xcont,
								 y, -dy, clipmask, control);
					state = 0;
				}
			}
		}

		if ((ytotal + xl) < 0) { fprintf(stderr,"AARGH-%d\n", __LINE__); BREAKPOINT() }
		if ((ytotal + xr+1) > 64000) { fprintf(stderr,"AARGH-%d\n", __LINE__); BREAKPOINT(); }

		if (control)
			memset(pic->control_map->index_data + ytotal + xl, control, xr-xl+1);

		oldytotal = ytotal;
		old_xr = xr;
		old_xl = xl;

	} while (1);
}


static void
_gfxr_auxbuf_fill(gfxr_pic_t *pic, int x, int y, int clipmask, int control)
{
	/* Fills the aux buffer and the control map (if control != 0) */
	int xl, xr;
	unsigned char fillmask = 0x78;
	int ytotal = y * 320;

	if (clipmask & 1)
		clipmask = 1; /* vis */
	else if (clipmask & 2)
		clipmask = 2; /* pri */
	else if (clipmask & 4)
		clipmask = 4; /* ctl */
	else return;

	clipmask |= fillmask; /* Bits 3-5 */

	if (pic->aux_map[ytotal + x] & clipmask)
		return;

	pic->aux_map[ytotal + x] |= fillmask;

	xl = x;
	while (xl && !(pic->aux_map[ytotal + xl - 1] & clipmask)) {
		--xl;
		pic->aux_map[ytotal + xl] |= fillmask;
	}

	xr = x;
	while ((xr < 319) && !(pic->aux_map[ytotal + xr + 1] & clipmask)) {
		++xr;
		pic->aux_map[ytotal + xr] |= fillmask;
	}

	if (control) /* Draw the same strip on the control map */
		memset(pic->control_map->index_data + ytotal + xl, control, xr - xl + 1);

	if (y > SCI_TITLEBAR_SIZE)
		_gfxr_auxbuf_fill_helper(pic, xl, xr, y, -1, clipmask, control);

	if (y < 199)
		_gfxr_auxbuf_fill_helper(pic, xl, xr, y, +1, clipmask, control);
}


static inline void
_gfxr_auxbuf_tag_line(gfxr_pic_t *pic, int pos, int width)
{
	int i;
	for (i = 0; i < width; i++)
		pic->aux_map[i+pos] |= FRESH_PAINT;
}


static void
_gfxr_auxbuf_spread(gfxr_pic_t *pic, int *min_x, int *min_y, int *max_x, int *max_y)
{
	/* Tries to spread by approximating the first derivation of the border function.
	** Draws to the current and the last line, thus taking up to twice as long as neccessary.
	** Other than that, it's O(n^2)
	*/

	int intervals_nr = 0, old_intervals_nr;
	int x, y, i, pos = 10*320;
	struct interval_struct {
		int xl, xr, tag;
	} intervals[2][160];

	*max_x = *max_y = -1;
	*min_x = *min_y = 320;

	if (!fillmagc) {
		fprintf(stderr,"------------------------------------------------\n");
		fprintf(stderr,"LineID:   ");
		for (i = 0; i < 5; i++)
			fprintf(stderr,"  %d       ", i);
		fprintf(stderr,"\n");
	}

	for (y = 10; y < 200; y++) {
		int ivi = y & 1; /* InterVal Index: Current intervals; !ivi is the list of old ones */
		int old_intervals_start_offset = 0;
		int width = 0;
		int j;

		old_intervals_nr = intervals_nr;
		intervals_nr = 0;

		for (x = 0; x < 321; x++)
			if (x < 320 && pic->aux_map[pos+x] & 0x10)
				width++;
			else if (width) { /* Found one interval */
				int xl = x - width;
				int xr = x - 1;
				int done = 0;
				int found_interval = 0;

				intervals[ivi][intervals_nr].xl = xl;
				intervals[ivi][intervals_nr].tag = 0;
				intervals[ivi][intervals_nr++].xr = xr;

				if (xl < *min_x)
					*min_x = xl;
				if (xr > *max_x)
					*max_x = xr;

				i = old_intervals_start_offset;
				while (!done && i < old_intervals_nr) {
					if (intervals[!ivi][i].xl > xr+1)
						done = 1;

					else if (intervals[!ivi][i].xr < xl-1) {
						int o_xl = intervals[!ivi][i].xl;
						int o_xr = intervals[!ivi][i].xr;
						if (o_xr == o_xl && !intervals[!ivi][i].tag) { /* thin bar */
							memcpy(intervals[ivi] + intervals_nr, intervals[ivi] + intervals_nr - 1, sizeof(struct interval_struct));
							memcpy(intervals[ivi] + intervals_nr - 1, intervals[!ivi] + i, sizeof(struct interval_struct));
							intervals[!ivi][i].tag = 1;
							pic->aux_map[pos - 320 + o_xl] |= FRESH_PAINT;
							++intervals_nr;
						}

						old_intervals_start_offset = i;
					}

					else {
						int k = i;
						int old_xl = intervals[!ivi][i].xl;
						int dwidth_l = abs(old_xl - xl);
						int old_xr, dwidth_r;
						int write_left_width, write_right_width;

						intervals[!ivi][i].tag = 1;
						while (k+1 < old_intervals_nr && intervals[!ivi][k+1].xl <= xr) {
							++k;
							intervals[!ivi][i].tag = 1;
						}

						old_xr = intervals[!ivi][k].xr;
						dwidth_r = abs(old_xr - xr);

						/* Current line */
						write_left_width = (dwidth_l > xl)? xl : dwidth_l;
						_gfxr_auxbuf_tag_line(pic, pos + xl - write_left_width, write_left_width);

						write_right_width = (dwidth_r + xr > 319)? 320 - xr : dwidth_r;
						_gfxr_auxbuf_tag_line(pic, pos + xr, write_right_width);

						if (xl - write_left_width < *min_x)
							*min_x = xl - write_left_width;
						if (xr + write_right_width > *max_x)
							*max_x = xr + write_right_width;

						/* Previous line */
						write_left_width = (dwidth_l > old_xl)? old_xl : dwidth_l;
						write_right_width = (dwidth_r + old_xr > 319)? 320 - old_xr : dwidth_r;

						if (i == k) { /* Only one predecessor interval */
							_gfxr_auxbuf_tag_line(pic, pos - 320 + old_xl - write_left_width, write_left_width);
							_gfxr_auxbuf_tag_line(pic, pos - 320 + old_xr, write_right_width);
						} else /* Fill entire line */
							_gfxr_auxbuf_tag_line(pic, pos - 320 + old_xl - write_left_width, old_xr - old_xl
									      + 1 + write_left_width + write_right_width);

						if (xl - write_left_width < *min_x)
							*min_x = xl - write_left_width;
						if (xr + write_right_width > *max_x)
							*max_x = xr + write_right_width;

						found_interval = done = 1;
					}
					i++;
				}
#if 0
				if (!found_interval && y) /* No 'parent' interval? */
					_gfxr_auxbuf_tag_line(pic, pos + xl - 320, xr - xl + 1);
#endif

				width = 0;
			}

		if (!fillmagc && intervals_nr) {
			fprintf(stderr,"AI L#%03d:", y);
			for (j = 0; j < intervals_nr; j++)
				fprintf(stderr, "%c[%03d,%03d]", intervals[ivi][j].tag? ' ':'-', intervals[ivi][j].xl, intervals[ivi][j].xr);
			fprintf(stderr,"\n");
		}

		if (intervals_nr) {
			if (y < *min_y)
				*min_y = y;
			*max_y = y;
		}

		pos += 320;
	}

	for (pos = 320*200 - 1; pos >= 320; pos--)
		if (pic->aux_map[pos - 320] & 0x40)
			pic->aux_map[pos] |= 0x40;

	if (*max_y < 199)
		(*max_y)++;
}



/*** Regular drawing operations ***/


#define PATTERN_FLAG_RECTANGLE 0x10
#define PATTERN_FLAG_USE_PATTERN 0x20

#define PIC_OP_FIRST 0xf0

enum {
	PIC_OP_SET_COLOR = 0xf0,
	PIC_OP_DISABLE_VISUAL = 0xf1,
	PIC_OP_SET_PRIORITY = 0xf2,
	PIC_OP_DISABLE_PRIORITY = 0xf3,
	PIC_OP_SHORT_PATTERNS = 0xf4,
	PIC_OP_MEDIUM_LINES = 0xf5,
	PIC_OP_LONG_LINES = 0xf6,
	PIC_OP_SHORT_LINES = 0xf7,
	PIC_OP_FILL = 0xf8,
	PIC_OP_SET_PATTERN = 0xf9,
	PIC_OP_ABSOLUTE_PATTERN = 0xfa,
	PIC_OP_SET_CONTROL = 0xfb,
	PIC_OP_DISABLE_CONTROL = 0xfc,
	PIC_OP_MEDIUM_PATTERNS = 0xfd,
	PIC_OP_OPX = 0xfe,
	PIC_OP_TERMINATE = 0xff
};

enum {
	PIC_OPX_SET_PALETTE_ENTRIES = 0,
	PIC_OPX_SET_PALETTE = 1,
	PIC_OPX_MONO0 = 2,
	PIC_OPX_MONO1 = 3,
	PIC_OPX_MONO2 = 4,
	PIC_OPX_MONO3 = 5,
	PIC_OPX_MONO4 = 6,
	PIC_OPX_EMBEDDED_VIEW,
	PIC_OPX_SET_PRIORITY_TABLE
};

#ifdef GFXR_DEBUG_PIC0
#define p0printf sciprintf
#else
#define p0printf if (0)
#endif


enum {
	ELLIPSE_SOLID, /* Normal filled ellipse */
	ELLIPSE_OR     /* color ORred to the buffer */
};

static void
_gfxr_fill_ellipse(gfxr_pic_t *pic, byte *buffer, int linewidth, int x, int y,
		   int rad_x, int rad_y, int color, int fillstyle)
{
	int xx = 0, yy = rad_y;
	int i, x_i, y_i;
	int xr = 2 * rad_x * rad_x;
	int yr = 2 * rad_y * rad_y;

	x_i = 1;
	y_i = xr * rad_y -1;
	i = y_i >> 1;

	while (yy >= 0) {
		int oldxx = xx;
		int oldyy = yy;

		if (i >= 0) {
			x_i += yr;
			i -= x_i + 1;
			++xx;
		}

		if (i < 0) {
			y_i -= xr;
			i += y_i - 1;
			--yy;
		}

		if (oldyy != yy) {
			int j;
			int offset0 = (y-oldyy) * linewidth;
			int offset1 = (y+oldyy) * linewidth;

			offset0 += x-oldxx;
			offset1 += x-oldxx;

			if (oldyy == 0)
				offset1 = 0; /* We never have to draw ellipses in the menu bar */

			oldyy = yy;

			switch (fillstyle) {

			case ELLIPSE_SOLID:
				memset(buffer + offset0, color, (oldxx << 1) + 1);
				if (offset1)
					memset(buffer + offset1, color, (oldxx << 1) + 1);
				break;

			case ELLIPSE_OR:
				for (j=0; j < (oldxx << 1) + 1; j++) {
					buffer[offset0 + j] |= color;
					if (offset1)
						buffer[offset1 + j] |= color;
				}
				break;

			default:
				fprintf(stderr,"%s L%d: Invalid ellipse fill mode!\n", __FILE__, __LINE__);
				return;

			}
		}
	}
}

static inline void
_gfxr_auxplot_brush(gfxr_pic_t *pic, byte *buffer, int yoffset, int offset, int plot,
		    int color, gfx_brush_mode_t brush_mode, int randseed)
{
	/* yoffset 63680, offset 320, plot 1, color 34, brush_mode 0, randseed 432)*/
	/* Auxplot: Used by plot_aux_pattern to plot to visual and priority */
	int xc, yc;
	int line_width = 320 * pic->mode->xfact;
	int full_offset = (yoffset * pic->mode->yfact + offset) * pic->mode->xfact;

	if (yoffset + offset >= 64000) {
		BREAKPOINT();
	}

	switch (brush_mode) {
	case GFX_BRUSH_MODE_SCALED:
		if (plot)
			for (yc = 0; yc < pic->mode->yfact; yc++) {
				memset(buffer + full_offset, color, pic->mode->xfact);
				full_offset += line_width;
			}
		break;

	case GFX_BRUSH_MODE_ELLIPSES:
		if (plot) {
			int x = offset * pic->mode->xfact + ((pic->mode->xfact -1) >> 1);
			int y = (yoffset / 320) * pic->mode->yfact + ((pic->mode->yfact -1) >> 1); /* Ouch! */

			_gfxr_fill_ellipse(pic, buffer, line_width, x, y, pic->mode->xfact >> 1, pic->mode->yfact >> 1,
					   color, ELLIPSE_SOLID);
		}
		break;

	case GFX_BRUSH_MODE_RANDOM_ELLIPSES:
		if (plot) {
			int x = offset * pic->mode->xfact + ((pic->mode->xfact -1) >> 1);
			int y = (yoffset / 320) * pic->mode->yfact + ((pic->mode->yfact -1) >> 1); /* Ouch! */
			int sizex = pic->mode->xfact >> 1;
			int sizey = pic->mode->yfact >> 1;

			srand(randseed);

			x -= (sizex * rand()*1.0)/(RAND_MAX + 1.0);
			x += (sizex * rand()*1.0)/(RAND_MAX + 1.0);
			y -= (sizey * rand()*1.0)/(RAND_MAX + 1.0);
			y += (sizey * rand()*1.0)/(RAND_MAX + 1.0);
			sizex = (sizex * rand()*1.0)/(RAND_MAX + 1.0);
			sizey = (sizey * rand()*1.0)/(RAND_MAX + 1.0);

			_gfxr_fill_ellipse(pic, buffer, line_width, x, y, pic->mode->xfact >> 1, pic->mode->yfact >> 1,
					   color, ELLIPSE_SOLID);
			srand(time(NULL)); /* Make sure we don't accidently forget to re-init the random number generator */
		}
		break;

	case GFX_BRUSH_MODE_MORERANDOM: {
		int mask = plot? 7 : 1;
		srand(randseed);
		for (yc = 0; yc < pic->mode->yfact; yc++) {
			for (xc = 0; xc < pic->mode->xfact; xc++)
				if ((rand() & 7) < mask)
					buffer[full_offset + xc] = color;
			full_offset += line_width;
		}
		srand(time(NULL)); /* Make sure we don't accidently forget to re-init the random number generator */
	}
	break;
	}
}

#define PLOT_AUX_PATTERN_NO_RANDOM -1

static void
_gfxr_plot_aux_pattern(gfxr_pic_t *pic, int x, int y, int size, int circle, int random,
		       int mask, int color, int priority, int control,
		       gfx_brush_mode_t brush_mode, int map_nr)
{
	/* Plots an appropriate pattern to the aux buffer and the control buffer,
	** if mask & GFX_MASK_CONTROL
	** random should be set to the random index, or -1 to disable
	*/

	/* These circle offsets uniquely identify the circles used by Sierra: */
	int circle_data[][8] = {
		{0},
		{1, 0},
		{2, 2, 1},
		{3, 3, 2, 1},
		{4, 4, 4, 3, 1},
		{5, 5, 4, 4, 3, 1},
		{6, 6, 6, 5, 5, 4, 2},
		{7, 7, 7, 6, 6, 5, 4, 2}};

	/* 'Random' fill patterns, provided by Carl Muckenhoupt: */
	byte random_data[32] = {
		0x20, 0x94, 0x02, 0x24, 0x90, 0x82, 0xa4, 0xa2, 0x82, 0x09, 0x0a, 0x22,
		0x12, 0x10, 0x42, 0x14, 0x91, 0x4a, 0x91, 0x11, 0x08, 0x12, 0x25, 0x10,
		0x22, 0xa8, 0x14, 0x24, 0x00, 0x50, 0x24, 0x04};

	/* 'Random' fill offsets, provided by Carl Muckenhoupt: */
	byte random_offset[128] = {
		0x00, 0x18, 0x30, 0xc4, 0xdc, 0x65, 0xeb, 0x48,
		0x60, 0xbd, 0x89, 0x05, 0x0a, 0xf4, 0x7d, 0x7d,
		0x85, 0xb0, 0x8e, 0x95, 0x1f, 0x22, 0x0d, 0xdf,
		0x2a, 0x78, 0xd5, 0x73, 0x1c, 0xb4, 0x40, 0xa1,
		0xb9, 0x3c, 0xca, 0x58, 0x92, 0x34, 0xcc, 0xce,
		0xd7, 0x42, 0x90, 0x0f, 0x8b, 0x7f, 0x32, 0xed,
		0x5c, 0x9d, 0xc8, 0x99, 0xad, 0x4e, 0x56, 0xa6,
		0xf7, 0x68, 0xb7, 0x25, 0x82, 0x37, 0x3a, 0x51,
		0x69, 0x26, 0x38, 0x52, 0x9e, 0x9a, 0x4f, 0xa7,
		0x43, 0x10, 0x80, 0xee, 0x3d, 0x59, 0x35, 0xcf,
		0x79, 0x74, 0xb5, 0xa2, 0xb1, 0x96, 0x23, 0xe0,
		0xbe, 0x05, 0xf5, 0x6e, 0x19, 0xc5, 0x66, 0x49,
		0xf0, 0xd1, 0x54, 0xa9, 0x70, 0x4b, 0xa4, 0xe2,
		0xe6, 0xe5, 0xab, 0xe4, 0xd2, 0xaa, 0x4c, 0xe3,
		0x06, 0x6f, 0xc6, 0x4a, 0xa4, 0x75, 0x97, 0xe1
	};

	int offset = 0, width = 0;
	int yoffset = (y - size) * 320;
	int i;
	int random_index = 0;
	gfx_pixmap_t *map = NULL;

	switch (map_nr) {
	case GFX_MASK_VISUAL: map = pic->visual_map; break;
	case GFX_MASK_PRIORITY: map = pic->priority_map; break;
	default: map = pic->control_map; break;
	}

	if (random >= 0)
		random_index = random_offset[random];

	if (!circle) {
		offset = -size;
		width = (size << 1) + 2;
	}

	for (i = -size; i <= size; i++) {
		int j;
		int height;

		if (circle) {
			offset = circle_data[size][abs(i)];
			height = width = (offset << 1) + 1;
			offset = -offset;
		} else height = width - 1;

		if (random == PLOT_AUX_PATTERN_NO_RANDOM) {

			if (mask & map_nr)
				memset(map->index_data + yoffset + offset + x, control, width);

			if (map_nr == GFX_MASK_CONTROL)
				for (j = x; j < x + width; j++)
					pic->aux_map[yoffset + offset + j] |= mask;

		} else { /* Semi-Random! */
			for (j = 0; j < height; j++) {
				if (random_data[random_index >> 3] & (0x80 >> (random_index & 7))) {
					/* The 'seemingly' random decision */
					if (mask & GFX_MASK_CONTROL)
						pic->control_map->index_data[yoffset + x + offset + j] = control;

					pic->aux_map[yoffset + x + offset + j] |= mask;

					if (mask & GFX_MASK_VISUAL)
						_gfxr_auxplot_brush(pic, pic->visual_map->index_data,
								    yoffset, x + offset + j,
								    1, color, brush_mode, random_index + x);

					if (mask & GFX_MASK_PRIORITY)
						_gfxr_auxplot_brush(pic, pic->priority_map->index_data,
								    yoffset, x + offset + j,
								    1, priority, brush_mode, random_index + x);

				} else {
					if (mask & GFX_MASK_VISUAL)
						_gfxr_auxplot_brush(pic, pic->visual_map->index_data,
								    yoffset, x + offset + j,
								    0, color, brush_mode, random_index + x);

					if (mask & GFX_MASK_PRIORITY)
						_gfxr_auxplot_brush(pic, pic->priority_map->index_data,
								    yoffset, x + offset + j,
								    0, priority, brush_mode, random_index + x);
				}
				random_index = (random_index + 1) & 0xff;
			}
		}

		yoffset += 320;
	}
}


static void
_gfxr_draw_pattern(gfxr_pic_t *pic, int x, int y, int color, int priority, int control, int drawenable,
		   int pattern_code, int pattern_size, int pattern_nr, int brush_mode)
{
	int xsize = (pattern_size + 1) * pic->mode->xfact - 1;
	int ysize = (pattern_size + 1) * pic->mode->yfact - 1;
	int scaled_x, scaled_y;
	rect_t boundaries;
	int max_x = (pattern_code & PATTERN_FLAG_RECTANGLE)? 318 : 319; /* Rectangles' width is size+1 */

	p0printf(stderr, "Pattern at (%d,%d) size %d, rand=%d, code=%02x\n", x, y, pattern_size, pattern_nr, pattern_code);

	y += SCI_TITLEBAR_SIZE;

	if (x - pattern_size < 0)
		x = pattern_size;

	if (y - pattern_size < SCI_TITLEBAR_SIZE)
		y = SCI_TITLEBAR_SIZE + pattern_size;

	if (x + pattern_size > max_x)
		x = max_x - pattern_size;

	if (y + pattern_size > 199)
		y = 199 - pattern_size;

	scaled_x = x * pic->mode->xfact + ((pic->mode->xfact - 1) >> 1);
	scaled_y = y * pic->mode->yfact + ((pic->mode->yfact - 1) >> 1);

	if (scaled_x < xsize)
		scaled_x = xsize;

	if (scaled_y < ysize + SCI_TITLEBAR_SIZE * pic->mode->yfact)
		scaled_y = ysize + SCI_TITLEBAR_SIZE * pic->mode->yfact;

	if (scaled_x > (320 * pic->mode->xfact) - 1 - xsize)
		scaled_x = (320 * pic->mode->xfact) - 1 - xsize;

	if (scaled_y > (200 * pic->mode->yfact) - 1 - ysize)
		scaled_y = (200 * pic->mode->yfact) - 1 - ysize;

	if (pattern_code & PATTERN_FLAG_RECTANGLE) {
		/* Rectangle */
		boundaries.x = scaled_x - xsize;
		boundaries.y = scaled_y - ysize;
		boundaries.xl = ((xsize + 1) << 1) + 1;
		boundaries.yl = (ysize << 1) + 1;


		if (pattern_code & PATTERN_FLAG_USE_PATTERN) {
			_gfxr_plot_aux_pattern(pic, x, y, pattern_size, 0, pattern_nr,
					       drawenable, color, priority,
					       control, brush_mode, GFX_MASK_CONTROL);
		} else {

			_gfxr_plot_aux_pattern(pic, x, y, pattern_size, 0,
					       PLOT_AUX_PATTERN_NO_RANDOM,
					       drawenable, 0, 0, control, 0, GFX_MASK_CONTROL);

			if (drawenable & GFX_MASK_VISUAL)
				gfx_draw_box_pixmap_i(pic->visual_map, boundaries, color);

			if (drawenable & GFX_MASK_PRIORITY)
				gfx_draw_box_pixmap_i(pic->priority_map, boundaries, priority);
		}

	} else {
		/* Circle */

		if (pattern_code & PATTERN_FLAG_USE_PATTERN) {

			_gfxr_plot_aux_pattern(pic, x, y, pattern_size, 1, pattern_nr,
					       drawenable, color, priority,
					       control, brush_mode, GFX_MASK_CONTROL);
		} else {

			_gfxr_plot_aux_pattern(pic, x, y, pattern_size, 1,
					       PLOT_AUX_PATTERN_NO_RANDOM,
					       drawenable, 0, 0, control, 0, GFX_MASK_CONTROL);

			if (pic->mode->xfact == 1 && pic->mode->yfact == 1) {

				if (drawenable & GFX_MASK_VISUAL)
					_gfxr_plot_aux_pattern(pic, x, y, pattern_size, 1,
							       PLOT_AUX_PATTERN_NO_RANDOM,
							       drawenable, 0, 0, color, 0, GFX_MASK_VISUAL);

				if (drawenable & GFX_MASK_PRIORITY)
					_gfxr_plot_aux_pattern(pic, x, y, pattern_size, 1,
							       PLOT_AUX_PATTERN_NO_RANDOM,
							       drawenable, 0, 0, priority, 0, GFX_MASK_PRIORITY);
			} else {

				if (drawenable & GFX_MASK_VISUAL)
					_gfxr_fill_ellipse(pic, pic->visual_map->index_data, 320 * pic->mode->xfact,
							   scaled_x, scaled_y, xsize, ysize,
							   color, ELLIPSE_SOLID);

				if (drawenable & GFX_MASK_PRIORITY)
					_gfxr_fill_ellipse(pic, pic->priority_map->index_data, 320 * pic->mode->xfact,
							   scaled_x, scaled_y, xsize, ysize,
							   priority, ELLIPSE_SOLID);
			}
		}
	}
}


static inline void
_gfxr_draw_subline(gfxr_pic_t *pic, int x, int y, int ex, int ey, int color, int priority, int drawenable)
{
	rect_t line;

	line.x = x;
	line.y = y;
	line.xl = ex - x;
	line.yl = ey - y;
        if (ex >= pic->visual_map->index_xl || ey >= pic->visual_map->index_yl || x < 0 || y < 0) {
                fprintf(stderr,"While drawing pic0: INVALID LINE %d,%d,%d,%d\n",
                        GFX_PRINT_RECT(line));
                return;
        }

	if (drawenable & GFX_MASK_VISUAL)
		gfx_draw_line_pixmap_i(pic->visual_map, line, color);

	if (drawenable & GFX_MASK_PRIORITY)
		gfx_draw_line_pixmap_i(pic->priority_map, line, priority);

}

static void
_gfxr_draw_line(gfxr_pic_t *pic, int x, int y, int ex, int ey, int color,
		int priority, int control, int drawenable, int line_mode,
		int cmd)
{
	int scale_x = pic->mode->xfact;
	int scale_y = pic->mode->yfact;
	int xc, yc;
	rect_t line;
	int mask;
	int partially_white = (drawenable & GFX_MASK_VISUAL)
		&& (((color & 0xf0) == 0xf0) || ((color & 0x0f) == 0x0f));

	line.x = x;
	line.y = y;
	line.xl = ex - x;
	line.yl = ey - y;

	if (x > 319 || y > 199 || x < 0 || y < 0
	    || ex > 319 || ey > 199 || ex < 0 || ey < 0) {
		GFXWARN("While building pic: Attempt to draw line (%d,%d) to (%d,%d): cmd was %d\n", x, y, ex, ey, cmd);
		return;
	}

	y += SCI_TITLEBAR_SIZE;
	ey += SCI_TITLEBAR_SIZE;

	if (drawenable & GFX_MASK_CONTROL) {

		p0printf(" ctl:%x", control);
		gfx_draw_line_pixmap_i(pic->control_map, gfx_rect(x, y, line.xl, line.yl), control);
	}


	/* Calculate everything that is changed to SOLID */
	mask = drawenable &
		(
			((color != 0xff)? 1 : 0)
			| ((priority)? 2 : 0)
			| ((control)? 4 : 0)
			);

	if (mask) {
		int mask2 = mask;
		if (partially_white)
			mask2 = mask &= ~GFX_MASK_VISUAL;
		_gfxr_auxbuf_line_draw(pic, line, mask, mask2);
	}

	/* Calculate everything that is changed to TRANSPARENT */
	mask = drawenable &
		(
			((color == 0xff)? 1 : 0)
			| ((!priority)? 2 : 0)
			| ((!control)? 4 : 0)
			);

	if (mask)
		_gfxr_auxbuf_line_clear(pic, line, ~mask);

	x *= scale_x;
	y *= scale_y;
	ex *= scale_x;
	ey *= scale_y;

	if (drawenable & GFX_MASK_VISUAL)
		p0printf(" col:%02x", color);

	if (drawenable & GFX_MASK_PRIORITY)
		p0printf(" pri:%x", priority);

	if (line_mode == GFX_LINE_MODE_FINE) {  /* Adjust lines to extend over the full visual */
		x = (x * ((320 + 1) * scale_x - 1)) / (320 * scale_x);
		y = (y * ((200 + 1) * scale_y - 1)) / (200 * scale_y);
		ex = (ex * ((320 + 1) * scale_x - 1)) / (320 * scale_x);
		ey = (ey * ((200 + 1) * scale_y - 1)) / (200 * scale_y);

		_gfxr_draw_subline(pic, x, y, ex, ey, color, priority, drawenable);
	} else {
		if (x == ex && y == ey) { /* Just one single point? */
			rect_t drawrect;
			drawrect.x = x;
			drawrect.y = y;
			drawrect.xl = scale_x;
			drawrect.yl = scale_y;

			if (drawenable & GFX_MASK_VISUAL)
				gfx_draw_box_pixmap_i(pic->visual_map, drawrect, color);

			if (drawenable & GFX_MASK_PRIORITY)
				gfx_draw_box_pixmap_i(pic->priority_map, drawrect, priority);

		} else {
			int width = scale_x;
			int height = scale_y;
			int x_offset = 0;
			int y_offset = 0;

			if (line_mode == GFX_LINE_MODE_FAST) {
				width = (width + 1) >> 1;
				height = (height + 1) >> 1;
				x_offset = (width >> 1);
				y_offset = (height >> 1);
			}

			for (xc = 0; xc < width; xc++)
				_gfxr_draw_subline(pic,
						   x + xc + x_offset, y + y_offset,
						   ex + xc + x_offset, ey + y_offset,
						   color, priority, drawenable);

			if (height > 0)
				for (xc = 0; xc < width; xc++)
					_gfxr_draw_subline(pic,
							   x + xc + x_offset, y + height - 1 + y_offset,
							   ex + xc + x_offset, ey + height - 1 + y_offset,
							   color, priority, drawenable);

			if (height > 1) {
				for (yc = 1; yc < height - 1; yc++)
					_gfxr_draw_subline(pic,
							   x + x_offset, y + yc + y_offset,
							   ex + x_offset, ey + yc + y_offset,
							   color, priority, drawenable);
				if (width > 0)
					for (yc = 1; yc < height - 1; yc++)
						_gfxr_draw_subline(pic,
								   x + width - 1 + x_offset, y + yc + y_offset,
								   ex + width - 1 + x_offset, ey + yc + y_offset,
								   color, priority, drawenable);
			}
		}
	}

	p0printf("\n");
}


#define IS_FILL_BOUNDARY(x) (((x) & legalmask) != legalcolor)

static void
_gfxr_fill_recursive(gfxr_pic_t *pic, int old_xl, int old_xr, int y, int dy, byte *bounds,
		     int legalcolor, int legalmask, int color, int priority, int drawenable)
{
	int linewidth = pic->mode->xfact * 320;
	int miny = pic->mode->yfact * SCI_TITLEBAR_SIZE;
	int maxy = pic->mode->yfact * 200;
	int xl, xr;
	int oldytotal = y * linewidth;
	int old_proj_y = -42;
	int proj_y;
	int proj_ytotal;
	int proj_x;
	int proj_xl_bound = 0;
	int proj_xr_bound = 0;

	do {
		int ytotal = oldytotal + (linewidth * dy);
		int xcont;
		int state;

		y += dy;
		proj_y = y / pic->mode->yfact;

		if (!fillc)
			return;
		else if (!fillmagc) { --fillc;
		}

		if (y < miny || y >= maxy) {
#ifdef FILL_RECURSIVE_DEBUG
			if (!fillmagc)
				fprintf(stderr,"ABRT on failed initial assertion!\n");
#endif
			return;
		}

		if (proj_y != old_proj_y) {
			/* First, find the projected coordinates, unless known already: */
			proj_ytotal = proj_y * 320;
			proj_x = old_xl / pic->mode->xfact;

			proj_xl_bound = proj_x;
			if (pic->aux_map[proj_ytotal + proj_xl_bound] & FRESH_PAINT) {
				while (proj_xl_bound &&
				       (pic->aux_map[proj_ytotal + proj_xl_bound - 1] & FRESH_PAINT))
					--proj_xl_bound;
			} else {
				while (proj_xl_bound < 319 &&
				       !(pic->aux_map[proj_ytotal + proj_xl_bound + 1] & FRESH_PAINT))
					++proj_xl_bound;

				if (proj_xl_bound < 319)
					++proj_xl_bound;
			}

			if (proj_xl_bound == 319 && !(pic->aux_map[proj_ytotal + proj_xl_bound] & FRESH_PAINT)) {
#ifdef FILL_RECURSIVE_DEBUG
				if (!fillmagc)
					fprintf(stderr,"ABRT because proj_xl_bound couldn't be found\n");
#endif
				return;
			}

			proj_xr_bound = (proj_xl_bound > proj_x)? proj_xl_bound : proj_x;
			while ((proj_xr_bound < 319) &&
			       (pic->aux_map[proj_ytotal + proj_xr_bound + 1] & FRESH_PAINT))
				++proj_xr_bound;

			if (!fillmagc) {
				fprintf(stderr,"l%d: {%d,%d} | ", proj_y, proj_xl_bound, proj_xr_bound);
				pic->aux_map[proj_y*320 + proj_xl_bound] |= 0x2;
				pic->aux_map[proj_y*320 + proj_xr_bound] |= 0x2;
			}

			proj_xl_bound *= pic->mode->xfact;
			if (proj_xl_bound)
				proj_xl_bound -= pic->mode->xfact - 1;

			if (proj_xr_bound < 319)
				++proj_xr_bound;
			proj_xr_bound *= pic->mode->xfact;
			proj_xr_bound += pic->mode->xfact -1;

			old_proj_y = proj_y;
		}

		/* Now we have the projected limits, get the real ones: */

		xl = (old_xl > proj_xl_bound)? old_xl : proj_xl_bound;
		if (!IS_FILL_BOUNDARY(bounds[ytotal + xl])) { /* go left as far as possible */
			while (xl > proj_xl_bound && (!IS_FILL_BOUNDARY(bounds[ytotal + xl - 1])))
				--xl;
		} else /* go right until the fillable area starts */
			while (xl < proj_xr_bound && (IS_FILL_BOUNDARY(bounds[ytotal + xl])))
				++xl;


#ifdef FILL_RECURSIVE_DEBUG
		if (!fillmagc)
			fprintf(stderr,"<%d,", xl);
#endif

		if ((xl > proj_xr_bound)
		    || (xl > old_xr)) {
#ifdef FILL_RECURSIVE_DEBUG
			if (!fillmagc)
				fprintf(stderr,"ABRT because xl > xr_bound\n");
#endif
			return;
		}

		xr = (xl > old_xl)? xl : old_xl;
		while (xr < proj_xr_bound && (!IS_FILL_BOUNDARY(bounds[ytotal + xr + 1])))
			++xr;

#ifdef FILL_RECURSIVE_DEBUG
		if (!fillmagc)
			fprintf(stderr,"%d> -> ", xr);
#endif

		if (IS_FILL_BOUNDARY(bounds[ytotal + xl])) {
#ifdef FILL_RECURSIVE_DEBUG
			if (!fillmagc)
				fprintf(stderr,"ABRT because xl illegal\n");
#endif
			return;
		}

		if (!fillmagc)
			fprintf(stderr,"[%d[%d,%d]%d]\n", proj_xl_bound, xl, xr, proj_xr_bound);

		if (xl < proj_xl_bound && xr - 3*pic->mode->xfact < proj_xl_bound) {
#ifdef FILL_RECURSIVE_DEBUG
			if (!fillmagc)
				fprintf(stderr,"ABRT interval left of zone\n");
#endif
			return;
		}

		if (xr > proj_xr_bound && xl + 3*pic->mode->xfact > proj_xr_bound) {
#ifdef FILL_RECURSIVE_DEBUG
			if (!fillmagc)
				fprintf(stderr,"ABRT because interval right of zone\n");
#endif
			return;
		}

		if (drawenable & GFX_MASK_VISUAL)
			memset(pic->visual_map->index_data + ytotal + xl, color, xr - xl + 1);

		if (drawenable & GFX_MASK_PRIORITY)
			memset(pic->priority_map->index_data + ytotal + xl, priority, xr - xl + 1);


		/* Check whether we need to recurse on branches in the same direction */
		state = 0;
		xcont = xr + 1;
		while (xcont <= old_xr) {
			if (IS_FILL_BOUNDARY(bounds[ytotal + xcont]))
				state = xcont;
			else if (state) { /* recurse */
#ifdef FILL_RECURSIVE_DEBUG
				if (!fillmagc)
					fprintf(stderr, "[%d[%d,%d],%d]: rec BRANCH %d [%d,%d] l%d\n", old_xl, xl, xr, old_xr, dy,
						state, xcont, y -dy);
#endif
				_gfxr_fill_recursive(pic, state, xcont, y - dy, dy, bounds, legalcolor,
						     legalmask, color, priority, drawenable);
				state = 0;
			}
			++xcont;
		}

		/* Check whether we need to recurse on backward branches: */
		/* left */
		if (xl < old_xl - 1) {
			state = 0;
			for (xcont = old_xl-1; xcont >= xl; xcont--) {
				if (IS_FILL_BOUNDARY(bounds[oldytotal + xcont]))
					state = xcont;
				else if (state) { /* recurse */
#ifdef FILL_RECURSIVE_DEBUG
					if (!fillmagc)
						fprintf(stderr, "[%d[%d,%d],%d]: rec BACK-LEFT %d [%d,%d] l%d\n", old_xl, xl, xr, old_xr, -dy,
							xcont, state, y);
#endif
					_gfxr_fill_recursive(pic, xcont, state, y, -dy, bounds,
							     legalcolor, legalmask, color, priority, drawenable);
					state = 0;
				}
			}
		}

		/* right */
		if (xr > old_xr + 1) {
			state = 0;
			for (xcont = old_xr + 1; xcont <= xr; xcont++) {
				if (IS_FILL_BOUNDARY(bounds[oldytotal + xcont]))
					state = xcont;
				else if (state) { /* recurse */
#ifdef FILL_RECURSIVE_DEBUG
					if (!fillmagc)
						fprintf(stderr, "[%d[%d,%d],%d]: rec BACK-RIGHT %d [%d,%d] l%d\n", old_xl, xl, xr, old_xr, -dy,
							state, xcont, y);
#endif
					_gfxr_fill_recursive(pic, state, xcont, y, -dy, bounds,
							     legalcolor, legalmask, color, priority, drawenable);
					state = 0;
				}
			}
		}

		oldytotal = ytotal;
		old_xl = xl;
		old_xr = xr;

	} while (1);
}



#define TEST_POINT(xx, yy) \
  if (pic->aux_map[(yy)*320 + (xx)] & FRESH_PAINT) { \
    mpos = (((yy) * 320 * pic->mode->yfact) + (xx)) * pic->mode->xfact; \
    for (iy = 0; iy < pic->mode->yfact; iy++) { \
      for (ix = 0; ix < pic->mode->xfact; ix++) \
	if (!IS_FILL_BOUNDARY(test_map[mpos + ix])) { \
	  *x = ix + (xx) * pic->mode->xfact; \
	  *y = iy + (yy) * pic->mode->yfact; \
	  return 0; \
	} \
      mpos += linewidth; \
    } \
  }

static inline int /* returns -1 on failure, 0 on success */
_gfxr_find_fill_point(gfxr_pic_t *pic, int min_x, int min_y, int max_x, int max_y, int x_320,
		      int y_200, int color, int drawenable, int *x, int *y)
{
	int linewidth = pic->mode->xfact * 320;
	int mpos, ix, iy;
	int size_x = (max_x - min_x + 1) >> 1;
	int size_y = (max_y - min_y + 1) >> 1;
	int mid_x = min_x + size_x;
	int mid_y = min_y + size_y;
	int max_size = (size_x > size_y)? size_x : size_y;
	int size;
	int legalcolor;
	int legalmask;
	byte *test_map;
	*x = x_320 * pic->mode->xfact;
	*y = y_200 * pic->mode->yfact;

	if (size_x < 0 || size_y < 0)
		return 0;

	if (drawenable & GFX_MASK_VISUAL) {
		test_map = pic->visual_map->index_data;

		if ((color & 0xf) == 0xf /* When dithering with white, do more
					 ** conservative checks  */
		    || (color & 0xf0) == 0xf0)
			legalcolor = 0xff;
		else
			legalcolor = 0xf0; /* Only check the second color */

		legalmask = legalcolor;
	} else if (drawenable & GFX_MASK_PRIORITY) {
		test_map = pic->priority_map->index_data;
		legalcolor = 0;
		legalmask = 0xf;
	} else return -3;

	TEST_POINT(x_320, y_200); /* Most likely candidate */
	TEST_POINT(mid_x, mid_y); /* Second most likely candidate */

	for (size = 1; size <= max_size; size++) {
		int i;

		if (size <= size_y) {
			int limited_size = (size > size_x)? size_x : size;

			for (i = mid_x - limited_size; i <= mid_x + limited_size; i++) {
				TEST_POINT(i, mid_y - size);
				TEST_POINT(i, mid_y + size);
			}
		}

		if (size <= size_x) {
			int limited_size = (size - 1 > size_y)? size_y : size - 1;

			for (i = mid_y - limited_size; i <= mid_y + limited_size; i++) {
				TEST_POINT(mid_x - size, i);
				TEST_POINT(mid_x + size, i);
			}
		}
	}

	return -1;
}

#undef TEST_POINT

static void
_gfxr_fill(gfxr_pic_t *pic, int x_320, int y_200, int color, int priority, int control, int drawenable)
{
	int linewidth = pic->mode->xfact * 320;
	int x, y;
	int xl, xr;
	int ytotal;
	int bitmask;
	byte *bounds = NULL;
	int legalcolor, legalmask;
	int min_x, min_y, max_x, max_y;

	if (pic->control_map->index_data[y_200 * 320 + x_320] != 0)
		drawenable &= ~GFX_MASK_CONTROL;

	if (color == 0xff)
		drawenable &= ~GFX_MASK_VISUAL;

	if (priority == 0)
		drawenable &= ~GFX_MASK_PRIORITY;

	_gfxr_auxbuf_fill(pic, x_320, y_200, drawenable, (drawenable & GFX_MASK_CONTROL)? control : 0);
	_gfxr_auxbuf_spread(pic, &min_x, &min_y, &max_x, &max_y);

	if (_gfxr_find_fill_point(pic, min_x, min_y, max_x, max_y, x_320, y_200, color, drawenable, &x, &y)) {
		/* GFXWARN("Could not find scaled fill point, but unscaled fill point was available!\n"); */
		drawenable &= GFX_MASK_PRIORITY;
		if (!drawenable)
			_gfxr_auxbuf_propagate_changes(pic, 0);
	}

	ytotal = y * linewidth;

	if (!drawenable)
		return;

	if (drawenable & GFX_MASK_VISUAL) {
		bounds = pic->visual_map->index_data;
		if ((color & 0xf) == 0xf /* When dithering with white, do more
					 ** conservative checks  */
		    || (color & 0xf0) == 0xf0)
			legalcolor = 0xff;
		else
			legalcolor = 0xf0; /* Only check the second color */

		legalmask = legalcolor;
	} else if (drawenable & GFX_MASK_PRIORITY) {
		bounds = pic->priority_map->index_data;
		legalcolor = 0;
		legalmask = 0xf;
	} else {
		legalcolor = 0;
		legalmask = 0xf;
	}

	if (!bounds || IS_FILL_BOUNDARY(bounds[ytotal + x]))
		return;

	if (bounds) {
		int proj_y = y_200;
		int proj_ytotal = proj_y * 320;
		int proj_x = x_320;
		int proj_xl_bound;
		int proj_xr_bound;
		int proj_xl, proj_xr;

		ytotal = y * linewidth;

		proj_xl_bound = proj_x;
		if (pic->aux_map[proj_ytotal + proj_xl_bound] & FRESH_PAINT) {
			while (proj_xl_bound &&
			       (pic->aux_map[proj_ytotal + proj_xl_bound - 1] & FRESH_PAINT))
				--proj_xl_bound;
		} else
			while (proj_xl_bound < 319 &&
			       !(pic->aux_map[proj_ytotal + proj_xl_bound + 1] & FRESH_PAINT))
				++proj_xl_bound;

		proj_xr_bound = (proj_xl_bound > proj_x)? proj_xl_bound : proj_x;
		while ((proj_xr_bound < 319) &&
		       (pic->aux_map[proj_ytotal + proj_xr_bound + 1] & FRESH_PAINT))
			++proj_xr_bound;

		proj_xl = proj_xl_bound;
		proj_xr = proj_xr_bound;

		proj_xl_bound *= pic->mode->xfact;
		if (proj_xl_bound)
			proj_xl_bound -= pic->mode->xfact -1;

		if (proj_xr_bound < 319)
			++proj_xr_bound;
		proj_xr_bound *= pic->mode->xfact;
		proj_xr_bound += pic->mode->xfact -1;

		xl = x;
		while (xl > proj_xl_bound && (!IS_FILL_BOUNDARY(bounds[ytotal + xl -1])))
			--xl;

		while (x < proj_xr_bound && (!IS_FILL_BOUNDARY(bounds[ytotal + x + 1])))
			++x;
		xr = x;

		if (drawenable & GFX_MASK_VISUAL)
			memset(pic->visual_map->index_data + ytotal + xl, color, xr - xl + 1);

		if (drawenable & GFX_MASK_PRIORITY)
			memset(pic->priority_map->index_data + ytotal + xl, priority, xr - xl + 1);

		_gfxr_fill_recursive(pic, xl, xr, y, -1, bounds, legalcolor, legalmask, color, priority, drawenable);
		_gfxr_fill_recursive(pic, xl, xr, y, +1, bounds, legalcolor, legalmask, color, priority, drawenable);
	}

	/* Now finish the aux buffer */
	bitmask = drawenable &
		(
			((color != 0xff)? 1 : 0)
			| ((priority)? 2 : 0)
			| ((control)? 4 : 0)
			);
	if (fillmagc)
		_gfxr_auxbuf_propagate_changes(pic, bitmask);
}


#define GET_ABS_COORDS(x, y) \
  temp = *(resource + pos++); \
  x = *(resource + pos++); \
  y = *(resource + pos++); \
  x |= (temp & 0xf0) << 4; \
  y |= (temp & 0x0f) << 8;

#define GET_REL_COORDS(x, y) \
  temp = *(resource + pos++); \
  if (temp & 0x80) \
    x -= ((temp >> 4) & 0x7); \
  else \
    x += (temp >> 4); \
  \
  if (temp & 0x08) \
    y -= (temp & 0x7); \
  else \
    y += (temp & 0x7);

#define GET_MEDREL_COORDS(oldx, oldy) \
  temp = *(resource + pos++); \
  if (temp & 0x80) \
    y = oldy - (temp & 0x7f); \
  else \
    y = oldy + temp; \
  x = oldx + *((signed char *) resource + pos++);


static void
_gfxr_vismap_remove_artifacts_old(gfxr_pic_t *pic)
{
	/* Check the visual map for things that look like artifacs and remove them,
	** if that appears to be appropriate.
	*/
	int x,y;
	int linewidth = 320 * pic->mode->xfact;
	int maxx = 320 * pic->mode->xfact - 1;
	int maxy = 200 * pic->mode->yfact - 1;
	int miny = SCI_TITLEBAR_SIZE * pic->mode->yfact;
	int offset = linewidth * miny;
	byte *vismap = pic->visual_map->index_data;
	byte *primap = pic->priority_map->index_data;

	if (pic->mode->xfact == 1 || pic->mode->yfact == 1)
		return;

	for (y = miny; y <= maxy; y++)
		for (x = 0; x <= maxx; x++) {
			if (vismap[offset] == 0xff) { /* Potential offender */
				int horiz = 0, vertic = 0;

				if ((x > 0 && vismap[offset-1] == 0xff)
				    || (x < maxx && vismap[offset+1] == 0xff))
					horiz = 1;

				if ((y > miny && vismap[offset-linewidth] == 0xff)
				    || (y < maxy && vismap[offset+linewidth] == 0xff))
					vertic = 1;

				if (!(horiz && vertic)) { /* otherwise, we're inside a white block */
					if (horiz) {
						if (y == miny) {
							vismap[offset] = vismap[offset + linewidth];
							primap[offset] = vismap[offset + linewidth];
						} else if (y == maxy) {
							vismap[offset] = vismap[offset - linewidth];
							primap[offset] = vismap[offset - linewidth];
						} else {
							if ((primap[offset + linewidth] == primap[offset - linewidth])
							    &&(vismap[offset - linewidth] == vismap[offset + linewidth])) {
								vismap[offset] = vismap[offset + linewidth];
								primap[offset] = primap[offset + linewidth];
							} else {
								vismap[offset] = (vismap[offset + linewidth] & 0x0f) | (vismap[offset - linewidth] & 0xf0);
								primap[offset] = (primap[offset + linewidth] + primap[offset - linewidth]) >> 1;
							}
						}
					} else { /* vertical or neither */
						if (x == 0) {
							vismap[offset] = vismap[offset + 1];
							primap[offset] = vismap[offset + 1];
						} else if (x == maxx) {
							vismap[offset] = vismap[offset - 1];
							primap[offset] = vismap[offset - 1];
						} else {
							if ((primap[offset + 1] == primap[offset - 1])
							    &&(vismap[offset - 1] == vismap[offset + 1])) {
								vismap[offset] = vismap[offset + 1];
								primap[offset] = primap[offset + 1];
							} else {
								vismap[offset] = (vismap[offset + 1] & 0x0f) | (vismap[offset - 1] & 0xf0);
								primap[offset] = (primap[offset + 1] + primap[offset - 1]) >> 1;
							}
						}
					}
				}
			}

			++offset;
		}

}


inline static void
check_and_remove_artifact(byte *dest, byte* srcp, int legalcolor, byte l, byte r, byte u, byte d)
{
	if (*dest == legalcolor) {
		if (*srcp == legalcolor)
			return;
		if (l) {
			if (srcp[-1] == legalcolor)
				return;
			if (u && srcp[-320 - 1] == legalcolor)
				return;
			if (d && srcp[320 - 1] == legalcolor)
				return;
		}
		if (r) {
			if (srcp[1] == legalcolor)
				return;
			if (u && srcp[-320 + 1] == legalcolor)
				return;
			if (d && srcp[320 + 1] == legalcolor)
				return;
		}

		if (u && srcp[-320] == legalcolor)
			return;

		if (d && srcp[-320] == legalcolor)
			return;

		*dest = *srcp;
	}
}


void
gfxr_remove_artifacts_pic0(gfxr_pic_t *dest, gfxr_pic_t *src)
{
	int x_320, y_200;
	int bound_x = dest->mode->xfact;
	int bound_y = dest->mode->yfact;
	int scaled_line_size = bound_x * 320;
	int read_offset = 0;

	assert(src->mode->xfact == 1);
	assert(src->mode->yfact == 1);

	if (bound_x == 1 && bound_y == 1) {
		/* D'Oh! */
		GFXWARN("attempt to remove artifacts from unscaled pic!\n");
		return;
	}

	for (y_200 = 0; y_200 < 200; y_200++) {
		for (x_320 = 0; x_320 < 320; x_320++) {
			int write_offset = (y_200 * bound_y * scaled_line_size) + (x_320 * bound_x);
			int sub_x, sub_y;
			byte *src_visualp = &(src->visual_map->index_data[read_offset]);
			byte *src_priorityp = &(src->priority_map->index_data[read_offset]);

			for (sub_y = 0; sub_y < bound_y; sub_y++) {
				for (sub_x = 0; sub_x < bound_x; sub_x++) {
					check_and_remove_artifact(dest->visual_map->index_data + write_offset,
								  src_visualp, (int)0xff,
								  (byte)x_320, (byte)(x_320 < 319), (byte)(y_200 > 10), (byte)(y_200 < 199));
					check_and_remove_artifact(dest->priority_map->index_data + write_offset,
								  src_priorityp, 0,
								  (byte)x_320, (byte)(x_320 < 319), (byte)(y_200 > 10), (byte)(y_200 < 199));
					++write_offset;
				}
				write_offset += scaled_line_size - bound_x;
			}
			++read_offset;
		}
	}

}


void
gfxr_draw_pic0(gfxr_pic_t *pic, int fill_normally, int default_palette, int size,
	       byte *resource, gfxr_pic0_params_t *style, int resid)
{
	const int default_palette_table[GFXR_PIC0_PALETTE_SIZE] = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0x88,
		0x88, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x88,
		0x88, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
		0x08, 0x91, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x88
	};

	const int default_priority_table[GFXR_PIC0_PALETTE_SIZE] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	};
	int palette[GFXR_PIC0_NUM_PALETTES][GFXR_PIC0_PALETTE_SIZE];
	int priority_table[GFXR_PIC0_PALETTE_SIZE];
	int i;
	int drawenable = GFX_MASK_VISUAL | GFX_MASK_PRIORITY;
	int priority = 0;
	int color = 0;
	int pattern_nr = 0;
	int pattern_code = 0;
	int pattern_size = 0;
	int control = 0;
	int pos = 0;
	int x, y;
	int oldx, oldy;
	int pal, index;
	int temp;
	int line_mode = style->line_mode;
	int fill_count = 0;
	byte op, opx;

#ifdef GFXR_DEBUG_PIC0
	fillmagc = atoi(getenv("FOO"));
	fillc = atoi(getenv("FOO2"));
#endif /* GFXR_DEBUG_PIC0 */

	/* Initialize palette */
	for (i = 0; i < GFXR_PIC0_NUM_PALETTES; i++)
		memcpy(palette[i], default_palette_table, sizeof(int) * GFXR_PIC0_PALETTE_SIZE);

	memcpy(priority_table, default_priority_table, sizeof(int) * GFXR_PIC0_PALETTE_SIZE);

	/* Main loop */
	while (pos < size) {
		op = *(resource + pos++);

		switch (op) {

		case PIC_OP_SET_COLOR:
			p0printf("Set color @%d\n", pos);

			pal = *(resource + pos++);
			index = pal % GFXR_PIC0_PALETTE_SIZE;
			pal /= GFXR_PIC0_PALETTE_SIZE;

			pal += default_palette;

			if (pal >= GFXR_PIC0_NUM_PALETTES) {
				GFXERROR("Attempt to access invalid palette %d\n", pal);
				return;
			}

			color = palette[pal][index];
#if 0
#ifdef GFXR_DEBUG_PIC0
			color &= 0x77;
#endif
#endif
			p0printf("  color <- %02x [%d/%d]\n", color, pal, index);
			drawenable |= GFX_MASK_VISUAL;
			break;


		case PIC_OP_DISABLE_VISUAL:
			p0printf("Disable visual @%d\n", pos);
			drawenable &= ~GFX_MASK_VISUAL;
			break;


		case PIC_OP_SET_PRIORITY:
			p0printf("Set priority @%d\n", pos);

			pal = *(resource + pos++);
			index = pal % GFXR_PIC0_PALETTE_SIZE;
			pal /= GFXR_PIC0_PALETTE_SIZE; /* Ignore pal */

			priority = priority_table[index];

			p0printf("  priority <- %d [%d/%d]\n", priority, pal, index);
			drawenable |= GFX_MASK_PRIORITY;
			break;


		case PIC_OP_DISABLE_PRIORITY:
			p0printf("Disable priority @%d\n", pos);
			drawenable &= ~GFX_MASK_PRIORITY;
			break;


		case PIC_OP_SHORT_PATTERNS:
			p0printf("Short patterns @%d\n", pos);
			if (pattern_code & PATTERN_FLAG_USE_PATTERN) {
				pattern_nr = ((*(resource + pos++)) >> 1) & 0x7f;
				p0printf("  pattern_nr <- %d\n", pattern_nr);
			}

			GET_ABS_COORDS(x, y);

			_gfxr_draw_pattern(pic, x, y, color, priority, control, drawenable, pattern_code,
					   pattern_size, pattern_nr, style->brush_mode);

			while (*(resource + pos) < PIC_OP_FIRST) {
				if (pattern_code & PATTERN_FLAG_USE_PATTERN) {
					pattern_nr = ((*(resource + pos++)) >> 1) & 0x7f;
					p0printf("  pattern_nr <- %d\n", pattern_nr);
				}

				GET_REL_COORDS(x, y);

				_gfxr_draw_pattern(pic, x, y, color, priority, control, drawenable, pattern_code,
						   pattern_size, pattern_nr, style->brush_mode);
			}
			break;


		case PIC_OP_MEDIUM_LINES:
			p0printf("Medium lines @%d\n", pos);
			GET_ABS_COORDS(oldx, oldy);
			while (*(resource + pos) < PIC_OP_FIRST) {
#if 0
				fprintf(stderr,"Medium-line: [%04x] from %d,%d, data %02x %02x (dx=%d)", pos, oldx, oldy,
					0xff & resource[pos], 0xff & resource[pos+1], *((signed char *) resource + pos + 1));
#endif
				GET_MEDREL_COORDS(oldx, oldy);
#if 0
				fprintf(stderr, " to %d,%d\n", x, y);
#endif
				_gfxr_draw_line(pic, oldx, oldy, x, y, color, priority, control, drawenable, line_mode, PIC_OP_MEDIUM_LINES);
				oldx = x; oldy = y;
			}
			break;


		case PIC_OP_LONG_LINES:
			p0printf("Long lines @%d\n", pos);
			GET_ABS_COORDS(oldx, oldy);
			while (*(resource + pos) < PIC_OP_FIRST) {
				GET_ABS_COORDS(x,y);
				_gfxr_draw_line(pic, oldx, oldy, x, y, color, priority, control, drawenable, line_mode, PIC_OP_LONG_LINES);
				oldx = x; oldy = y;
			}
			break;


		case PIC_OP_SHORT_LINES:
			p0printf("Short lines @%d\n", pos);
			GET_ABS_COORDS(oldx, oldy);
			x = oldx; y = oldy;
			while (*(resource + pos) < PIC_OP_FIRST) {
				GET_REL_COORDS(x,y);
				_gfxr_draw_line(pic, oldx, oldy, x, y, color, priority, control, drawenable, line_mode, PIC_OP_SHORT_LINES);
				oldx = x; oldy = y;
			}
			break;


		case PIC_OP_FILL:
			p0printf("Fill @%d\n", pos);
			while (*(resource + pos) < PIC_OP_FIRST) {
				/*fprintf(stderr,"####################\n"); */
				GET_ABS_COORDS(x, y);
				p0printf("Abs coords %d,%d\n", x, y);
				/*fprintf(stderr,"C=(%d,%d)\n", x, y + SCI_TITLEBAR_SIZE);*/
				_gfxr_fill(pic, x, y + SCI_TITLEBAR_SIZE, (fill_normally)? color : 0, priority, control, drawenable);

				if (fill_count++ > SCI_PIC0_MAX_FILL) {
					sci_sched_yield();
					fill_count = 0;
				}

#ifdef GFXR_DEBUG_PIC0
				if (!fillmagc) {
					int x,y;
					if (getenv("FOO1"))
						for (x = 0; x < 320; x++)
							for (y = 0; y < 200; y++) {
								int aux = pic->aux_map[x + y*320];
								int pix = (aux & 0xf);
								int i;

								if (aux & 0x40) {
									if (x == 0 || !(pic->aux_map[x-1 + y * 320] & 0x40))
										for (i = 0; i < pic->mode->yfact; i++)
											pic->visual_map->index_data[(x + ((y*pic->mode->yfact)+i)*320) * pic->mode->xfact] ^= 0xff;

									if (x == 319 || !(pic->aux_map[x+1 + y * 320] & 0x40))
										for (i = 0; i < pic->mode->yfact; i++)
											pic->visual_map->index_data[pic->mode->xfact - 1 +(x + ((y*pic->mode->yfact)+i)*320) * pic->mode->xfact] ^= 0xff;

									if (y == 0 || !(pic->aux_map[x + (y-1) * 320] & 0x40))
										for (i = 0; i < pic->mode->yfact; i++)
											pic->visual_map->index_data[i+(x + ((y*pic->mode->yfact))*320) * pic->mode->xfact] ^= 0xff;

									if (y == 199 || !(pic->aux_map[x + (y+1) * 320] & 0x40))
										for (i = 0; i < pic->mode->yfact; i++)
											pic->visual_map->index_data[i+(x + ((y*pic->mode->yfact)+pic->mode->yfact - 1)*320) * pic->mode->xfact] ^= 0xff;
								}

								pix |= (aux & 0x40) >> 4;
								pix |= (pix << 4);

								pic->visual_map->index_data[x + y*320*pic->mode->xfact] = pix;
							}
					return;
				} --fillmagc;
#endif /* GFXR_DEBUG_PIC0 */
			}
			break;


		case PIC_OP_SET_PATTERN:
			p0printf("Set pattern @%d\n", pos);
			pattern_code = (*(resource + pos++));
			pattern_size = pattern_code & 0x07;
			break;


		case PIC_OP_ABSOLUTE_PATTERN:
			p0printf("Absolute pattern @%d\n", pos);
			while (*(resource + pos) < PIC_OP_FIRST) {
				if (pattern_code & PATTERN_FLAG_USE_PATTERN) {
					pattern_nr = ((*(resource + pos++)) >> 1) & 0x7f;
					p0printf("  pattern_nr <- %d\n", pattern_nr);
				}

				GET_ABS_COORDS(x, y);

				_gfxr_draw_pattern(pic, x, y, color, priority, control, drawenable, pattern_code,
						   pattern_size, pattern_nr, style->brush_mode);
			}
			break;


		case PIC_OP_SET_CONTROL:
			p0printf("Set control @%d\n", pos);
			control = (*(resource + pos++)) & 0xf;
			drawenable |= GFX_MASK_CONTROL;
			break;


		case PIC_OP_DISABLE_CONTROL:
			p0printf("Disable control @%d\n", pos);
			drawenable &= ~GFX_MASK_CONTROL;
			break;


		case PIC_OP_MEDIUM_PATTERNS:
			p0printf("Medium patterns @%d\n", pos);
			if (pattern_code & PATTERN_FLAG_USE_PATTERN) {
				pattern_nr = ((*(resource + pos++)) >> 1) & 0x7f;
				p0printf("  pattern_nr <- %d\n", pattern_nr);
			}

			GET_ABS_COORDS(oldx, oldy);

			_gfxr_draw_pattern(pic, oldx, oldy, color, priority, control, drawenable, pattern_code,
					   pattern_size, pattern_nr, style->brush_mode);

			x = oldx; y = oldy;
			while (*(resource + pos) < PIC_OP_FIRST) {
				if (pattern_code & PATTERN_FLAG_USE_PATTERN) {
					pattern_nr = ((*(resource + pos++)) >> 1) & 0x7f;
					p0printf("  pattern_nr <- %d\n", pattern_nr);
				}

				GET_MEDREL_COORDS(x, y);

				_gfxr_draw_pattern(pic, x, y, color, priority, control, drawenable, pattern_code,
						   pattern_size, pattern_nr, style->brush_mode);
			}
			break;


		case PIC_OP_OPX:
			opx = *(resource + pos++);
			p0printf("OPX: ");

			switch (opx) {


			case PIC_OPX_SET_PALETTE_ENTRIES:
				p0printf("Set palette entry @%d\n", pos);
				while (*(resource + pos) < PIC_OP_FIRST) {
					index = *(resource + pos++);
					pal = index / GFXR_PIC0_PALETTE_SIZE;
					index %= GFXR_PIC0_PALETTE_SIZE;

					if (pal >= GFXR_PIC0_NUM_PALETTES) {
						GFXERROR("Attempt to write to invalid palette %d\n", pal);
						return;
					}
					palette[pal][index] = *(resource + pos++);
				}
				break;


			case PIC_OPX_SET_PALETTE:
				p0printf("Set palette @%d\n", pos);
				pal = *(resource + pos++);
				if (pal >= GFXR_PIC0_NUM_PALETTES) {
					GFXERROR("Attempt to write to invalid palette %d\n", pal);
					return;
				}

				p0printf("  palette[%d] <- (", pal);
				for (index = 0; index < GFXR_PIC0_PALETTE_SIZE; index++) {
					palette[pal][index] = *(resource + pos++);
					if (index > 0)
						p0printf(",");
					if (!(index & 0x7))
						p0printf("[%d]=", index);
					p0printf("%02x", palette[pal][index]);
				}
				p0printf(")\n");
				break;


			case PIC_OPX_MONO0:
				p0printf("Monochrome opx 0 @%d\n", pos);
				pos += 41;
				break;


			case PIC_OPX_MONO1:
			case PIC_OPX_MONO3:
				++pos;
				p0printf("Monochrome opx %d @%d\n", opx, pos);
				break;


			case PIC_OPX_MONO2:
			case PIC_OPX_MONO4: /* Monochrome ops: Ignored by us */
				p0printf("Monochrome opx %d @%d\n", opx, pos);
				break;


			case PIC_OPX_EMBEDDED_VIEW:
				GFXWARN("Embedded view @%d\n", pos);
				GFXWARN("-- not implemented- aborting --\n");
				return;
				break;


			case PIC_OPX_SET_PRIORITY_TABLE: {
				int i;
				int *pri_table;

				p0printf("Set priority table @%d\n", pos);
				if (!pic->internal)
					pic->internal = sci_malloc(16 * sizeof(int));

				pri_table = pic->internal;

				pri_table[0] = 0;
				pri_table[15] = 190;

				for (i = 1; i < 15; i++)
					pri_table[i] = resource[pos++];
			}
			break;


			default: sciprintf("%s L%d: Warning: Unknown opx %02x\n", __FILE__, __LINE__, op);
				return;
			}
			break;

		case PIC_OP_TERMINATE:
			p0printf("Terminator\n");
			/* WARNING( "ARTIFACT REMOVAL CODE is commented out!") */
			/* _gfxr_vismap_remove_artifacts(); */
			return;

		default: GFXWARN("Unknown op %02x\n", op);
			return;
		}
	}

	GFXWARN("Reached end of pic resource %04x\n", resid);
}


void
gfxr_dither_pic0(gfxr_pic_t *pic, int dmode, int pattern)
{
	int xl = pic->visual_map->index_xl;
	int yl = pic->visual_map->index_yl;
	int xfrob_max = (pattern == GFXR_DITHER_PATTERN_1)? 1 : pic->mode->xfact;
	int yfrob_max = (pattern == GFXR_DITHER_PATTERN_1)? 1 : pic->mode->yfact;
	int xfrobc = 0, yfrobc = 0;
	int selection = 0;
	int x, y;
	byte *data = pic->visual_map->index_data;

	if (dmode == GFXR_DITHER_MODE_F256)
		return; /* Nothing to do */

	if (dmode == GFXR_DITHER_MODE_D16) { /* Limit to 16 colors */
		pic->visual_map->colors = gfx_sci0_image_colors[sci0_palette];
		pic->visual_map->colors_nr = GFX_SCI0_IMAGE_COLORS_NR;
	}

	for (y = 0; y < yl; y++) {
		for (x = 0; x < xl; x++) {

			switch (dmode) {

			case GFXR_DITHER_MODE_D16:
				if (selection)
					*data = (*data & 0xf0) >> 4;
				else
					*data = (*data & 0xf);
				break;

			case GFXR_DITHER_MODE_D256:
				if (selection)
					*data = ((*data & 0xf) << 4) | ((*data & 0xf0) >> 4);
				break;

			default:
				GFXERROR("Invalid dither mode %d!\n", dmode);
				return;
			}

			++data;

			if (++xfrobc == xfrob_max) {
				selection = !selection;
				xfrobc = 0;
			}
		}

		if (++yfrobc == yfrob_max) {
			selection = !selection;
			yfrobc = 0;
		}
	}
}

