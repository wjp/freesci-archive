/***************************************************************************
 kgraphics.c Copyright (C) 1999,2000,01 Christoph Reichenbach


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

    Christoph Reichenbach (CR) [jameson@linuxgames.com]

***************************************************************************/

#include <sciresource.h>
#include <engine.h>
#include <gfx_widgets.h>
#include <sci_graphics.h>
#include <sci_widgets.h>

/* Graph subfunctions */
#define K_GRAPH_GET_COLORS_NR 2
#define K_GRAPH_DRAW_LINE 4
#define K_GRAPH_SAVE_BOX 7
#define K_GRAPH_RESTORE_BOX 8
#define K_GRAPH_FILL_BOX_BACKGROUND 9
#define K_GRAPH_FILL_BOX_FOREGROUND 10
#define K_GRAPH_FILL_BOX_ANY 11
#define K_GRAPH_UPDATE_BOX 12
#define K_GRAPH_REDRAW_BOX 13
#define K_GRAPH_ADJUST_PRIORITY 14

/* Control types and flags */
#define K_CONTROL_BUTTON 1
#define K_CONTROL_TEXT 2
#define K_CONTROL_EDIT 3
#define K_CONTROL_ICON 4
#define K_CONTROL_CONTROL 6
#define K_CONTROL_BOX 10

#define ADD_TO_CURRENT_PORT(widget) \
  if (s->port) \
       s->port->add(GFXWC(s->port), GFXW(widget)); \
  else \
       s->picture_port->add(GFXWC(s->visual), GFXW(widget));

#define ADD_TO_CURRENT_PICTURE_PORT(widget) \
  if (s->port) \
       s->port->add(GFXWC(s->port), GFXW(widget)); \
  else \
       s->picture_port->add(GFXWC(s->picture_port), GFXW(widget));

#define ADD_TO_CURRENT_FG_WIDGETS(widget) \
  ADD_TO_CURRENT_PICTURE_PORT(widget)

#define ADD_TO_CURRENT_BG_WIDGETS(widget) \
  ADD_TO_CURRENT_PICTURE_PORT(widget)

#define FULL_REDRAW()\
  if (s->visual) \
       s->visual->draw(GFXW(s->visual), gfxw_point_zero); \
  gfxop_update(s->gfx_state);

#define FULL_INSPECTION()\
  if (s->visual) \
       s->visual->print(GFXW(s->visual), 0);

static inline void
handle_gfx_error(int line, char *file)
{
	exit(1); /* FIXME: Replace this with a longjmp() eventually */
}

#define GFX_ASSERT(x) { \
	int val = !!(x); \
	if (val) { \
		if (val == GFX_ERROR) \
			SCIkwarn(SCIkWARNING, "GFX subsystem returned error on \"" #x "\"!\n"); \
		else {\
                        SCIkwarn(SCIkERROR, "GFX subsystem fatal error condition on \"" #x "\"!\n"); \
                        BREAKPOINT(); \
			handle_gfx_error(__LINE__, __FILE__); \
	        } \
        }\
}

#define ASSERT(x) { \
       int val = !!(x); \
       if (!val) { \
                        SCIkwarn(SCIkERROR, "GFX subsystem fatal error condition on \"" #x "\"!\n"); \
                        BREAKPOINT(); \
			handle_gfx_error(__LINE__, __FILE__); \
       } \
}


static void
assert_primary_widget_lists(state_t *s)
{
	if (!s->dyn_views) {
		rect_t bounds = s->picture_port->bounds;

		s->dyn_views = gfxw_new_list(bounds, GFXW_LIST_SORTED);
		s->dyn_views->flags |= GFXW_FLAG_IMMUNE_TO_SNAPSHOTS;
		ADD_TO_CURRENT_PICTURE_PORT(s->dyn_views);
	}

	if (!s->drop_views) {
		rect_t bounds = s->picture_port->bounds;

		s->drop_views = gfxw_new_list(bounds, GFXW_LIST_SORTED);
		s->drop_views->flags |= GFXW_FLAG_IMMUNE_TO_SNAPSHOTS;
		ADD_TO_CURRENT_PICTURE_PORT(s->drop_views);
	}
}

static void
reparentize_primary_widget_lists(state_t *s, gfxw_port_t *newport)
{
	if (!newport)
		newport = s->picture_port;

	gfxw_remove_widget_from_container(s->dyn_views->parent, GFXW(s->dyn_views));

	newport->add(GFXWC(newport), GFXW(s->dyn_views));
}

int
graph_save_box(state_t *s, rect_t area)
{
	int handle = kalloc(s, HUNK_TYPE_GFXBUFFER, sizeof(gfxw_snapshot_t *));
	gfxw_snapshot_t **ptr = (gfxw_snapshot_t **) kmem(s, handle);

	*ptr = gfxw_make_snapshot(s->visual, area);

	return handle;
}


void
graph_restore_box(state_t *s, int handle)
{
	gfxw_snapshot_t **ptr;
	int port_nr = s->port->ID;

	if (!handle) {
		SCIkwarn(SCIkWARNING, "Attempt to restore box with zero handle\n");
		return;
	}

	ptr = (gfxw_snapshot_t **) kmem(s, handle);

	if (!ptr) {
		SCIkwarn(SCIkWARNING, "Attempt to restore invalid handle %04x\n", handle);
		return;
	}

	while (port_nr > 2 && !(s->port->flags & GFXW_FLAG_IMMUNE_TO_SNAPSHOTS)
	       &&(gfxw_widget_matches_snapshot(*ptr, GFXW(s->port)))) {
		/* This shouldn't ever happen, actually, since windows (ports w/ ID > 2) should all be immune */
		gfxw_port_t *newport = gfxw_find_port(s->visual, port_nr);
		SCIkwarn(SCIkERROR, "Port %d is not immune against snapshots!\n", s->port->ID);
		port_nr--;
		if (newport)
			s->port = newport;
	}

	if (s->dyn_views && gfxw_widget_matches_snapshot(*ptr, GFXW(s->dyn_views->parent))) {
		gfxw_container_t *parent = s->dyn_views->parent;

		do {
			parent = parent->parent;
		} while (parent && (gfxw_widget_matches_snapshot(*ptr, GFXW(parent))));

		if (!parent) {
			SCIkwarn(SCIkERROR, "Attempted widget mass destruction by a snapshot\n");
			BREAKPOINT();
		}

		reparentize_primary_widget_lists(s, (gfxw_port_t *) parent);
	}


	if (!ptr) {
		SCIkwarn(SCIkERROR, "Attempt to restore invalid snaphot with handle %04x!\n", handle);
		return;
	}

	gfxw_restore_snapshot(s->visual, *ptr);
	free(*ptr);

	kfree(s, handle);
}


static gfx_color_t
graph_map_ega_color(state_t *s, int color, int priority, int control)
{
	gfx_color_t retval = s->ega_colors[(color >=0 && color < 16)? color : 0];

	gfxop_set_color(s->gfx_state, &retval, (color < 0)? -1 : retval.visual.r, retval.visual.g, retval.visual.b,
			(color == -1)? 255 : 0, priority, control);

	return retval;
}

/* --- */


void
kSetCursor(state_t *s, int funct_nr, int argc, heap_ptr argp)
{

	if (PARAM(1)) {
		s->mouse_pointer_nr = PARAM(0);
	} else 
		s->mouse_pointer_nr = GFXOP_NO_POINTER;

	GFX_ASSERT(gfxop_set_pointer_cursor(s->gfx_state, s->mouse_pointer_nr));

	if (argc > 2) {
		point_t newpos = gfx_point(PARAM(2) + s->port->bounds.x,
					   PARAM(3) + s->port->bounds.y);

		GFX_ASSERT(gfxop_set_pointer_position(s->gfx_state, newpos));
	}
}

static inline void
_ascertain_port_contents(gfxw_port_t *port)
{
	if (!port->contents)
		port->contents = (gfxw_widget_t *) gfxw_new_list(port->bounds, 0);
}

void
kShow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	s->pic_visible_map = sci_ffs(UPARAM_OR_ALT(0, 1)) - 1;

	CHECK_THIS_KERNEL_FUNCTION;
	s->pic_not_valid = 2;
}


void
kPicNotValid(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	CHECK_THIS_KERNEL_FUNCTION;
	s->acc = s->pic_not_valid;
	if (argc)
		s->pic_not_valid = PARAM(0);
}

void
_k_redraw_box(state_t *s, int x1, int y1, int x2, int y2)
{
	int i;
WARNING( "_k_redraw_box: Fixme!")
#if 0

  view_object_t *list = s->dyn_views;

  sciprintf("Reanimating %d views\n", s->dyn_views_nr);


  for (i=0;i<s->dyn_views_nr;i++)
  {
    list[i].underBits=graph_save_box(s,
	       list[i].nsLeft,
	       list[i].nsTop,
	       list[i].nsRight-list[i].nsLeft,
	       list[i].nsBottom-list[i].nsTop,
	       SCI_MAP_VISUAL | SCI_MAP_PRIORITY);
    draw_view0(s->pic, s->ports[0], 
	       list[i].nsLeft, list[i].nsTop,
	       list[i].priority, list[i].loop,
	       list[i].cel, 0, list[i].view);	
  }
 
  graph_update_box(s, x1, y1, x2-x1, y2-y1);

  for (i=0;i<s->dyn_views_nr;i++)
  {
    graph_restore_box(s, list[i].underBits);
    list[i].underBits=0;
  }
#endif
}

void
_k_graph_rebuild_port_with_color(state_t *s, gfx_color_t newbgcolor)
{
	gfxw_port_t *port = s->port;
	gfxw_port_t *newport = sciw_new_window(s, port->zone, port->font_nr, port->color, newbgcolor,
					       s->titlebar_port->font_nr, s->ega_colors[15], s->ega_colors[8],
					       port->title_text, port->port_flags & ~WINDOW_FLAG_TRANSPARENT);

	if (s->dyn_views) {
		int found = 0;
		gfxw_container_t *parent = s->dyn_views->parent;

		while (parent && !(found |= (GFXW(parent) == GFXW(port))))
			parent = parent->parent;

		s->dyn_views = NULL;
	}

	port->parent->add(GFXWC(port->parent), GFXW(newport));
	port->widfree(GFXW(port));
}


