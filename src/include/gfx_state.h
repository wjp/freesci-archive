/***************************************************************************
 gfx_state.h Copyright (C) 2000 Christoph Reichenbach


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
/* Graphical state management */

#ifndef _GFX_STATE_H_
#define _GFX_STATE_H_

#include <gfx_state_internal.h>

/* Enable the next line to keep a list of pointers to all widgets, with up to the specified amount
** of members (/SLOW/) */
#define GFXW_DEBUG_WIDGETS 2048

/* Terminology
**
** Two special terms are used in here: /equivalent/ and /clear/. Their meanings
** in this context are as follows:
**
** /clear/: Clearing a widget means overwriting the space it occupies in the back
** buffer with data from the static buffer. This affects both the visual and the
** priority buffer, the static buffer (and any effect the widget may have had on
** it) is not touched.
**
** /equivalent/: Two Widgets A and B are equivalent if and only if either of the
** following conditions is met:
** a) Both A and B are text widgets, and they occupy the same bounding rectangle.
** b) Both A and B are dynview widgets, and they have the same unique ID
** Note that /equivalent/ is not really an equivalence relation- while it is ob-
** viously transitive and symmetrical, it is not reflexive (e.g. a box widget
** is not /equivalent/ to itself), although this might be a nice addition for the
** future.
*/


/*********************************/
/* Fundamental widget operations */
/*********************************/

gfxw_widget_t *
gfxw_draw(gfx_state_t *state, gfxw_widget_t *widget);
/* Draws all dirty parts of the specified widgets (and its potential sub-widgets)
** Parameters: (gfx_state_t *) state: The state the widgets should be drawn to
**             (gfxw_widget_t *) widget: The widget to draw
** Returns   : (gfxw_widget_t *) widget
** This operation should usually be called on the root widget (if there is one), and
** may be costly, since it requires all active widgets to be traversed. It's only
** O(n), though.
*/

void
gfxw_free(gfx_state_t *state, gfxw_widget_t *widget);
/* Frees the specified widget, and all sub-widgets it might contain
** Parameters: (gfx_state_t *) state: The state the widgets are to be /clear/ed from
**             (gfxw_widget_t *) widget: The widget to free
** Returns   : (void)
** All freed widgets are /clear/ed as well.
*/

gfxw_widget_t *
gfxw_tag(gfxw_widget_t *widget);
/* Tags the widget and all possible sub-widgets it may contain
** Parameters: (gfxw_widget_t *) widget: The widget to tag
** Returns   : (gfxw_widget_t *) widget
** Tagged widgets can only be untagged by being overwritten by /equivalent/ widgets.
*/

void
gfxw_free_tagged(gfxw_widget_t *widget);
/* Frees the widget (if it is tagged) and all tagged sub-widgets it contains
** Parameters: (gfxw_widget_t *) widget: The root of the widget tree to check
** Returns   : (void)
** Note that freed widgets are removed from memory, but _not_ /clear/ed.
** gfxw_tag() and gfxw_free_tagged() are typically used to handle display lists
** of dynviews.
*/

gfxw_container_t *
gfxw_add_to_front(gfxw_container_t *container, gfxw_widget_t *widget);
/* Adds a widget to the top of a container
** Parameters: (gfxw_container_t *) containner: The container the widget is added to
**             (gfxw_widget_t *) widget: The widget to add
** Returns   : (gfxw_container_t *) container
** Widgets on top of a container will be drawn last, so they are potentially drawn on
** top of other widgets
** This function is O(1)
*/

gfxw_container_t *
gfxw_add_to_back(gfxw_container_t *container, gfxw_widget_t *widget);
/* Adds a widget to the bottom of a contained
** Parameters: (gfxw_container_t *) container: The container the widget is added to
**             (gfxw_widget_t *) widget: The widget to add
** Returns   : (gfxw_container_t *) container
** This function is O(1)
*/

