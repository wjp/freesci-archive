/***************************************************************************
 kernel.c Copyright (C) 1999 Christoph Reichenbach


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
       s->visual->add(GFXWC(s->visual), GFXW(widget));

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
kGraph(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int color, priority, special;
  gfx_color_t gfxcolor;
  rect_t area;
  gfxw_port_t *port = s->port;
  int redraw_port = 0;

  switch(PARAM(0)) {

  case K_GRAPH_GET_COLORS_NR:

    s->acc = (sci_version < SCI_VERSION_1) ? 0x10 : 0x100; /* number of colors */
    break;
  
  case K_GRAPH_DRAW_LINE:

    color = PARAM(5);
    priority = PARAM(6);
    special = PARAM(7);
    area = gfx_rect(PARAM(2), PARAM(1) , PARAM(4), PARAM(3));
  
    gfxcolor = s->ega_colors[color];
    gfxcolor.priority = priority;
    gfxcolor.control = special;

    gfxcolor.mask = !(color & 0x80) | (!(priority & 0x80) << 1) | (!(special & 0x80) << 2);

    redraw_port = 1;
    port->add(GFXWC(port),
	      (gfxw_widget_t *) gfxw_new_line(area, gfxcolor, GFX_LINE_MODE_FAST, GFX_LINE_STYLE_NORMAL));
		
    break;

  case K_GRAPH_SAVE_BOX:

WARNING(fixme)
#if 0
    s->acc = graph_save_box(s, PARAM(2), PARAM(1), PARAM(4), PARAM(3), PARAM(5));
#endif
    break;

  case K_GRAPH_RESTORE_BOX:

WARNING(fixme)
#if 0
    graph_restore_box(s, PARAM(1));
#endif
    break;

  case K_GRAPH_FILL_BOX_BACKGROUND:

WARNING(fixme)
#if 0
    graph_clear_box(s, port->xmin, port->ymin,
		    port->xmax - port->xmin + 1, port->ymax - port->ymin + 1,
		    port->bgcolor);
    redraw_port = 1;
#endif
    CHECK_THIS_KERNEL_FUNCTION;
    break;

  case K_GRAPH_FILL_BOX_FOREGROUND:

WARNING(fixme)
#if 0
    graph_clear_box(s, port->xmin, port->ymin,
		    port->xmax - port->xmin + 1, port->ymax - port->ymin + 1,
		    port->color);
    CHECK_THIS_KERNEL_FUNCTION;