void
kGraph(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	gfx_color_t gfxcolor;
	rect_t area;
	gfxw_port_t *port = s->port;
	int redraw_port = 0;

	area = gfx_rect(PARAM(2), PARAM(1) , PARAM(4), PARAM(3));

	area.xl = area.xl - area.x; /* Since the actual coordinates are absolute */
	area.yl = area.yl - area.y;
  
	switch(PARAM(0)) {

	case K_GRAPH_GET_COLORS_NR:

		s->acc = (sci_version < SCI_VERSION_1) ? 0x10 : 0x100; /* number of colors */
		break;
  
	case K_GRAPH_DRAW_LINE:

		gfxcolor = graph_map_ega_color(s, PARAM(5) & 0xf, PARAM_OR_ALT(6, -1), PARAM_OR_ALT(7, -1));

		SCIkdebug(SCIkGRAPHICS, "draw_line((%d, %d), (%d, %d), col=%d, p=%d, c=%d, mask=%d)\n",
			  PARAM(2), PARAM(1), PARAM(4), PARAM(3), PARAM(5), PARAM_OR_ALT(6, -1), PARAM_OR_ALT(7, -1),
			  gfxcolor.mask);

		redraw_port = 1;
		ADD_TO_CURRENT_BG_WIDGETS(GFXW(gfxw_new_line(area, gfxcolor, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));
		
		break;

	case K_GRAPH_SAVE_BOX:

		area.x += s->port->zone.x;
		area.y += s->port->zone.y;
  
		s->acc = graph_save_box(s, area);
		break;

	case K_GRAPH_RESTORE_BOX:

		graph_restore_box(s, PARAM(1));
	break;

	case K_GRAPH_FILL_BOX_BACKGROUND:

		_k_graph_rebuild_port_with_color(s, port->bgcolor);
		port = s->port;

		redraw_port = 1;
		CHECK_THIS_KERNEL_FUNCTION;
		break;

	case K_GRAPH_FILL_BOX_FOREGROUND:

		_k_graph_rebuild_port_with_color(s, port->color);
		port = s->port;

		redraw_port = 1;
		CHECK_THIS_KERNEL_FUNCTION;
		break;

	case K_GRAPH_FILL_BOX_ANY: {

		gfx_color_t color = graph_map_ega_color(s, PARAM(6), PARAM_OR_ALT(7, -1), PARAM_OR_ALT(8, -1));

		color.mask = UPARAM(5);

		SCIkdebug(SCIkGRAPHICS, "fill_box_any((%d, %d), (%d, %d), col=%d, p=%d, c=%d, mask=%d)\n",
			  PARAM(2), PARAM(1), PARAM(4), PARAM(3), PARAM(6), PARAM_OR_ALT(7, -1), PARAM_OR_ALT(8, -1),
			  UPARAM(5));

		ADD_TO_CURRENT_BG_WIDGETS(gfxw_new_box(s->gfx_state, area, color, color, GFX_BOX_SHADE_FLAT));
		CHECK_THIS_KERNEL_FUNCTION;

	}
	break;

	case K_GRAPH_UPDATE_BOX: {

		SCIkdebug(SCIkGRAPHICS, "update_box(%d, %d, %d, %d)\n",
			  PARAM(1), PARAM(2), PARAM(3), PARAM(4));

		area.x += s->port->zone.x;
		area.y += s->port->zone.y;

		gfxop_update_box(s->gfx_state, area);
		CHECK_THIS_KERNEL_FUNCTION;

	}
	break;

	case K_GRAPH_REDRAW_BOX: {

		CHECK_THIS_KERNEL_FUNCTION;

		SCIkdebug(SCIkGRAPHICS, "redraw_box(%d, %d, %d, %d)\n",
			  PARAM(1), PARAM(2), PARAM(3), PARAM(4));

		area.x += s->port->zone.x;
		area.y += s->port->zone.y;

		if (s->dyn_views && s->dyn_views->parent == GFXWC(s->port))
			s->dyn_views->draw(GFXW(s->dyn_views), gfx_point(0, 0));
		
		gfxop_update_box(s->gfx_state, area);

	}

	break;

	case K_GRAPH_ADJUST_PRIORITY:

		SCIkdebug(SCIkGRAPHICS, "adjust_priority(%d, %d)\n", PARAM(1), PARAM(2));
		s->priority_first = PARAM(1) - 10;
		s->priority_last = PARAM(2) - 10;
		break;

	default:
    
		CHECK_THIS_KERNEL_FUNCTION;
		SCIkdebug(SCIkSTUB, "Unhandled Graph() operation %04x\n", PARAM(0));

	}

	if (redraw_port)
		FULL_REDRAW();

	gfxop_update(s->gfx_state);
}


void
kTextSize(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int width, height;
	int heap_text = UPARAM(1);
	char *text = s->heap + heap_text;
	heap_ptr dest = UPARAM(0);
	int maxwidth = PARAM_OR_ALT(3, 0);
	int font_nr = UPARAM(2);

	if (maxwidth < 0)
		maxwidth = 0;

	if (!*text) { /* Empty text */
		PUT_HEAP(dest + 0, 0);
		PUT_HEAP(dest + 2, 0);
		PUT_HEAP(dest + 4, 0);
		PUT_HEAP(dest + 6, 0);

		return;
	}

	GFX_ASSERT(gfxop_get_text_params(s->gfx_state, font_nr, text,
					 maxwidth? maxwidth : MAX_TEXT_WIDTH_MAGIC_VALUE, &width, &height));
	SCIkdebug(SCIkSTRINGS, "GetTextSize '%s' -> %dx%d\n", text, width, height);

	PUT_HEAP(dest + 0, 0);
	PUT_HEAP(dest + 2, 0);
	PUT_HEAP(dest + 4, height);
	PUT_HEAP(dest + 6, maxwidth? maxwidth : width);
}



void
kWait(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	GTimeVal time;
	int SleepTime = PARAM(0);

	sci_get_current_time (&time);

	s->acc = ((time.tv_usec - s->last_wait_time.tv_usec) * 60 / 1000000) +
		(time.tv_sec - s->last_wait_time.tv_sec) * 60;

	memcpy(&(s->last_wait_time), &time, sizeof(GTimeVal));

	GFX_ASSERT(gfxop_usleep(s->gfx_state, SleepTime * 1000000 / 60));
}


void
kCoordPri(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int y = PARAM(0);

	s->acc = VIEW_PRIORITY(y);
}


void
kPriCoord(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int priority = PARAM(0);

	s->acc = PRIORITY_BAND_FIRST(priority);
}



void
kDirLoop(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = PARAM(0);
	word angle = UPARAM(1);
	int view = GET_SELECTOR(obj, view);
	int signal = UGET_SELECTOR(obj, signal);
	int loop;
	int maxloops;

	if (signal & _K_VIEW_SIG_FLAG_DOESNT_TURN)
		return;

	angle %= 360;

	if (angle < 45)
		loop = 3;
	else if (angle < 135)
		loop = 0;
	else if (angle < 225)
		loop = 2;
	else if (angle < 314)
		loop = 1;
	else
		loop = 3;

	maxloops = gfxop_lookup_view_get_loops(s->gfx_state, view);

	if (maxloops == GFX_ERROR) {
		SCIkwarn(SCIkERROR, "Invalid view.%03d\n", view);
		PUT_SELECTOR(obj, loop, 0xffff); /* Invalid */
		return;
	} else if (loop >= maxloops) {
		SCIkwarn(SCIkWARNING, "With view.%03d: loop %d > maxloop %d\n", view, loop, maxloops);
		loop = 0;
	}

	PUT_SELECTOR(obj, loop, loop);
}

void
kSetJump(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr object = UPARAM(0);
	int dx = PARAM(1);
	int dy = PARAM(2);
	int gy = PARAM(3);
	int t1 = 1;
	int t2;
	int x = 0;
	int y = 0;

	CHECK_THIS_KERNEL_FUNCTION;

	if ((dx)&&(abs(dy)>dx)) t1=(2*abs(dy))/dx;
  
	SCIkdebug(SCIkBRESEN, "t1: %d\n", t1);

	t1--;

	do {
		t1++;
		t2=t1*abs(dx)+dy;
	} while (abs(2*t2)<abs(dx));

	SCIkdebug(SCIkBRESEN, "t1: %d, t2: %d\n", t1, t2);
 
	if (t2) {
		x=sqrt(abs((gy*dx*dx)/(2.0*t2)));
		if (t2 *dx < 0)
			x = -x;
	}
	y=abs(t1*y);
	if (dx>=0) y=-y;
	if ((dy<0)&&(!y)) y=-sqrt(-2*gy*dy);

	SCIkdebug(SCIkBRESEN, "SetJump for object at %x", object);
	SCIkdebug(SCIkBRESEN, "xStep: %d, yStep: %d\n", x, y);

	PUT_SELECTOR(object, xStep, x);
	PUT_SELECTOR(object, yStep, y);

}


#define _K_BRESEN_AXIS_X 0
#define _K_BRESEN_AXIS_Y 1

void
initialize_bresen(state_t *s, int funct_nr, int argc, heap_ptr argp, heap_ptr mover, int step_factor,
		  int deltax, int deltay)
{
	heap_ptr client = GET_SELECTOR(mover, client);
	int stepx = GET_SELECTOR(client, xStep) * step_factor;
	int stepy = GET_SELECTOR(client, yStep) * step_factor;
	int numsteps_x = stepx? (abs(deltax) + stepx-1) / stepx : 0;
	int numsteps_y = stepy? (abs(deltay) + stepy-1) / stepy : 0;
	int bdi, i1;
	int numsteps;
	int deltax_step;
	int deltay_step;

	if (numsteps_x > numsteps_y) {
		numsteps = numsteps_x;
		deltax_step = (deltax < 0)? -stepx : stepx;
		deltay_step = numsteps? deltay / numsteps : deltay;
	} else { /* numsteps_x <= numsteps_y */
		numsteps = numsteps_y;
		deltay_step = (deltay < 0)? -stepy : stepy;
		deltax_step = numsteps? deltax / numsteps : deltax;
	}

	/*  if (abs(deltax) > abs(deltay)) {*/ /* Bresenham on y */
	if (numsteps_y < numsteps_x) {

		PUT_SELECTOR(mover, b_xAxis, _K_BRESEN_AXIS_Y);
		PUT_SELECTOR(mover, b_incr, (deltay < 0)? -1 : 1);
		i1 = 2 * (abs(deltay) - abs(deltay_step * numsteps)) * abs(deltax_step);
		bdi = -abs(deltax);

	} else { /* Bresenham on x */

		PUT_SELECTOR(mover, b_xAxis, _K_BRESEN_AXIS_X);
		PUT_SELECTOR(mover, b_incr, (deltax < 0)? -1 : 1);
		i1= 2 * (abs(deltax) - abs(deltax_step * numsteps)) * abs(deltay_step);
		bdi = -abs(deltay);

	}

	PUT_SELECTOR(mover, dx, deltax_step);
	PUT_SELECTOR(mover, dy, deltay_step);

	SCIkdebug(SCIkBRESEN, "Init bresen for mover %04x: d=(%d,%d)\n", mover, deltax, deltay);
	SCIkdebug(SCIkBRESEN, "    steps=%d, mv=(%d, %d), i1= %d, i2=%d\n",
		  numsteps, deltax_step, deltay_step, i1, bdi*2);

	PUT_SELECTOR(mover, b_di, bdi);
	PUT_SELECTOR(mover, b_i1, i1);
	PUT_SELECTOR(mover, b_i2, bdi * 2);

}

void
kInitBresen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr mover = UPARAM(0);
	heap_ptr client = GET_SELECTOR(mover, client);

	int deltax = GET_SELECTOR(mover, x) - GET_SELECTOR(client, x);
	int deltay = GET_SELECTOR(mover, y) - GET_SELECTOR(client, y);
	initialize_bresen(s, funct_nr, argc, argp, mover, PARAM_OR_ALT(1, 1), deltax, deltay);
}


void
kDoBresen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr mover = PARAM(0);
	heap_ptr client = GET_SELECTOR(mover, client);

	int x = GET_SELECTOR(client, x);
	int y = GET_SELECTOR(client, y);
	int oldx, oldy, destx, desty, dx, dy, bdi, bi1, bi2, movcnt, bdelta, axis;

	int completed = 0;

	oldx = x;
	oldy = y;
	destx = GET_SELECTOR(mover, x);
	desty = GET_SELECTOR(mover, y);
	dx = GET_SELECTOR(mover, dx);
	dy = GET_SELECTOR(mover, dy);
	bdi = GET_SELECTOR(mover, b_di);
	bi1 = GET_SELECTOR(mover, b_i1);
	bi2 = GET_SELECTOR(mover, b_i2);
	movcnt = GET_SELECTOR(mover, b_movCnt);
	bdelta = GET_SELECTOR(mover, b_incr);
	axis = GET_SELECTOR(mover, b_xAxis);

	if ((bdi += bi1) >= 0) {
		bdi += bi2;

		if (axis == _K_BRESEN_AXIS_X)
			dx += bdelta;
		else
			dy += bdelta;
	}

	PUT_SELECTOR(mover, b_di, bdi);
	PUT_SELECTOR(mover, b_movCnt, movcnt + 1);

	x += dx;
	y += dy;


	if ((((x <= destx) && (oldx >= destx)) || ((x >= destx) && (oldx <= destx)))
	    && (((y <= desty) && (oldy >= desty)) || ((y >= desty) && (oldy <= desty))))
		/* Whew... in short: If we have reached or passed our target position */
		{
			x = destx;
			y = desty;
      
			if (s->selector_map.completed > -1)
				PUT_SELECTOR(mover, completed, completed = 1); /* Finish! */

			SCIkdebug(SCIkBRESEN, "Finished mover %04x\n", mover);
		}


	PUT_SELECTOR(client, x, x);
	PUT_SELECTOR(client, y, y);

	SCIkdebug(SCIkBRESEN, "New data: (x,y)=(%d,%d), di=%d\n", x, y, bdi);

	invoke_selector(INV_SEL(client, canBeHere, 0), 0);

	if (s->acc) { /* Contains the return value */
		s->acc = completed;
		return;
	} else {
		word signal = UGET_SELECTOR(client, signal);

		PUT_SELECTOR(client, x, oldx);
		PUT_SELECTOR(client, y, oldy);
		if (s->selector_map.completed > -1)
			PUT_SELECTOR(mover, completed, 1);
 
		PUT_SELECTOR(mover, x, oldx);
		PUT_SELECTOR(mover, y, oldy);    

		PUT_SELECTOR(client, signal, (signal | _K_VIEW_SIG_FLAG_HIT_OBSTACLE));

		if (s->selector_map.completed > -1)
			PUT_SELECTOR(mover, completed, completed = 1); /* Finish! */

		SCIkdebug(SCIkBRESEN, "Finished mover %04x by collision\n", mover);
		s->acc = 1;
	}

}


static abs_rect_t
set_base(state_t *s, heap_ptr object);


