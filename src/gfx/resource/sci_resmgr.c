/***************************************************************************
 sci_resmgr.c Copyright (C) 2000 Christoph Reichenbach

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
/* The interpreter-specific part of the resource manager, for SCI */

#include <sci_memory.h>
#include <sciresource.h>
#include <gfx_widgets.h>
#include <gfx_resmgr.h>
#include <gfx_options.h>
#include <gfx_sci.h>

int
gfxr_interpreter_options_hash(gfx_resource_types_t type, int version,
			      gfx_options_t *options,
			      void *internal, int palette)
{
	gfx_sci_options_t *sci_options = (gfx_sci_options_t *) internal;

	switch (type) {

	case GFX_RESOURCE_TYPE_VIEW:
		return palette;

	case GFX_RESOURCE_TYPE_PIC:
		if (version >= SCI_VERSION_01_VGA)
			return 0;
		else
			return (options->pic0_unscaled)? 0x10000 :
				(options->pic0_dither_mode << 12)
				| (options->pic0_dither_pattern << 8)
				| (options->pic0_brush_mode << 4)
				| (options->pic0_line_mode);

	case GFX_RESOURCE_TYPE_FONT:
		return 0;

	case GFX_RESOURCE_TYPE_CURSOR:
		return 0;

	case GFX_RESOURCE_TYPES_NR:
	default:
		GFXERROR("Invalid resource type: %d\n", type);
		return -1;
	}
}


gfxr_pic_t *
gfxr_interpreter_init_pic(int version, gfx_mode_t *mode, int ID, void *internal)
{
	gfx_sci_options_t *sci_options = (gfx_sci_options_t *) internal;

	if (version >= SCI_VERSION_01_VGA)
	    return gfxr_init_pic1(mode, ID); else
	    return gfxr_init_pic(mode, ID);
}


void
gfxr_interpreter_clear_pic(int version, gfxr_pic_t *pic, void *internal)
{
	gfx_sci_options_t *sci_options = (gfx_sci_options_t *) internal;

	gfxr_clear_pic0(pic);
}


int
gfxr_interpreter_calculate_pic(gfx_resstate_t *state, gfxr_pic_t *scaled_pic, gfxr_pic_t *unscaled_pic,
			       int flags, int default_palette, int nr, void *internal)
{
	resource_mgr_t *resmgr = (resource_mgr_t *) state->misc_payload;
	resource_t *res = scir_find_resource(resmgr, sci_pic, nr, 0);
	int need_unscaled = unscaled_pic != NULL;
	gfxr_pic1_params_t style1, basic_style1;
	gfxr_pic0_params_t style0, basic_style0;
	
	gfx_sci_options_t *sci_options = (gfx_sci_options_t *) internal;

	basic_style0.line_mode = GFX_LINE_MODE_CORRECT;
	basic_style0.brush_mode = GFX_BRUSH_MODE_SCALED;

	style0.line_mode = state->options->pic0_line_mode;
	style0.brush_mode = state->options->pic0_brush_mode;

	basic_style1.line_mode = GFX_LINE_MODE_CORRECT;
	basic_style1.brush_mode = GFX_BRUSH_MODE_SCALED;
	basic_style1.pic_port_bounds = state->options->pic_port_bounds;
	
	style1.line_mode = state->options->pic0_line_mode;
	style1.brush_mode = state->options->pic0_brush_mode;
	style1.pic_port_bounds = state->options->pic_port_bounds;

	if (!res || !res->data)
		return GFX_ERROR;

	if (state->version >= SCI_VERSION_01_VGA) {
		if (need_unscaled)
			gfxr_draw_pic1(unscaled_pic, flags, default_palette, res->size, res->data, &basic_style1, res->id);

		if (scaled_pic && scaled_pic->undithered_buffer)
			memcpy(scaled_pic->visual_map->index_data, scaled_pic->undithered_buffer, scaled_pic->undithered_buffer_size);

		gfxr_draw_pic1(scaled_pic, flags, default_palette, res->size, res->data, &style1, res->id);
	} else {
		if (need_unscaled)
			gfxr_draw_pic0(unscaled_pic, flags, default_palette, res->size, res->data, &basic_style0, res->id);

		if (scaled_pic && scaled_pic->undithered_buffer)
			memcpy(scaled_pic->visual_map->index_data, scaled_pic->undithered_buffer, scaled_pic->undithered_buffer_size);

		gfxr_draw_pic0(scaled_pic, flags, default_palette, res->size, res->data, &style0, res->id);
		if (need_unscaled)
			gfxr_remove_artifacts_pic0(scaled_pic, unscaled_pic);

		if (!scaled_pic->undithered_buffer)
			scaled_pic->undithered_buffer = sci_malloc(scaled_pic->undithered_buffer_size);

		memcpy(scaled_pic->undithered_buffer, scaled_pic->visual_map->index_data, scaled_pic->undithered_buffer_size);

		gfxr_dither_pic0(scaled_pic, state->options->pic0_dither_mode, state->options->pic0_dither_pattern);
	}

	/* Mark default palettes */
	if (scaled_pic)
		scaled_pic->visual_map->loop = default_palette;

	if (unscaled_pic)
		unscaled_pic->visual_map->loop = default_palette;

	return GFX_OK;
}