gfxw_container_t *
gfxw_add(gfx_state_t *state, gfxw_container_t *container, gfxw_widget_t *widget);
/* Adds a widget to a container widget
** Parameters: (gfx_state_t *) state: The state to add to (needed for /clear/ing)
**             (gfxw_container_t *) container: The container the widget is added to
**             (gfxw_widget_t *) widget: The widget to add
** Returns   : (gfxw_container_t *) container
** If widget is /equivalent/ to one of the widgets in the container, it overwrites
** the widget it is equivalent to rather than adding itself to the container. Overwriting
** an equivalent widget /clear/s the original widget. If used as the exclusive insertion
** method for one container, it guarantees that the contents of the container are sorted
** (by INSERTION SORT, obviously).
** This function is O(n)
*/

void
gfxw_print(gfxw_widget_t *widget);
/* Print a widget with sciprintf
** Parameters: (gfxw_widget_t *) widget: The widget to print
** Returns   : (void)
*/

/***************************/
/* Basic widget generation */
/***************************/

/*-- Primitive types --*/

gfxw_box_t *
gfxw_new_box(gfx_state_t *state, rect_t area, gfx_color_t color1, gfx_color_t color2, gfx_box_shade_t shade_type);
/* Creates a new box
** Parameters: (gfx_state_t *) state: The (optional) state
**             (rect_t) area: The box's dimensions, relative to its container widget
**             (gfx_color_t) color1: The primary color
**             (gfx_color_t) color1: The secondary color (ignored if shading is disabled)
**             (gfx_box_shade_t) shade_type: The shade type for the box
** Returns   : (gfxw_box_t *) The resulting box widget
** The graphics state- if non-NULL- is used here for some optimizations.
*/

gfxw_primitive_t *
gfxw_new_rect(rect_t rect, gfx_color_t color, gfx_line_mode_t line_mode, gfx_line_style_t line_style);
/* Creates a new rectangle
** Parameters: (rect_t) rect: The rectangle area
**             (gfx_color_t) color: The rectangle's color
**             (gfx_line_mode_t) line_mode: The line mode for the lines that make up the rectangle
**             (gfx_line_style_t) line_style: The rectangle's lines' style
** Returns   : (gfxw_primitive_t *) The newly allocated rectangle widget (a Primitive)
*/

gfxw_primitive_t *
gfxw_new_line(rect_t line, gfx_color_t color, gfx_line_mode_t line_mode, gfx_line_style_t line_style);
/* Creates a new line
** Parameters: (rect_t) line: The line origin and relative destination coordinates
**             (gfx_color_t) color: The line's color
**             (gfx_line_mode_t) line_mode: The line mode to use for drawing
**             (gfx_line_style_t) line_style: The line style
** Returns   : (gfxw_primitive_t *) The newly allocated line widget (a Primitive)
*/


/* Whether the view should be static */
#define GFXW_VIEW_FLAG_STATIC (1 << 0)

/* Whether the view should _not_ apply its x/y offset modifyers */
#define GFXW_VIEW_FLAG_DONT_MODIFY_OFFSET (1 << 1)

gfxw_view_t *
gfxw_new_view(gfx_state_t *state, point_t pos, int view, int loop, int cel, int priority, int control,
	      gfx_alignment_t halign, gfx_alignment_t valign, int flags);
/* Creates a new view (a cel, actually)
** Parameters: (gfx_state_t *) state: The graphics state
**             (point_t) pos: The position to place the view at
**             (int x int x int) view, loop, cel: The global cel ID
**             (int) priority: The priority to use for drawing, or -1 for none
**             (int) control: The value to write to the control map, or -1 for none
**             (gfx_alignment_t x gfx_alignment_t) halign, valign: Horizontal and vertical
**                                                                 cel alignment
**             (int) flags: Any combination of GFXW_VIEW_FLAGs
** Returns   : (gfxw_cel_t *) A newly allocated cel according to the specs
*/

gfxw_dyn_view_t *
gfxw_new_dyn_view(gfx_state_t *state, point_t pos, int z, int view, int loop, int cel,
		  int priority, int control, gfx_alignment_t halign, gfx_alignment_t valign);