static inline int
collides_with(state_t *s, abs_rect_t area, heap_ptr other_obj, int funct_nr, int argc, heap_ptr argp)
{
	int other_signal = UGET_SELECTOR(other_obj, signal);
	abs_rect_t other_area = set_base(s, other_obj);

	SCIkdebug(SCIkBRESEN, "OtherSignal=%04x, z=%04x obj=%04x\n", other_signal,
		  (other_signal & (_K_VIEW_SIG_FLAG_DISPOSE_ME | _K_VIEW_SIG_FLAG_IGNORE_ACTOR)), other_obj);
	if ((other_signal & (_K_VIEW_SIG_FLAG_DISPOSE_ME | _K_VIEW_SIG_FLAG_IGNORE_ACTOR |
			     _K_VIEW_SIG_FLAG_HIDDEN)) == 0) {
					/* check whether the other object ignores actors */

		SCIkdebug(SCIkBRESEN, "  against (%d,%d) to (%d,%d)\n",
			  other_area.x, other_area.y, other_area.xend, other_area.yend);


		if (((other_area.xend > area.x)
		     && (other_area.x <= area.xend)) /* [other_x, other_xend] intersects [x, xend]? */
		    &&
		    ((other_area.yend > area.y)
		     && (other_area.y <= area.yend))) /* [other_y, other_yend] intersects [y, yend]? */
			return 1;

	}

	SCIkdebug(SCIkBRESEN, " (no)\n");
	return 0;
}


void
kCanBeHere(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = UPARAM(0);
	heap_ptr cliplist = UPARAM_OR_ALT(1, 0);
	gfxw_port_t *port = s->port;
	word signal;

	int x = GET_SELECTOR(obj, brLeft);
	int y = GET_SELECTOR(obj, brTop);
	int xend = GET_SELECTOR(obj, brRight);
	int yend = GET_SELECTOR(obj, brBottom);
	int xl = xend - x;
	int yl = yend - y;
	rect_t zone = gfx_rect(x + port->zone.x, y + port->zone.y, xl, yl);
	abs_rect_t abs_zone = set_base(s, obj);
	word edgehit;

	signal = UGET_SELECTOR(obj, signal);
	SCIkdebug(SCIkBRESEN,"Checking collision: (%d,%d) to (%d,%d), obj=%04x, sig=%04x, cliplist=%04x\n",
		  x, y, xend, yend, obj, signal, cliplist);

	s->acc = !(((word)GET_SELECTOR(obj, illegalBits))
		   & (edgehit = gfxop_scan_bitmask(s->gfx_state, zone, GFX_MASK_CONTROL)));

	SCIkdebug(SCIkBRESEN, "edgehit = %04x\n", edgehit);
	if (s->acc == 0)
		return; /* Can'tBeHere */
	if (signal & (_K_VIEW_SIG_FLAG_REMOVE | _K_VIEW_SIG_FLAG_IGNORE_ACTOR))
		s->acc= signal & (_K_VIEW_SIG_FLAG_REMOVE|_K_VIEW_SIG_FLAG_IGNORE_ACTOR); /* CanBeHere- it's either being disposed, or it ignores actors anyway */

	if (cliplist) {
		heap_ptr node = GET_HEAP(cliplist + LIST_FIRST_NODE);

		s->acc = 0; /* Assume that we Can'tBeHere... */

		while (node) { /* Check each object in the list against our bounding rectangle */
			heap_ptr other_obj = UGET_HEAP(node + LIST_NODE_VALUE);
			if (other_obj != obj) { /* Clipping against yourself is not recommended */

				if (collides_with(s, abs_zone, other_obj, funct_nr, argc, argp))
				    return;

			} /* if (other_obj != obj) */
			node = GET_HEAP(node + LIST_NEXT_NODE); /* Move on */
		}
	}
	/*
	if (s->dyn_views) {
		gfxw_dyn_view_t *widget = (gfxw_dyn_view_t *) s->dyn_views->contents;

		SCIkdebug(SCIkBRESEN, "Checking vs dynviews:\n");

	        while (widget) {
			if (widget->ID && widget->ID != obj)
				if (collides_with(s, abs_zone, widget->ID, funct_nr, argc, argp))
					return;
			widget = (gfxw_dyn_view_t *) widget->next;
		}
	}
	*/
	if (!s->acc)
		s->acc = 1;
	SCIkdebug(SCIkBRESEN, " -> %04x\n", s->acc);

}  /* CanBeHere */



void
kCelHigh(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int view = PARAM(0);
	int loop = PARAM(1);
	int cel = PARAM(2);
	int height, width;
	point_t offset;

	if (argc != 3) {
		SCIkwarn(SCIkWARNING, "CelHigh called with %d parameters!\n", argc);
	}

	if (gfxop_get_cel_parameters(s->gfx_state, view, loop, cel, &width, &height, &offset))
		SCIkwarn(SCIkERROR, "Invalid loop (%d) or cel (%d) in view.%d (0x%x), or view invalid\n", loop, cel, view, view);
	else
		s->acc = height;
}

void
kCelWide(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int view = PARAM(0);
	int loop = PARAM(1);
	int cel = PARAM(2);
	int height, width;
	point_t offset;

	if (argc != 3) {
		SCIkwarn(SCIkWARNING, "CelHigh called with %d parameters!\n", argc);
	}

	if (gfxop_get_cel_parameters(s->gfx_state, view, loop, cel, &width, &height, &offset))
		SCIkwarn(SCIkERROR, "Invalid loop (%d) or cel (%d) in view.%d (0x%x), or view invalid\n", loop, cel, view, view);
	else
		s->acc = width;
}



void
kNumLoops(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = PARAM(0);
	int view = GET_SELECTOR(obj, view);
	int loops_nr = gfxop_lookup_view_get_loops(s->gfx_state, view);

	CHECK_THIS_KERNEL_FUNCTION;

	if (loops_nr < 0) {
		SCIkwarn(SCIkERROR, "view.%d (0x%x) not found\n", view, view);
		return;
	}

	s->acc = loops_nr;

	SCIkdebug(SCIkGRAPHICS, "NumLoops(view.%d) = %d\n", view, s->acc);
}


void
kNumCels(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = PARAM(0);
	int loop = UGET_SELECTOR(obj, loop);
	int view = GET_SELECTOR(obj, view);
	int cel = 0xffff;

	CHECK_THIS_KERNEL_FUNCTION;

	if (gfxop_check_cel(s->gfx_state, view, &loop, &cel)) { /* OK, this is a hack and there's a
							       ** real function to calculate cel numbers... */
		SCIkwarn(SCIkERROR, "view.%d (0x%x) not found\n", PARAM(0), PARAM(0));
		return;
	}

	s->acc = cel + 1;

	SCIkdebug(SCIkGRAPHICS, "NumCels(view.%d, %d) = %d\n", view, loop, s->acc);
}

void
kOnControl(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int arg = 0;
	int map, xstart, ystart;
	int xlen = 1, ylen = 1;

	CHECK_THIS_KERNEL_FUNCTION;

	if (argc == 2 || argc == 4)
		map = 4;
	else {
		arg = 1;
		map = PARAM(0);
	}

	ystart = PARAM(arg+1);
	xstart = PARAM(arg);

	if (argc > 3) {
		ylen = PARAM(arg+3) - ystart;
		xlen = PARAM(arg+2) - xstart;
		if (xlen > 1)
			--xlen;
		if (ylen > 1)
			--ylen;
	}

	s->acc = gfxop_scan_bitmask(s->gfx_state, gfx_rect(xstart, ystart + 10, xlen, ylen), map);

	/*  { */
/*  		int x, y; */
/*  		fprintf(stderr,"Area %d,%d %dx%d returned %04x\n", xstart, ystart + 10, xlen, ylen, s->acc); */

/*  		fprintf(stderr, "     /  %d\n", xstart - 1); */
/*  		for (y = -1; y <= ylen+1; y++) { */
/*  			fprintf(stderr," %3d : ", y + ystart + 10); */
/*  			for (x = -1; x <= xlen+1; x++) */
/*  				fprintf(stderr, "%1x ", ffs(gfxop_scan_bitmask(s->gfx_state, gfx_rect(xstart + x, ystart + 10 + y, */
/*  												      1, 1), map)) - 1); */
/*  			fprintf(stderr, "\n"); */
/*  		} */
/*  	} */
}

void
_k_view_list_free_backgrounds(state_t *s, view_object_t *list, int list_nr);


void
kDrawPic(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int pic_nr = PARAM(0);
	int add_to_pic = 1;
	int palette = PARAM_OR_ALT(3, 0);
	gfx_color_t transparent = s->wm_port->bgcolor;

	CHECK_THIS_KERNEL_FUNCTION;

	if (s->version < SCI_VERSION_FTU_NEWER_DRAWPIC_PARAMETERS) {
		if (!PARAM_OR_ALT(2, 0))
			add_to_pic = 0;
	} else
		if (PARAM_OR_ALT(2, 1))
			add_to_pic = 0;


	gfxop_disable_dirty_frames(s->gfx_state);

	s->old_screen = gfxop_grab_pixmap(s->gfx_state, gfx_rect(0, 10, 320, 190));

	SCIkdebug(SCIkGRAPHICS,"Drawing pic.%03d\n", PARAM(0));

	if (!s->pics) {
		s->pics = malloc(sizeof(drawn_pic_t) * (s->pics_nr = 8));
		s->pics_drawn_nr = 0;
	}

	if (add_to_pic) {
		if (s->pics_nr == s->pics_drawn_nr) {
			s->pics_nr += 4;
			s->pics = realloc(s->pics, sizeof(drawn_pic_t) * s->pics_nr);
		}
		s->pics[s->pics_drawn_nr].palette = palette;
		s->pics[s->pics_drawn_nr++].nr = pic_nr;
		GFX_ASSERT(gfxop_add_to_pic(s->gfx_state, pic_nr, 1, palette));
	} else {
		s->pics_drawn_nr = 1;
		s->pics[0].nr = pic_nr;
		s->pics[0].palette = palette;
		GFX_ASSERT(gfxop_new_pic(s->gfx_state, pic_nr, 1, palette));
	}

	s->wm_port->widfree(GFXW(s->wm_port));
	s->picture_port->widfree(GFXW(s->picture_port));

	s->wm_port = gfxw_new_port(s->visual, NULL, gfx_rect(0, 10, 320, 190), s->ega_colors[0], transparent);
	s->picture_port = gfxw_new_port(s->visual, NULL, gfx_rect(0, 10, 320, 190), s->ega_colors[0], transparent);

	s->visual->add(GFXWC(s->visual), GFXW(s->picture_port));
	s->visual->add(GFXWC(s->visual), GFXW(s->wm_port));

	s->port = s->picture_port;

	if (argc > 1)
		s->pic_animate = PARAM(1); /* The animation used during kAnimate() later on */

	s->dyn_views = NULL;
	s->drop_views = NULL;

	s->priority_first = 42;
	s->priority_last = 180;
	s->pic_not_valid = 1;
	s->pic_is_new = 1;

}


static abs_rect_t
set_base(state_t *s, heap_ptr object)
{
	int x, y, original_y, z, ystep, xsize, ysize;
	int xbase, ybase, xend, yend;
	int view, loop, cel;
	int xmod = 0, ymod = 0;
	abs_rect_t retval;

	if (lookup_selector(s, object, s->selector_map.brLeft, NULL)
	    != SELECTOR_VARIABLE) 
		return; /* non-fatal */

	x = GET_SELECTOR(object, x);
	original_y = y = GET_SELECTOR(object, y);

	if (s->selector_map.z > -1)
		z = GET_SELECTOR(object, z);
	else
		z = 0;

	y -= z; /* Subtract z offset */

	ystep = GET_SELECTOR(object, yStep);
	view = GET_SELECTOR(object, view);
	loop = GET_SELECTOR(object, loop);
	cel = GET_SELECTOR(object, cel);

	if (gfxop_check_cel(s->gfx_state, view, &loop, &cel)) {
		xsize = ysize = xmod = ymod = 0;
	} else {
		point_t offset = gfx_point(0, 0);

		gfxop_get_cel_parameters(s->gfx_state, view, loop, cel,
					 &xsize, &ysize, &offset);

		xmod = offset.x;
		ymod = offset.y;
	}

	xbase = x - xmod - (xsize) / 2;
	xend = xbase + xsize;
	yend = y /*- ymod*/ + 1;
	ybase = yend - ystep;

	SCIkdebug(SCIkBASESETTER, "(%d,%d)+/-(%d,%d), (%d x %d) -> (%d, %d) to (%d, %d)\n",
		  x, y, xmod, ymod, xsize, ysize, xbase, ybase, xend, yend);

	retval.x = xbase;
	retval.y = ybase;
	retval.xend = xend;
	retval.yend = yend;

	return retval;
}
  

void
_k_base_setter(state_t *s, heap_ptr object)
{
	abs_rect_t absrect = set_base(s, object);

	PUT_SELECTOR(object, brLeft, absrect.x);
	PUT_SELECTOR(object, brRight, absrect.xend);
	PUT_SELECTOR(object, brTop, absrect.y);
	PUT_SELECTOR(object, brBottom, absrect.yend);

}

void
kBaseSetter(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr object = PARAM(0);

	CHECK_THIS_KERNEL_FUNCTION;

	_k_base_setter(s, object);

} /* kBaseSetter */