#endif
    break;

  case K_GRAPH_FILL_BOX_ANY: {

    int x = PARAM(2);
    int y = PARAM(1);

WARNING(fixme)
#if 0
    SCIkdebug(SCIkGRAPHICS, "fill_box_any(%d, %d, %d, %d) map=%d (%d %d)\n",
	      PARAM(1), PARAM(2), PARAM(3), PARAM(4), PARAM(5), PARAM(6), PARAM_OR_ALT(7, -1));

    graph_fill_box_custom(s, x + s->port->xmin, y + s->port->ymin,
			  PARAM(4)-x, PARAM(3)-y, PARAM(6), PARAM_OR_ALT(7, -1),
			  PARAM_OR_ALT(8, -1), UPARAM(5));
    CHECK_THIS_KERNEL_FUNCTION;
#endif

  }
  break;

  case K_GRAPH_UPDATE_BOX: {

    int x = PARAM(2);
    int y = PARAM(1);

WARNING(fixme)
#if 0
    SCIkdebug(SCIkGRAPHICS, "update_box(%d, %d, %d, %d)\n",
	      PARAM(1), PARAM(2), PARAM(3), PARAM(4));

    (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_BOX, x, y + 10, PARAM(4)-x+1, PARAM(3)-y+1);
    CHECK_THIS_KERNEL_FUNCTION;
#endif

  }
  break;

  case K_GRAPH_REDRAW_BOX: {

    CHECK_THIS_KERNEL_FUNCTION;

WARNING(fixme)
#if 0
    SCIkdebug(SCIkGRAPHICS, "redraw_box(%d, %d, %d, %d)\n",
	      PARAM(1), PARAM(2), PARAM(3), PARAM(4));

    if (!s->dyn_views_nr)
      graph_update_box(s, PARAM(2), PARAM(1), PARAM(4), PARAM(3)); else
      _k_redraw_box(s, PARAM(2), PARAM(1), PARAM(4), PARAM(3));
#endif
      
  }

  break;

  case K_GRAPH_ADJUST_PRIORITY:

    fprintf(stderr,"ADJP(%d,%d)\n", PARAM(1), PARAM(2));
    SCIkdebug(SCIkGRAPHICS, "adjust_priority(%d, %d)\n", PARAM(1), PARAM(2));
    s->priority_first = PARAM(1) - 10;
    s->priority_last = PARAM(2) - 10;
    break;

  default:
    
    CHECK_THIS_KERNEL_FUNCTION;
    SCIkdebug(SCIkSTUB, "Unhandled Graph() operation %04x\n", PARAM(0));
    
  }

  port->draw(GFXW(port), gfxw_point_zero);
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

	g_get_current_time (&time);

	s-> acc = ((time.tv_usec - s->last_wait_time.tv_usec) * 60 / 1000000) +
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
 
	if (t2) x=sqrt((gy*dx*dx)/(2*t2));
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
		word signal = GET_SELECTOR(client, signal);

		PUT_SELECTOR(client, x, oldx);
		PUT_SELECTOR(client, y, oldy);
		if (s->selector_map.completed > -1)
			PUT_SELECTOR(mover, completed, 1);
 
		PUT_SELECTOR(mover, x, oldx);
		PUT_SELECTOR(mover, y, oldy);    

		PUT_SELECTOR(client, signal, (signal | _K_VIEW_SIG_FLAG_HIT_OBSTACLE));

		SCIkdebug(SCIkBRESEN, "Finished mover %04x by collision\n", mover);
		s->acc = 1;
	}

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
	rect_t zone = gfx_rect(x + port->zone.x, y + port->zone.y, xl - 1, yl - 1);
	word edgehit;

	signal = GET_SELECTOR(obj, signal);
	SCIkdebug(SCIkBRESEN,"Checking collision: (%d,%d) to (%d,%d), obj=%04x, sig=%04x, cliplist=%04x\n",
		  x, y, xend, yend, obj, signal, cliplist);

	s->acc = !(((word)GET_SELECTOR(obj, illegalBits))
		   & (edgehit = gfxop_scan_bitmask(s->gfx_state, zone, GFX_MASK_CONTROL)));

	SCIkdebug(SCIkBRESEN, "edgehit = %04x\n", edgehit);
	if (s->acc == 0)
		return; /* Can'tBeHere */
	if (signal & (_K_VIEW_SIG_FLAG_DONT_RESTORE | signal & _K_VIEW_SIG_FLAG_IGNORE_ACTOR))
		s->acc= signal & (_K_VIEW_SIG_FLAG_DONT_RESTORE|_K_VIEW_SIG_FLAG_IGNORE_ACTOR); /* CanBeHere- it's either being disposed, or it ignores actors anyway */

	if (cliplist) {
		heap_ptr node = GET_HEAP(cliplist + LIST_FIRST_NODE);

		s->acc = 0; /* Assume that we Can'tBeHere... */

		while (node) { /* Check each object in the list against our bounding rectangle */
			heap_ptr other_obj = UGET_HEAP(node + LIST_NODE_VALUE);
			if (other_obj != obj) { /* Clipping against yourself is not recommended */

				int other_signal = GET_SELECTOR(other_obj, signal);
				SCIkdebug(SCIkBRESEN, "OtherSignal=%04x, z=%04x\n", other_signal,
					  (other_signal & (_K_VIEW_SIG_FLAG_DONT_RESTORE | _K_VIEW_SIG_FLAG_IGNORE_ACTOR)));
				if ((other_signal & (_K_VIEW_SIG_FLAG_DONT_RESTORE | _K_VIEW_SIG_FLAG_IGNORE_ACTOR | _K_VIEW_SIG_FLAG_NO_UPDATE)) == 0) {
					/* check whether the other object ignores actors */

					int other_x = GET_SELECTOR(other_obj, brLeft);
					int other_y = GET_SELECTOR(other_obj, brTop);
					int other_xend = GET_SELECTOR(other_obj, brRight);
					int other_yend = GET_SELECTOR(other_obj, brBottom);
					SCIkdebug(SCIkBRESEN, "  against (%d,%d) to (%d, %d)\n",
						  other_x, other_y, other_xend, other_yend);


					if (((other_xend > x) && (other_x <= xend)) /* [other_x, other_xend] intersects [x, xend]? */
					    &&
					    ((other_yend > y) && (other_y <= yend))) /* [other_y, other_yend] intersects [y, yend]? */
						return;

				}

				SCIkdebug(SCIkBRESEN, " (no)\n");

			} /* if (other_obj != obj) */
			node = GET_HEAP(node + LIST_NEXT_NODE); /* Move on */
		}
	}

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
	int result;
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
	int result;
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
}

void
_k_view_list_free_backgrounds(state_t *s, view_object_t *list, int list_nr);
void
_k_view_list_dispose(state_t *s, gfxw_list_t **list_ptr);

void
kDrawPic(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int pic_nr = PARAM(0);
	int add_to_pic = 1;
	CHECK_THIS_KERNEL_FUNCTION;


	if (s->version < SCI_VERSION_FTU_NEWER_DRAWPIC_PARAMETERS) {
		if (!PARAM_OR_ALT(2, 0))
			add_to_pic = 0;
	} else
		if (PARAM_OR_ALT(2, 1))
			add_to_pic = 0;


	gfxop_disable_dirty_frames(s->gfx_state);

	s->old_screen = gfxop_grab_pixmap(s->gfx_state, gfx_rect(0, 10, 320, 190));

	if (add_to_pic) {
		if (s->picture_port->contents) {
			s->picture_port->free_contents(GFXWC(s->picture_port));
			s->picture_port->contents = NULL;
			s->dyn_views = s->pic_views = s->bg_widgets = NULL;
		}
		GFX_ASSERT(gfxop_add_to_pic(s->gfx_state, pic_nr, 1, PARAM_OR_ALT(3, 0)));
	} else {
		GFX_ASSERT(gfxop_new_pic(s->gfx_state, pic_nr, 1, PARAM_OR_ALT(3, 0)));
	}

	SCIkdebug(SCIkGRAPHICS,"Drawing pic.%03d\n", PARAM(0));

	if (argc > 1)
		s->pic_animate = PARAM(1); /* The animation used during kAnimate() later on */

	if (s->dyn_views)
		s->dyn_views->free(GFXW(s->dyn_views));

	if (s->pic_views)
		s->pic_views->free(GFXW(s->pic_views));

	if (s->bg_widgets)
		s->bg_widgets->free(GFXW(s->bg_widgets));

	s->dyn_views = s->pic_views = s->bg_widgets = NULL;

	s->priority_first = 42;
	s->priority_last = 180;
	s->pic_not_valid = 1;
	s->pic_is_new = 1;

}