/* Creates a new dyn view
** Parameters: (gfx_state_t *) state: The graphics state
**             (point_t) pos: The position to place the dynamic view at
**             (int) z: The z coordinate
**             (int x int x int) view, loop, cel: The global cel ID
**             (int) priority: The priority to use for drawing, or -1 for none
**             (int) control: The value to write to the control map, or -1 for none
**             (gfx_alignment_t x gfx_alignment_t) halign, valign: Horizontal and vertical
**                                                                 cel alignment
** Returns   : (gfxw_cel_t *) A newly allocated cel according to the specs
** Dynamic views are non-pic views with a unique global identifyer. This allows for drawing
** optimizations when they move or change shape.
*/

gfxw_text_t *
gfxw_new_text(gfx_state_t *state, rect_t area, int font, char *text, gfx_alignment_t halign,
	      gfx_alignment_t valign, gfx_color_t color1, gfx_color_t color2,
	      gfx_color_t bgcolor, int single_line);
/* Creates a new text widget
** Parameters: (gfx_state_t *) state: The state the text is to be calculated from
**             (rect_t) area: The area the text is to be confined to (the yl value is only
**                            relevant for text aligment, though)
**             (int) font: The number of the font to use
**             (gfx_alignment_t x gfx_alignment_t) halign, valign: Horizontal and
**                                                                 vertical text alignment
**             (gfx_color_t x gfx_color_t) color1, color2: Text foreground colors (if not equal,
**                                                         The foreground is dithered between them)
**             (gfx_color_t) bgcolor: Text background color
**             (int) single_line: 0 if '\n's should be respected, non-zero if the text should
**                                be forced into a single line
** Returns   : (gfx_text_t *) The resulting text widget
*/

gfxw_widget_t *
gfxw_set_id(gfxw_widget_t *widget, int ID);
/* Sets a widget's ID
** Parmaeters: (gfxw_widget_t *) widget: The widget whose ID should be set
**             (int) ID: The ID to set
** Returns   : (gfxw_widget_t *) widget
** A widget ID is unique within the container it is stored in, if and only if it was
** added to that container with gfxw_add().
*/

/*-- Container types --*/

gfxw_list_t *
gfxw_new_list(rect_t area);
/* Creates a new list widget
** Parameters: (rect_t) area: The area covered by the list (absolute position)
** Returns   : (gfxw_list_t *) A newly allocated list widget
** List widgets are also referred to as Display Lists.
*/

gfxw_visual_t *
gfxw_new_visual(gfx_state_t *state, int font);
/* Creates a new visual widget
** Parameters: (gfx_state_t *) state: The graphics state
**             (int) font: The default font number for contained ports
** Returns   : (gfxw_list_t *) A newly allocated visual widget
** Visual widgets are containers for port widgets.
*/


gfxw_port_t *
gfxw_new_port(gfxw_visual_t *visual, gfxw_port_t *predecessor, rect_t area, gfx_color_t fgcolor, gfx_color_t bgcolor);
/* Creates a new port widget with the default settings
** Paramaters: (gfxw_visual_t *) visual: The visual the port is added to
**             (gfxw_port_t *) predecessor: The port's predecessor
**             (rect_t) area: The screen area covered by the port (absolute position)
**             (gfx_color_t) fgcolor: Foreground drawing color
**             (gfx_color_t) bgcolor: Background color
** Returns   : (gfxw_port_t *) A newly allocated port widget
** A port differentiates itself from a list in that it contains additional information,
** and an optional title (stored in a display list).
** Ports are assigned implicit IDs identifying their position within the port stack.
*/

gfxw_port_t *
gfxw_remove_port(gfxw_visual_t *visual, gfxw_port_t *port);
/* Removes the toplevel port from a visual
** Parameters: (gfxw_visual_t *) visual: The visual the port is to be removed from
**             (gfxw_port_t *) port: The port to remove
** Returns   : (gfxw_port_t *) port: The new active port, or NULL if no ports are left
*/

gfxw_port_t *
gfxw_find_port(gfxw_visual_t *visual, int ID);
/* Retrieves a port with the specified ID
** Parmaeters: (gfxw_visual_t *) visual: The visual the port is to be retreived from
**             (int) ID: The port's ID
** Returns   : (gfxw_port_t *) The requested port, or NULL if it didn't exist
** This function is O(n).
*/


#endif /* !_GFX_STATE_H_ */