void _k_set_now_seen(state_t *s, heap_ptr object)
{
	int x, y, z;
	int xbase, ybase, xend, yend, xsize, ysize;
	int view, loop, cel;
	int xmod = 0, ymod = 0;

	if (lookup_selector(s, object, s->selector_map.nsTop, NULL)
	    != SELECTOR_VARIABLE) { return; } /* This isn't fatal */
      
	x = GET_SELECTOR(object, x);
	y = GET_SELECTOR(object, y);

	if (s->selector_map.z > -1)
		z = GET_SELECTOR(object, z);
	else
		z = 0;

	y -= z; /* Subtract z offset */

	view = GET_SELECTOR(object, view);
	loop = GET_SELECTOR(object, loop);
	cel = GET_SELECTOR(object, cel);

	if (gfxop_check_cel(s->gfx_state, view, &loop, &cel)) {
		xsize = ysize = xmod = ymod = 0;
	} else {
		point_t offset = gfx_point(0, 0);

		gfxop_get_cel_parameters(s->gfx_state, view, loop, cel,
					 &xsize, &ysize, &offset);

		xmod = offset.x;
		ymod = offset.y;
	}

	xbase = x + xmod - (xsize >> 1);
	xend = xbase + xsize;
	yend = y + ymod + 1; /* +1: Magic modifier */
	ybase = yend - ysize;

	PUT_SELECTOR(object, nsLeft, xbase);
	PUT_SELECTOR(object, nsRight, xend);
	PUT_SELECTOR(object, nsTop, ybase);
	PUT_SELECTOR(object, nsBottom, yend);
}


void
kSetNowSeen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{

	heap_ptr object = PARAM(0);

	CHECK_THIS_KERNEL_FUNCTION;
	_k_set_now_seen(s, object);

} /* kSetNowSeen */




static void
_k_draw_control(state_t *s, heap_ptr obj, int inverse);


void
kDrawControl(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = UPARAM(0);

	CHECK_THIS_KERNEL_FUNCTION;

	_k_draw_control(s, obj, 0);
	FULL_REDRAW();
}


void
kHiliteControl(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = UPARAM(0);

	CHECK_THIS_KERNEL_FUNCTION;

	_k_draw_control(s, obj, 1);
}


#define _K_EDIT_DELETE \
 if (cursor < textlen) { \
  memmove(text + cursor, text + cursor + 1, textlen - cursor +1); \
}

#define _K_EDIT_BACKSPACE \
 if (cursor) { \
  --cursor;    \
  memmove(text + cursor, text + cursor + 1, textlen - cursor +1); \
  --textlen; \
}



void
kEditControl(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = UPARAM(0);
	heap_ptr event = UPARAM(1);

	CHECK_THIS_KERNEL_FUNCTION;

	if (obj) {
		word ct_type = GET_SELECTOR(obj, type);
		switch (ct_type) {

		case K_CONTROL_EDIT:
			if (event && (GET_SELECTOR(event, type) == SCI_EVT_KEYBOARD)) {
				int max = GET_SELECTOR(obj, max);
				int cursor = GET_SELECTOR(obj, cursor);
				int modifiers = GET_SELECTOR(event, modifiers);
				byte key = GET_SELECTOR(event, message);

				char *text = s->heap + UGET_SELECTOR(obj, text);
				int textlen = strlen(text);

				if (cursor > textlen)
					cursor = textlen;

				if (modifiers & SCI_EVM_CTRL) {

					switch (tolower(key)) {
					case 'a': cursor = 0; break;
					case 'e': cursor = textlen; break;
					case 'f': if (cursor < textlen) ++cursor; break;
					case 'b': if (cursor > 0) --cursor; break;
					case 'k': text[cursor] = 0; break; /* Terminate string */
					case 'h': _K_EDIT_BACKSPACE; break;
					case 'd': _K_EDIT_DELETE; break;
					}
					PUT_SELECTOR(event, claimed, 1);

				} else if (modifiers & SCI_EVM_ALT) { /* Ctrl has precedence over Alt */

					switch (tolower(key)) {
					case 'f': while ((cursor < textlen) && (text[cursor++] != ' ')); break;
					case 'b': while ((cursor > 0) && (text[--cursor - 1] != ' ')); break;
					}
					PUT_SELECTOR(event, claimed, 1);

				} else if (key < 31) {

					PUT_SELECTOR(event, claimed, 1);

					switch(key) {
					case SCI_K_BACKSPACE: _K_EDIT_BACKSPACE; break;
					default:
						PUT_SELECTOR(event, claimed, 0);
					}

				} else if ((key >= SCI_K_HOME) && (key <= SCI_K_DELETE)) {

					switch(key) {
					case SCI_K_HOME: cursor = 0; break;
					case SCI_K_END: cursor = textlen; break;
					case SCI_K_RIGHT: if (cursor + 1 <= textlen) ++cursor; break;
					case SCI_K_LEFT: if (cursor > 0) --cursor; break;
					case SCI_K_DELETE: _K_EDIT_DELETE; break;
					}

					PUT_SELECTOR(event, claimed, 1);
				} else if ((key > 31) && (key < 128)) {
					int inserting = (modifiers & SCI_EVM_INSERT);
            
					if (modifiers & (SCI_EVM_RSHIFT | SCI_EVM_LSHIFT))
						key = toupper(key);
					if (modifiers & SCI_EVM_CAPSLOCK)
						key = toupper(key);
					if (modifiers & ((SCI_EVM_RSHIFT | SCI_EVM_LSHIFT) & SCI_EVM_CAPSLOCK))
						key = tolower(key);
					modifiers &= ~(SCI_EVM_RSHIFT | SCI_EVM_LSHIFT | SCI_EVM_CAPSLOCK);

					if (cursor == textlen) {
						if (textlen < max) {
							text[cursor++] = key;
							text[cursor] = 0; /* Terminate string */
						}
					} else if (inserting) {
						if (textlen < max) {
							int i;

							for (i = textlen + 2; i >= cursor; i--)
								text[i] = text[i - 1];
							text[cursor++] = key;

						}
					} else { /* Overwriting */
						text[cursor++] = key;
					}

					PUT_SELECTOR(event, claimed, 1);
				}
          
				PUT_SELECTOR(obj, cursor, cursor); /* Write back cursor position */
			}

		case K_CONTROL_BOX:
		case K_CONTROL_BUTTON:
			if (event) PUT_SELECTOR(event, claimed, 1);
			_k_draw_control(s, obj, 0);
			s->acc = 0;
			break;

		case K_CONTROL_TEXT: {
			int state = GET_SELECTOR(obj, state);
			PUT_SELECTOR(obj, state, state | CONTROL_STATE_DITHER_FRAMED);
			_k_draw_control(s, obj, 0);
			PUT_SELECTOR(obj, state, state);
		}
		break;

		default:
			SCIkwarn(SCIkWARNING, "Attempt to edit control type %d\n", ct_type);
		}
	}
}


static void
_k_draw_control(state_t *s, heap_ptr obj, int inverse)
{
	int x = GET_SELECTOR(obj, nsLeft);
	int y = GET_SELECTOR(obj, nsTop);
	int xl = GET_SELECTOR(obj, nsRight) - x;
	int yl = GET_SELECTOR(obj, nsBottom) - y;
	rect_t area = gfx_rect(x, y, xl, yl);

	int font_nr = GET_SELECTOR(obj, font);
	char *text = s->heap + UGET_SELECTOR(obj, text);
	int view = GET_SELECTOR(obj, view);
	int cel = GET_SELECTOR(obj, cel);
	int loop = GET_SELECTOR(obj, loop);

	int type = GET_SELECTOR(obj, type);
	int state = GET_SELECTOR(obj, state);
	int cursor;

	switch (type) {

	case K_CONTROL_BUTTON:

		SCIkdebug(SCIkGRAPHICS, "drawing button %04x to %d,%d\n", obj, x, y);
		ADD_TO_CURRENT_BG_WIDGETS(sciw_new_button_control(s->port, obj, area, text, font_nr,
								  state & CONTROL_STATE_FRAMED,
								  inverse, state & CONTROL_STATE_GRAY));
		break;

	case K_CONTROL_TEXT:

		SCIkdebug(SCIkGRAPHICS, "drawing text %04x to %d,%d\n", obj, x, y);

		ADD_TO_CURRENT_BG_WIDGETS(sciw_new_text_control(s->port, obj, area, text, font_nr,
							  ((s->version < SCI_VERSION_FTU_CENTERED_TEXT_AS_DEFAULT)?
							  ALIGN_LEFT : ALIGN_CENTER), !!(state & CONTROL_STATE_DITHER_FRAMED),
							  inverse));
		break;

	case K_CONTROL_EDIT:
		SCIkdebug(SCIkGRAPHICS, "drawing edit control %04x to %d,%d\n", obj, x, y);

		cursor = GET_SELECTOR(obj, cursor);
		ADD_TO_CURRENT_BG_WIDGETS(sciw_new_edit_control(s->port, obj, area, text, font_nr, cursor, inverse));
		break;

	case K_CONTROL_ICON:

		SCIkdebug(SCIkGRAPHICS, "drawing icon control %04x to %d,%d\n", obj, x, y -1);

		ADD_TO_CURRENT_BG_WIDGETS(sciw_new_icon_control(s->port, obj, area, view, loop, cel,
							  state & CONTROL_STATE_FRAMED, inverse));
		break;

	case K_CONTROL_CONTROL: {
		char **entries_list = NULL;
		char *seeker;
		int entries_nr;
		int lsTop = UGET_SELECTOR(obj, lsTop);
		int list_top = 0;
		int selection = 0;
		int i;

		SCIkdebug(SCIkGRAPHICS, "drawing list control %04x to %d,%d\n", obj, x, y);
		cursor = UGET_SELECTOR(obj, cursor);

		entries_nr = 0;
		seeker = text;
		while (seeker[0]) { /* Count string entries in NULL terminated string list */
			++entries_nr;
			seeker += SCI_MAX_SAVENAME_LENGTH;
		}

		if (entries_nr) { /* determine list_top, selection, and the entries_list */
			seeker = text;
			entries_list = malloc(sizeof(char *) * entries_nr);
			for (i = 0; i < entries_nr; i++) {
				entries_list[i] = seeker;
				seeker += SCI_MAX_SAVENAME_LENGTH;
				if ((seeker - ((char *)s->heap)) == lsTop)
					list_top = i + 1;
				if ((seeker - ((char *)s->heap)) == cursor)
					selection = i + 1;
			}
		}

		ADD_TO_CURRENT_BG_WIDGETS(sciw_new_list_control(s->port, obj, area, font_nr, entries_list, entries_nr,
							  list_top, selection, inverse));
		if (entries_nr)
			free(entries_list);
	}
	break;

	case K_CONTROL_BOX:
		break;

	default:
		SCIkwarn(SCIkWARNING, "Unknown control type: %d at %04x, at (%d, %d) size %d x %d\n",
			 type, obj, x, y, xl, yl);
	}

	if (!s->pic_not_valid) {
		FULL_REDRAW();
	}
}


static inline void
draw_to_control_map(state_t *s, gfxw_dyn_view_t *view, int pri_top_management, int funct_nr, int argc, int argp)
{
	int priority = view->color.priority; 
	int pri_top = PRIORITY_BAND_FIRST(priority);
	int top, bottom, right, left;
	int has_nsrect = (view->ID <=0)? 0 : lookup_selector(s, view->ID, s->selector_map.nsBottom, NULL) == SELECTOR_VARIABLE;

	if (has_nsrect) {
		int obj = view->ID;

		top = GET_SELECTOR(obj, nsTop);
		bottom = GET_SELECTOR(obj, nsBottom);
		left = GET_SELECTOR(obj, nsLeft);
		right = GET_SELECTOR(obj, nsRight);
	} else {
		int width, height;
		point_t offset;

		GFX_ASSERT(gfxop_get_cel_parameters(s->gfx_state, view->view, view->loop, view->cel,
						    &width, &height, &offset));

		left = view->pos.x + offset.x - (width >> 1);
		right = left + width;
		bottom = view->pos.y + offset.y + 1;
		top = bottom - height;
	}

	if (top < pri_top && pri_top_management)
		top = pri_top;

	if (bottom < top) {
		int foo = top;
		top = bottom;
		bottom = foo;
	}

	if (!(view->signalp && (GET_HEAP(view->signalp) & _K_VIEW_SIG_FLAG_IGNORE_ACTOR))) {
		gfxw_box_t *box;
		gfx_color_t color;
		abs_rect_t abs_bounds = set_base(s, (heap_ptr) view->ID);

		gfxop_set_color(s->gfx_state, &color, -1, -1, -1, -1, -1, 0xf);

		SCIkdebug(SCIkGRAPHICS,"    adding control block (%d,%d)to(%d,%d)\n", left, top,
			  right, bottom);

		box = gfxw_new_box(s->gfx_state, gfx_rect(abs_bounds.x, abs_bounds.y, abs_bounds.xend - abs_bounds.x + 1, abs_bounds.yend - abs_bounds.y + 1)
/*gfx_rect(left, top, right - left + 1, bottom - top + 1)*/,
				   color, color, GFX_BOX_SHADE_FLAT);

		assert_primary_widget_lists(s);

		ADD_TO_CURRENT_PICTURE_PORT(box);
	}
}


