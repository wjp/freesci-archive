/***************************************************************************
 gfx_state_internal.h Copyright (C) 2000 Christoph Reichenbach


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

#ifndef _GFX_STATE_INTERNAL_H_
#define _GFX_STATE_INTERNAL_H_

#include <gfx_tools.h>
#include <gfx_options.h>
#include <gfx_operations.h>
#include <gfx_resmgr.h>
#include <gfx_system.h>


#define GFXW_FLAG_VISIBLE (1<<0)
#define GFXW_FLAG_OPAQUE (1<<1)
#define GFXW_FLAG_CONTAINER (1<<2)
#define GFXW_FLAG_DIRTY (1<<3)
#define GFXW_FLAG_TAGGED (1<<4)


typedef enum {
	GFXW_, /* Base widget */

	GFXW_BOX,
	GFXW_RECT,
	GFXW_LINE,
	GFXW_VIEW,
	GFXW_STATIC_VIEW,
	GFXW_DYN_VIEW,
	GFXW_TEXT,

	GFXW_CONTAINER,

	GFXW_LIST,
	GFXW_SORTED_LIST,
	GFXW_VISUAL,
	GFXW_PORT
} gfxw_widget_types_t;


#define GFXW_MAGIC_VALID 0xC001
#define GFXW_MAGIC_INVALID 0xbad

#define GFXW_NO_ID -1

struct _gfxw_widget;
struct _gfxw_container_widget;
struct _gfxw_visual;

typedef int gfxw_point_op(struct _gfxw_widget *, point_t);
typedef int gfxw_visual_op(struct _gfxw_widget *, struct _gfxw_visual *);
typedef int gfxw_op(struct _gfxw_widget *);
typedef int gfxw_op_int(struct _gfxw_widget *, int);
typedef int gfxw_bin_op(struct _gfxw_widget *, struct _gfxw_widget *);

#define WIDGET_COMMON \
   int magic; /* Extra check after typecasting */ \
   int flags; /* Widget flags */ \
   gfxw_widget_types_t type; \
   rect_t bounds; /* Boundaries */ \
   struct _gfxw_widget *next; /* Next widget in widget list */ \
   int ID; /* Unique ID or GFXW_NO_ID */ \
   struct _gfxw_container_widget *parent; /* The parent widget, or NULL if not owned */ \
   struct _gfxw_visual *visual; /* The owner visual */ \
   gfxw_point_op *draw; /* Draw widget (if dirty) and anything else required for the display to be consistant */ \
   gfxw_op *free; /* Remove widget (and any sub-widgets it may contain) */ \
   gfxw_op *tag; /* Tag the specified widget */ \
   gfxw_op_int *print; /* Prints the widget's contents, using sciprintf. Second parameter is indentation. */ \
   gfxw_bin_op *compare_to; /* a.compare_to(a, b) returns <0 if a<b, =0 if a=b and >0 if a>b */ \
   gfxw_bin_op *equals; /* a equals b if both cause the same data to be displayed */ \
   gfxw_bin_op *superarea_of; /* a superarea_of b <=> for each pixel of b there exists an opaque pixel in a at the same location */ \
   gfxw_visual_op *set_visual; /* Sets the visual the widget belongs to */

typedef struct _gfxw_widget {
	WIDGET_COMMON
} gfxw_widget_t;


#define GFXW_IS_BOX(widget) ((widget)->type == GFXW_BOX)
typedef struct {
	WIDGET_COMMON
	gfx_color_t color1, color2;
	gfx_box_shade_t shade_type;
} gfxw_box_t;


#define GFXW_IS_PRIMITIVE(widget) ((widget)->type == GFXW_RECT || (widget)->type == GFXW_LINE)
typedef struct {
	WIDGET_COMMON
	gfx_color_t color;
	gfx_line_mode_t line_mode;
	gfx_line_style_t line_style;
} gfxw_primitive_t;



