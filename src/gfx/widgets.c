/***************************************************************************
 widgets.c Copyright (C) 2000 Christoph Reichenbach


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

#include <gfx_widgets.h>

#ifdef GFXW_DEBUG_WIDGETS
gfxw_widget_t *debug_widgets[GFXW_DEBUG_WIDGETS];
int debug_widget_pos = 0;
#define inline
static void
_gfxw_debug_add_widget(gfxw_widget_t *widget)
{
	if (debug_widget_pos == GFXW_DEBUG_WIDGETS) {
		GFXERROR("WIDGET DEBUG: Allocated the maximum of %d widgets- Aborting!\n", GFXW_DEBUG_WIDGETS);
		BREAKPOINT();
	}
	debug_widgets[debug_widget_pos++] = widget;
	GFXDEBUG("Added widget: %d active\n", debug_widget_pos);
}

static void
_gfxw_debug_remove_widget(gfxw_widget_t *widget) {
	int i;
	int found = 0;
	for (i = 0; i < debug_widget_pos; i++) {
		if (debug_widgets[i] == widget) {
			memcpy(debug_widgets + i, debug_widgets + i + 1,
			       (sizeof (gfxw_widget_t *)) * (debug_widget_pos - i - 1));
			debug_widgets[debug_widget_pos--] = NULL;
			found++;
		}
	}

	if (found > 1) {
		GFXERROR("While removing widget: Found it %d times!\n", found);
		BREAKPOINT();
	}

	if (found == 0) {
		GFXERROR("Failed to remove widget!\n");
		BREAKPOINT();
	}
	GFXDEBUG("Removed widget: %d active now\n", debug_widget_pos);
}
#else /* !GFXW_DEBUG_WIDGETS */
#define _gfxw_debug_add_widget(a)
#define _gfxw_debug_remove_widget(a)
#endif

static gfxw_widget_t *
_gfxw_new_widget(int size, int type)
{
	gfxw_widget_t *widget = malloc(size);
	widget->magic = GFXW_MAGIC_VALID;
	widget->next = NULL;
	widget->type = type;
	widget->bounds = gfx_rect(0, 0, 0, 0);
	widget->flags = GFXW_FLAG_DIRTY;
	widget->ID = GFXW_NO_ID;

	_gfxw_debug_add_widget(widget);

	return widget;
}


static inline int
verify_widget(gfxw_widget_t *widget)
{
	if (!widget) {
		GFXERROR("Attempt to use NULL widget\n");
#ifdef GFXW_DEBUG_WIDGETS
		BREAKPOINT();
#endif /* GFXW_DEBUG_WIDGETS */
		return 1;
	} else if (widget->magic != GFXW_MAGIC_VALID) {
		if (widget->magic = GFXW_MAGIC_INVALID) {
			GFXERROR("Attempt to use invalidated widget\n");
		} else {
			GFXERROR("Attempt to use non-widget\n");
		}
#ifdef GFXW_DEBUG_WIDGETS
		BREAKPOINT();
#endif /* GFXW_DEBUG_WIDGETS */
		return 1;
	}
	return 0;
}

#define VERIFY_WIDGET(w) \
  if (verify_widget((gfxw_widget_t *)(w))) { GFXERROR("Error occured while validating widget\n"); }

static void
_gfxw_unallocate_widget(gfx_state_t *state, gfxw_widget_t *widget)
{
	if (GFXW_IS_TEXT(widget)) {
		gfxw_text_t *text = (gfxw_text_t *) widget;

		if (!state) {
			GFXERROR("Attempt to free text without supplying mode to free it from!\n");
			BREAKPOINT();
		} else

		if (text->text_handle) {
			gfxop_free_text(state, text->text_handle);
			text->text_handle = NULL;
		}
	}

	widget->magic = GFXW_MAGIC_INVALID;
	free(widget);
	_gfxw_debug_remove_widget(widget);
}

#define WIDGET_OPERATION(op) int op(gfx_state_t *state, gfxw_widget_t *widget, rect_t bounds)
/*
** Parameters: (gfx_state_t *) state: The graphics state to use, or NULL if nothing should be drawn
**             (gfxw_widget_t *) widget: The widget to operate on
**             (rect_t) bounds: The boundaries of the underlying window
** Returns   : (int) Whether the widget should be removed (non-zero) or not (zero)
*/


#define OPERATE_FLAG_LIST 1
#define OPERATE_FLAG_PRINT 2