static void
_k_view_list_do_postdraw(state_t *s, gfxw_list_t *list)
{
	gfxw_dyn_view_t *widget = (gfxw_dyn_view_t *) list->contents;

	while (widget) {
		int obj = widget->ID;

		if (widget->signal & (_K_VIEW_SIG_FLAG_FREESCI_PRIVATE | _K_VIEW_SIG_FLAG_REMOVE | _K_VIEW_SIG_FLAG_NO_UPDATE) == _K_VIEW_SIG_FLAG_FREESCI_PRIVATE) {
			int has_nsrect = lookup_selector(s, obj, s->selector_map.nsBottom, NULL) == SELECTOR_VARIABLE;

			if (has_nsrect) {
				int temp;

				temp = GET_SELECTOR(obj, nsLeft);
				PUT_SELECTOR(obj, lsLeft, temp);

				temp = GET_SELECTOR(obj, nsRight);
				PUT_SELECTOR(obj, lsRight, temp);

				temp = GET_SELECTOR(obj, nsTop);
				PUT_SELECTOR(obj, lsTop, temp);

				temp = GET_SELECTOR(obj, nsBottom);
				PUT_SELECTOR(obj, lsBottom, temp);
			}

			if (widget->signal & _K_VIEW_SIG_FLAG_HIDDEN)
				widget->signal |= _K_VIEW_SIG_FLAG_REMOVE;
		}

		if (widget->signalp) {
			widget->signal &= 0xffff;
			PUT_HEAP(widget->signalp, widget->signal); /* Write back signal */
		}

		widget = (gfxw_dyn_view_t *) widget->next;
	}
}

static int _k_animate_ran = 0;

int
_k_view_list_dispose_loop(state_t *s, heap_ptr list_addr, gfxw_dyn_view_t *widget,
			  int funct_nr, int argc, int argp)
     /* disposes all list members flagged for disposal; funct_nr is the invoking kfunction */
     /* returns non-zero IFF views were dropped */
{
	int signal;
	int dropped = 0;

	_k_animate_ran = 0;

	if (widget) {
		int retval;
		/* Recurse: */
		retval = _k_view_list_dispose_loop(s, list_addr, (gfxw_dyn_view_t *) widget->next, funct_nr, argc, argp);

		if (retval == -1) /* Bail out on annihilation, rely on re-start from Animate() */
			return -1;

		if (GFXW_IS_DYN_VIEW(widget) && (widget->ID != GFXW_NO_ID)) {
			if ((signal = (UGET_HEAP(widget->signalp))) & _K_VIEW_SIG_FLAG_DISPOSE_ME) {
				int tempid = widget->ID;

				if (widget->under_bitsp) { /* Is there a bg picture left to clean? */
					word mem_handle = widget->under_bits = GET_HEAP(widget->under_bitsp);

					if (mem_handle) {
						kfree(s, mem_handle);
						PUT_HEAP(widget->under_bitsp, widget->under_bits = 0);
					}
				}

				if (invoke_selector(INV_SEL(tempid, delete, 1), 0))
					SCIkwarn(SCIkWARNING, "Object at %04x requested deletion, but does not have"
						 " a delete funcselector\n", tempid);
				if (_k_animate_ran) {
					SCIkwarn(SCIkWARNING, "Object at %04x invoked kAnimate() during deletion!\n", tempid);
					return dropped;
				}

				SCIkdebug(SCIkGRAPHICS, "Freeing %04x with signal=%04x\n", widget->ID, signal);

				if (!(signal & _K_VIEW_SIG_FLAG_HIDDEN)) {
					SCIkdebug(SCIkGRAPHICS, "Adding view at %04x to background\n", widget->ID);
					if (!(gfxw_remove_id(widget->parent, widget->ID) == GFXW(widget))) {
						SCIkwarn(SCIkERROR, "Attempt to remove view with ID %04x from list failed!\n", widget->ID);
						BREAKPOINT();
					}

					s->drop_views->add(GFXWC(s->drop_views), GFXW(widget));

					draw_to_control_map(s, widget, 0, funct_nr, argc, argp);
					widget->draw_bounds.y += s->dyn_views->bounds.y - widget->parent->bounds.y;
					widget->draw_bounds.x += s->dyn_views->bounds.x - widget->parent->bounds.x;
					dropped = 1;
				}
				else {
					SCIkdebug(SCIkGRAPHICS, "Deleting view at %04x\n", widget->ID);
					widget->flags |= GFXW_FLAG_VISIBLE;
					gfxw_annihilate(GFXW(widget));
					return -1; /* restart: Done in Animate() */
				}
			}
		}

	}

	return dropped;
}


#define _K_MAKE_VIEW_LIST_CYCLE 1
#define _K_MAKE_VIEW_LIST_CALC_PRIORITY 2
#define _K_MAKE_VIEW_LIST_DRAW_TO_CONTROL_MAP 4

static gfxw_dyn_view_t *
_k_make_dynview_obj(state_t *s, heap_ptr obj, int options, int nr, int funct_nr, int argc, int argp)
{
	short oldloop, oldcel;
	int cel, loop, view_nr = GET_SELECTOR(obj, view);
	int has_nsrect = lookup_selector(s, obj, s->selector_map.nsBottom, NULL) == SELECTOR_VARIABLE;
	int under_bits, signal;
	heap_ptr under_bitsp, signalp;
	point_t pos;
	int z;
	gfxw_dyn_view_t *widget;

	SCIkdebug(SCIkGRAPHICS, " - Adding %04x\n", obj);

	obj = obj;

	pos.x = GET_SELECTOR(obj, x);
	pos.y = GET_SELECTOR(obj, y);

	pos.y++; /* Sierra appears to do something like this */

	z = GET_SELECTOR(obj, z);

	/* !-- nsRect used to be checked here! */

	loop = oldloop = GET_SELECTOR(obj, loop);
	cel = oldcel = GET_SELECTOR(obj, cel);

	/* Clip loop and cel, write back if neccessary */
	GFX_ASSERT(gfxop_check_cel(s->gfx_state, view_nr, &loop, &cel));

	if (oldloop != loop)
		PUT_SELECTOR(obj, loop, loop);

	if (oldcel != cel)
		PUT_SELECTOR(obj, cel, cel);
    
	if (lookup_selector(s, obj, s->selector_map.underBits, &(under_bitsp))
	    != SELECTOR_VARIABLE) {
		under_bitsp = under_bits = 0;
		SCIkdebug(SCIkGRAPHICS, "Object at %04x has no underBits\n", obj);
	} else under_bits = GET_HEAP(under_bitsp);

	if (lookup_selector(s, obj, s->selector_map.signal, &(signalp))
	    != SELECTOR_VARIABLE) {
		signal = signalp = 0;
		SCIkdebug(SCIkGRAPHICS, "Object at %04x has no signal selector\n", obj);
	} else {
		signal = UGET_HEAP(signalp);
		SCIkdebug(SCIkGRAPHICS, "    with signal = %04x\n", signal);
	}

	widget = gfxw_new_dyn_view(s->gfx_state, pos, z, view_nr, loop, cel,
				   -1, -1, ALIGN_CENTER, ALIGN_BOTTOM, nr);

	if (widget) {

		widget = (gfxw_dyn_view_t *) gfxw_set_id(GFXW(widget), obj);
		widget = gfxw_dyn_view_set_params(widget, under_bits, under_bitsp, signal, signalp);
		widget->flags |= GFXW_FLAG_IMMUNE_TO_SNAPSHOTS; /* Only works the first time 'round */

		return widget;
	} else {
		SCIkwarn(SCIkWARNING, "Could not generate dynview widget for %d/%d/%d\n", view_nr, loop, cel);
		return NULL;
	}
}


static void
_k_make_view_list(state_t *s, gfxw_list_t **widget_list, heap_ptr list, int options, int funct_nr, int argc, int argp)
     /* Creates a view_list from a node list in heap space. Returns the list, stores the
     ** number of list entries in *list_nr. Calls doit for each entry if cycle is set.
     ** argc, argp, funct_nr should be the same as in the calling kernel function.
     */
{
	heap_ptr node;
	int sequence_nr = 0;
	gfxw_dyn_view_t *widget;

	if (!*widget_list) {
		SCIkwarn(SCIkERROR, "make_view_list with widget_list == ()\n");
		BREAKPOINT();
	};

	assert_primary_widget_lists(s);
	/* In case one of the views' doit() does a DrawPic... */

	node = GET_HEAP(list + LIST_LAST_NODE);

	SCIkdebug(SCIkGRAPHICS, "Making list from %04x\n", list);

	if (GET_HEAP(list - 2) != 0x6) { /* heap size check */
		SCIkwarn(SCIkWARNING, "Attempt to draw non-list at %04x\n", list);
		return;
	}

	node = UGET_HEAP(list + LIST_LAST_NODE);
	while (node) {
		heap_ptr obj = GET_HEAP(node + LIST_NODE_VALUE); /* The object we're using */
		gfxw_dyn_view_t *widget;

		widget = _k_make_dynview_obj(s, obj, options, sequence_nr++, funct_nr, argc, argp);

		if (widget) {
			GFX_ASSERT((*widget_list)->add(GFXWC(*widget_list), GFXW(widget)));

			if (options & _K_MAKE_VIEW_LIST_CYCLE) {
				if (!(widget->signal & _K_VIEW_SIG_FLAG_FROZEN))

					SCIkdebug(SCIkGRAPHICS, "  invoking %04x::doit()\n", obj);
					invoke_selector(INV_SEL(obj, doit, 1), 0); /* Call obj::doit() if neccessary */

			}
		}

		node = UGET_HEAP(node + LIST_PREVIOUS_NODE); /* Next node */
	}


	widget = (gfxw_dyn_view_t *) (*widget_list)->contents;

	while(widget) { /* Read back widget values */
		if (widget->signalp)
			widget->signal = widget->signal & _K_VIEW_SIG_FLAG_FREESCI_PRIVATE
				| UGET_HEAP(widget->signalp);

		widget = (gfxw_dyn_view_t *) widget->next;
	}
}


static void
_k_prepare_view_list(state_t *s, gfxw_list_t *list, int options, int funct_nr, int argc, heap_ptr argp)
{
	gfxw_dyn_view_t *view = (gfxw_dyn_view_t *) list->contents;

	while (view) {
		heap_ptr obj = view->ID;
		int priority, _priority;
		int has_nsrect = (view->ID <=0)? 0 : lookup_selector(s, view->ID, s->selector_map.nsBottom, NULL) == SELECTOR_VARIABLE;
		int oldsignal = view->signal;

		_k_set_now_seen(s, view->ID);
		_priority = VIEW_PRIORITY((view->pos.y)); /* Accomodate difference between interpreter and gfx engine */

		if (has_nsrect
		    && !(view->signal & _K_VIEW_SIG_FLAG_FIX_PRI_ON)) { /* Calculate priority */

			if (options & _K_MAKE_VIEW_LIST_CALC_PRIORITY)
				PUT_SELECTOR(obj, priority, _priority);

			priority = _priority;

		} else /* DON'T calculate the priority */
			priority = GET_SELECTOR(obj, priority);

		if ((options & _K_MAKE_VIEW_LIST_DRAW_TO_CONTROL_MAP) && priority <= 0)
			priority = _priority; /* Always for picviews */

		view->color.priority = priority;
		if (priority > -1)
			view->color.mask |= GFX_MASK_PRIORITY;
		else
			view->color.mask &= ~GFX_MASK_PRIORITY;
		
		if (options & _K_MAKE_VIEW_LIST_DRAW_TO_CONTROL_MAP)
			draw_to_control_map(s, view, 1, funct_nr, argc, argp);


		/* Extreme Pattern Matching ugliness ahead... */
		if ((view->signal & _K_VIEW_SIG_FLAG_NO_UPDATE
		     && (  /* Brainwashed by LISP */
			 (view->signal & (_K_VIEW_SIG_FLAG_UPDATED | _K_VIEW_SIG_FLAG_FORCE_UPDATE))
			 || ((view->signal & (_K_VIEW_SIG_FLAG_HIDDEN | _K_VIEW_SIG_FLAG_REMOVE)) == _K_VIEW_SIG_FLAG_REMOVE)
			 || ((view->signal & (_K_VIEW_SIG_FLAG_HIDDEN | _K_VIEW_SIG_FLAG_REMOVE | _K_VIEW_SIG_FLAG_ALWAYS_UPDATE)) == _K_VIEW_SIG_FLAG_ALWAYS_UPDATE)
			 || ((view->signal & (_K_VIEW_SIG_FLAG_HIDDEN | _K_VIEW_SIG_FLAG_ALWAYS_UPDATE)) == (_K_VIEW_SIG_FLAG_HIDDEN | _K_VIEW_SIG_FLAG_ALWAYS_UPDATE))
			 )
		     )
		    ||
		    (!(view->signal & _K_VIEW_SIG_FLAG_NO_UPDATE)
		     && (!!(view->signal & _K_VIEW_SIG_FLAG_STOP_UPDATE) ^ !!(view->signal & _K_VIEW_SIG_FLAG_ALWAYS_UPDATE))
		     )
		    ) {
			view->signal |= _K_VIEW_SIG_FLAG_FREESCI_PRIVATE;
			s->pic_not_valid++;
		}

		SCIkdebug(SCIkGRAPHICS, "  dv[%04x]: signal %04x -> %04x\n", view->ID, oldsignal, view->signal);

		if (view->signal & _K_VIEW_SIG_FLAG_NO_UPDATE)
			view->signal &= ~_K_VIEW_SIG_FLAG_STOP_UPDATE;
		else
			view->signal &= ~_K_VIEW_SIG_FLAG_FORCE_UPDATE;

		view = (gfxw_dyn_view_t *) view->next;
	}
}

