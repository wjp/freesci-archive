/***************************************************************************
 gfx_resource.c Copyright (C) 2000 Christoph Reichenbach


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

#include <gfx_system.h>
#include <gfx_resource.h>
#include <gfx_tools.h>

gfx_mode_t mode_1x1_color_index = { /* Fake 1x1 mode */
	/* xfact */ 1, /* yfact */ 1,
	/* bytespp */ 1,
	/* flags */ 0,
	/* palette */ NULL,

	/* color masks */ 0, 0, 0, 0,
	/* color shifts */ 0, 0, 0, 0
};


static void
gfxr_free_loop(gfx_driver_t *driver, gfxr_loop_t *loop)
{
	int i;

	if (loop->cels) {
		for (i = 0; i < loop->cels_nr; i++)
			if (loop->cels[i])
				gfx_free_pixmap(driver, loop->cels[i]);

		free(loop->cels);
	}
}

void
gfxr_free_view(gfx_driver_t *driver, gfxr_view_t *view)
{
	int i;

	if (view->colors && !(view->flags & GFX_PIXMAP_FLAG_EXTERNAL_PALETTE))
		free(view->colors);

	if (view->loops) {
		for (i = 0; i < view->loops_nr; i++)
			gfxr_free_loop(driver, view->loops + i);

		free(view->loops);
	}
	free(view);
}



/* Now construct the pixmap scaling functions */
#define EXTRA_BYTE_OFFSET 0
#define SIZETYPE guint8
#define FUNCNAME _gfx_xlate_pixmap_unfiltered_1
#define FUNCNAME_LINEAR _gfx_xlate_pixmap_linear_1
#define FUNCNAME_TRILINEAR _gfx_xlate_pixmap_trilinear_1
#define COPY_BYTES 1
#include "gfx_pixmap_scale.c"
#undef COPY_BYTES

#define SIZETYPE guint16
#define FUNCNAME _gfx_xlate_pixmap_unfiltered_2
#define FUNCNAME_LINEAR _gfx_xlate_pixmap_linear_2
#define FUNCNAME_TRILINEAR _gfx_xlate_pixmap_trilinear_2
#define COPY_BYTES 2
#include "gfx_pixmap_scale.c"
#undef COPY_BYTES

#ifdef WORDS_BIGENDIAN
# undef EXTRA_BYTE_OFFSET
# define EXTRA_BYTE_OFFSET 1
#endif /* WORDS_BIGENDIAN */
#define SIZETYPE guint32
#define FUNCNAME _gfx_xlate_pixmap_unfiltered_3
#define FUNCNAME_LINEAR _gfx_xlate_pixmap_linear_3
#define FUNCNAME_TRILINEAR _gfx_xlate_pixmap_trilinear_3
#define COPY_BYTES 3
#include "gfx_pixmap_scale.c"
#undef COPY_BYTES
#ifdef WORDS_BIGENDIAN
# undef EXTRA_BYTE_OFFSET
# define EXTRA_BYTE_OFFSET 0
#endif /* WORDS_BIGENDIAN */

#define SIZETYPE guint32
#define FUNCNAME _gfx_xlate_pixmap_unfiltered_4
#define FUNCNAME_LINEAR _gfx_xlate_pixmap_linear_4
#define FUNCNAME_TRILINEAR _gfx_xlate_pixmap_trilinear_4
#define COPY_BYTES 4
#include "gfx_pixmap_scale.c"
#undef COPY_BYTES
#undef EXTRA_BYTE_OFFSET
#undef SIZETYPE

static inline void
_gfx_xlate_pixmap_unfiltered(gfx_mode_t *mode, gfx_pixmap_t *pxm, int scale)
{
	switch (mode->bytespp) {

	case 1:_gfx_xlate_pixmap_unfiltered_1(mode, pxm, scale);
		break;

	case 2:_gfx_xlate_pixmap_unfiltered_2(mode, pxm, scale);
		break;

	case 3:_gfx_xlate_pixmap_unfiltered_3(mode, pxm, scale);
		break;

	case 4:_gfx_xlate_pixmap_unfiltered_4(mode, pxm, scale);
		break;

	default:
		GFXERROR("Invalid mode->bytespp=%d\n", mode->bytespp);
					
	}

	if (pxm->flags & GFX_PIXMAP_FLAG_SCALED_INDEX) {
		pxm->xl = pxm->index_xl;
		pxm->yl = pxm->index_yl;
	} else {
		pxm->xl = pxm->index_xl * mode->xfact;
		pxm->yl = pxm->index_yl * mode->yfact;
	}
}

