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


#define DRAWLINE_FUNC _gfx_draw_line_buffer_1
#define PIXELWIDTH 1
#include "gfx_line.c"
#undef PIXELWIDTH
#undef DRAWLINE_FUNC

#define DRAWLINE_FUNC _gfx_draw_line_buffer_2
#define PIXELWIDTH 2
#include "gfx_line.c"
#undef PIXELWIDTH
#undef DRAWLINE_FUNC

#define DRAWLINE_FUNC _gfx_draw_line_buffer_3
#define PIXELWIDTH 3
#include "gfx_line.c"
#undef PIXELWIDTH
#undef DRAWLINE_FUNC

#define DRAWLINE_FUNC _gfx_draw_line_buffer_4
#define PIXELWIDTH 4
#include "gfx_line.c"
#undef PIXELWIDTH
#undef DRAWLINE_FUNC

inline void
gfx_draw_line_buffer(byte *buffer, int linewidth, int pixelwidth, rect_t line, unsigned int color)
{
	switch (pixelwidth) {

	case 1:
		_gfx_draw_line_buffer_1(buffer, linewidth, line, color);
		return;

	case 2:
		_gfx_draw_line_buffer_2(buffer, linewidth, line, color);
		return;

	case 3:
		_gfx_draw_line_buffer_3(buffer, linewidth, line, color);
		return;

	case 4:
		_gfx_draw_line_buffer_4(buffer, linewidth, line, color);
		return;

	default:
		GFXERROR("pixelwidth=%d not supported!\n", pixelwidth);
		return;

	}
}




void
gfx_draw_line_pixmap_i(gfx_pixmap_t *pxm, rect_t line, int color)
{
	gfx_draw_line_buffer(pxm->index_data, pxm->index_xl, 1, line, color);
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
		     int priority_skip, int flags)
{
	int maxx = 320 * mode->xfact;
	int maxy = 200 * mode->yfact;
	byte *src = pxm->data;
	byte *alpha = pxm->alpha_map? pxm->alpha_map : pxm->data;
	byte *priority_pos = priority_dest;
	int alpha_mask, alpha_min, alpha_max;
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
        if (!(flags & GFX_CROSSBLIT_FLAG_DATA_IS_HOMED))
	        dest += dest_coords.x * bpp;
	priority_pos += dest_coords.x * priority_skip;

	/* Set y offsets */
        if (!(flags & GFX_CROSSBLIT_FLAG_DATA_IS_HOMED))
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

#define ALPHA_FACTOR1 0x80
#define ALPHA_FACTOR2 0xc0
	if (alpha_mask & 0xff) {
		alpha_min = ((alpha_mask * ALPHA_FACTOR1) >> 8) & alpha_mask;
		alpha_max = ((alpha_mask * ALPHA_FACTOR2) >> 8) & alpha_mask;
	} else {
		alpha_min = ((alpha_mask >> 8) * ALPHA_FACTOR1) & alpha_mask;
		alpha_max = ((alpha_mask >> 8) * ALPHA_FACTOR2) & alpha_mask;
	}

	if (!alpha_mask)
		 _gfx_crossblit_simple(dest, src, dest_line_width, pxm->xl * bpp, 
				       xl, yl, bpp);
	else

	if (priority == GFX_NO_PRIORITY)
		switch (bpp) {

		case 1: _gfx_crossblit_8(dest, src, dest_line_width, pxm->xl * bpp, 
					 xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
					 alpha_mask, alpha_min, alpha_max);
			break;

		case 2: _gfx_crossblit_16(dest, src, dest_line_width, pxm->xl * bpp, 
					 xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
					 alpha_mask, alpha_min, alpha_max);
			break;

		case 3: _gfx_crossblit_24(dest, src, dest_line_width, pxm->xl * bpp, 
					 xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
					 alpha_mask, alpha_min, alpha_max);
			break;

		case 4: _gfx_crossblit_32(dest, src, dest_line_width, pxm->xl * bpp, 
					 xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
					 alpha_mask, alpha_min, alpha_max);
			break;

		default: GFXERROR("Invalid mode->bytespp: %d\n", mode->bytespp);
			return GFX_ERROR;

		}


	else switch (mode->bytespp) { /* priority */

	case 1: _gfx_crossblit_8_P(dest, src, dest_line_width, pxm->xl * bpp, 
				   xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
				   alpha_mask, alpha_min, alpha_max, priority_pos,
				   priority_line_width, priority_skip, priority);
		break;

	case 2: _gfx_crossblit_16_P(dest, src, dest_line_width, pxm->xl * bpp, 
				    xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
				    alpha_mask, alpha_min, alpha_max, priority_pos,
				    priority_line_width, priority_skip, priority);
		break;

	case 3: _gfx_crossblit_24_P(dest, src, dest_line_width, pxm->xl * bpp, 
				    xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
				    alpha_mask, alpha_min, alpha_max, priority_pos,
				    priority_line_width, priority_skip, priority);
		break;

	case 4: _gfx_crossblit_32_P(dest, src, dest_line_width, pxm->xl * bpp, 
				    xl, yl, alpha, bytes_per_alpha_line, bytes_per_alpha_pixel,
				    alpha_mask, alpha_min, alpha_max, priority_pos,
				    priority_line_width, priority_skip, priority);
		break;

	default: GFXERROR("Invalid mode->bytespp: %d\n", mode->bytespp);
		return GFX_ERROR;

	}

	return GFX_OK;
}