static inline void
tabulate(int c)
{
	while (c--)
		sciprintf(" ");
}

static void
_gfxw_print_widget(gfxw_widget_t *widget);

static gfxw_widget_t *
_gfxw_operate(gfx_state_t *state, int indent, gfxw_widget_t *widget, WIDGET_OPERATION(operation), rect_t bounds, int flags)
{
	gfxw_widget_t *next;
	VERIFY_WIDGET(widget);

	if (flags & OPERATE_FLAG_PRINT) {
		tabulate(indent);
		_gfxw_print_widget(widget);
	}

	if (widget->flags & GFXW_FLAG_CONTAINER) {
		gfxw_container_t *cwidget = (gfxw_container_t *) widget;

		if (GFXW_IS_PORT(cwidget)) {
			gfxw_port_t *port = (gfxw_port_t *) cwidget;
			if (port->decorations) {
				if (flags & OPERATE_FLAG_PRINT) {
					tabulate(indent);
					sciprintf("--decorations:\n");
				}
				port->decorations = (gfxw_container_t *) _gfxw_operate(state, indent + 4,
                                     (gfxw_widget_t *) port->decorations, operation, cwidget->zone, flags & ~OPERATE_FLAG_LIST);
			}
		}
		if (cwidget->contents) {
			if (flags & OPERATE_FLAG_PRINT) {
				tabulate(indent);
				sciprintf("--contents:\n");
			}
			cwidget->contents = _gfxw_operate(state, indent + 4, cwidget->contents, operation, cwidget->zone, flags | OPERATE_FLAG_LIST);
		}

	}

	if (flags & OPERATE_FLAG_LIST && widget->next)
		widget->next = next = _gfxw_operate(state, indent, widget->next, operation, bounds, flags);
	else next = NULL;

	if (operation(state, widget, bounds)) {
		_gfxw_unallocate_widget(state, widget);

		return next;
	}

	return widget;
}


static WIDGET_OPERATION(_gfxw_op_free)
{
	if (state)
		gfxop_clear_box(state, widget->bounds);

	return 1;
}

static WIDGET_OPERATION(_gfxw_op_free_tagged)
{
	return (widget->flags & GFXW_FLAG_TAGGED);
}

static WIDGET_OPERATION(_gfxw_op_tag)
{
	if (!GFXW_IS_TEXT(widget)) /* Text must not be tagged */
		widget->flags |= GFXW_FLAG_TAGGED;
	return 0;
}

static rect_t no_rect = {-1, -1, -1, -1}; /* Never matched by gfx_rects_overlap() */
static rect_t fullscreen_rect = {0, 0, 320, 200};

void
gfxw_free(gfx_state_t *state, gfxw_widget_t *widget)
{
	_gfxw_operate(state, 0, widget, _gfxw_op_free, no_rect, 0);
}

gfxw_widget_t *
gfxw_tag(gfxw_widget_t *widget)
{
	return (_gfxw_operate(NULL, 0, widget, _gfxw_op_tag, no_rect, 0));
}

void
gfxw_free_tagged(gfxw_widget_t *widget)
{
	if (_gfxw_operate(NULL, 0, widget, _gfxw_op_free_tagged, no_rect, 0) != widget) {
		GFXERROR("Error: Attempt to free list element manually (by tag)- segfault imminent!\n");
		BREAKPOINT();
	}
}


static inline int
_gfxw_widget_needs_check(gfxw_widget_t *widget)
{
	return (widget->type == GFXW_DYN_VIEW || widget->type == GFXW_STATIC_VIEW);
}


static inline int
_gfxw_widget_compare_to(gfx_state_t *state, gfxw_widget_t *old, gfxw_widget_t *new)
     /* Returns -1 if new < old */
{
	if (old->type == new->type) {
		switch(old->type) {

		case GFXW_DYN_VIEW: {
			gfxw_dyn_view_t *oldd = (gfxw_dyn_view_t *) old;
			gfxw_dyn_view_t *newd = (gfxw_dyn_view_t *) new;

			if (newd->bounds.y < oldd->bounds.y)
				return -1;
			if (newd->z < oldd->z)
				return -1;

			return 0; /* Nope */
		}

		default:
			return 0;

		}
	}
}