static inline void
_gfx_xlate_pixmap_linear(gfx_mode_t *mode, gfx_pixmap_t *pxm, int scale)
{
	if (mode->palette || !scale) { /* fall back to unfiltered */
		_gfx_xlate_pixmap_unfiltered(mode, pxm, scale);
		return;
	}

	pxm->xl = pxm->index_xl * mode->xfact;
	pxm->yl = pxm->index_yl * mode->yfact;

	switch (mode->bytespp) {

	case 1:_gfx_xlate_pixmap_linear_1(mode, pxm, scale);
		break;

	case 2:_gfx_xlate_pixmap_linear_2(mode, pxm, scale);
		break;

	case 3:_gfx_xlate_pixmap_linear_3(mode, pxm, scale);
		break;

	case 4:_gfx_xlate_pixmap_linear_4(mode, pxm, scale);
		break;

	default:
		GFXERROR("Invalid mode->bytespp=%d\n", mode->bytespp);
					
	}

}

static inline void
_gfx_xlate_pixmap_trilinear(gfx_mode_t *mode, gfx_pixmap_t *pxm, int scale)
{
	if (mode->palette || !scale) { /* fall back to unfiltered */
		_gfx_xlate_pixmap_unfiltered(mode, pxm, scale);
		return;
	}

	pxm->xl = pxm->index_xl * mode->xfact;
	pxm->yl = pxm->index_yl * mode->yfact;

	switch (mode->bytespp) {

	case 1:_gfx_xlate_pixmap_trilinear_1(mode, pxm, scale);
		break;

	case 2:_gfx_xlate_pixmap_trilinear_2(mode, pxm, scale);
		break;

	case 3:_gfx_xlate_pixmap_trilinear_3(mode, pxm, scale);
		break;

	case 4:_gfx_xlate_pixmap_trilinear_4(mode, pxm, scale);
		break;

	default:
		GFXERROR("Invalid mode->bytespp=%d\n", mode->bytespp);
					
	}

}

void
gfx_xlate_pixmap(gfx_pixmap_t *pxm, gfx_mode_t *mode, gfx_xlate_filter_t filter)
{
	int was_allocated = 0;

	if (mode->palette
	    && !(pxm->flags & GFX_PIXMAP_FLAG_PALETTE_ALLOCATED)) {
		int i;

		for (i = 0; i < pxm->colors_nr; i++) {
			if (gfx_alloc_color(mode->palette, pxm->colors + i) < 0) {
				GFXWARN("Failed to allocate color %d/%d in pixmap (color %02x/%02x/%02x)!\n",
					i, pxm->colors_nr, pxm->colors[i].r, pxm->colors[i].g, pxm->colors[i].b);
				pxm->colors[i].global_index = 0;
			}
			/*
			GFXDEBUG("alloc(%02x/%02x/%02x) -> %d\n",  pxm->colors[i].r,pxm->colors[i].g,pxm->colors[i].b,pxm->colors[i].global_index);
			*/
		}

		pxm->flags |= GFX_PIXMAP_FLAG_PALETTE_ALLOCATED;
	}


	if (!pxm->data) {
		pxm->data = malloc(mode->xfact * mode->yfact * pxm->index_xl * pxm->index_yl * mode->bytespp + 1);
		/* +1: Eases coying on BE machines in 24 bpp packed mode */
		/* Assume that memory, if allocated already, will be sufficient */

		/* Allocate alpha map */
		if (!mode->alpha_mask && pxm->colors_nr < GFX_PIC_COLORS)
			pxm->alpha_map = malloc(mode->xfact * mode->yfact * pxm->index_xl * pxm->index_yl + 1);
	} else
		was_allocated = 1;

	switch (filter) {

	case GFX_XLATE_FILTER_NONE: _gfx_xlate_pixmap_unfiltered(mode, pxm, !(pxm->flags & GFX_PIXMAP_FLAG_SCALED_INDEX));
		break;

	case GFX_XLATE_FILTER_LINEAR: _gfx_xlate_pixmap_linear(mode, pxm, !(pxm->flags & GFX_PIXMAP_FLAG_SCALED_INDEX));
		break;

	case GFX_XLATE_FILTER_TRILINEAR: _gfx_xlate_pixmap_trilinear(mode, pxm, !(pxm->flags & GFX_PIXMAP_FLAG_SCALED_INDEX));
		break;

	default:
		GFXERROR("Attempt to filter pixmap %04x in invalid mode #%d\n", pxm->ID, filter);

		if (!was_allocated) {
			if (!mode->alpha_mask && pxm->colors_nr < GFX_PIC_COLORS)
				free(pxm->alpha_map);
			free(pxm->data);
		}
	}
}


void
gfxr_free_pic(gfx_driver_t *driver, gfxr_pic_t *pic)
{
	fprintf(stderr,"Freeing pic %p, ditherb %p\n", pic, pic->undithered_buffer);
	gfx_free_pixmap(driver, pic->visual_map);
	gfx_free_pixmap(driver, pic->priority_map);
	gfx_free_pixmap(driver, pic->control_map);
	pic->visual_map = NULL;
	pic->priority_map = NULL;
	pic->control_map = NULL;
	if (pic->undithered_buffer)
		free(pic->undithered_buffer);
	pic->undithered_buffer = 0;
	free(pic);
}