static void
_k_update_signals_in_view_list(gfxw_list_t *old_list, gfxw_list_t *new_list)
{ /* O(n^2)... a bit painful, but much faster than the redraws it helps prevent */
	gfxw_dyn_view_t *old_widget = (gfxw_dyn_view_t *) old_list->contents;

	/* Traverses all old widgets, updates them with signals from the new widgets.
	** This is done to avoid evil hacks in widget.c; widgets with unique IDs are
	** replaced there iff they are NOT equal_to a new widget with the same ID.
	** If they were replaced every time, we'd be doing far too many redraws.
	*/

	while (old_widget) {
		gfxw_dyn_view_t *new_widget = (gfxw_dyn_view_t *) new_list->contents;

		while (new_widget && new_widget->ID != old_widget->ID)
			new_widget = (gfxw_dyn_view_t *) new_widget->next;

		if (new_widget)
			old_widget->signal = new_widget->signal;

		old_widget = (gfxw_dyn_view_t *) old_widget->next;
	}
}

static void
_k_view_list_kryptonize(gfxw_widget_t *v)
{
	if (v) {
		v->flags &= ~GFXW_FLAG_IMMUNE_TO_SNAPSHOTS;
		_k_view_list_kryptonize(v->next);
	}
}

static void
_k_raise_topmost_in_view_list(state_t *s, gfxw_list_t *list, gfxw_dyn_view_t *view)
{
	if (view) {
		gfxw_dyn_view_t *next = (gfxw_dyn_view_t *) view->next;

		if ((view->signal & (_K_VIEW_SIG_FLAG_NO_UPDATE | _K_VIEW_SIG_FLAG_HIDDEN | _K_VIEW_SIG_FLAG_ALWAYS_UPDATE)) == 0) {
			SCIkdebug(SCIkGRAPHICS, "Forcing precedence 2 at [%04x] with %04x\n", view->ID, view->signal);
			view->force_precedence = 2;
		}

		gfxw_remove_widget_from_container(view->parent, GFXW(view));

		if (view->signal & _K_VIEW_SIG_FLAG_HIDDEN)
			gfxw_hide_widget(GFXW(view));
 		else
			gfxw_show_widget(GFXW(view));

		list->add(GFXWC(list), GFXW(view));

		_k_raise_topmost_in_view_list(s, list, next);
	}
}


static void
_k_redraw_view_list(state_t *s, gfxw_list_t *list)
{
	gfxw_dyn_view_t *view = (gfxw_dyn_view_t *) list->contents;

	while (view) {

		SCIkdebug(SCIkGRAPHICS, "  dv[%04x]: signal %04x\n", view->ID, view->signal);

		if (view->signal & _K_VIEW_SIG_FLAG_NO_UPDATE) {
			view->signal &= ~_K_VIEW_SIG_FLAG_FORCE_UPDATE;
			if (view->signal & _K_VIEW_SIG_FLAG_UPDATED)
				view->signal &= ~(_K_VIEW_SIG_FLAG_UPDATED | _K_VIEW_SIG_FLAG_NO_UPDATE);
		} else { /* NO_UPD is not set */
			if (view->signal & _K_VIEW_SIG_FLAG_STOP_UPDATE) {
				view->signal &= ~_K_VIEW_SIG_FLAG_STOP_UPDATE;
				view->signal |= _K_VIEW_SIG_FLAG_NO_UPDATE;
			}
		}

		SCIkdebug(SCIkGRAPHICS, "    at substep 6: signal %04x\n", view->signal);

		if (view->signal & _K_VIEW_SIG_FLAG_ALWAYS_UPDATE)
			view->signal &= ~(_K_VIEW_SIG_FLAG_STOP_UPDATE | _K_VIEW_SIG_FLAG_UPDATED
					  | _K_VIEW_SIG_FLAG_NO_UPDATE | _K_VIEW_SIG_FLAG_FORCE_UPDATE);

		SCIkdebug(SCIkGRAPHICS, "    at substep 11/14: signal %04x\n", view->signal);

		if (!(view->signal & _K_VIEW_SIG_FLAG_NO_UPDATE)) {
			if (view->signal & _K_VIEW_SIG_FLAG_HIDDEN)
				view->signal |= _K_VIEW_SIG_FLAG_REMOVE;
			else
				view->signal &= ~_K_VIEW_SIG_FLAG_REMOVE;
		} else if (!(view->signal & _K_VIEW_SIG_FLAG_HIDDEN))
			view->force_precedence = 1;

		SCIkdebug(SCIkGRAPHICS, "    -> signal %04x\n", view->signal);

		view = (gfxw_dyn_view_t *) view->next;
	}
}


/* Flags for _k_draw_view_list */
/* Whether some magic with the base object's "signal" selector should be done: */
#define _K_DRAW_VIEW_LIST_USE_SIGNAL 1
/* This flag draws all views with the "DISPOSE_ME" flag set: */ 
#define _K_DRAW_VIEW_LIST_DISPOSEABLE 2
/* Use this one to draw all views with "DISPOSE_ME" NOT set: */
#define _K_DRAW_VIEW_LIST_NONDISPOSEABLE 4

void
_k_draw_view_list(state_t *s, gfxw_list_t *list, int flags)
     /* Draws list_nr members of list to s->pic. */
{
	gfxw_dyn_view_t *widget = (gfxw_dyn_view_t *) list->contents;

	if (GFXWC(s->port) != GFXWC(s->dyn_views->parent))
		return; /* Return if the pictures are meant for a different port */

	while (widget) {
		if (GFXW_IS_DYN_VIEW(widget) && widget->ID) {
			word signal = (flags & _K_DRAW_VIEW_LIST_USE_SIGNAL)? UGET_HEAP(widget->signalp) : 0;

			if (signal & _K_VIEW_SIG_FLAG_HIDDEN)
				gfxw_hide_widget(GFXW(widget));
			else
				gfxw_show_widget(GFXW(widget));

			if (!(flags & _K_DRAW_VIEW_LIST_USE_SIGNAL)
			    || ((flags & _K_DRAW_VIEW_LIST_DISPOSEABLE) && (signal & _K_VIEW_SIG_FLAG_DISPOSE_ME))
			    || ((flags & _K_DRAW_VIEW_LIST_NONDISPOSEABLE) && !(signal & _K_VIEW_SIG_FLAG_DISPOSE_ME))) {

				if (flags & _K_DRAW_VIEW_LIST_USE_SIGNAL) {
					signal &= ~(_K_VIEW_SIG_FLAG_STOP_UPDATE | _K_VIEW_SIG_FLAG_UPDATED |
						    _K_VIEW_SIG_FLAG_NO_UPDATE | _K_VIEW_SIG_FLAG_FORCE_UPDATE);
					/* Clear all of those flags */

					if (signal & _K_VIEW_SIG_FLAG_HIDDEN)
						gfxw_hide_widget(GFXW(widget));
					else
						gfxw_show_widget(GFXW(widget));

					PUT_HEAP(widget->signalp, signal); /* Write the changes back */
				};

			} /* ...if we're drawing disposeables and this one is disposeable, or if we're drawing non-
			  ** disposeables and this one isn't disposeable */
		}

		widget = (gfxw_dyn_view_t *) widget->next;
	} /* while (widget) */

}

void
kAddToPic(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	gfxw_list_t *pic_views;
	heap_ptr list = PARAM(0);
	CHECK_THIS_KERNEL_FUNCTION;

	assert_primary_widget_lists(s);

	if (argc > 1) {
		int view, cel, loop, x, y, priority, control;
		gfxw_widget_t *widget;

		view = UPARAM(0);
		cel = UPARAM(1);
		loop = UPARAM(2);
		x = PARAM(3);
		y = PARAM(4);
		priority = PARAM(5);
		control = PARAM(6);
		
		widget = GFXW(gfxw_new_dyn_view(s->gfx_state, gfx_point(x,y), 0, view, loop, cel,
						priority, control, ALIGN_CENTER, ALIGN_BOTTOM, 0));
		/*		draw_to_control_map(s, (gfxw_dyn_view_t *) widget, 1, funct_nr, argc, argp); */

		if (!widget) {
			SCIkwarn(SCIkERROR, "Attempt to single-add invalid picview (%d/%d/%d)\n", view, loop, cel);
		} else {
			ADD_TO_CURRENT_PICTURE_PORT(widget);
		}
			
	} else {

		if (!list)
			return;

		pic_views = gfxw_new_list(s->picture_port->bounds, 0);

		SCIkdebug(SCIkGRAPHICS, "Preparing picview list...\n");
		_k_make_view_list(s, &pic_views, list, 0, funct_nr, argc, argp);
		_k_prepare_view_list(s, pic_views, _K_MAKE_VIEW_LIST_DRAW_TO_CONTROL_MAP, funct_nr, argc, argp);
		/* Store pic views for later re-use */

		SCIkdebug(SCIkGRAPHICS, "Drawing picview list...\n");
		ADD_TO_CURRENT_PICTURE_PORT(pic_views);
		_k_draw_view_list(s, pic_views, _K_DRAW_VIEW_LIST_NONDISPOSEABLE | _K_DRAW_VIEW_LIST_DISPOSEABLE);
		/* Draw relative to the bottom center */
		SCIkdebug(SCIkGRAPHICS, "Returning.\n");
	}
	reparentize_primary_widget_lists(s, s->port);
}


void
kGetPort(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	s->acc = s->port->ID;
}


void
kSetPort(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	unsigned int port_nr = PARAM(0);
	gfxw_port_t *new_port;

	CHECK_THIS_KERNEL_FUNCTION;
  
	new_port = gfxw_find_port(s->visual, port_nr);
  
	if (!new_port) {
		SCIkwarn(SCIkERROR, "Invalid port %04x requested\n", port_nr);
		return;
	}

	s->port->draw(GFXW(s->port), gfxw_point_zero); /* Update the port we're leaving */
	s->port = new_port;
}


void
kDrawCel(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int view = PARAM(0);
	int loop = PARAM(1);
	int cel = PARAM(2);
	int x = PARAM(3);
	int y = PARAM(4);
	int priority = PARAM_OR_ALT(5, -1);
	gfxw_view_t *new_view;

	CHECK_THIS_KERNEL_FUNCTION;

	if (!view) {
		SCIkwarn(SCIkERROR, "Attempt to draw non-existing view.%03n\n", view);
		return;
	}

	if (gfxop_check_cel(s->gfx_state, view, &loop, &cel)) {
		SCIkwarn(SCIkERROR, "Attempt to draw non-existing view.%03n\n", view);
		return;
	}

	SCIkdebug(SCIkGRAPHICS, "DrawCel((%d,%d), (view.%d, %d, %d), p=%d)\n", x, y, view, loop,
		  cel, priority);

	new_view = gfxw_new_view(s->gfx_state, gfx_point(x, y), view, loop, cel, priority, -1,
				 ALIGN_LEFT, ALIGN_TOP, GFXW_VIEW_FLAG_DONT_MODIFY_OFFSET);

	ADD_TO_CURRENT_BG_WIDGETS(new_view);
	FULL_REDRAW();
}



void
kDisposeWindow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	unsigned int goner_nr = PARAM(0);
	gfxw_port_t *goner;
	gfxw_port_t *pred;

	CHECK_THIS_KERNEL_FUNCTION;

	goner = gfxw_find_port(s->visual, goner_nr);

	if ((goner_nr < 3) || (goner == NULL)) {
		SCIkwarn(SCIkERROR, "Removal of invalid window %04x requested\n", goner_nr);
		return;
	}

	if (GFXWC(s->dyn_views->parent) == GFXWC(goner))
		reparentize_primary_widget_lists(s, (gfxw_port_t *) goner->parent);

	pred = gfxw_remove_port(s->visual, goner);

	if (goner == s->port) /* Did we kill the active port? */
		s->port = pred;

	s->port = gfxw_find_port(s->visual, goner_nr - 1);
	if (!s->port) {
		SCIkwarn(SCIkERROR, "Falling back to previous port %d failed! Using widget predecessor!\n", goner_nr - 1);
		s->port = pred;
	}

	gfxop_update(s->gfx_state);
}


