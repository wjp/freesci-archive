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

#define SCI_SPECIAL_CHAR_ARROW_UP 0x18
#define SCI_SPECIAL_CHAR_ARROW_DOWN 0x19

void
sciw_set_status_bar(state_t *s, gfxw_port_t *status_bar, char *text)
{
	gfx_state_t *state;
	gfxw_list_t *list;
	fprintf(stderr,"SETTING STATUS BAR!\n");
	SCI_MEMTEST;

	if (!status_bar->visual) {
		GFXERROR("Attempt to change title bar without visual!\n");
		return;
	}

	state = status_bar->visual->gfx_state;

	if (!state) {
		GFXERROR("Attempt to change title bar with stateless visual!\n");
		return;
	}

	list = gfxw_new_list(status_bar->bounds, 0);

	if (status_bar->contents) {
		status_bar->contents->free(status_bar->contents);
		status_bar->contents = NULL;
		status_bar->nextpp = &(status_bar->contents);
	}

	if (text) {
		gfxw_box_t *bgbox = gfxw_new_box(state, gfx_rect(0, 0, status_bar->bounds.xl, status_bar->bounds.yl - 1),
						 s->ega_colors[0xf], s->ega_colors[0xf], GFX_BOX_SHADE_FLAT);
		gfxw_primitive_t *line = gfxw_new_line(gfx_rect(0, status_bar->bounds.yl - 1, status_bar->bounds.xl, 0),
						 s->ega_colors[0], GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL);
		gfxw_text_t *textw = gfxw_new_text(state, gfx_rect(0, 0, status_bar->bounds.xl, status_bar->bounds.yl),
						  status_bar->font_nr, text, ALIGN_LEFT, ALIGN_CENTER,
						  s->ega_colors[0], s->ega_colors[0], s->ega_colors[0xf], GFXR_FONT_FLAG_NO_NEWLINES);


		list->add((gfxw_container_t *) list, (gfxw_widget_t *) bgbox);
		list->add((gfxw_container_t *) list, (gfxw_widget_t *) line);
		list->add((gfxw_container_t *) list, (gfxw_widget_t *) textw);

	} else {
		gfxw_box_t *bgbox = gfxw_new_box(state, gfx_rect(0, 0, status_bar->bounds.xl, status_bar->bounds.yl - 1),
						 s->ega_colors[0], s->ega_colors[0], GFX_BOX_SHADE_FLAT);

		list->add((gfxw_container_t *) list, (gfxw_widget_t *) bgbox);
	}

	list->add(GFXWC(status_bar), GFXW(list));

	status_bar->draw(GFXW(status_bar), gfxw_point_zero);
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
					     frame.xl + 1 + shadow_offset, frame.yl + 1 + shadow_offset), 0);

	if (!(flags & WINDOW_FLAG_TRANSPARENT))
		/* Draw window background */
		decorations->add((gfxw_container_t *) decorations, (gfxw_widget_t *)
				 gfxw_new_box(state, gfx_rect(1, (flags & WINDOW_FLAG_TITLE)? 11 : 1, area.xl, area.yl),
					      bgcolor, bgcolor, GFX_BOX_SHADE_FLAT));

	if (flags & WINDOW_FLAG_TITLE) {
		/* Add window title */
		rect_t title_rect = gfx_rect(1, 1, area.xl, 10);

		decorations->add((gfxw_container_t *) decorations, (gfxw_widget_t *)
				 gfxw_new_box(state, title_rect, title_bgcolor, title_bgcolor, GFX_BOX_SHADE_FLAT));

		decorations->add((gfxw_container_t *) decorations, (gfxw_widget_t *)
				 gfxw_new_text(state, title_rect, title_font, title,
					       ALIGN_CENTER, ALIGN_CENTER, title_color, title_color,
					       title_bgcolor, GFXR_FONT_FLAG_NO_NEWLINES));
	}

	if (!(flags & WINDOW_FLAG_NOFRAME)) {
		/* Draw backdrop shadow */

		if (gfxop_set_color(state, &black, 0, 0, 0, 0x80, bgcolor.priority, -1)) {
			GFXERROR("Could not get black/semitrans color entry!\n");
			return NULL;
		}

		decorations->add((gfxw_container_t *) decorations, (gfxw_widget_t *)
				 gfxw_new_box(state, gfx_rect(shadow_offset + 1, frame.yl - 1,
							      frame.xl - 3, shadow_offset),
					      black, black, GFX_BOX_SHADE_FLAT));

		decorations->add((gfxw_container_t *) decorations, (gfxw_widget_t *)
				 gfxw_new_box(state, gfx_rect(frame.xl - 1, shadow_offset + 1,
							      shadow_offset, frame.yl - 2),
					      black, black, GFX_BOX_SHADE_FLAT));

		/* Draw frame */

		if (gfxop_set_color(state, &black, 0, 0, 0, 0, bgcolor.priority, -1)) {
			GFXERROR("Could not get black color entry!\n");
			return NULL;
		}

		decorations->add((gfxw_container_t *) decorations, (gfxw_widget_t *)
				 gfxw_new_rect(gfx_rect(0, 0, frame.xl-1, frame.yl-1),
					       black, GFX_LINE_MODE_FINE, GFX_LINE_STYLE_NORMAL));

		if (flags & WINDOW_FLAG_TITLE)
			decorations->add((gfxw_container_t *) decorations, (gfxw_widget_t *)
					 gfxw_new_line(gfx_rect(1, 10, frame.xl - 3, 0),
						       black, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL));

	}

	win->decorations = decorations;
	decorations->parent = GFXWC(win);

	return win;
}