#define VIEW_COMMON \
	WIDGET_COMMON \
	point_t pos; /* Implies the value of 'bounds' in WIDGET_COMMON */ \
	gfx_color_t color; \
	int view, loop, cel; \

#define GFXW_IS_VIEW(widget) ((widget)->type == GFXW_VIEW || (widget)->type == GFXW_STATIC_VIEW || (widget)->type == GFXW_DYN_VIEW)
typedef struct {
	VIEW_COMMON
} gfxw_view_t;

#define GFXW_IS_DYN_VIEW(widget) ((widget)->type == GFXW_DYN_VIEW)
typedef struct {
	VIEW_COMMON
	int z; /* The z coordinate: Added to y, but used for sorting */ 
} gfxw_dyn_view_t;



#define GFXW_IS_TEXT(widget) ((widget)->type == GFXW_TEXT)
typedef struct {
	WIDGET_COMMON
	int font_nr;
	char *text;
	gfx_alignment_t halign, valign;
	gfx_color_t color1, color2, bgcolor;
	char single_line;
	gfx_text_handle_t *text_handle;
} gfxw_text_t;


/* Container widgets */

typedef int gfxw_unary_container_op(struct _gfxw_container_widget *);
typedef int gfxw_container_op(struct _gfxw_container_widget *, gfxw_widget_t *);
typedef int gfxw_rect_op(struct _gfxw_container_widget *, rect_t, int);

#define WIDGET_CONTAINER \
   WIDGET_COMMON \
   rect_t zone; /* The writeable zone (absolute) for contained objects */ \
   gfx_dirty_rect_t *dirty; /* List of dirty rectangles */ \
   gfxw_widget_t *contents; \
   gfxw_widget_t **nextpp; /* Pointer to the 'next' pointer in the last entry in contents */ \
   gfxw_unary_container_op *free_tagged; /* Free all tagged contained widgets */ \
   gfxw_unary_container_op *free_contents; /* Free all contained widgets */ \
   gfxw_rect_op *add_dirty_abs; /* Add an absolute dirty rectangle */ \
   gfxw_rect_op *add_dirty_rel; /* Add a relative dirty rectangle */ \
   gfxw_container_op *add;  /* Append widget to an appropriate position (for view and control lists) */


typedef struct _gfxw_container_widget {
	WIDGET_CONTAINER
} gfxw_container_t;


#define GFXW_IS_CONTAINER(widget) ((widget)->type == GFXW_PORT || (widget)->type == GFXW_VISUAL || \
				   (widget)->type == GFXW_SORTED_LIST || (widget)->type == GFXW_LIST)

#define GFXW_IS_LIST(widget) ((widget)->type == GFXW_LIST || (widget)->type == GFXW_SORTED_LIST)
typedef gfxw_container_t gfxw_list_t;

#define GFXW_IS_VISUAL(widget) ((widget)->type == GFXW_VISUAL)
typedef struct _gfxw_visual {
	WIDGET_CONTAINER;
	struct _gfxw_port **port_refs; /* References to ports */
	int port_refs_nr;
	int font_nr; /* Default font */
	gfx_state_t *gfx_state;
}  gfxw_visual_t;

#define GFXW_IS_PORT(widget) ((widget)->type == GFXW_PORT)
typedef struct _gfxw_port {
	WIDGET_CONTAINER
	gfxw_list_t *decorations; /* optional window decorations- drawn before the contents */
	gfxw_widget_t *port_bg; /* Port background widget or NULL */
	struct _gfxw_port *next_port; /* Pointer to the next port in the port list */
	gfx_color_t color, bgcolor;
	int font_nr;
	point_t draw_pos; /* Drawing position */
	byte gray_text; /* Whether text is 'grayed out' (dithered) */
} gfxw_port_t;

#undef WIDGET_COMMON
#undef WIDGET_CONTAINER

#endif /* !_GFX_STATE_INTERNAL_H_ */