void
_k_base_setter(state_t *s, heap_ptr object)
{
	int x, y, original_y, z, ystep, xsize, ysize;
	int xbase, ybase, xend, yend;
	int view, loop, cel;
	int xmod = 0, ymod = 0;

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
  
	PUT_SELECTOR(object, brLeft, xbase);
	PUT_SELECTOR(object, brRight, xend);
	PUT_SELECTOR(object, brTop, ybase);
	PUT_SELECTOR(object, brBottom, yend);

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

	_k_dyn_view_list_prepare_change(s);
	_k_draw_control(s, obj, 0);
	_k_dyn_view_list_accept_change(s);
}


void
kHiliteControl(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = UPARAM(0);

	CHECK_THIS_KERNEL_FUNCTION;

	_k_dyn_view_list_prepare_change(s);
	_k_draw_control(s, obj, 1);
	_k_dyn_view_list_accept_change(s);
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
				int x = GET_SELECTOR(obj, nsLeft);
				int y = GET_SELECTOR(obj, nsTop);
				int xl = GET_SELECTOR(obj, nsRight) - x + 1;
				int yl = GET_SELECTOR(obj, nsBottom) - y + 1;
				int max = GET_SELECTOR(obj, max);
				int cursor = GET_SELECTOR(obj, cursor);
				int modifiers = GET_SELECTOR(event, modifiers);
				byte key = GET_SELECTOR(event, message);

				char *text = s->heap + UGET_SELECTOR(obj, text);
				int textlen = strlen(text);

				gfxw_port_t *port = s->port;

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
	int xl = GET_SELECTOR(obj, nsRight) - x + 1;
	int yl = GET_SELECTOR(obj, nsBottom) - y + 1;
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
		ADD_TO_CURRENT_PORT(sciw_new_button_control(s->port, obj, area, text, font_nr,
                                                            state & CONTROL_STATE_FRAMED,
                                                            inverse, state & CONTROL_STATE_GRAY));
		break;

	case K_CONTROL_TEXT:

		SCIkdebug(SCIkGRAPHICS, "drawing text %04x to %d,%d\n", obj, x, y);
		ADD_TO_CURRENT_PORT(sciw_new_text_control(s->port, obj, area, text, font_nr,
							  (s->version < SCI_VERSION_FTU_CENTERED_TEXT_AS_DEFAULT)
							  ? ALIGN_LEFT : ALIGN_CENTER, state & CONTROL_STATE_DITHER_FRAMED,
							  inverse));
		break;

	case K_CONTROL_EDIT:

		SCIkdebug(SCIkGRAPHICS, "drawing edit control %04x to %d,%d\n", obj, x, y);

		cursor = GET_SELECTOR(obj, cursor);
		ADD_TO_CURRENT_PORT(sciw_new_edit_control(s->port, obj, area, text, font_nr, cursor, inverse));
		break;

	case K_CONTROL_ICON:

		SCIkdebug(SCIkGRAPHICS, "drawing icon control %04x to %d,%d\n", obj, x, y -1);


		ADD_TO_CURRENT_PORT(sciw_new_icon_control(s->port, obj, area, view, loop, cel,
							  state & CONTROL_STATE_FRAMED, inverse));
		break;

	case K_CONTROL_CONTROL: {
		char **entries_list = NULL;
		char *seeker;
		int entries_nr;
		int lsTop = GET_SELECTOR(obj, lsTop);
		int list_top = 0;
		int selection = 0;
		int i;

		SCIkdebug(SCIkGRAPHICS, "drawing list control %04x to %d,%d\n", obj, x, y);
		cursor = GET_SELECTOR(obj, cursor);

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

		ADD_TO_CURRENT_PORT(sciw_new_list_control(s->port, obj, area, font_nr, entries_list, entries_nr,
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

void
_k_dyn_view_list_prepare_change(state_t *s)
     /* Removes all views in anticipation of a new window or text */
{
WARNING(fixme)
#if 0
  view_object_t *list = s->dyn_views;
  int list_nr = s->dyn_views_nr;
  int i;

  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
    word signal = GET_HEAP(list[i].signalp);

    if (!(signal & _K_VIEW_SIG_FLAG_NO_UPDATE)) {

      if (list[i].underBitsp && !(signal & _K_VIEW_SIG_FLAG_DONT_RESTORE)) {
	word under_bits = UGET_HEAP(list[i].underBitsp);
	if (under_bits == 0xffff) {
	  under_bits = list[i].underBits;
	  SCIkwarn(SCIkWARNING, "underBits for obj %04x set to 0xffff, using older %04x instead\n", list[i].obj, under_bits);
	} else
	  list[i].underBits = under_bits;

	if (under_bits) {

	  SCIkdebug(SCIkGRAPHICS, "Restoring BG for obj %04x with signal %04x\n", list[i].obj, signal);
	  graph_restore_box(s, under_bits);

	  PUT_HEAP(list[i].underBitsp, list[i].underBits = 0); /* Restore and mark as restored */
	}
      }
    } /* if NOT (signal & _K_VIEW_SIG_FLAG_NO_UPDATE) */
  } /* For each list member */
#endif
}

void
_k_restore_view_list_backgrounds(state_t *s, view_object_t *list, int list_nr)
     /* Restores the view backgrounds of list_nr members of list */
{
WARNING(fixme)
#if 0
  int i;

  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
    word signal = GET_HEAP(list[i].signalp);
    SCIkdebug(SCIkGRAPHICS, "Trying to restore with signal = %04x\n", signal);

    if (signal & _K_VIEW_SIG_FLAG_NO_UPDATE) {

      if (signal & _K_VIEW_SIG_FLAG_UPDATE_ENDED)
	PUT_HEAP(list[i].signalp, (signal & ~_K_VIEW_SIG_FLAG_UPDATE_ENDED) |
		 _K_VIEW_SIG_FLAG_NO_UPDATE);

      continue; /* Don't restore this view */
    } else {

      if (list[i].underBitsp && !(signal & _K_VIEW_SIG_FLAG_DONT_RESTORE)) {
	word under_bits = UGET_HEAP(list[i].underBitsp);
	if (under_bits == 0xffff) {
	  under_bits = list[i].underBits;
	  SCIkwarn(SCIkWARNING, "underBits for obj %04x set to 0xffff, using older %04x instead\n", list[i].obj, under_bits);
	} else
	  list[i].underBits = under_bits;

	if (under_bits) {

	  SCIkdebug(SCIkGRAPHICS, "Restoring BG for obj %04x with signal %04x\n", list[i].obj, signal);
	  graph_restore_box(s, under_bits);

	  PUT_HEAP(list[i].underBitsp, list[i].underBits = 0); /* Restore and mark as restored */
	}
      }
      signal &= ~_K_VIEW_SIG_FLAG_UNKNOWN_6;

      if (signal & _K_VIEW_SIG_FLAG_UPDATING)
	signal &= ~(_K_VIEW_SIG_FLAG_UPDATING |
		    _K_VIEW_SIG_FLAG_NO_UPDATE); /* Clear those two flags */

    } /* if NOT (signal & _K_VIEW_SIG_FLAG_NO_UPDATE) */
  } /* For each list member */
#endif
}

void
_k_view_list_free_backgrounds(state_t *s, view_object_t *list, int list_nr)
     /* Frees all backgrounds from the list without restoring; called by DrawPic */
{
  int i;

WARNING(fixme)
#if 0
  SCIkdebug(SCIkMEM, "Freeing bglist: %d entries\n", list_nr);
  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
    int handle = list[i].underBits;

    if (handle)
      kfree(s, handle);

    PUT_HEAP(list[i].underBitsp, list[i].underBits = 0);
  }
#endif
}

void
_k_save_view_list_backgrounds(state_t *s, view_object_t *list, int list_nr)
     /* Stores the view backgrounds of list_nr members of list in their underBits selectors */
{
WARNING(fixme)
#if 0
  int i;

  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
    int handle;

    if (!(list[i].underBitsp))
      continue; /* No underbits? */

    if (GET_HEAP(list[i].underBitsp))
      continue; /* Don't overwrite an existing backup */

    SCIkdebug(SCIkGRAPHICS, "Saving BG for obj %04x with signal %04x\n",
	      list[i].obj, UGET_HEAP(list[i].signalp));

    handle = view0_backup_background(s, list[i].x, list[i].y,
				     list[i].loop, list[i].cel, list[i].view);

    PUT_HEAP(list[i].underBitsp, list[i].underBits = handle);
  }
#endif
}