/*----------------*/
/**** Controls ****/
/*----------------*/
static inline rect_t
_move_rect(rect_t rect, point_t point)
{
	return gfx_rect(rect.x + point.x, rect.y + point.y, rect.xl, rect.yl);
}


gfxw_list_t *
_sciw_add_text_to_list(gfxw_list_t *list, gfxw_port_t *port, rect_t zone, char *text,
		       int font, gfx_alignment_t align, char framed, char inverse, int flags,
		       char gray_text)
{
	gfx_color_t *color1, *color2, *bgcolor;

	if (inverse) {
		color1 = color2 = &(port->bgcolor);
		bgcolor = &(port->color);
	} else if (gray_text) {
		bgcolor = color1 = &(port->bgcolor);
		color2 = &(port->color);
	} else {
		color1 = color2 = &(port->color);
		bgcolor = &(port->bgcolor);
	}

	list->add(GFXWC(list), GFXW(gfxw_new_text(port->visual->gfx_state, zone,
						  font, text, align, ALIGN_TOP,
						  *color1, *color2, *bgcolor, flags)));
	if (framed)
		list->add(GFXWC(list),
			  GFXW(gfxw_new_rect(zone, *color2, GFX_LINE_MODE_FINE,
					     GFX_LINE_STYLE_STIPPLED)));
	return list;
}