void
kNewWindow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	gfxw_port_t *window;
	int x, y, xl, yl, flags;
	gfx_color_t bgcolor;
	int priority;

	CHECK_THIS_KERNEL_FUNCTION;

	y = PARAM(0);
	x = PARAM(1);
	yl = PARAM(2) - y;
	xl = PARAM(3) - x;

	y += s->wm_port->bounds.y - 1;

	if (x+xl > 319)
		x -= ((x+xl) - 319);

	flags = PARAM(5);

	bgcolor = s->ega_colors[PARAM_OR_ALT(8, 15)];
	priority = PARAM_OR_ALT(6, -1);
	bgcolor.mask = GFX_MASK_VISUAL | ((priority >= 0)? GFX_MASK_PRIORITY : 0);
	bgcolor.priority = priority;

	SCIkdebug(SCIkGRAPHICS, "New window with params %d, %d, %d, %d\n", PARAM(0), PARAM(1), PARAM(2), PARAM(3));
	window = sciw_new_window(s, gfx_rect(x, y, xl, yl), s->titlebar_port->font_nr, 
				 s->ega_colors[PARAM_OR_ALT(7, 0)], bgcolor, s->titlebar_port->font_nr,
				 s->ega_colors[15], s->ega_colors[8], s->heap + UPARAM(4), flags);

	ADD_TO_CURRENT_PORT(window);
	FULL_REDRAW();

	window->draw(GFXW(window), gfxw_point_zero);
	gfxop_update(s->gfx_state);

	s->port = window; /* Set active port */

	s->acc = window->ID;
}


#define K_ANIMATE_CENTER_OPEN_H  0 /* horizontally open from center */
#define K_ANIMATE_CENTER_OPEN_V  1 /* vertically open from center */
#define K_ANIMATE_RIGHT_OPEN     2 /* open from right */
#define K_ANIMATE_LEFT_OPEN      3 /* open from left */
#define K_ANIMATE_BOTTOM_OPEN    4 /* open from bottom */
#define K_ANIMATE_TOP_OPEN       5 /* open from top */
#define K_ANIMATE_BORDER_OPEN_F  6 /* open from edges to center */
#define K_ANIMATE_CENTER_OPEN_F  7 /* open from center to edges */
#define K_ANIMATE_OPEN_CHECKERS  8 /* open random checkboard */
#define K_ANIMATE_BORDER_CLOSE_H_CENTER_OPEN_H  9 /* horizontally close to center,reopen from center */
#define K_ANIMATE_BORDER_CLOSE_V_CENTER_OPEN_V 10 /* vertically close to center, reopen from center */
#define K_ANIMATE_LEFT_CLOSE_RIGHT_OPEN        11 /* close to right, reopen from right */
#define K_ANIMATE_RIGHT_CLOSE_LEFT_OPEN        12 /* close to left,  reopen from left */
#define K_ANIMATE_TOP_CLOSE_BOTTOM_OPEN        13 /* close to bottom, reopen from bottom */
#define K_ANIMATE_BOTTOM_CLOSE_TOP_OPEN        14 /* close to top, reopen from top */
#define K_ANIMATE_CENTER_CLOSE_F_BORDER_OPEN_F 15 /* close from center to edges,
						  ** reopen from edges to center */
#define K_ANIMATE_BORDER_CLOSE_F_CENTER_OPEN_F 16 /* close from edges to center, reopen from
						  ** center to edges */
#define K_ANIMATE_CLOSE_CHECKERS_OPEN_CHECKERS 17 /* close random checkboard, reopen */

#define K_ANIMATE_OPEN_SIMPLE 100 /* No animation */


#define GRAPH_BLANK_BOX(s, x, y, xl, yl, color) GFX_ASSERT(gfxop_fill_box(s->gfx_state, gfx_rect(x, y, xl, yl), s->ega_colors[color]));

#define GRAPH_UPDATE_BOX(s, x, y, xl, yl) GFX_ASSERT(gfxop_draw_pixmap(s->gfx_state, newscreen, gfx_rect(x, (y) - 10, xl, yl), gfx_point(x, y)));


static void
animate_do_animation(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int i, remaining_checkers;
	char checkers[32 * 19];

	gfx_pixmap_t *newscreen = gfxop_grab_pixmap(s->gfx_state, gfx_rect(0, 10, 320, 190));

	if (!newscreen) {
		SCIkwarn(SCIkERROR, "Failed to allocate 'newscreen'!\n");
		return;
	}

	GFX_ASSERT(gfxop_draw_pixmap(s->gfx_state, s->old_screen, gfx_rect(0, 0, 320, 190), gfx_point(0, 10)));
	gfxop_update_box(s->gfx_state, gfx_rect(0, 0, 320, 200));

	SCIkdebug(SCIkGRAPHICS, "Animating pic opening type %x\n", s->pic_animate);

	gfxop_enable_dirty_frames(s->gfx_state);

	if (s->animation_delay < 1)
		s->pic_animate = K_ANIMATE_OPEN_SIMPLE;

	switch(s->pic_animate) {
	case K_ANIMATE_BORDER_CLOSE_H_CENTER_OPEN_H :

		for (i = 0; i < 160; i++) {
			GRAPH_BLANK_BOX(s, i, 10, 1, 190, 0);
			gfxop_update(s->gfx_state);
			GRAPH_BLANK_BOX(s, 319-i, 10, 1, 190, 0);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, s->animation_delay);
			process_sound_events(s);
		}

	case K_ANIMATE_CENTER_OPEN_H :

		for (i = 159; i >= 0; i--) {
			GRAPH_UPDATE_BOX(s, i, 10, 1, 190);
			gfxop_update(s->gfx_state);
			GRAPH_UPDATE_BOX(s, 319-i, 10, 1, 190);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, s->animation_delay);
			process_sound_events(s);
		}
		break;


	case K_ANIMATE_BORDER_CLOSE_V_CENTER_OPEN_V :
		
		for (i = 0; i < 95; i++) {
			GRAPH_BLANK_BOX(s, 0, i + 10, 320, 1, 0);
			gfxop_update(s->gfx_state);
			GRAPH_BLANK_BOX(s, 0, 199 - i, 320, 1, 0);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, 2 * s->animation_delay);
			process_sound_events(s);
		}

	case K_ANIMATE_CENTER_OPEN_V :

		for (i = 94; i >= 0; i--) {
			GRAPH_UPDATE_BOX(s, 0, i + 10, 320, 1);
			gfxop_update(s->gfx_state);
			GRAPH_UPDATE_BOX(s, 0, 199 - i, 320, 1);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, 2 * s->animation_delay);
			process_sound_events(s);
		}
		break;


	case K_ANIMATE_LEFT_CLOSE_RIGHT_OPEN :

		for(i = 0; i < 320; i++) {
			GRAPH_BLANK_BOX(s, i, 10, 1, 190, 0);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, s->animation_delay / 2);
			process_sound_events(s);
		}

	case K_ANIMATE_RIGHT_OPEN :
		for(i = 319; i >= 0; i--) {
			GRAPH_UPDATE_BOX(s, i, 10, 1, 190);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, s->animation_delay / 2);
			process_sound_events(s);
		}
		break;


	case K_ANIMATE_RIGHT_CLOSE_LEFT_OPEN :

		for(i = 319; i >= 0; i--) {
			GRAPH_BLANK_BOX(s, i, 10, 1, 190, 0);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, s->animation_delay / 2);
			process_sound_events(s);
		}

	case K_ANIMATE_LEFT_OPEN :

		for(i = 0; i < 320; i++) {
			GRAPH_UPDATE_BOX(s, i, 10, 1, 190);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, s->animation_delay / 2);
			process_sound_events(s);
		}
		break;


	case K_ANIMATE_TOP_CLOSE_BOTTOM_OPEN :

		for (i = 10; i < 200; i++) {
			GRAPH_BLANK_BOX(s, 0, i, 320, 1, 0);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, s->animation_delay);
			process_sound_events(s);
		}

	case K_ANIMATE_BOTTOM_OPEN :

		for (i = 199; i >= 10; i--) {
			GRAPH_UPDATE_BOX(s, 0, i, 320, 1);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, s->animation_delay);
			process_sound_events(s);
		}
		break;


	case K_ANIMATE_BOTTOM_CLOSE_TOP_OPEN :

		for (i = 199; i >= 10; i--) {
			GRAPH_BLANK_BOX(s, 0, i, 320, 1, 0);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, s->animation_delay);
			process_sound_events(s);
		}

	case K_ANIMATE_TOP_OPEN :

		for (i = 10; i < 200; i++) {
			GRAPH_UPDATE_BOX(s, 0, i, 320, 1);
			gfxop_update(s->gfx_state);
			gfxop_usleep(s->gfx_state, s->animation_delay);
			process_sound_events(s);
		}
		break;


	case K_ANIMATE_CENTER_CLOSE_F_BORDER_OPEN_F :

		for (i = 31; i >= 0; i--) {
			int height = i * 3;
			int width = i * 5;

			GRAPH_BLANK_BOX(s, width, 10 + height, 5, 190 - 2*height, 0);
			gfxop_update(s->gfx_state);
			GRAPH_BLANK_BOX(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height, 0);
			gfxop_update(s->gfx_state);

			GRAPH_BLANK_BOX(s, width, 10 + height, 320 - 2*width, 3, 0);
			gfxop_update(s->gfx_state);
			GRAPH_BLANK_BOX(s, width, 200 - 3 - height, 320 - 2*width, 3, 0);
			gfxop_update(s->gfx_state);

			gfxop_usleep(s->gfx_state, 4 * s->animation_delay);
			process_sound_events(s);
		}

	case K_ANIMATE_BORDER_OPEN_F :

		for (i = 0; i < 32; i++) {
			int height = i * 3;
			int width = i * 5;

			GRAPH_UPDATE_BOX(s, width, 10 + height, 5, 190 - 2*height);
			gfxop_update(s->gfx_state);
			GRAPH_UPDATE_BOX(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height);
			gfxop_update(s->gfx_state);

			GRAPH_UPDATE_BOX(s, width, 10 + height, 320 - 2*width, 3);
			gfxop_update(s->gfx_state);
			GRAPH_UPDATE_BOX(s, width, 200 - 3 - height, 320 - 2*width, 3);
			gfxop_update(s->gfx_state);

			gfxop_usleep(s->gfx_state, 4 * s->animation_delay);
			process_sound_events(s);
		}

		break;


	case K_ANIMATE_BORDER_CLOSE_F_CENTER_OPEN_F :

		for (i = 0; i < 32; i++) {
			int height = i * 3;
			int width = i * 5;

			GRAPH_BLANK_BOX(s, width, 10 + height, 5, 190 - 2*height, 0);
			gfxop_update(s->gfx_state);
			GRAPH_BLANK_BOX(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height, 0);
			gfxop_update(s->gfx_state);

			GRAPH_BLANK_BOX(s, width, 10 + height, 320 - 2*width, 4, 0);
			gfxop_update(s->gfx_state);
			GRAPH_BLANK_BOX(s, width, 200 - 4 - height, 320 - 2*width, 4, 0);
			gfxop_update(s->gfx_state);

			gfxop_usleep(s->gfx_state, 7 * s->animation_delay);
			process_sound_events(s);
		}

	case K_ANIMATE_CENTER_OPEN_F :

		for (i = 31; i >= 0; i--) {
			int height = i * 3;
			int width = i * 5;

			GRAPH_UPDATE_BOX(s, width, 10 + height, 5, 190 - 2*height);
			gfxop_update(s->gfx_state);
			GRAPH_UPDATE_BOX(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height);
			gfxop_update(s->gfx_state);

			GRAPH_UPDATE_BOX(s, width, 10 + height, 320 - 2 * width, 3);
			gfxop_update(s->gfx_state);
			GRAPH_UPDATE_BOX(s, width, 200 - 3 - height, 320 - 2 * width, 3);
			gfxop_update(s->gfx_state);

			gfxop_usleep(s->gfx_state, 7 * s->animation_delay);
			process_sound_events(s);
		}

		break;


	case K_ANIMATE_CLOSE_CHECKERS_OPEN_CHECKERS :

		memset(checkers, 0, sizeof(checkers));
		remaining_checkers = 19 * 32;

		while (remaining_checkers) {
			int x, y, checker = 1 + (int) (1.0 * remaining_checkers*rand()/(RAND_MAX+1.0));
			i = -1;

			while (checker)
				if (checkers[++i] == 0) --checker;
			checkers[i] = 1; /* Mark checker as used */

			x = i % 32;
			y = i / 32;

			GRAPH_BLANK_BOX(s, x * 10, 10 + y * 10, 10, 10, 0);
			gfxop_update(s->gfx_state);
			if (remaining_checkers & 1) {
				gfxop_usleep(s->gfx_state, s->animation_delay / 4);
			}

			--remaining_checkers;
			process_sound_events(s);
		}

	case K_ANIMATE_OPEN_CHECKERS :

		memset(checkers, 0, sizeof(checkers));
		remaining_checkers = 19 * 32;

		while (remaining_checkers) {
			int x, y, checker = 1 + (int) (1.0 * remaining_checkers * rand()/(RAND_MAX+1.0));
			i = -1;

			while (checker)
				if (checkers[++i] == 0) --checker;
			checkers[i] = 1; /* Mark checker as used */

			x = i % 32;
			y = i / 32;

			GRAPH_UPDATE_BOX(s, x * 10, 10 + y * 10, 10, 10);

			gfxop_update(s->gfx_state);
			if (remaining_checkers & 1) {
				gfxop_usleep(s->gfx_state, s->animation_delay / 4);
			}

			--remaining_checkers;
			process_sound_events(s);
		}

		break;


	default:
		if (s->pic_animate != K_ANIMATE_OPEN_SIMPLE)
			SCIkwarn(SCIkWARNING, "Unknown opening animation 0x%02x\n", s->pic_animate);

		GRAPH_UPDATE_BOX(s, 0, 10, 320, 190);
	}

	GFX_ASSERT(gfxop_free_pixmap(s->gfx_state, s->old_screen));
	GFX_ASSERT(gfxop_free_pixmap(s->gfx_state, newscreen));
	s->old_screen = NULL;

}