void
_k_view_list_dispose_loop(state_t *s, heap_ptr list_addr, gfxw_list_t *list,
			  int funct_nr, int argc, int argp)
     /* disposes all list members flagged for disposal; funct_nr is the invoking kfunction */
{
	int i;
	gfxw_dyn_view_t *widget = (gfxw_dyn_view_t *) list->contents;

	while (widget) {
		if (GFXW_IS_DYN_VIEW(widget) && (widget->ID != GFXW_NO_ID)) {
			if (UGET_HEAP(widget->signalp) & _K_VIEW_SIG_FLAG_DISPOSE_ME) {

				if (widget->under_bitsp) { /* Is there a bg picture left to clean? */
					word mem_handle = widget->under_bits = GET_HEAP(widget->under_bitsp);

					if (mem_handle) {
						kfree(s, mem_handle);
						PUT_HEAP(widget->under_bitsp, widget->under_bits = 0);
					}
				}


				if (invoke_selector(INV_SEL(widget->ID, delete, 1), 0))
					SCIkwarn(SCIkWARNING, "Object at %04x requested deletion, but does not have"
						 " a delete funcselector\n", widget->ID);
			}
		}

		widget = (gfxw_dyn_view_t *) widget->next;
	}
}


void
_k_invoke_view_list(state_t *s, heap_ptr list, int funct_nr, int argc, int argp)
     /* Invokes all elements of a view list. funct_nr is the number of the calling funcion. */
{
	heap_ptr node = GET_HEAP(list + LIST_LAST_NODE);

	while (node) {
		heap_ptr obj = UGET_HEAP(node + LIST_NODE_VALUE); /* The object we're using */
		word signal = GET_SELECTOR(obj, signal);

		if (lookup_selector(s, obj, s->selector_map.baseSetter, NULL) == SELECTOR_METHOD)
			invoke_selector(INV_SEL(obj, baseSetter, 1), 0); /* SCI-implemented base setter */
		else
			_k_base_setter(s, obj);

    
		PUT_SELECTOR(obj, signal, signal | _K_VIEW_SIG_FLAG_UPDATING);

		if (!(signal & _K_VIEW_SIG_FLAG_FROZEN)) {
			word ubitsnew, ubitsold = GET_SELECTOR(obj, underBits);
			invoke_selector(INV_SEL(obj, doit, 1), 0); /* Call obj::doit() if neccessary */
			ubitsnew = GET_SELECTOR(obj, underBits);
		}

		node = UGET_HEAP(node + LIST_PREVIOUS_NODE);
	}
}