static inline int
_gfxw_widget_merge_if_equals(gfx_state_t *state, gfxw_widget_t *old, gfxw_widget_t *new)
     /* Assumes that, if 1 is returned, 'new' is freed */
{
	if (old->type == new->type) {
		switch(old->type) {

		case GFXW_DYN_VIEW: {
			gfxw_dyn_view_t *oldd = (gfxw_dyn_view_t *) old;
			gfxw_dyn_view_t *newd = (gfxw_dyn_view_t *) new;

			if (oldd->ID == newd->ID) {
				gfxw_widget_t *old_next = old->next;

				if (oldd->view == newd->view
				    && oldd->loop == newd->loop
				    && oldd->cel == newd->cel
				    && oldd->bounds.x == newd->bounds.x
				    && oldd->bounds.y == newd->bounds.y
				    && oldd->color.mask == newd->color.mask
				    && (!(oldd->color.mask & GFX_MASK_PRIORITY)
					|| oldd->color.priority == newd->color.priority)
				    && (!(oldd->color.mask & GFX_MASK_CONTROL)
					|| oldd->color.control == newd->color.control))
					return 1; /* Old and new one are identical; no need to do anything */

				gfxop_clear_box(state, oldd->bounds);

				memcpy(oldd, newd, sizeof(gfxw_dyn_view_t));
				oldd->next = old_next;
				return 1;
			}
		}

		case GFXW_TEXT: {
			gfxw_text_t *oldt = (gfxw_text_t *) old;
			gfxw_text_t *newt = (gfxw_text_t *) new;

			if (oldt->bounds.x == newt->bounds.x
			    && oldt->bounds.y == newt->bounds.y) {
				gfx_text_handle_t *swap = newt->text_handle;
				newt->text_handle = oldt->text_handle;
				oldt->text_handle = swap; /* This way, the old handle will be freed by the calling function */
				oldt->bounds = newt->bounds;

				if (strcmp(oldt->text, newt->text)
				    || oldt->font_nr != newt->font_nr
				    || oldt->bounds.x != newt->bounds.x
				    || oldt->bounds.y != newt->bounds.y
				    || oldt->bounds.xl != newt->bounds.xl
				    || oldt->bounds.yl != newt->bounds.yl)
					oldt->flags |= GFXW_FLAG_DIRTY;

				return 1;
			}
		}

		default: {
			/* ignore (required for IRIX cc, maybe it's even required by ANSI C (?)) */
		}

		}
	}
	return 0;
}

gfxw_container_t *
gfxw_add_to_front(gfxw_container_t *container, gfxw_widget_t *widget)
{
	VERIFY_WIDGET(container);
	VERIFY_WIDGET(widget);

	if (widget->next) {
		GFXERROR("Attempt to add part of a list to a container!\n");
		sciprintf("Offending container:");
		gfxw_print((gfxw_widget_t *) container);
		sciprintf("Offending widget:");
		gfxw_print((gfxw_widget_t *) widget);
	}

	if (container->lastp == &(container->contents))
		container->lastp = &(widget->next);

	widget->next = container->contents;
	container->contents = widget;

	return container;
}

gfxw_container_t *
gfxw_add_to_back(gfxw_container_t *container, gfxw_widget_t *widget)
{
	VERIFY_WIDGET(container);
	VERIFY_WIDGET(widget);

	if (widget->next) {
		GFXERROR("Attempt to add part of a list to a container!\n");
		sciprintf("Offending container:");
		gfxw_print((gfxw_widget_t *) container);
		sciprintf("Offending widget:");
		gfxw_print((gfxw_widget_t *) widget);
	}

	*(container->lastp) = widget;
	container->lastp = &(widget->next);
	gfxw_print(widget);

	return container;
}

gfxw_container_t *
gfxw_add(gfx_state_t *state, gfxw_container_t *container, gfxw_widget_t *widget)
{
	VERIFY_WIDGET(container);
	VERIFY_WIDGET(widget);

	if (_gfxw_widget_needs_check(widget)) {
		gfxw_widget_t *seeker = container->contents;
		gfxw_widget_t **oldp = &(container->contents);

		while (seeker && seeker != widget) {
			if (_gfxw_widget_merge_if_equals(state, seeker, widget) == 1) {
				_gfxw_unallocate_widget(state, widget);
				*oldp = seeker->next;
				seeker->next = NULL;
				widget = seeker;
			} else {
				oldp = &(seeker->next);
				seeker = seeker->next;
			}
		}

		/* Replacing didn't work, so let's sort it in */
		seeker = container->contents;
		oldp = &(container->contents);

		while (seeker) {
			if (_gfxw_widget_compare_to(state, seeker, widget) == -1) {
				/* Sort in before the 'seeker' */
				*oldp = widget;
				widget->next = seeker;
				return container;
			}
			seeker = seeker->next;
			oldp = &(seeker->next);
		}
	}

	*(container->lastp) = widget;
	container->lastp = &(widget->next);
	gfxw_print(widget);

	return container;
}