gfxw_list_t *
sciw_new_button_control(gfxw_port_t *port, int ID, rect_t zone, char *text, int font, char selected, char inverse, char grayed_out)
{
        gfx_color_t *frame_col = (inverse)? &(port->bgcolor) : &(port->color);
	gfxw_list_t *list = gfxw_new_list(_move_rect(zone, gfx_point(port->zone.x, port->zone.y)), 0);

	gfxw_set_id(GFXW(list), ID);

	zone.x = 0;
	zone.y = 0;

        if (inverse)
                list->add(GFXWC(list), GFXW(gfxw_new_box(NULL, gfx_rect(zone.x + 1, zone.y + 1, zone.xl - 2, zone.yl - 2),
							 port->color, port->color, GFX_BOX_SHADE_FLAT)));

        list = _sciw_add_text_to_list(list, port, gfx_rect(zone.x, zone.y + 2, zone.xl, zone.yl),
				      text, font, ALIGN_CENTER, 0, inverse, GFXR_FONT_FLAG_EAT_TRAILING_LF, grayed_out);

        list->add(GFXWC(list),
                  GFXW(gfxw_new_rect(zone, *frame_col, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));

        if (selected)
                list->add(GFXWC(list),
                          GFXW(gfxw_new_rect(gfx_rect(zone.x + 1, zone.y + 1, zone.xl - 2, zone.yl - 2),
                                             *frame_col, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));

	return list;
}


gfxw_list_t *
sciw_new_text_control(gfxw_port_t *port, int ID, rect_t zone, char *text, int font,
		      gfx_alignment_t align, char framed, char inverse)
{
	gfxw_list_t *list = gfxw_new_list(_move_rect(zone, gfx_point(port->zone.x, port->zone.y)), 0);

	gfxw_set_id(GFXW(list), ID);

	zone.x = 0;
	zone.y = 0;

	return _sciw_add_text_to_list(list, port, zone, text, font, align, framed, inverse, 0, port->gray_text);
}


gfxw_list_t *
sciw_new_edit_control(gfxw_port_t *port, int ID, rect_t zone, char *text, int font, int cursor,
		      char inverse)
{
	gfxw_list_t *list = gfxw_new_list(_move_rect(zone, gfx_point(port->zone.x, port->zone.y)), 0);
	gfxw_text_t *text_handle;
	char *textdup = malloc(strlen(text) + 1);

	gfxw_set_id(GFXW(list), ID);
	zone.x = 0;
	zone.y = 1;

	strncpy(textdup, text, cursor);

	if (cursor <= strlen(text))
		textdup[cursor] = 0; /* terminate */

	if (cursor > 0) {
		text_handle = gfxw_new_text(port->visual->gfx_state, zone,
					    font, textdup, ALIGN_LEFT, ALIGN_TOP,
					    port->color, port->color, port->bgcolor, GFXR_FONT_FLAG_NO_NEWLINES);

		list->add(GFXWC(list), GFXW(text_handle));
		zone.x += text_handle->width;
	}

	if (cursor < strlen(text)) {
		textdup[0] = text[cursor];
		textdup[1] = 0;
		text_handle =  gfxw_new_text(port->visual->gfx_state, zone,
					     font, textdup, ALIGN_LEFT, ALIGN_TOP,
					     port->bgcolor, port->bgcolor, port->color, GFXR_FONT_FLAG_NO_NEWLINES);
		list->add(GFXWC(list), GFXW(text_handle));
		zone.x += text_handle->width;
	};

	if (cursor+1 < strlen(text)) {
		text_handle = gfxw_new_text(port->visual->gfx_state, zone,
					    font, text + cursor + 1, ALIGN_LEFT, ALIGN_TOP,
					    port->color, port->color, port->bgcolor, GFXR_FONT_FLAG_NO_NEWLINES);
		list->add(GFXWC(list), GFXW(text_handle));
		zone.x += text_handle->width;
	};

	if (cursor == strlen(text))
		list->add(GFXWC(list), GFXW(gfxw_new_line(gfx_rect(zone.x + 1, zone.y, 0, zone.yl - 1),
							  port->color, GFX_LINE_MODE_FAST, GFX_LINE_STYLE_NORMAL)));


	zone.x = zone.y = 0;

        list->add(GFXWC(list),
                  GFXW(gfxw_new_rect(zone, port->color, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));

	free(textdup);
	return list;
}


gfxw_list_t *
sciw_new_icon_control(gfxw_port_t *port, int ID, rect_t zone, int view, int loop, int cel,
		      char frame, char inverse)
{
	gfxw_list_t *list = gfxw_new_list(_move_rect(zone, gfx_point(port->zone.x, port->zone.y)), 0);
	gfxw_widget_t *icon;
	gfxw_set_id(GFXW(list), ID);

	if (!port->visual) {
		GFXERROR("Attempting to create icon control for virtual port!\n");
		return NULL;
	}

	zone.x = 0;
	zone.y = 0;

	icon = GFXW(gfxw_new_view(port->visual->gfx_state, gfx_point(zone.x, zone.y), view, loop, cel, -1, -1,
				  ALIGN_LEFT, ALIGN_TOP, GFXW_VIEW_FLAG_DONT_MODIFY_OFFSET));

	if (!icon) {
		GFXERROR("Attempt to create icon control with cel %d/%d/%d (invalid)\n", view, loop, cel);
		return NULL;
	}

	list->add(GFXWC(list), icon);

	return list;
}


gfxw_list_t *
sciw_new_list_control(gfxw_port_t *port, int ID, rect_t zone, int font_nr, char **entries_list,
		      int entries_nr, int list_top, int selection, char inverse)
{
	gfxw_list_t *list = gfxw_new_list(_move_rect(zone, gfx_point(port->zone.x, port->zone.y)), 0);
	char arr_up[2], arr_down[2];
	int i;
	int font_height = gfxop_get_font_height(port->visual->gfx_state, font_nr);
	int columns = (zone.yl - 20);

	if (font_height <= 0) {
		GFXERROR("Attempt to create list control with invalid font %d\n", font_nr);
		list->free(GFXWC(list));
		return NULL;
	}

	columns /= font_height;

	gfxw_set_id(GFXW(list), ID);

	arr_up[0] = SCI_SPECIAL_CHAR_ARROW_UP;
	arr_down[0] = SCI_SPECIAL_CHAR_ARROW_DOWN;
	arr_up[1] = arr_down[1] = 0;

	zone.x = 0;
	zone.y = 0;

        list->add(GFXWC(list),
                  GFXW(gfxw_new_rect(zone, port->color, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));

        list->add(GFXWC(list),
                  GFXW(gfxw_new_rect(gfx_rect(zone.x, zone.y + 10, zone.xl, zone.yl - 20),
				     port->color, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));

	zone.x = 1;
	zone.y = 11;

	for (i = list_top; columns-- && i < entries_nr; i++) {

		if (i == selection)
			list->add(GFXWC(list),
				  GFXW(gfxw_new_text(port->visual->gfx_state, gfx_rect(zone.x, zone.y, zone.xl - 2, font_height),
						     port->font_nr, entries_list[i], ALIGN_LEFT, ALIGN_TOP,
						     port->color, port->color, port->bgcolor, GFXR_FONT_FLAG_NO_NEWLINES)));
		else
			list->add(GFXWC(list),
				  GFXW(gfxw_new_text(port->visual->gfx_state, gfx_rect(zone.x, zone.y, zone.xl - 2, font_height),
						     port->font_nr, entries_list[i], ALIGN_LEFT, ALIGN_TOP,
						     port->bgcolor, port->bgcolor, port->color, GFXR_FONT_FLAG_NO_NEWLINES)));

		zone.y += font_height;
	}

	/* Add up arrow */
	list->add(GFXWC(list),
		  GFXW(gfxw_new_text(port->visual->gfx_state, gfx_rect(1, 1, zone.xl-2, 8),
				     port->font_nr, arr_up, ALIGN_CENTER, ALIGN_CENTER,
				     port->color, port->color, port->bgcolor, 0)));

	/* Add down arrow */
	list->add(GFXWC(list),
		  GFXW(gfxw_new_text(port->visual->gfx_state, gfx_rect(1, zone.yl-9, zone.xl-2, 8),
				     port->font_nr, arr_down, ALIGN_CENTER, ALIGN_CENTER,
				     port->color, port->color, port->bgcolor, 0)));

	return list;
}