static int _cmp_view_object(const void *obj1, const void *obj2) /* Used for qsort() later on */
{
  int retval = (((view_object_t *)obj1)->real_y) - (((view_object_t *)obj2)->real_y);
  if (retval == 0)
    retval = (((view_object_t *)obj1)->z) - (((view_object_t *)obj2)->z);
  if (retval == 0)
    return (((view_object_t *)obj2)->index_nr) - (((view_object_t *)obj1)->index_nr);
  return retval;
}

#define _K_MAKE_VIEW_LIST_CYCLE 1
#define _K_MAKE_VIEW_LIST_CALC_PRIORITY 2
#define _K_MAKE_VIEW_LIST_DRAW_TO_CONTROL_MAP 4

void
_k_make_view_list(state_t *s, gfxw_list_t *widget_list, heap_ptr list, int options, int funct_nr, int argc, int argp)
     /* Creates a view_list from a node list in heap space. Returns the list, stores the
     ** number of list entries in *list_nr. Calls doit for each entry if cycle is set.
     ** argc, argp, funct_nr should be the same as in the calling kernel function.
     */
{
	heap_ptr node;
	int i;

	if (!widget_list) {
		SCIkwarn(SCIkERROR, "make_view_list with widget_list == ()\n");
		BREAKPOINT();
	};

	widget_list->tag(GFXW(widget_list));

	if (options & _K_MAKE_VIEW_LIST_CYCLE)
		_k_invoke_view_list(s, list, funct_nr, argc, argp); /* Invoke all objects if requested */

	node = GET_HEAP(list + LIST_LAST_NODE);
  
	SCIkdebug(SCIkGRAPHICS, "Making list from %04x\n", list);

	if (GET_HEAP(list - 2) != 0x6) { /* heap size check */
		SCIkwarn(SCIkWARNING, "Attempt to draw non-list at %04x\n", list);
		return;
	}

	node = UGET_HEAP(list + LIST_LAST_NODE);
	while (node) {
		short oldloop, oldcel;
		heap_ptr obj = GET_HEAP(node + LIST_NODE_VALUE); /* The object we're using */
		int cel, loop, view_nr = GET_SELECTOR(obj, view);
		int has_nsrect = lookup_selector(s, obj, s->selector_map.nsBottom, NULL) == SELECTOR_VARIABLE;
		int under_bits, signal;
		heap_ptr under_bitsp, signalp;
		point_t pos;
		int z, priority, _priority;
		gfxw_dyn_view_t *widget;

		SCIkdebug(SCIkGRAPHICS, " - Adding %04x\n", obj);

		obj = obj;

		pos.x = GET_SELECTOR(obj, x);
		pos.y = GET_SELECTOR(obj, y);
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
		}

		_priority = VIEW_PRIORITY(pos.y);

		if (has_nsrect
		    && !(UGET_HEAP(signalp) & _K_VIEW_SIG_FLAG_FIX_PRI_ON)) { /* Calculate priority */

			if (options & _K_MAKE_VIEW_LIST_CALC_PRIORITY)
				PUT_SELECTOR(obj, priority, _priority);

			priority = _priority;
		} else /* DON'T calculate the priority */
			priority = GET_SELECTOR(obj, priority);

		if (options & _K_MAKE_VIEW_LIST_DRAW_TO_CONTROL_MAP) {
			int pri_top = PRIORITY_BAND_FIRST(_priority);
			int top, bottom, right, left;

			if (priority <= 0)
				priority = _priority; /* Picviews always get their priority set */


			if (has_nsrect) {
				top = GET_SELECTOR(obj, nsTop);
				bottom = GET_SELECTOR(obj, nsBottom);
				left = GET_SELECTOR(obj, nsLeft);
				right = GET_SELECTOR(obj, nsRight);
			} else {
				int width, height;
				point_t offset;

				GFX_ASSERT(gfxop_get_cel_parameters(s->gfx_state, view_nr, loop, cel,
								    &width, &height, &offset));

				left = pos.x + offset.x - (width >> 1);
				right = left + width;
				bottom = pos.y + offset.y + 1;
				top = bottom - height;
			}


			if (top < pri_top)
				top = pri_top;

			if (bottom < top) {
				int foo = top;
				top = bottom;
				bottom = foo;
			}

			if (!(signalp && (GET_HEAP(signalp) & _K_VIEW_SIG_FLAG_IGNORE_ACTOR))) {
				gfxw_box_t *box;
				gfx_color_t color;

				gfxop_set_color(s->gfx_state, &color, -1, -1, -1, -1, -1, 0xf);

				SCIkdebug(SCIkGRAPHICS,"    adding control block (%d,%d)to(%d,%d)\n", left, top,
					  right, bottom);

				box = gfxw_new_box(s->gfx_state, gfx_rect(left, top, right - left + 1, bottom - top + 1),
						   color, color, GFX_BOX_SHADE_FLAT);

				s->picture_port->add(GFXWC(s->picture_port), GFXW(box));
			}
		}

		widget = gfxw_new_dyn_view(s->gfx_state, pos, z, view_nr, loop, cel,
					   priority, -1, ALIGN_CENTER, ALIGN_BOTTOM);

		if (widget) {

			widget = (gfxw_dyn_view_t *) gfxw_set_id(GFXW(widget), obj);
			widget = gfxw_dyn_view_set_params(widget, under_bits, under_bitsp, signal, signalp);

			GFX_ASSERT(widget_list->add(GFXWC(widget_list), GFXW(widget)));

			/*    s->pic_not_valid++; *//* There ought to be some kind of check here... */
		} else {
			SCIkwarn(SCIkWARNING, "Could not generate dynview widget for %d/%d/%d\n", view_nr, loop, cel);
		}

		node = UGET_HEAP(node + LIST_PREVIOUS_NODE); /* Next node */
	}

}