static inline rect_t
gfxw_rects_merge(rect_t a, rect_t b)
{
	if (a.xl == -1) /* no_rect */
		return b;
	if (b.xl == -1) /* no_rect */
		return a;

	return gfx_rects_merge(a, b);
}

static inline int
gfxw_rect_isin(rect_t a, rect_t b)
     /* Returns whether forall x. x in a -> x in b */
{
	return b.x >= a.x
		&& b.y >= a.y
		&& (b.x + b.xl) <= (a.x + a.xl)
		&& (b.y + b.yl) <= (a.y + a.yl);
}

/* Unfortunately, this function can't be mapped into the _gfxw_operate() scheme without breaking
** that function's (relatively) clean design considerably. Sorry for that. I'm open for suggestions,
** though...
*/
static rect_t
_gfxw_propagate_dirtyness(gfxw_widget_t *widget, int list)
{
	rect_t retval;
	VERIFY_WIDGET(widget);

	if (widget->flags & GFXW_FLAG_VISIBLE
	    && widget->flags & GFXW_FLAG_DIRTY) {
		retval = widget->bounds;
	} else retval = no_rect;

	if (widget->flags & GFXW_FLAG_CONTAINER) {
		gfxw_container_t *cwidget = (gfxw_container_t *) widget;

		if (cwidget->contents)
			retval = gfxw_rects_merge(retval, _gfxw_propagate_dirtyness(cwidget->contents, 1));

		if (GFXW_IS_PORT(cwidget)) {
			gfxw_port_t *port = (gfxw_port_t *) cwidget;
			if (port->decorations)
				retval = gfxw_rects_merge(retval, _gfxw_propagate_dirtyness((gfxw_widget_t *) port->decorations, 0));
		}
	}

	if (list && widget->next)
		retval = gfxw_rects_merge(retval,  _gfxw_propagate_dirtyness(widget->next, list));

	if (widget->flags & GFXW_FLAG_VISIBLE) {
		if (widget->flags & GFXW_FLAG_DIRTY)
				retval = gfx_rects_merge(retval, widget->bounds);
		else if (gfx_rects_overlap(retval, widget->bounds)) {
			/* Visible but not dirty */

			if (gfxw_rect_isin(retval, widget->bounds))
				retval = no_rect; /* Everything dirty is 'caught' by the opaque widget */

			widget->flags |= GFXW_FLAG_DIRTY;
		}
	}

	return retval;
}


#define GFX_ASSERT(__x) \
  { \
	  int retval = (__x); \
	  if (retval == GFX_ERROR) { \
		  GFXERROR("Error occured while drawing widget!\n"); \
		  return; \
	  } else if (retval == GFX_FATAL) { \
		  GFXERROR("Fatal error occured while drawing widget!\nGraphics state invalid; aborting program..."); \
		  exit(1); \
	  } \
  }


static inline rect_t
windowize_box(rect_t window, rect_t box)
{
	return gfx_rect(box.x + window.x, box.y + window.y, box.xl, box.yl);
}

static void
_gfxw_draw_box(gfx_state_t *state, rect_t window, gfxw_box_t *box)
{
	rect_t box_bounds = windowize_box(window, box->bounds);

fprintf(stderr,"Drawing box to %d,%d,%d,%d  mask %04x\n", box_bounds.x, box_bounds.y, box_bounds.xl, box_bounds.yl, box->color1.mask);
	GFX_ASSERT(gfxop_draw_box(state, box_bounds, box->color1, box->color2, box->shade_type));
}

static void
_gfxw_draw_primitive(gfx_state_t *state, rect_t window, gfxw_primitive_t *primitive,
		     int draw_op(gfx_state_t *, rect_t, gfx_color_t, gfx_line_mode_t, gfx_line_style_t))
{
	rect_t primitive_bounds = windowize_box(window, primitive->bounds);

	GFX_ASSERT(draw_op(state, primitive_bounds, primitive->color, primitive->line_mode, primitive->line_style));
}