gfxr_view_t *
gfxr_interpreter_get_view(gfx_resstate_t *state, int nr, void *internal, int palette)
{
	resource_mgr_t *resmgr = (resource_mgr_t *) state->misc_payload;
	resource_t *res = scir_find_resource(resmgr, sci_view, nr, 0);
	gfx_sci_options_t *sci_options = (gfx_sci_options_t *) internal;
	int resid = GFXR_RES_ID(GFX_RESOURCE_TYPE_VIEW, nr);

	if (!res || !res->data)
		return NULL;

	if (state->version < SCI_VERSION_01) palette=-1;

	if (state->version < SCI_VERSION_01_VGA)
		return gfxr_draw_view0(resid, res->data, res->size, palette);
	else
		return gfxr_draw_view1(resid, res->data, res->size); 

}


gfx_bitmap_font_t *
gfxr_interpreter_get_font(gfx_resstate_t *state, int nr, void *internal)
{
	resource_mgr_t *resmgr = (resource_mgr_t *) state->misc_payload;
	resource_t *res = scir_find_resource(resmgr, sci_font, nr, 0);
	gfx_sci_options_t *sci_options = (gfx_sci_options_t *) internal;

	if (!res || !res->data)
		return NULL;

	return gfxr_read_font(res->id, res->data, res->size);
}


gfx_pixmap_t *
gfxr_interpreter_get_cursor(gfx_resstate_t *state, int nr, void *internal)
{
	resource_mgr_t *resmgr = (resource_mgr_t *) state->misc_payload;
	resource_t *res = scir_find_resource(resmgr, sci_cursor, nr, 0);
	gfx_sci_options_t *sci_options = (gfx_sci_options_t *) internal;
	int resid = GFXR_RES_ID(GFX_RESOURCE_TYPE_CURSOR, nr);

	if (!res || !res->data)
		return NULL;

	if (state->version >= SCI_VERSION_1_1) {
		GFXWARN("Attempt to retreive cursor in SCI1.1 or later\n");
		return NULL;
	}

	if (state->version == SCI_VERSION_0)
		return gfxr_draw_cursor0(resid, res->data, res->size);
	else
		return gfxr_draw_cursor01(resid, res->data, res->size);
}


int *
gfxr_interpreter_get_resources(gfx_resstate_t *state, gfx_resource_types_t type,
			       int version, int *entries_nr, void *internal)
{
	resource_mgr_t *resmgr = (resource_mgr_t *) state->misc_payload;
	int restype;
	int *resources;
	int count = 0;
	int top = sci_max_resource_nr[version] + 1;
	int i;
	gfx_sci_options_t *sci_options = (gfx_sci_options_t *) internal;

	switch (type) {

	case GFX_RESOURCE_TYPE_VIEW: restype = sci_view;
		break;

	case GFX_RESOURCE_TYPE_PIC: restype = sci_pic;
		break;

	case GFX_RESOURCE_TYPE_CURSOR: restype = sci_cursor;
		break;

	case GFX_RESOURCE_TYPE_FONT: restype = sci_font;
		break;

	default:
		GFX_DEBUG("Unsupported resource %d\n", type);
		return NULL; /* unsupported resource */

	}

	resources = sci_malloc(sizeof(int) * top);

	for (i = 0; i < top; i++)
		if (scir_test_resource(resmgr, restype, i))
			resources[count++] = i;

	*entries_nr = count;

	return resources;
}

gfx_pixmap_color_t *
gfxr_interpreter_get_static_palette(gfx_resstate_t *state, int version, int *colors_nr, void *internal)
{
	gfx_sci_options_t *sci_options = (gfx_sci_options_t *) internal;

	if (version >= SCI_VERSION_01_VGA)
		return gfxr_interpreter_get_palette(state, version, colors_nr, internal, 999);

	*colors_nr = GFX_SCI0_PIC_COLORS_NR;
	return gfx_sci0_pic_colors;
}

gfx_pixmap_color_t *
gfxr_interpreter_get_palette(gfx_resstate_t *state, int version, int *colors_nr, 
			     void *internal, int nr)
{
	gfx_sci_options_t *sci_options = (gfx_sci_options_t *) internal;
	resource_mgr_t *resmgr = (resource_mgr_t *) state->misc_payload;
	resource_t *res;

	if (version < SCI_VERSION_01_VGA)
		return NULL;

	res = scir_find_resource(resmgr, sci_palette, nr, 0);
	if (!res || !res->data)
		return NULL;

	switch (version)
	{
	case SCI_VERSION_01_VGA :
	case SCI_VERSION_1_EARLY :
	case SCI_VERSION_1_LATE :
		return gfxr_read_pal1(res->id, colors_nr, res->data, res->size);
	case SCI_VERSION_1_1 :
	case SCI_VERSION_32 :
		GFX_DEBUG("Palettes are not yet supported in this SCI version\n");
		return NULL;

	default:
		BREAKPOINT();
		return NULL;
	}
}

int
gfxr_interpreter_needs_multicolored_pointers(int version, void *internal)
{
	gfx_sci_options_t *sci_options = (gfx_sci_options_t *) internal;

	return (version > SCI_VERSION_1);
}