void
_k_view_list_dispose(state_t *s, gfxw_list_t **list_ptr)
     /* Unallocates all list element data, frees the list */
{
	gfxw_list_t *list = *list_ptr;

	if (!(*list_ptr))
		return; /* Nothing to do :-( */

fprintf(stderr,"Error: DISPOSING LIST!\n");
BREAKPOINT();

WARNING(fixme)
  /*  _k_view_list_free_backgrounds(s, list, *list_nr_ptr);*/

	list->free(GFXW(list));
	*list_ptr = NULL;
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
	int i;
	int argc = 0, argp = 0, funct_nr = -1; /* Kludges to work around INV_SEL dependancies */

	gfxw_dyn_view_t *widget = (gfxw_dyn_view_t *) list->contents;

	if (s->port != s->dyn_view_port)
		return; /* Return if the pictures are meant for a different port */

	while (widget) {

		if (GFXW_IS_DYN_VIEW(widget) && widget->ID) {
			word signal = (flags & _K_DRAW_VIEW_LIST_USE_SIGNAL)? GET_HEAP(widget->signalp) : 0;

			if (!(flags & _K_DRAW_VIEW_LIST_USE_SIGNAL)
			    || ((flags & _K_DRAW_VIEW_LIST_DISPOSEABLE) && (signal & _K_VIEW_SIG_FLAG_DISPOSE_ME))
			    || ((flags & _K_DRAW_VIEW_LIST_NONDISPOSEABLE) && !(signal & _K_VIEW_SIG_FLAG_DISPOSE_ME))) {

				/* Now, if we either don't use signal OR if signal allows us to draw, do so: */
				if (!(flags & _K_DRAW_VIEW_LIST_USE_SIGNAL) || (!(signal & _K_VIEW_SIG_FLAG_NO_UPDATE) &&
										!(signal & _K_VIEW_SIG_FLAG_HIDDEN))) {

					_k_set_now_seen(s, widget->ID);

					SCIkdebug(SCIkGRAPHICS, "Drawing obj %04x with signal %04x\n", widget->ID, signal);
					SCIkdebug(SCIkGRAPHICS, "  to (%d,%d) v/l/c = (%d,%d,%d), pri=%d\n", widget->pos.x, widget->pos.y,
						  widget->view, widget->loop, widget->cel, widget->color.priority);

				}

				if (flags & _K_DRAW_VIEW_LIST_USE_SIGNAL) {
					signal &= ~(_K_VIEW_SIG_FLAG_UPDATE_ENDED | _K_VIEW_SIG_FLAG_UPDATING |
						    _K_VIEW_SIG_FLAG_NO_UPDATE | _K_VIEW_SIG_FLAG_UNKNOWN_6);
					/* Clear all of those flags */

					PUT_HEAP(widget->signalp, signal); /* Write the changes back */
				} else continue;

			} /* ...if we're drawing disposeables and this one is disposeable, or if we're drawing non-
			  ** disposeables and this one isn't disposeable */
		}

		widget = (gfxw_dyn_view_t *) widget->next;
	} /* while (widget) */
	widget = (gfxw_dyn_view_t *) widget->next;

	FULL_REDRAW();
}

void
_k_dyn_view_list_accept_change(state_t *s)
     /* Restores all views after backupping their new bgs */
{
WARNING(fixme)
#if 0
  view_object_t *list = s->dyn_views;
  int list_nr = s->dyn_views_nr;
  int i;

  int oldvp = s->view_port;

  if (s->view_port != s->dyn_view_port)
    return; /* Return if the pictures are meant for a different port */

  s->view_port = 0; /* WM Viewport */
  _k_save_view_list_backgrounds(s, list, list_nr);
  s->view_port = oldvp;

  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
    word signal = GET_HEAP(list[i].signalp);

    if (!(signal & _K_VIEW_SIG_FLAG_NO_UPDATE) && !(signal & _K_VIEW_SIG_FLAG_HIDDEN)) {
      SCIkdebug(SCIkGRAPHICS, "Drawing obj %04x with signal %04x\n", list[i].obj, signal);

      _k_clip_view(list[i].view, &(list[i].loop), &(list[i].cel));

      draw_view0(s->pic, &(s->wm_port),
		 list[i].x, list[i].y, list[i].priority, list[i].loop, list[i].cel,
		 GRAPHICS_VIEW_CENTER_BASE | GRAPHICS_VIEW_USE_ADJUSTMENT, list[i].view);
    }
  }
#endif
}


void
kAddToPic(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr list = PARAM(0);
  CHECK_THIS_KERNEL_FUNCTION;

WARNING(fixme)
#if 0
  if (s->pic_views_nr) {
    g_free(s->pic_views);
    s->pic_views = NULL;
  }

  if (!list)
    return;

  SCIkdebug(SCIkGRAPHICS, "Preparing list...\n");
  s->pic_views = _k_make_view_list(s, list, &(s->pic_views_nr), _K_MAKE_VIEW_LIST_DRAW_TO_CONTROL_MAP, funct_nr, argc, argp);
  /* Store pic views for later re-use */

  SCIkdebug(SCIkGRAPHICS, "Drawing list...\n");
  _k_draw_view_list(s, s->pic_views, s->pic_views_nr, _K_DRAW_VIEW_LIST_NONDISPOSEABLE | _K_DRAW_VIEW_LIST_DISPOSEABLE);
  /* Draw relative to the bottom center */
  SCIkdebug(SCIkGRAPHICS, "Returning.\n");
#endif
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

	ADD_TO_CURRENT_PORT(new_view);
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

	pred = gfxw_remove_port(s->visual, goner);

	if (goner == s->port) /* Did we kill the active port? */
		s->port = pred;

	FULL_REDRAW();

	gfxop_update(s->gfx_state);
}