_gfxw_draw_view(gfx_state_t *state, rect_t window, gfxw_view_t *view,
		int draw_op(gfx_state_t *, int, int, int, point_t, gfx_color_t))
{
	point_t pos = gfx_point(window.x + view->pos.x, window.y + view->pos.y);

	GFX_ASSERT(draw_op(state, view->view, view->loop, view->cel, pos, view->color));
}

static void
_gfxw_draw_text(gfx_state_t *state, rect_t window, gfxw_text_t *text)
{
	rect_t text_box = windowize_box(window, text->bounds);

	if (!text->text_handle) {
		text->text_handle =
			gfxop_new_text(state, text->font_nr, text->text, text->bounds.xl,
				       text->halign, text->valign, text->color1,
				       text->color2, text->bgcolor, text->single_line);

	}

fprintf(stderr,"Drawing text\n");
	GFX_ASSERT(gfxop_draw_text(state, text->text_handle, text_box));
}

static WIDGET_OPERATION(_gfxw_op_draw)
{
	gfxop_set_clip_zone(state, bounds);

	if (widget->flags & GFXW_FLAG_VISIBLE
	    && widget->flags & GFXW_FLAG_DIRTY) {

		switch (widget->type) {
		case GFXW_BOX:
			_gfxw_draw_box(state, bounds, (gfxw_box_t *) widget);
			break;

		case GFXW_RECT:
		case GFXW_LINE:
			_gfxw_draw_primitive(state, bounds, (gfxw_primitive_t *) widget,
					     (widget->type == GFXW_RECT)? gfxop_draw_rectangle : gfxop_draw_line);
			break;

		case GFXW_VIEW:
		case GFXW_STATIC_VIEW:
		case GFXW_DYN_VIEW:
			_gfxw_draw_view(state, bounds, (gfxw_view_t *) widget,
					(widget->type == GFXW_STATIC_VIEW)? gfxop_draw_cel_static : gfxop_draw_cel);
			break;

		case GFXW_TEXT:
			_gfxw_draw_text(state, bounds, (gfxw_text_t *) widget);
			break;

		case GFXW_LIST:
		case GFXW_PORT:
			break;

		case GFXW_:
		case GFXW_CONTAINER:
			GFXERROR("Abstract class encountered as object type: %d\n", widget->type);
			BREAKPOINT();
			break;

		default:
			GFXERROR("Invalid class %d encountered as object type while drawing\n", widget->type);
			BREAKPOINT();
			break;
		}

		widget->flags &= ~GFXW_FLAG_DIRTY;
	}
	return 0;
}


gfxw_widget_t *
gfxw_draw(gfx_state_t *state, gfxw_widget_t *widget)
{
	rect_t dirty_zone = _gfxw_propagate_dirtyness(widget, 0);
	return (_gfxw_operate(state, 0, widget, _gfxw_op_draw, dirty_zone, 0));
}



/* Constructors (well, more like factory methods)  */

gfxw_box_t *
gfxw_new_box(gfx_state_t *state, rect_t area, gfx_color_t color1, gfx_color_t color2, gfx_box_shade_t shade_type)
{
	gfxw_box_t *widget = (gfxw_box_t *) _gfxw_new_widget(sizeof(gfxw_box_t), GFXW_BOX);

	widget->bounds = area;
	widget->color1 = color1;
	widget->color2 = color2;
	widget->shade_type = shade_type;

	widget->flags |= GFXW_FLAG_VISIBLE;

	if (color1.mask & GFX_MASK_VISUAL
	    && (state->driver->mode->palette
		|| (!color1.alpha && !color2.alpha)))
		widget->flags |= GFXW_FLAG_OPAQUE;

	return widget;
}


static inline gfxw_primitive_t *
_gfxw_new_primitive(rect_t area, gfx_color_t color, gfx_line_mode_t mode, gfx_line_style_t style, int type)
{
	gfxw_primitive_t *widget = (gfxw_primitive_t *) _gfxw_new_widget(sizeof(gfxw_primitive_t), type);
	widget->bounds = area;
	widget->color = color;
	widget->line_mode = mode;
	widget->line_style = style;

	widget->flags |= GFXW_FLAG_VISIBLE;
	return widget;
}

