/***************************************************************************
 gfx_support.c Copyright (C) 2000 Christoph Reichenbach


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
/* Graphics support functions for drivers and replacements for driver functions
** for use with the graphical state manager
*/

#include <gfx_system.h>
#include <gfx_tools.h>

#define LINEMACRO(startx, starty, deltalinear, deltanonlinear, linearvar, nonlinearvar, \
                  linearend, nonlinearstart, linearmod, nonlinearmod) \
   x = (startx); y = (starty); \
   incrNE = ((deltalinear) > 0)? (deltalinear) : -(deltalinear); \
   incrNE <<= 1; \
   deltanonlinear <<= 1; \
   incrE = ((deltanonlinear) > 0) ? -(deltanonlinear) : (deltanonlinear);  \
   d = nonlinearstart-1;  \
   while (linearvar != (linearend)) { \
     buffer[linewidth * y + x] = color; \
     linearvar += linearmod; \
     if ((d+=incrE) < 0) { \
       d += incrNE; \
       nonlinearvar += nonlinearmod; \
     }; \
   }; \
  buffer[linewidth * y + x] = color;

inline void
gfx_draw_line_buffer(byte *buffer, int linewidth, rect_t line, int color)
{
  /*void dither_line(picture_t buffers, int curx, int cury, short x1, short y1,
    int col1, int col2, int priority, int special, char drawenable)*/

  int dx, dy, incrE, incrNE, d, finalx, finaly;
  int x = line.x;
  int y = line.y;
  dx = line.xl;
  dy = line.yl;
  finalx = x + dx;
  finaly = y + dy;

  dx = abs(dx);
  dy = abs(dy);

  if (dx > dy) {
    if (finalx < x) {
      if (finaly < y) { /* llu == left-left-up */
	LINEMACRO(x, y, dx, dy, x, y, finalx, dx, -1, -1);
      } else {         /* lld */
	LINEMACRO(x, y, dx, dy, x, y, finalx, dx, -1, 1);
      }
    } else { /* x1 >= x */
      if (finaly < y) { /* rru */
	LINEMACRO(x, y, dx, dy, x, y, finalx, dx, 1, -1);
      } else {         /* rrd */
	LINEMACRO(x, y, dx, dy, x, y, finalx, dx, 1, 1);
      }
    }
  } else { /* dx <= dy */
    if (finaly < y) {
      if (finalx < x) { /* luu */
	LINEMACRO(x, y, dy, dx, y, x, finaly, dy, -1, -1);
      } else {         /* ruu */
	LINEMACRO(x, y, dy, dx, y, x, finaly, dy, -1, 1);
      }
    } else { /* y1 >= y */
      if (finalx < x) { /* ldd */
	LINEMACRO(x, y, dy, dx, y, x, finaly, dy, 1, -1);
      } else {         /* rdd */
	LINEMACRO(x, y, dy, dx, y, x, finaly, dy, 1, 1);
      }
    }
  }
}

#undef LINEMACRO



void
gfx_draw_line_pixmap_i(gfx_pixmap_t *pxm, rect_t line, int color)
{
  gfx_draw_line_buffer(pxm->index_data, pxm->index_xl, line, color);
}




void
gfx_draw_box_buffer(byte *buffer, int linewidth, rect_t zone, int color)
{
  byte *dest = buffer + zone.x + (linewidth * zone.y);
  int i;

  if (zone.xl <= 0 || zone.yl <= 0)
    return;

  for (i = 0; i < zone.yl; i++) {
    memset(dest, color, zone.xl);
    dest += linewidth;
  }
}


void
gfx_draw_box_pixmap_i(gfx_pixmap_t *pxm, rect_t box, int color)
{
  gfx_clip_box_basic(&box, pxm->index_xl - 1, pxm->index_yl - 1);

  gfx_draw_box_buffer(pxm->index_data, pxm->index_xl, box, color);
}


/* Import various crossblit functions */
#undef USE_PRIORITY
#undef FUNCTION_NAME
#undef BYTESPP

# define FUNCTION_NAME _gfx_crossblit_8
# define BYTESPP 1
# include "gfx_crossblit.c"

# undef FUNCTION_NAME
# undef BYTESPP
# define FUNCTION_NAME _gfx_crossblit_16
# define BYTESPP 2
# include "gfx_crossblit.c"

# undef FUNCTION_NAME
# undef BYTESPP
# define FUNCTION_NAME _gfx_crossblit_24
# define BYTESPP 3
# include "gfx_crossblit.c"

# undef FUNCTION_NAME
# undef BYTESPP
# define FUNCTION_NAME _gfx_crossblit_32
# define BYTESPP 4
# include "gfx_crossblit.c"

#define USE_PRIORITY

# undef FUNCTION_NAME
# undef BYTESPP
# define FUNCTION_NAME _gfx_crossblit_8_P
# define BYTESPP 1
# include "gfx_crossblit.c"

# undef FUNCTION_NAME
# undef BYTESPP
# define FUNCTION_NAME _gfx_crossblit_16_P
# define BYTESPP 2
# include "gfx_crossblit.c"

# undef FUNCTION_NAME
# undef BYTESPP
# define FUNCTION_NAME _gfx_crossblit_24_P
# define BYTESPP 3
# include "gfx_crossblit.c"

# undef FUNCTION_NAME
# undef BYTESPP
# define FUNCTION_NAME _gfx_crossblit_32_P
# define BYTESPP 4
# include "gfx_crossblit.c"

