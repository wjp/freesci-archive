/***************************************************************************
 sci_widgets.c Copyright (C) 2000 Christoph Reichenbach


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

#include <gfx_operations.h>
#include <gfx_widgets.h>
#include <sci_widgets.h>
#include <engine.h>


void
sciw_set_status_bar(state_t *s, gfxw_port_t *status_bar, char *text)
{
	gfx_state_t *state = s->gfx_state;
	fprintf(stderr,"SETTING STATUS BAR!\n");
	SCI_MEMTEST;

	if (status_bar->contents) {
		gfxw_free(state, status_bar->contents);
		status_bar->contents = NULL;
		status_bar->lastp = &(status_bar->contents);
	}

	if (text) {
		gfxw_box_t *bgbox = gfxw_new_box(state, gfx_rect(0, 0, status_bar->bounds.xl, status_bar->bounds.yl - 1),
						 s->ega_colors[0xf], s->ega_colors[0xf], GFX_BOX_SHADE_FLAT);
		gfxw_primitive_t *line = gfxw_new_line(gfx_rect(0, status_bar->bounds.yl - 1, status_bar->bounds.xl, 0),
						 s->ega_colors[0], GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL);
		gfxw_list_t *list = gfxw_new_list(status_bar->bounds);
		gfxw_text_t *textw = gfxw_new_text(state, gfx_rect(0, 0, status_bar->bounds.xl, status_bar->bounds.yl),
						  status_bar->font_nr, text, ALIGN_LEFT, ALIGN_CENTER,
						  s->ega_colors[0], s->ega_colors[0], s->ega_colors[0xf], 1);

		gfxw_add(state, (gfxw_container_t *) list, (gfxw_widget_t *) textw);
		gfxw_add(state, (gfxw_container_t *) list, (gfxw_widget_t *) line);
		gfxw_add(state, (gfxw_container_t *) list, (gfxw_widget_t *) bgbox);

		gfxw_add(state, (gfxw_container_t *) status_bar, (gfxw_widget_t *) list);
	}

	SCI_MEMTEST;
	sciprintf("vv---------------\n");
	gfxw_print((gfxw_widget_t *) status_bar);
	sciprintf("^^---------------\n");
	gfxw_draw(state, (gfxw_widget_t *) status_bar);
	gfxop_update(state);
}


gfxw_port_t *
sciw_new_window(state_t *s, rect_t area, int font, gfx_color_t color, gfx_color_t bgcolor,
		int title_font, gfx_color_t title_color, gfx_color_t title_bgcolor,
		char *title, int flags)
{
	gfxw_visual_t *visual = s->visual;
	gfx_state_t *state = s->gfx_state;
	int shadow_offset = 2;
	rect_t frame;
	gfx_color_t black;
	gfxw_port_t *win = gfxw_new_port(visual, s->port, area, color, bgcolor);
	gfxw_list_t *decorations;
	
	win->font_nr = font;

	if (flags & WINDOW_FLAG_DONTDRAW)
		flags = WINDOW_FLAG_TRANSPARENT | WINDOW_FLAG_NOFRAME;

	if (flags == (WINDOW_FLAG_TRANSPARENT | WINDOW_FLAG_NOFRAME))
		return win; /* Fully transparent window */ 


	if (flags & WINDOW_FLAG_TITLE)
		frame = gfx_rect(area.x -1, area.y -11, area.xl + 2, area.yl + 12);
	else
		frame = gfx_rect(area.x -1, area.y -1, area.xl + 2, area.yl + 2);

	/* Set visible window boundaries */
	win->bounds = gfx_rect(frame.x, frame.y, frame.xl + shadow_offset, frame.yl + shadow_offset);

	decorations = gfxw_new_list(gfx_rect(frame.x, frame.y,
					     frame.xl + 1 + shadow_offset, frame.yl + 1 + shadow_offset));

	if (!(flags & WINDOW_FLAG_TRANSPARENT))
		/* Draw window background */
		decorations = (gfxw_list_t *)
			gfxw_add_to_back((gfxw_container_t *)
					 decorations, (gfxw_widget_t *)
					 gfxw_new_box(state, gfx_rect(1, (flags & WINDOW_FLAG_TITLE)? 11 : 1, area.xl, area.yl),
						      bgcolor, bgcolor, GFX_BOX_SHADE_FLAT));

	if (flags & WINDOW_FLAG_TITLE) {
		/* Add window title */
		rect_t title_rect = gfx_rect(1, 1, area.xl, 10);

		decorations = (gfxw_list_t *)
			gfxw_add_to_front((gfxw_container_t *)
					  decorations, (gfxw_widget_t *)
					  gfxw_new_box(state, title_rect, title_bgcolor, title_bgcolor, GFX_BOX_SHADE_FLAT));

		decorations = (gfxw_list_t *)
			gfxw_add_to_front((gfxw_container_t *)
					  decorations, (gfxw_widget_t *)
					  gfxw_new_text(state, title_rect, title_font, title,
							ALIGN_CENTER, ALIGN_CENTER, title_color, title_color,
							title_bgcolor, 1));
	}

	if (!(flags & WINDOW_FLAG_NOFRAME)) {
		/* Draw backdrop shadow */

		if (gfxop_set_color(state, &black, 0, 0, 0, 0x80, bgcolor.priority, -1)) {
			GFXERROR("Could not get black/semitrans color entry!\n");
			return NULL;
		}

		decorations = (gfxw_list_t *)
			gfxw_add_to_back((gfxw_container_t *)
					 decorations, (gfxw_widget_t *)
					 gfxw_new_box(state, gfx_rect(frame.xl - 1, shadow_offset + 1,
								      shadow_offset, frame.yl - 2),
						      black, black, GFX_BOX_SHADE_FLAT));

		decorations = (gfxw_list_t *)
			gfxw_add_to_back((gfxw_container_t *)
					 decorations, (gfxw_widget_t *)
					 gfxw_new_box(state, gfx_rect(shadow_offset + 1, frame.yl - 1,
								      frame.xl - 3, shadow_offset),
						      black, black, GFX_BOX_SHADE_FLAT));

		/* Draw frame */

		if (gfxop_set_color(state, &black, 0, 0, 0, 0, bgcolor.priority, -1)) {
			GFXERROR("Could not get black color entry!\n");
			return NULL;
		}

		decorations = (gfxw_list_t *)
			gfxw_add_to_front((gfxw_container_t *)
					  decorations, (gfxw_widget_t *)
					  gfxw_new_rect(gfx_rect(0, 0, frame.xl-1, frame.yl-1),
							black, GFX_LINE_MODE_FINE, GFX_LINE_STYLE_NORMAL));

		if (flags & WINDOW_FLAG_TITLE)
			decorations = (gfxw_list_t *)
				gfxw_add_to_front((gfxw_container_t *)
						  decorations, (gfxw_widget_t *)
						  gfxw_new_line(gfx_rect(1, 10, frame.xl - 3, 0),
								black, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL));
		/*			decorations = (gfxw_list_t *)
				gfxw_add_to_front((gfxw_container_t *)
						  decorations, (gfxw_widget_t *)
						  gfxw_new_rect(gfx_rect(0, 0, frame.xl, 11),
						  black, GFX_LINE_MODE_FINE, GFX_LINE_STYLE_NORMAL));*/

	}

	win->decorations = decorations;
	return win;
}