gfxw_primitive_t *
gfxw_new_rect(rect_t rect, gfx_color_t color, gfx_line_mode_t line_mode, gfx_line_style_t line_style)
{
	return _gfxw_new_primitive(rect, color, line_mode, line_style, GFXW_RECT);
}


gfxw_primitive_t *
gfxw_new_line(rect_t line, gfx_color_t color, gfx_line_mode_t line_mode, gfx_line_style_t line_style)
{
	return _gfxw_new_primitive(line, color, line_mode, line_style, GFXW_LINE);
}


gfxw_view_t *
_gfxw_new_simple_view(gfx_state_t *state, point_t pos, int view, int loop, int cel, int priority, int control,
		      gfx_alignment_t halign, gfx_alignment_t valign, int size, int type)
{
	gfxw_view_t *widget;
	int width, height;
	point_t offset;

	if (!state) {
		GFXERROR("Attempt to create view widget with NULL state!\n");
		return NULL;
	}

	if (gfxop_get_cel_parameters(state, view, loop, cel, &width, &height, &offset)) {
		GFXERROR("Attempt to retreive cel parameters for (%d/%d/%d) failed (Maybe the values weren't checked beforehand?)\n",
			 view, cel, loop);
		return NULL;
	}

	widget = (gfxw_view_t *) _gfxw_new_widget(size, type);

	widget->pos = pos;
	widget->color.mask =
		(priority < 0)? 0 : GFX_MASK_PRIORITY
		| (control < 0)? 0 : GFX_MASK_CONTROL;
	widget->color.priority = priority;
	widget->color.control = control;
	widget->view = view;
	widget->loop = loop;
	widget->cel = cel;

	if (halign == ALIGN_CENTER)
	  widget->pos.x -= width >> 1;
	else if (halign == ALIGN_RIGHT)
	  widget->pos.x -= width;

	if (valign == ALIGN_CENTER)
	  widget->pos.y -= height >> 1;
	else if (valign == ALIGN_BOTTOM)
	  widget->pos.y -= height;

	widget->bounds = gfx_rect(widget->pos.x - offset.x, widget->pos.y - offset.y, width, height);

	widget->flags |= GFXW_FLAG_VISIBLE;

	return widget;
}

gfxw_view_t *
gfxw_new_view(gfx_state_t *state, point_t pos, int view, int loop, int cel, int priority, int control,
	      gfx_alignment_t halign, gfx_alignment_t valign, int flags)
{
	if (flags & GFXW_VIEW_FLAG_DONT_MODIFY_OFFSET) {
		int foo;
		point_t offset;
		gfxop_get_cel_parameters(state, view, loop, cel, &foo, &foo, &offset);
fprintf(stderr,"Augmenting with (%d,%d)\n", pos.x, pos.y);
		pos.x += offset.x;
		pos.y += offset.y;
	}

	return _gfxw_new_simple_view(state, pos, view, loop, cel, priority, control, halign, valign, 
				     sizeof(gfxw_view_t), (flags & GFXW_VIEW_FLAG_STATIC) ? GFXW_STATIC_VIEW : GFXW_VIEW);
}

gfxw_dyn_view_t *
gfxw_new_dyn_view(gfx_state_t *state, point_t pos, int z, int view, int loop, int cel, int priority, int control,
		  gfx_alignment_t halign, gfx_alignment_t valign)
{
	gfxw_dyn_view_t *widget =
		(gfxw_dyn_view_t *) _gfxw_new_simple_view(state, pos, view, loop, cel, priority, halign, valign,
							  control, sizeof(gfxw_dyn_view_t),
							  GFXW_DYN_VIEW);
	if (!widget) {
		GFXERROR("Invalid view widget (%d/%d/%d)!\n", view, loop, cel);
		return NULL;
	}

	widget->pos.y += z;
	widget->z = z;

	return widget;
}

gfxw_text_t *
gfxw_new_text(gfx_state_t *state, rect_t area, int font, char *text, gfx_alignment_t halign,
	      gfx_alignment_t valign, gfx_color_t color1, gfx_color_t color2,
	      gfx_color_t bgcolor, int single_line)
{
	gfxw_text_t *widget = (gfxw_text_t *)
		_gfxw_new_widget(sizeof(gfxw_text_t), GFXW_TEXT);

	widget->bounds = area;

	widget->font_nr = font;
	widget->text = text;
	widget->halign = halign;
	widget->valign = valign;
	widget->color1 = color1;
	widget->color2 = color2;
	widget->bgcolor = bgcolor;
	widget->single_line = single_line;
	widget->text_handle = NULL;

	widget->flags |= GFXW_FLAG_VISIBLE;

	return widget;
}