#undef USE_PRIORITY
#undef FUNCTION_NAME
#undef BYTESPP

void
_gfx_crossblit_simple(byte *dest, byte *src, int dest_line_width, int src_line_width, 
		      int xl, int yl, int bpp)
{
	int line_width = xl * bpp;
	int i;

	for (i = 0; i < yl; i++) {
		memcpy(dest, src, line_width);
		dest += dest_line_width;
		src += src_line_width;
	}
}

int
gfx_crossblit_pixmap(gfx_mode_t *mode, gfx_pixmap_t *pxm, int priority,
		     rect_t src_coords,
		     rect_t dest_coords, byte *dest, int dest_line_width,
		     byte *priority_dest, int priority_line_width,
		     int priority_skip)
{
	int maxx = 320 * mode->xfact;
	int maxy = 200 * mode->yfact;
	byte *src = pxm->data;
	byte *alpha = pxm->alpha_map? pxm->alpha_map : pxm->data;
	byte *priority_pos = priority_dest;
	int alpha_mask;
	int bpp = mode->bytespp;
	int bytes_per_alpha_pixel = pxm->alpha_map? 1 : bpp;
	int bytes_per_alpha_line =  bytes_per_alpha_pixel * pxm->xl;
	int xl = pxm->xl, yl = pxm->yl;
	int xoffset = (dest_coords.x < 0)? - dest_coords.x : 0;
	int yoffset = (dest_coords.y < 0)? - dest_coords.y : 0;

	if (dest_coords.x + xl >= maxx)
		xl = maxx - dest_coords.x;
	if (dest_coords.y + yl >= maxy)
		yl = maxy - dest_coords.y;

	xl -= xoffset;
	yl -= yoffset;

	if (!pxm->data)
		return GFX_ERROR;

	if (xl <= 0 || yl <= 0)
		return GFX_OK;

	/* Set destination offsets */

	/* Set x offsets */
	dest += dest_coords.x * bpp;
	priority_pos += dest_coords.x * priority_skip;

	/* Set y offsets */
	dest += dest_coords.y * dest_line_width;
	priority_pos += dest_coords.y * priority_line_width;

	/* Set source offsets */
	if (xoffset += src_coords.x) {
		dest_coords.x = 0;
		src += xoffset * bpp;
		alpha += xoffset * bytes_per_alpha_pixel;
	}


	if (yoffset += src_coords.y) {
		dest_coords.y = 0;
		src += yoffset * bpp * pxm->xl;
		alpha += yoffset * bytes_per_alpha_line;
	}

	/* Adjust length for clip box */
	if (xl > src_coords.xl)
		xl = src_coords.xl;
	if (yl > src_coords.yl)
		yl = src_coords.yl;

	/* now calculate alpha */
	if (pxm->alpha_map)
		alpha_mask = 0xff;
	else {
		alpha_mask = mode->alpha_mask;
		if (!alpha_mask && (priority != GFX_NO_PRIORITY)) {
			GFXERROR("Invalid alpha mode: both pxm->alpha_map and alpha_mask are white!\n");
			return GFX_ERROR;
		}

		if (alpha_mask) {
			while (!(alpha_mask & 0xff)) {
				alpha_mask >>= 8;
				alpha++;
			}
			alpha_mask &= 0xff;
		}
	}

	if (!alpha_mask)
		 _gfx_crossblit_simple(dest, src, dest_line_width, pxm->xl * bpp, 
				       xl, yl, bpp);
	else

	if (priority == GFX_NO_PRIORITY)
		switch (bpp) {

		case 1: _gfx_crossblit_8(dest, src, dest_line_width, pxm->xl * bpp, 
					 xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
					 alpha_mask);
			break;

		case 2: _gfx_crossblit_16(dest, src, dest_line_width, pxm->xl * bpp, 
					 xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
					 alpha_mask);
			break;

		case 3: _gfx_crossblit_24(dest, src, dest_line_width, pxm->xl * bpp, 
					 xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
					 alpha_mask);
			break;

		case 4: _gfx_crossblit_32(dest, src, dest_line_width, pxm->xl * bpp, 
					 xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
					 alpha_mask);
			break;

		default: GFXERROR("Invalid mode->bytespp: %d\n", mode->bytespp);
			return GFX_ERROR;

		}


	else switch (mode->bytespp) { /* priority */

	case 1: _gfx_crossblit_8_P(dest, src, dest_line_width, pxm->xl * bpp, 
				   xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
				   alpha_mask, priority_pos, priority_line_width, priority_skip,
				   priority);
		break;

	case 2: _gfx_crossblit_16_P(dest, src, dest_line_width, pxm->xl * bpp, 
				    xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
				    alpha_mask, priority_pos, priority_line_width, priority_skip,
				    priority);
		break;

	case 3: _gfx_crossblit_24_P(dest, src, dest_line_width, pxm->xl * bpp, 
				    xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
				    alpha_mask, priority_pos, priority_line_width, priority_skip,
				    priority);
		break;

	case 4: _gfx_crossblit_32_P(dest, src, dest_line_width, pxm->xl * bpp, 
				    xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
				    alpha_mask, priority_pos, priority_line_width, priority_skip,
				    priority);
		break;

	default: GFXERROR("Invalid mode->bytespp: %d\n", mode->bytespp);
		return GFX_ERROR;

	}

	return GFX_OK;
}