void
kNewWindow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	gfxw_port_t *window;
	int x, y, xl, yl, flags;
	gfx_color_t bgcolor;
	char *text;
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

	SCI_MEMTEST;

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

void
kAnimate(state_t *s, int funct_nr, int argc, heap_ptr argp)
     /* Animations are supposed to take a maximum of s->animation_delay milliseconds. */
{
	int i, remaining_checkers;
	char checkers[32 * 19];
	heap_ptr cast_list = UPARAM_OR_ALT(0, 0);
	int cycle = UPARAM_OR_ALT(1, 0);
	int open_animation = 0;
	gfx_pixmap_t *oldscreen = NULL;

	CHECK_THIS_KERNEL_FUNCTION;

	/*  SCI_MEMTEST; */
	process_sound_events(s); /* Take care of incoming events (kAnimate is called semi-regularly) */

	open_animation = (s->pic_is_new) && (s->pic_not_valid);

	if (open_animation)
		gfxop_clear_box(s->gfx_state, gfx_rect(0, 10, 320, 190)); /* Propagate pic */


	if (!s->dyn_views) {
		s->dyn_views = gfxw_new_list(s->picture_port->bounds, GFXW_LIST_SORTED);
		s->picture_port->add(GFXWC(s->picture_port), GFXW(s->dyn_views));
	}

	if (!s->bg_widgets) {
		s->bg_widgets = gfxw_new_list(s->picture_port->bounds, GFXW_LIST);
		s->picture_port->add(GFXWC(s->picture_port), GFXW(s->bg_widgets));
	}

	if (!cast_list)
		s->dyn_views->tag(GFXW(s->dyn_views));

	if (cast_list) {
		s->dyn_view_port = s->port; /* The list is valid for view_port */

		_k_make_view_list(s, s->dyn_views, cast_list, (cycle? _K_MAKE_VIEW_LIST_CYCLE : 0)
				  | _K_MAKE_VIEW_LIST_CALC_PRIORITY, funct_nr, argc, argp);
		/* Initialize pictures- Steps 3-9 in Lars' V 0.1 list */

		SCIkdebug(SCIkGRAPHICS, "Handling PicViews:\n");
		/*		if (s->pic_not_valid)
				_k_draw_view_list(s, s->pic_views, 0);*/ /* Step 10 */
		SCIkdebug(SCIkGRAPHICS, "Handling Dyn:\n");

		/*		_k_restore_view_list_backgrounds(s, s->dyn_views, s->dyn_views_nr); */
		_k_draw_view_list(s, s->dyn_views, _K_DRAW_VIEW_LIST_DISPOSEABLE | _K_DRAW_VIEW_LIST_USE_SIGNAL);
		/*		_k_save_view_list_backgrounds(s, s->dyn_views, s->dyn_views_nr); */
		_k_draw_view_list(s, s->dyn_views, _K_DRAW_VIEW_LIST_NONDISPOSEABLE | _K_DRAW_VIEW_LIST_USE_SIGNAL); /* Step 11 */
		/* Step 15 */
		_k_view_list_dispose_loop(s, cast_list, s->dyn_views, funct_nr, argc, argp);
	} /* if (cast_list) */


	if (open_animation) {
		gfx_pixmap_t *newscreen = gfxop_grab_pixmap(s->gfx_state, gfx_rect(0, 10, 320, 190));

		if (!newscreen) {
			SCIkwarn(SCIkERROR, "Failed to allocate 'newscreen'!\n");
			return;
		}

		GFX_ASSERT(gfxop_draw_pixmap(s->gfx_state, s->old_screen, gfx_rect(0, 0, 320, 190), gfx_point(0, 10)));
		gfxop_update_box(s->gfx_state, gfx_rect(0, 0, 320, 200));

		SCIkdebug(SCIkGRAPHICS, "Animating pic opening type %x\n", s->pic_animate);

		gfxop_enable_dirty_frames(s->gfx_state);

		switch(s->pic_animate) {
		case K_ANIMATE_BORDER_CLOSE_H_CENTER_OPEN_H :

			for (i = 0; i < 160; i++) {
				GRAPH_BLANK_BOX(s, i, 10, 1, 190, 0);
				GRAPH_BLANK_BOX(s, 319-i, 10, 1, 190, 0);
				gfxop_update(s->gfx_state);
				gfxop_usleep(s->gfx_state, s->animation_delay);
				process_sound_events(s);
			}

		case K_ANIMATE_CENTER_OPEN_H :

			for (i = 159; i >= 0; i--) {
				GRAPH_UPDATE_BOX(s, i, 10, 1, 190);
				GRAPH_UPDATE_BOX(s, 319-i, 10, 1, 190);
				gfxop_update(s->gfx_state);
				gfxop_usleep(s->gfx_state, s->animation_delay);
				process_sound_events(s);
			}
			break;


		case K_ANIMATE_BORDER_CLOSE_V_CENTER_OPEN_V :

			for (i = 0; i < 95; i++) {
				GRAPH_BLANK_BOX(s, 0, i + 10, 320, 1, 0);
				GRAPH_BLANK_BOX(s, 0, 199 - i, 320, 1, 0);
				gfxop_update(s->gfx_state);
				gfxop_usleep(s->gfx_state, 2 * s->animation_delay);
				process_sound_events(s);
			}

		case K_ANIMATE_CENTER_OPEN_V :

			for (i = 94; i >= 0; i--) {
				GRAPH_UPDATE_BOX(s, 0, i + 10, 320, 1);
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
				GRAPH_BLANK_BOX(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height, 0);
			  
				GRAPH_BLANK_BOX(s, width, 10 + height, 320 - 2*width, 3, 0);
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
				GRAPH_UPDATE_BOX(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height);

				GRAPH_UPDATE_BOX(s, width, 10 + height, 320 - 2*width, 3);
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
				GRAPH_BLANK_BOX(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height, 0);

				GRAPH_BLANK_BOX(s, width, 10 + height, 320 - 2*width, 4, 0);
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
				GRAPH_UPDATE_BOX(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height);

				GRAPH_UPDATE_BOX(s, width, 10 + height, 320 - 2 * width, 3);
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

		s->pic_is_new = 0;
		s->pic_not_valid = 0;
		return;


		GFX_ASSERT(gfxop_free_pixmap(s->gfx_state, s->old_screen));
		GFX_ASSERT(gfxop_free_pixmap(s->gfx_state, newscreen));
		s->old_screen = NULL;
	} /* if (open_animation) */

	s->pic_not_valid = 0;

}

void
kShakeScreen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int shakes = PARAM_OR_ALT(0, 1);
  int i;
WARNING(fixme)
#if 0
  for (i = 0; i < shakes; i++) {
    (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_ALL, 0, -10,0,0);
    gfxop_usleep(s->gfx_state, 15000);
    (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_ALL, 0, 10,0,0);
    gfxop_usleep(s->gfx_state, 15000);
  }
  (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_ALL, 0,0,0,0);
#endif
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
WARNING(fixme)
#if 0
  int argpt;
  int textp = UPARAM(0);
  int index = UPARAM(1);
  int width = -1;
  int temp;
  int save_under = 0;
  char *text;
  resource_t *font_resource;
  port_t *port = s->port;
  port_t save;
  int update_immediately = 1;

  save=*port;
  port->alignment = ALIGN_LEFT;

  CHECK_THIS_KERNEL_FUNCTION;


  text = kernel_lookup_text(s, textp, index);

  if (textp < 1000)
    argpt = 2;
  else
    argpt = 1;

  while (argpt < argc) {

    switch(PARAM(argpt++)) {

    case K_DISPLAY_SET_COORDS:

      port->x = PARAM(argpt++);
      port->y = PARAM(argpt++) + 1; /* ? */
      SCIkdebug(SCIkGRAPHICS, "Display: set_coords(%d, %d)\n", port->x, port->y);
      break;

    case K_DISPLAY_SET_ALIGNMENT:

      port->alignment = PARAM(argpt++);
      SCIkdebug(SCIkGRAPHICS, "Display: set_align(%d)\n", port->alignment);
      break;

    case K_DISPLAY_SET_COLOR:

      port->color = PARAM(argpt++);
      SCIkdebug(SCIkGRAPHICS, "Display: set_color(%d)\n", port->color);
      break;

    case K_DISPLAY_SET_BGCOLOR:

      port->bgcolor = PARAM(argpt++);
      SCIkdebug(SCIkGRAPHICS, "Display: set_bg_color(%d)\n", port->bgcolor);
      break;

    case K_DISPLAY_SET_GRAYTEXT:

      port->gray_text = PARAM(argpt++);
      SCIkdebug(SCIkGRAPHICS, "Display: set_graytext(%d)\n", port->gray_text);
      break;

    case K_DISPLAY_SET_FONT:

      font_resource = findResource(sci_font, temp = PARAM(argpt++));

      port->font_nr = temp;

      if (font_resource)
	port->font = font_resource->data;
      else
	port->font = NULL;

      SCIkdebug(SCIkGRAPHICS, "Display: set_font(\"font.%03d\")\n", PARAM(argpt - 1));
      break;

    case K_DISPLAY_WIDTH:

      width = PARAM(argpt);
      argpt++;
      SCIkdebug(SCIkGRAPHICS, "Display: set_width(%d)\n", width);
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

  if (save_under) {    /* Backup */
    int _width, _height;

    get_text_size(text, port->font, width, &_width, &_height);
    _width = port->xmax - port->xmin; /* To avoid alignment calculations */

    s->acc = graph_save_box(s, port->x + port->xmin, port->y + port->ymin, _width, _height, 3);

    SCIkdebug(SCIkGRAPHICS, "Saving (%d, %d) size (%d, %d) as %04x\n",
	      port->x + port->xmin, port->y + port->ymin, _width, _height, s->acc);

  }


  SCIkdebug(SCIkGRAPHICS, "Display: Commiting text '%s'\n", text);

  _k_dyn_view_list_prepare_change(s);

  /*  if (s->view_port)
      graph_fill_port(s, port, port->bgcolor); */ /* Only fill if we aren't writing to the main view */

  text_draw(s->pic, port, text, width);

  _k_dyn_view_list_accept_change(s);

  if ((!s->pic_not_valid)&&update_immediately) { /* Refresh if drawn to valid picture */
    graph_update_box(s, port->xmin, port->ymin + 1,
		     port->xmax - port->xmin + 1, port->ymax - port->ymin + 1);
    SCIkdebug(SCIkGRAPHICS, "Refreshing display...\n");
  }

  *port=save;
#endif
}