/*-- Container types --*/

gfxw_container_t *
_gfxw_new_container_widget(rect_t area, int size, int type)
{
	gfxw_container_t *widget = (gfxw_container_t *)
		_gfxw_new_widget(size, type);

	widget->bounds = widget->zone = area;
	widget->contents = NULL;
	widget->lastp = &(widget->contents);

	widget->flags |= GFXW_FLAG_VISIBLE | GFXW_FLAG_CONTAINER;

	return widget;
}

gfxw_list_t *
gfxw_new_list(rect_t area)
{
	return (gfxw_list_t *) _gfxw_new_container_widget(area, sizeof(gfxw_list_t), GFXW_LIST);
}

gfxw_visual_t *
gfxw_new_visual(gfx_state_t *state, int font)
{
	gfxw_visual_t *visual = (gfxw_visual_t *) _gfxw_new_container_widget(gfx_rect(0, 0, 320, 200), sizeof(gfxw_visual_t), GFXW_VISUAL);

	visual->font_nr = font;
	visual->last_port = NULL;
	visual->ports_nr = 0;
	visual->gfx_state = state;

	return visual;
}


static int
_visual_find_free_ID(gfxw_visual_t *visual)
{
	gfxw_port_t *port = (gfxw_port_t *) visual->contents;
	int id, highest;

	if (!port)
		return 0;

	id = 1;
	highest = 0;

	while (port && port->ID <= id) {
		if (port->ID == id)
			id++;

		port = (gfxw_port_t *) port->next;
	}

	return id;
}

gfxw_port_t *
gfxw_new_port(gfxw_visual_t *visual, gfxw_port_t *predecessor, rect_t area, gfx_color_t fgcolor, gfx_color_t bgcolor)
{
	gfxw_widget_t **foo;
	gfxw_port_t **seekerp;
	gfxw_port_t *widget = (gfxw_port_t *)
		_gfxw_new_container_widget(area, sizeof(gfxw_port_t), GFXW_PORT);

	VERIFY_WIDGET(visual);

	widget->predecessor = predecessor;
	widget->decorations = NULL;
	widget->draw_pos = gfx_point(0, 0);
	widget->gray_text = 0;
	widget->color = fgcolor;
	widget->bgcolor = bgcolor;
	widget->font_nr = visual->font_nr;
	widget->ID = _visual_find_free_ID(visual);

	foo = &(visual->contents);
	seekerp = (gfxw_port_t **) foo; /* For some reason, this is neccessary... */

	while (*seekerp && (*seekerp)->ID < widget->ID)
		seekerp = (gfxw_port_t **) &((*seekerp)->next);

	widget->next = (gfxw_widget_t *) *seekerp;
	*seekerp = (gfxw_port_t *) widget;

	if (!widget->next) /* Last list member? */
		visual->lastp = &(widget->next); /* Update lastpp */

	return widget;
}


gfxw_port_t *
gfxw_remove_port(gfxw_visual_t *visual, gfxw_port_t *port)
{
	gfxw_widget_t **foo;
	gfxw_port_t **seekerp;
	gfxw_port_t **found_p = NULL;
	gfxw_port_t *predecessor;

	VERIFY_WIDGET(visual);
	VERIFY_WIDGET(port);

	if (!visual->contents) {
		GFXWARN("Attempt to remove port from empty visual\n");
		return NULL;
	}


	foo = (&(visual->contents));
	seekerp = (gfxw_port_t **) foo;

	while (*seekerp) {


		if (*seekerp == port)
			found_p = seekerp;
		else if ((*seekerp)->predecessor == port)
			gfxw_remove_port(visual, *seekerp); /* Recurse to dependant ports */

		foo = &((*seekerp)->next);
		seekerp = (gfxw_port_t **) foo;
	}

	if (found_p) {
		*found_p = (gfxw_port_t *) port->next;

		if (port == visual->last_port) {
			visual->last_port = predecessor;

			if (visual->last_port)
				visual->lastp = &(visual->last_port->next);
			else
				visual->lastp = &(visual->contents);
		}
	} else GFXWARN("Attempt to remove port with ID #%d from visual!\n", port->ID);


	predecessor = port->predecessor;

	gfxw_free(visual->gfx_state, (gfxw_widget_t *) port);

	return predecessor;
}