void
kAnimate(state_t *s, int funct_nr, int argc, heap_ptr argp)
     /* Animations are supposed to take a maximum of s->animation_delay milliseconds. */
{
	heap_ptr cast_list = UPARAM_OR_ALT(0, 0);
	int cycle = UPARAM_OR_ALT(1, 0);
	int old_pnv = s->pic_not_valid; /* Backup old value */
	int open_animation = 0;

	CHECK_THIS_KERNEL_FUNCTION;

	process_sound_events(s); /* Take care of incoming events (kAnimate is called semi-regularly) */
	_k_animate_ran = 1; /* Used by some of the invoked functions to check for recursion, which may,
			    ** after all, damage the cast list  */


	open_animation = (s->pic_is_new) && (s->pic_not_valid);
	s->pic_is_new = 0;

	assert_primary_widget_lists(s);

	if (!s->dyn_views->contents /* Only reparentize empty dynview list */
	    && ((GFXWC(s->port) != GFXWC(s->dyn_views->parent)) /* If dynviews are on other port... */
		|| (s->dyn_views->next))) /* ... or not on top of the view list */
		reparentize_primary_widget_lists(s, s->port);

	if (cast_list) {
		gfxw_list_t *templist = gfxw_new_list(s->dyn_views->bounds, 0);

		_k_make_view_list(s, &(templist), cast_list, (cycle? _K_MAKE_VIEW_LIST_CYCLE : 0)
				  | _K_MAKE_VIEW_LIST_CALC_PRIORITY, funct_nr, argc, argp);

		if (s->pic_is_new) { /* Happens if DrawPic() is executed by a dynview (yes, that happens) */
			kAnimate(s, funct_nr, argc, argp);
			return;
		}

		SCIkdebug(SCIkGRAPHICS, "Handling Dynviews (..step 9 inclusive):\n");
		_k_prepare_view_list(s, templist, _K_MAKE_VIEW_LIST_CALC_PRIORITY, funct_nr, argc, argp);

		if (s->pic_not_valid) {
			SCIkdebug(SCIkGRAPHICS, "PicNotValid=%d -> Subalgorithm:\n");
			_k_redraw_view_list(s, templist);
		}

		_k_update_signals_in_view_list(s->dyn_views, templist);
		s->dyn_views->tag(GFXW(s->dyn_views));

		_k_raise_topmost_in_view_list(s, s->dyn_views, (gfxw_dyn_view_t *) templist->contents);

		templist->widfree(GFXW(templist));
		s->dyn_views->free_tagged(GFXWC(s->dyn_views)); /* Free obsolete dynviews */
	} /* if (cast_list) */

	if (open_animation) {
		gfxop_clear_box(s->gfx_state, gfx_rect(0, 10, 320, 190)); /* Propagate pic */
		s->visual->add_dirty_abs(GFXWC(s->visual), gfx_rect_fullscreen, 0);
		/* Mark screen as dirty so picviews will be drawn correctly */
		FULL_REDRAW();

		animate_do_animation(s, funct_nr, argc, argp);
	} /* if (open_animation) */

	if (cast_list) {
		int retval;
		int reparentize = 0;

		s->pic_not_valid = 0;

		_k_view_list_do_postdraw(s, s->dyn_views);

		/* _k_view_list_dispose_loop() returns -1 if it requested a re-start, so we do just that. */
		while ((retval = _k_view_list_dispose_loop(s, cast_list, (gfxw_dyn_view_t *) s->dyn_views->contents, funct_nr, argc, argp) < 0))
			reparentize = 1;

		if (s->drop_views->contents) {
			s->drop_views = gfxw_new_list(s->dyn_views->bounds, 0);
			s->drop_views->flags |= GFXW_FLAG_IMMUNE_TO_SNAPSHOTS;
			ADD_TO_CURRENT_PICTURE_PORT(s->drop_views);
		} else {
			assert(s->drop_views);
			gfxw_remove_widget_from_container(s->drop_views->parent, GFXW(s->drop_views));
			ADD_TO_CURRENT_PICTURE_PORT(s->drop_views);
		}

		if ((reparentize | retval)
		    && (GFXWC(s->port) == GFXWC(s->dyn_views->parent)) /* If dynviews are on the same port... */
		    && (s->dyn_views->next)) /* ... and not on top of the view list...  */
			reparentize_primary_widget_lists(s, s->port); /* ...then reparentize. */

		_k_view_list_kryptonize(s->dyn_views->contents);
	}

	FULL_REDRAW();

}

void
kShakeScreen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int shakes = PARAM_OR_ALT(0, 1);
	gfx_pixmap_t *screen = gfxop_grab_pixmap(s->gfx_state, gfx_rect(0, 0, 320, 200));
	int i;

	gfxop_set_clip_zone(s->gfx_state, gfx_rect_fullscreen);

	for (i = 0; i < shakes; i++) {
		gfxop_draw_box(s->gfx_state, gfx_rect(0, 190, 320, 10), s->ega_colors[0], s->ega_colors[0], GFX_BOX_SHADE_FLAT);
		gfxop_draw_pixmap(s->gfx_state, screen, gfx_rect(0, 10, 320, 190), gfx_point(0, 0));
		gfxop_update(s->gfx_state);
		gfxop_usleep(s->gfx_state, 50000);

		gfxop_draw_box(s->gfx_state, gfx_rect(0, 0, 320, 10), s->ega_colors[0], s->ega_colors[0], GFX_BOX_SHADE_FLAT);
		gfxop_draw_pixmap(s->gfx_state, screen, gfx_rect(0, 0, 320, 190), gfx_point(0, 10));
		gfxop_update(s->gfx_state);
		gfxop_usleep(s->gfx_state, 50000);
	}

	gfxop_draw_pixmap(s->gfx_state, screen, gfx_rect(0, 0, 320, 200), gfx_point(0, 0));
	gfxop_free_pixmap(s->gfx_state, screen);
	gfxop_update(s->gfx_state);
}

#define K_DISPLAY_SET_COORDS 100
#define K_DISPLAY_SET_ALIGNMENT 101
#define K_DISPLAY_SET_COLOR 102
#define K_DISPLAY_SET_BGCOLOR 103
#define K_DISPLAY_SET_GRAYTEXT 104
#define K_DISPLAY_SET_FONT 105
#define K_DISPLAY_WIDTH 106
#define K_DISPLAY_SAVE_UNDER 107
#define K_DISPLAY_RESTORE_UNDER 108
#define K_DONT_UPDATE_IMMEDIATELY 121


void
kDisplay(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int argpt;
	int textp = UPARAM(0);
	int index = UPARAM(1);
	int temp;
	int save_under = 0;
	gfx_color_t transparent;
	char *text;
	gfxw_port_t *port = (s->port)? s->port : s->picture_port;
	int update_immediately = 1;

	gfx_color_t *color0, *color1, *bg_color;
	gfx_alignment_t halign = ALIGN_LEFT;
	rect_t area = gfx_rect(port->draw_pos.x, port->draw_pos.y, 320, 200);
	int gray = port->gray_text;
	int font_nr = port->font_nr;
	gfxw_text_t *text_handle;

	transparent.mask = 0;

	color0 = &(port->color);
	bg_color = &(port->bgcolor);

	CHECK_THIS_KERNEL_FUNCTION;

	text = kernel_lookup_text(s, textp, index);

	if (textp < 1000)
		argpt = 2;
	else
		argpt = 1;

	while (argpt < argc) {

		switch(PARAM(argpt++)) {

		case K_DISPLAY_SET_COORDS:

			area.x = PARAM(argpt++);
			area.y = PARAM(argpt++);
			SCIkdebug(SCIkGRAPHICS, "Display: set_coords(%d, %d)\n", area.x, area.y);
			break;

		case K_DISPLAY_SET_ALIGNMENT:

			halign = PARAM(argpt++);
			SCIkdebug(SCIkGRAPHICS, "Display: set_align(%d)\n", halign);
			break;

		case K_DISPLAY_SET_COLOR:

			temp = PARAM(argpt++);
			SCIkdebug(SCIkGRAPHICS, "Display: set_color(%d)\n", temp);
			if (temp >= 0 && temp <= 15)
				color0 = &(s->ega_colors[temp]);
			else if (temp == -1)
				color0 = &transparent;
			else
				SCIkwarn(SCIkWARNING, "Display: Attempt to set invalid fg color %d\n", temp);
			break;

		case K_DISPLAY_SET_BGCOLOR:

			temp = PARAM(argpt++);
			SCIkdebug(SCIkGRAPHICS, "Display: set_bg_color(%d)\n", temp);
			if (temp >= 0 && temp <= 15)
				bg_color = &(s->ega_colors[temp]);
			else if (temp == -1)
				bg_color = &transparent;
			else
				SCIkwarn(SCIkWARNING, "Display: Attempt to set invalid bg color %d\n", temp);
			break;

		case K_DISPLAY_SET_GRAYTEXT:

			gray = PARAM(argpt++);
			SCIkdebug(SCIkGRAPHICS, "Display: set_graytext(%d)\n", gray);
			break;

		case K_DISPLAY_SET_FONT:

			font_nr = PARAM(argpt++);

			SCIkdebug(SCIkGRAPHICS, "Display: set_font(\"font.%03d\")\n", font_nr);
			break;

		case K_DISPLAY_WIDTH:

			area.xl = PARAM(argpt);
			argpt++;
			SCIkdebug(SCIkGRAPHICS, "Display: set_width(%d)\n", area.xl);
			break;

		case K_DISPLAY_SAVE_UNDER:

			save_under = 1;
			SCIkdebug(SCIkGRAPHICS, "Display: set_save_under()\n");
			break;

		case K_DISPLAY_RESTORE_UNDER:

			SCIkdebug(SCIkGRAPHICS, "Display: restore_under(%04x)\n", UPARAM(argpt));
			graph_restore_box(s, UPARAM(argpt));
			update_immediately = 1;
			argpt++;
			return;

		case K_DONT_UPDATE_IMMEDIATELY:
    
			update_immediately=0;
			SCIkdebug(SCIkGRAPHICS, "Display: set_dont_update()\n");
			argpt++;
			break;
      
		default:
			SCIkdebug(SCIkGRAPHICS, "Unknown Display() command %x\n", PARAM(argpt-1));
			return;
		}
	}

	if (gray)
		color1 = bg_color;
	else
		color1 = color0;

	assert_primary_widget_lists(s);

	text_handle = gfxw_new_text(s->gfx_state, area, font_nr, text, halign,
				    ALIGN_TOP, *color0, *color1, *bg_color, 0);

	if (!text_handle) {
		SCIkwarn(SCIkERROR, "Display: Failed to create text widget!\n");
		return;
	}

	if (save_under) {    /* Backup */
		rect_t save_area = text_handle->bounds;
		save_area.x += port->bounds.x;
		save_area.y += port->bounds.y;

		s->acc = graph_save_box(s, save_area);
		text_handle->serial++; /* This is evil! */

		SCIkdebug(SCIkGRAPHICS, "Saving (%d, %d) size (%d, %d) as %04x\n",
			  save_area.x, save_area.y, save_area.xl, save_area.yl, s->acc);

	}


	SCIkdebug(SCIkGRAPHICS, "Display: Commiting text '%s'\n", text);

	ADD_TO_CURRENT_FG_WIDGETS(text_handle);

	if ((!s->pic_not_valid)&&update_immediately) { /* Refresh if drawn to valid picture */
		FULL_REDRAW();
		SCIkdebug(SCIkGRAPHICS, "Refreshing display...\n");
	}
}