gfxw_port_t *
gfxw_find_port(gfxw_visual_t *visual, int ID)
{
	gfxw_port_t *retval;
	VERIFY_WIDGET(visual);

	if (ID < 0)
		return NULL;

	retval = (gfxw_port_t *) visual->contents;

	while (retval && retval->ID != ID) {
		VERIFY_WIDGET(retval);
		retval = (gfxw_port_t *) retval->next;
	}

	return retval;
}


static WIDGET_OPERATION(_gfxw_nop)
{
	return 0;
}

void
gfxw_print(gfxw_widget_t *widget)
{
	_gfxw_operate(NULL, 0, widget, _gfxw_nop, gfx_rect(0, 0, 320, 200), OPERATE_FLAG_PRINT);
}

static void
_gfxw_print_widget(gfxw_widget_t *widget)
{
	char flags_list[] = "VOCDT";
	int i;
	gfxw_view_t *view = (gfxw_view_t *) widget;
	gfxw_dyn_view_t *dyn_view = (gfxw_dyn_view_t *) widget;
	gfxw_text_t *text = (gfxw_text_t *) widget;
	gfxw_list_t *list = (gfxw_list_t *) widget;
	gfxw_port_t *port = (gfxw_port_t *) widget;
	gfxw_primitive_t *primitive = (gfxw_primitive_t *) widget;

	if (widget->magic == GFXW_MAGIC_VALID)
		sciprintf("v ");
	else if (widget->magic == GFXW_MAGIC_INVALID)
		sciprintf("INV ");

	if (widget->ID != GFXW_NO_ID)
		sciprintf("#%08x ", widget->ID);

	switch (widget->type) {
	case GFXW_BOX:
		sciprintf("BOX");
		break;
	case GFXW_RECT:
		sciprintf("RECT");
		break;
	case GFXW_LINE:
		sciprintf("LINE");
		break;
	case GFXW_VIEW:
		sciprintf("VIEW");
		sciprintf("(%d/%d/%d)@(%d,%d)[p:%d,c:%d]", view->view, view->loop, view->cel, view->pos.x, view->pos.y,
			  (view->color.mask & GFX_MASK_PRIORITY)? view->color.priority : -1,
			  (view->color.mask & GFX_MASK_CONTROL)? view->color.control : -1);
		break;
	case GFXW_STATIC_VIEW:
		sciprintf("PICVIEW");
		sciprintf("(%d/%d/%d)@(%d,%d)[p:%d,c:%d]", view->view, view->loop, view->cel, view->pos.x, view->pos.y,
			  (view->color.mask & GFX_MASK_PRIORITY)? view->color.priority : -1,
			  (view->color.mask & GFX_MASK_CONTROL)? view->color.control : -1);
		break;
	case GFXW_DYN_VIEW:
		sciprintf("DYNVIEW");
		sciprintf("(%d/%d/%d)@(%d,%d)[p:%d,c:%d]", view->view, view->loop, view->cel, view->pos.x, view->pos.y,
			  (view->color.mask & GFX_MASK_PRIORITY)? view->color.priority : -1,
			  (view->color.mask & GFX_MASK_CONTROL)? view->color.control : -1);
		sciprintf(" z=%d", dyn_view->z);
		break;
	case GFXW_TEXT:
		sciprintf("TEXT");
		sciprintf("\"%s\"", text->text);
		break;
	case GFXW_LIST:
		sciprintf("LIST");
		break;
	case GFXW_VISUAL:
		sciprintf("VISUAL");
	case GFXW_PORT:
		sciprintf("PORT");
		sciprintf(" font=%d drawpos=(%d,%d)", port->font_nr, port->draw_pos.x, port->draw_pos.y);
		if (port->gray_text)
			sciprintf(" (gray)");
		break;
	case GFXW_:
		sciprintf("<W>");
		break;
	case GFXW_CONTAINER:
		sciprintf("<CONT>");
		break;
	default:
		sciprintf("<bad %d>", widget->type);
	}
	sciprintf("[(%d,%d)(%dx%d)]", widget->bounds.x, widget->bounds.y, widget->bounds.xl, widget->bounds.yl);

	for (i = 0; i < strlen(flags_list); i++)
		if (widget->flags & (1 << i))
			sciprintf("%c", flags_list[i]);

	sciprintf("\n");
}
