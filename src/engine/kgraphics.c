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

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/

#include <engine.h>

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

inline void
_k_clip_loop_cel(int *loop, int *cel, byte *data)
/* clips loop (and cel if != NULL)
** data must point to the view resource data block
*/
{
  int maxloop = view0_loop_count(data) - 1;

  if (*loop > maxloop)
    *loop = maxloop;

  if (cel) {
    int maxcel = view0_cel_count(*loop, data) - 1;

    if (*cel > maxcel)
      *cel = maxcel;
  }
}

void
kSetCursor(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  free_mouse_cursor(s->mouse_pointer);

  if (PARAM(1)) {
    resource_t *resource = findResource(sci_cursor, PARAM(0));
    byte *data = (resource)? resource->data : NULL;
    if (data) {
      s->mouse_pointer = calc_mouse_cursor(data);
      s->mouse_pointer_nr = PARAM(0);
    } else {
      s->mouse_pointer = NULL;
      s->mouse_pointer_nr = -1;
    }

  } else
    s->mouse_pointer = NULL;

  if (argc > 2) {
    s->pointer_x = PARAM(2) + s->ports[s->view_port]->xmin;
    s->pointer_y = PARAM(3) + s->ports[s->view_port]->ymin; /* Port-relative */
  }

  (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0,0,0,0); /* Update mouse pointer */
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
kGraph(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int color, priority, special;
  port_t *port = s->ports[s->view_port];

  _k_dyn_view_list_prepare_change(s);

  switch(PARAM(0)) {

  case K_GRAPH_GET_COLORS_NR:

    s->acc = (sci_version < SCI_VERSION_1) ? 0x10 : 0x100; /* number of colors */
    break;

  case K_GRAPH_DRAW_LINE:

    color = PARAM(5);
    priority = PARAM(6);
    special = PARAM(7);
    dither_line(s->pic, PARAM(2), PARAM(1) , PARAM(4), PARAM(3),
		color, color, priority, special,
		!(color & 0x80) | (!(priority & 0x80) << 1) | (!(special & 0x80) << 2));
    break;

  case K_GRAPH_SAVE_BOX:

    s->acc = graph_save_box(s, PARAM(2), PARAM(1), PARAM(4), PARAM(3), PARAM(5));
    break;

  case K_GRAPH_RESTORE_BOX:

    graph_restore_box(s, PARAM(1));
    break;

  case K_GRAPH_FILL_BOX_BACKGROUND:

    graph_clear_box(s, port->xmin, port->ymin,
		    port->xmax - port->xmin + 1, port->ymax - port->ymin + 1,
		    port->bgcolor);
    CHECK_THIS_KERNEL_FUNCTION;
    break;

  case K_GRAPH_FILL_BOX_FOREGROUND:

    graph_clear_box(s, port->xmin, port->ymin,
		    port->xmax - port->xmin + 1, port->ymax - port->ymin + 1,
		    port->color);
    CHECK_THIS_KERNEL_FUNCTION;
    break;

  case K_GRAPH_FILL_BOX_ANY: {

    int x = PARAM(2);
    int y = PARAM(1);

    SCIkdebug(SCIkGRAPHICS, "fill_box_any(%d, %d, %d, %d) map=%d (%d %d)\n",
	      PARAM(1), PARAM(2), PARAM(3), PARAM(4), PARAM(5), PARAM(6), PARAM_OR_ALT(7, -1));

    graph_fill_box_custom(s, x + s->ports[s->view_port]->xmin, y + s->ports[s->view_port]->ymin,
			  PARAM(4)-x, PARAM(3)-y, PARAM(6), PARAM_OR_ALT(7, -1),
			  PARAM_OR_ALT(8, -1), UPARAM(5));
    CHECK_THIS_KERNEL_FUNCTION;

  }
  break;

  case K_GRAPH_UPDATE_BOX: {

    int x = PARAM(2);
    int y = PARAM(1);

    SCIkdebug(SCIkGRAPHICS, "update_box(%d, %d, %d, %d)\n",
	      PARAM(1), PARAM(2), PARAM(3), PARAM(4));

    (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_BOX, x, y + 10, PARAM(4)-x+1, PARAM(3)-y+1);
    CHECK_THIS_KERNEL_FUNCTION;

  }
  break;

  case K_GRAPH_REDRAW_BOX: {

    CHECK_THIS_KERNEL_FUNCTION;

    SCIkdebug(SCIkGRAPHICS, "redraw_box(%d, %d, %d, %d)\n",
	      PARAM(1), PARAM(2), PARAM(3), PARAM(4));

    if (!s->dyn_views_nr)
      graph_update_box(s, PARAM(2), PARAM(1), PARAM(4), PARAM(3)); else
      ; /* Not handled yet */
      
    SCIkwarn(SCIkWARNING, "KERNEL_GRAPH_REDRAW_BOX: stub\n");

  }

  break;

  case K_GRAPH_ADJUST_PRIORITY:

    s->priority_first = PARAM(1) - 10;
    s->priority_last = PARAM(2) - 10;
    break;

  default:
    
    CHECK_THIS_KERNEL_FUNCTION;
    SCIkdebug(SCIkSTUB, "Unhandled Graph() operation %04x\n", PARAM(0));
    
  }

  _k_dyn_view_list_accept_change(s);
}


void
kTextSize(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int width, height;
  heap_ptr heap_text = UPARAM(1);
  char *text = s->heap + heap_text;
  heap_ptr dest = UPARAM(0);
  int maxwidth = UPARAM_OR_ALT(3, 0);
  resource_t *fontres = findResource(sci_font, UPARAM(2));

  if (!*text) { /* Empty text */
    PUT_HEAP(dest + 0, 0);
    PUT_HEAP(dest + 2, 0);
    PUT_HEAP(dest + 4, 0);
    PUT_HEAP(dest + 6, 0);

    return;
  }

  if (!maxwidth)
    maxwidth = MAX_TEXT_WIDTH_MAGIC_VALUE;

  if (!fontres) {
    SCIkwarn(SCIkERROR, "font.%03d not found!\n", UPARAM(2));
    return;
  }

  get_text_size(text, fontres->data, maxwidth, &width, &height);

  PUT_HEAP(dest + 0, 0);
  PUT_HEAP(dest + 2, 0);
  PUT_HEAP(dest + 4, height);
  PUT_HEAP(dest + 6, width); /* The reason for this is unknown to me */
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

  (*s->gfx_driver->Wait)(s, SleepTime * 1000000 / 60);
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
  resource_t *viewres = findResource(sci_view, view);
  int loop;
  int maxloops;

  if (signal & _K_VIEW_SIG_FLAG_DOESNT_TURN)
    return;

  if (!viewres) {
    SCIkwarn(SCIkERROR, "Invalid view.%03d\n", view);
    PUT_SELECTOR(obj, loop, 0xffff); /* Invalid */
    return;
  }

  if (angle > 360) {
    SCIkwarn(SCIkERROR, "Invalid angle %d\n", angle);
    PUT_SELECTOR(obj, loop, 0xffff);
    return;
  }

  if (angle < 45)
    loop = 3;
  else
    if (angle < 135)
      loop = 0;
  else
    if (angle < 225)
      loop = 2;
  else
    if (angle < 314)
      loop = 1;
  else
    loop = 3;

  maxloops = view0_loop_count(viewres->data);

  if (loop >= maxloops) {
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

  do
    {
      t1++;
      t2=t1*abs(dx)+dy;
    }
  while (abs(2*t2)<abs(dx));

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

  PUT_SELECTOR(mover, b_movCnt, numsteps);

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

  PUT_SELECTOR(mover, b_movCnt, movcnt - 1);

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
  word signal;

  int x = GET_SELECTOR(obj, brLeft);
  int y = GET_SELECTOR(obj, brTop);
  int xend = GET_SELECTOR(obj, brRight);
  int yend = GET_SELECTOR(obj, brBottom);
  int xl = xend - x + 1;
  int yl = yend - y + 1;
  word edgehit;

  signal = GET_SELECTOR(obj, signal);
  SCIkdebug(SCIkBRESEN,"Checking collision: (%d,%d) to (%d,%d), obj=%04x, sig=%04x, cliplist=%04x\n",
      x, y, xend, yend, obj, signal, cliplist);

  s->acc = !(((word)GET_SELECTOR(obj, illegalBits))
	     & (edgehit = graph_on_control(s, x, y + 10, xl, yl, SCI_MAP_CONTROL)));

  if (s->acc == 0)
    return; /* Can'tBeHere */
  if ((signal & _K_VIEW_SIG_FLAG_DONT_RESTORE) || (signal & _K_VIEW_SIG_FLAG_IGNORE_ACTOR))
  {
    s->acc=signal & (_K_VIEW_SIG_FLAG_DONT_RESTORE|_K_VIEW_SIG_FLAG_IGNORE_ACTOR); /* CanBeHere- it's either being disposed, or it ignores actors anyway */
    return;
  }
  if (cliplist) {
    heap_ptr node = GET_HEAP(cliplist + LIST_FIRST_NODE);

    s->acc = 0; /* Assume that we Can'tBeHere... */

    while (node) { /* Check each object in the list against our bounding rectangle */
      heap_ptr other_obj = UGET_HEAP(node + LIST_NODE_VALUE);
      if (other_obj != obj) { /* Clipping against yourself is not recommended */

	int other_signal = GET_SELECTOR(other_obj, signal);
	SCIkdebug(SCIkBRESEN, "OtherSignal=%04x, z=%04x\n", other_signal, (other_signal & (_K_VIEW_SIG_FLAG_DONT_RESTORE | _K_VIEW_SIG_FLAG_IGNORE_ACTOR)));
	if ((other_signal & (_K_VIEW_SIG_FLAG_DONT_RESTORE | _K_VIEW_SIG_FLAG_IGNORE_ACTOR)) == 0) {
	  /* check whether the other object ignores actors */

	  int other_x = GET_SELECTOR(other_obj, brLeft);
	  int other_y = GET_SELECTOR(other_obj, brTop);
	  int other_xend = GET_SELECTOR(other_obj, brRight);
	  int other_yend = GET_SELECTOR(other_obj, brBottom);
	  SCIkdebug(SCIkBRESEN, "  against (%d,%d) to (%d, %d)\n",
		    other_x, other_y, other_xend, other_yend);


	  if (((other_xend >= x) && (other_x <= xend)) /* [other_x, other_xend] intersects [x, xend]? */
	      &&
	      ((other_yend >= y) && (other_y <= yend))) /* [other_y, other_yend] intersects [y, yend]? */
	    return;

	}

		SCIkdebug(SCIkBRESEN, " (no)\n");

      } /* if (other_obj != obj) */
      node = GET_HEAP(node + LIST_NEXT_NODE); /* Move on */
    }
  }

  s->acc = 1;
  /* CanBeHere */
}



void
kCelHigh(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *viewres = findResource(sci_view, PARAM(0));
  int result;

  if (!viewres) {
    SCIkwarn(SCIkERROR, "view.%d (0x%x) not found\n", PARAM(0), PARAM(0));
    return;
  }

  s->acc = result = view0_cel_height(PARAM(1), PARAM(2), viewres->data);
  if (result < 0)
    SCIkwarn(SCIkERROR, "Invalid loop (%d) or cel (%d) in view.%d (0x%x)\n", PARAM(1), PARAM(2), PARAM(0), PARAM(0));
}

void
kCelWide(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *viewres = findResource(sci_view, PARAM(0));
  int result;

  if (!viewres) {
    SCIkwarn(SCIkERROR, "view.%d (0x%x) not found\n", PARAM(0), PARAM(0));
    return;
  }

  s->acc = result = view0_cel_width(PARAM(1), PARAM(2), viewres->data);
  if (result < 0)
    SCIkwarn(SCIkERROR, "Invalid loop (%d) or cel (%d) in view.%d (0x%x)\n", PARAM(1), PARAM(2), PARAM(0), PARAM(0));
}



void
kNumLoops(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = PARAM(0);
  int view;
  resource_t *viewres = findResource(sci_view, view = GET_SELECTOR(obj, view));

  CHECK_THIS_KERNEL_FUNCTION;

  if (!viewres) {
    SCIkwarn(SCIkERROR, "view.%d (0x%x) not found\n", view, view);
    return;
  }

  s->acc = view0_loop_count(viewres->data);
  SCIkdebug(SCIkGRAPHICS, "NumLoops(view.%d) = %d\n", view, s->acc);
}


void
kNumCels(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = PARAM(0);
  int loop = UGET_SELECTOR(obj, loop);
  int view;
  resource_t *viewres = findResource(sci_view, view = GET_SELECTOR(obj, view));

  CHECK_THIS_KERNEL_FUNCTION;

  if (!viewres) {
    SCIkwarn(SCIkERROR, "view.%d (0x%x) not found\n", PARAM(0), PARAM(0));
    return;
  }

  _k_clip_loop_cel(&loop, NULL, viewres->data);
  s->acc = view0_cel_count(loop, viewres->data);
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
  }
  s->acc = graph_on_control(s, xstart, ystart + 10, xlen, ylen, map);

}

void
_k_view_list_free_backgrounds(state_t *s, view_object_t *list, int list_nr);
void
_k_view_list_dispose(state_t *s, view_object_t **list_ptr, int *list_nr_ptr);

void
kDrawPic(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *resource = findResource(sci_pic, PARAM(0));
  CHECK_THIS_KERNEL_FUNCTION;

  if (resource) {

    if (s->version < SCI_VERSION_FTU_NEWER_DRAWPIC_PARAMETERS) {
      if (!PARAM_OR_ALT(2, 0)) {
	clear_picture(s->pic, 15);
	_k_view_list_dispose(s, &(s->dyn_views), &(s->dyn_views_nr));
      }
    } else
      if (PARAM_OR_ALT(2, 1)) {
	clear_picture(s->pic, 15);
	_k_view_list_dispose(s, &(s->dyn_views), &(s->dyn_views_nr));
      }

    draw_pic0(s->pic, 1, PARAM_OR_ALT(3, 0), resource->data);

    SCIkdebug(SCIkGRAPHICS,"Drawing pic.%03d\n", PARAM(0));

    if (argc > 1)
      s->pic_animate = PARAM(1); /* The animation used during kAnimate() later on */

    s->priority_first = 42;
    s->priority_last = 200;
    s->pic_not_valid = 1;
    s->pic_is_new = 1;

    if (s->pic_views_nr)
      g_free(s->pic_views);
    s->pic_views_nr = 0;

  } else
    SCIkwarn(SCIkERROR, "Request to draw non-existing pic.%03d\n", PARAM(0));
}



void
_k_base_setter(state_t *s, heap_ptr object)
{
  int x, y, original_y, z, ystep, xsize, ysize;
  int xbase, ybase, xend, yend;
  int view, loop, cell;
  int xmod = 0, ymod = 0;
  resource_t *viewres;

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
  cell = GET_SELECTOR(object, cel);

  viewres = findResource(sci_view, view);

  if (!viewres)
    xsize = ysize = 0;
  else {
    _k_clip_loop_cel(&loop, &cell, viewres->data);
    xsize = view0_cel_width(loop, cell, viewres->data);
    ysize = view0_cel_height(loop, cell, viewres->data);
  }


  if ((xsize < 0) || (ysize < 0))
    xsize = ysize = 0; /* Invalid view/loop */
  else
    view0_base_modify(loop, cell, viewres->data, &xmod, &ymod);


  /*  fprintf(stderr,"(xm,ym)=(%d,%d)\n", xmod, ymod); */
  
  xbase = x - xmod - (xsize) / 2;
  xend = xbase + xsize;
  yend = y - ymod + 1;
  ybase = yend - ystep;

  PUT_SELECTOR(object, brLeft, xbase);
  PUT_SELECTOR(object, brRight, xend);
  PUT_SELECTOR(object, brTop, ybase);
  PUT_SELECTOR(object, brBottom, yend);


  if (s->debug_mode & (1 << SCIkBASESETTER_NR)) {
    graph_clear_box(s, xbase, ybase + 10, xend-xbase+1, yend-ybase+1, VIEW_PRIORITY(original_y));
    (*s->gfx_driver->Wait)(s, 100000);
  }
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
  int view, loop, cell;
  int xmod = 0, ymod = 0;
  resource_t *viewres;  

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
  cell = GET_SELECTOR(object, cel);

  viewres = findResource(sci_view, view);

  if (!viewres)
    xsize = ysize = 0;
  else {
    xsize = view0_cel_width(loop, cell, viewres->data);
    ysize = view0_cel_height(loop, cell, viewres->data);
  }

  if ((xsize < 0) || (ysize < 0))
    xsize = ysize = 0; /* Invalid view/loop */
  else
    view0_base_modify(loop, cell, viewres->data, &xmod, &ymod);

  xbase = x + xmod - (xsize) / 2;
  xend = xbase + xsize;
  yend = y + ymod + 1; /* +1: Magic modifier */
  ybase = yend - ysize;
  /*  if (object == 0x756) {
    int nsl, nsr, nst, nsb, brl, brr, brt, brb;
    nsl = GET_SELECTOR(object, nsLeft);
    nsr = GET_SELECTOR(object, nsRight);
    nst = GET_SELECTOR(object, nsTop);
    nsb = GET_SELECTOR(object, nsBottom);

    brl = GET_SELECTOR(object, brLeft);
    brr = GET_SELECTOR(object, brRight);
    brt = GET_SELECTOR(object, brTop);
    brb = GET_SELECTOR(object, brBottom);
        if (nsl != xbase
	|| nsr != xend
	|| nst != ybase
	|| nsb != yend)
          fprintf(stderr,">>O@%04x (%d,%d,%d)+(%d,%d) (%dx%d) [(%d %d)(%d %d)] => [(%d %d)(%d %d)] {(%d %d)(%d %d)}\n",
	    object, x, y, z, xmod, ymod, xsize, ysize, nsl, nsr, nst, nsb, xbase, xend, ybase, yend, brl, brr, brt, brb);
  }*/

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

	  port_t *port = s->ports[s->view_port];

	  if (cursor > textlen)
	    cursor = textlen;

	  graph_fill_box_custom(s, x + port->xmin, y + port->ymin,
				xl, yl, port->bgcolor, -1, 0, 1); /* Clear input box background */

	  /*	  fprintf(stderr,"EditControl: mod=%04x, key=%04x, maxlen=%04x, cursor=%04x\n",
		  modifiers, key, max, cursor);*/

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

          } 
          else if (key < 31) {

	    PUT_SELECTOR(event, claimed, 1);

	    switch(key) {
	    case SCI_K_BACKSPACE: _K_EDIT_BACKSPACE; break;
	    default:
	      PUT_SELECTOR(event, claimed, 0);
	    }

	  } 

          else if ((key >= SCI_K_HOME) && (key <= SCI_K_DELETE))
          {
            switch(key) {
	    case SCI_K_HOME: cursor = 0; break;
	    case SCI_K_END: cursor = textlen; break;
	    case SCI_K_RIGHT: if (cursor + 1 <= textlen) ++cursor; break;
	    case SCI_K_LEFT: if (cursor > 0) --cursor; break;
	    case SCI_K_DELETE: _K_EDIT_DELETE; break;
	    }
	    PUT_SELECTOR(event, claimed, 1);
          }
          
          else if ((key > 31) && (key < 128)) 
          {
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
      PUT_SELECTOR(obj, state, state | SELECTOR_STATE_DITHER_FRAMED);
      _k_draw_control(s, obj, 0);
      PUT_SELECTOR(obj, state, state);
    }
    break;

    default:
      SCIkwarn(SCIkWARNING, "Attempt to edit control type %d\n", ct_type);
    }
  }
}


static inline void
_k_clip_view(byte *view, int *loop, int *cel)
     /* Checks if *loop and *cel are valid; if they aren't, they're clipped to the highest possible
     ** valid values.
     */
{
  int maxloops, maxcels;

  maxloops = view0_loop_count(view) - 1;
  if (*loop > maxloops)
    *loop = maxloops;

  maxcels = view0_cel_count(*loop, view) - 1;
  if (*cel >= maxcels)
    *cel = maxcels;
}


static void
_k_draw_control(state_t *s, heap_ptr obj, int inverse)
{
  int x = GET_SELECTOR(obj, nsLeft);
  int y = GET_SELECTOR(obj, nsTop);
  int xl = GET_SELECTOR(obj, nsRight) - x + 1;
  int yl = GET_SELECTOR(obj, nsBottom) - y + 1;

  int font_nr = GET_SELECTOR(obj, font);
  char *text = s->heap + UGET_SELECTOR(obj, text);
  int view = GET_SELECTOR(obj, view);
  int cel = GET_SELECTOR(obj, cel);
  int loop = GET_SELECTOR(obj, loop);

  int type = GET_SELECTOR(obj, type);
  int state = GET_SELECTOR(obj, state);
  int cursor;

  resource_t *font_res;
  resource_t *view_res;

  if (inverse) { /* Ugly hack */
    port_t *port = s->ports[s->view_port];
    int foo = port->bgcolor;
    port->bgcolor = port->color;
    port->color = foo;
  }

  switch (type) {

  case K_CONTROL_BUTTON:

    font_res  = findResource(sci_font, font_nr);
    SCIkdebug(SCIkGRAPHICS, "drawing button %04x to %d,%d\n", obj, x, y);
    if (!font_res) {
      SCIkwarn(SCIkERROR, "Font.%03d not found!\n", font_nr);
      break;
    }
    graph_draw_control_button(s, s->ports[s->view_port], state, x, y, xl, yl, text, font_res->data);
    break;

  case K_CONTROL_TEXT:

    font_res  = findResource(sci_font, font_nr);
    SCIkdebug(SCIkGRAPHICS, "drawing text %04x to %d,%d\n", obj, x, y);
    if (!font_res) {
      SCIkwarn(SCIkERROR, "Font.%03d not found!\n", font_nr);
      break;
    }
    graph_draw_control_text(s, s->ports[s->view_port], state & ~ SELECTOR_STATE_FRAMED,
			    x, y, 0, xl, yl, text, font_res->data,
			    (s->version < SCI_VERSION_FTU_CENTERED_TEXT_AS_DEFAULT)
			    ? ALIGN_TEXT_LEFT : ALIGN_TEXT_CENTER);
    break;

  case K_CONTROL_EDIT:

    font_res  = findResource(sci_font, font_nr);
    SCIkdebug(SCIkGRAPHICS, "drawing edit control %04x to %d,%d\n", obj, x, y);
    if (!font_res) {
      SCIkwarn(SCIkERROR, "Font.%03d not found!\n", font_nr);
      break;
    }

    cursor = GET_SELECTOR(obj, cursor);

    graph_draw_control_edit(s, s->ports[s->view_port], state | SELECTOR_STATE_FRAMED, x, y, xl, yl, cursor,
			     text, font_res->data);
    break;

  case K_CONTROL_ICON:

    view_res = findResource(sci_view, view);
    SCIkdebug(SCIkGRAPHICS, "drawing icon control %04x to %d,%d\n", obj, x, y -1);

    if (!view_res) {
      SCIkwarn(SCIkERROR, "View.%03d not found!\n", font_nr);
      break;
    }
    graph_draw_control_icon(s, s->ports[s->view_port], state, x, y-1, xl, yl,
			     view_res->data, loop, cel);
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

    font_res  = findResource(sci_font, font_nr);
    if (!font_res) {
      SCIkwarn(SCIkERROR, "Font.%03d not found!\n", font_nr);
      break;
    }
    graph_draw_control_control(s, s->ports[s->view_port], state, x, y, xl, yl,
				entries_list, entries_nr, list_top, selection, font_res->data);
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

  if (!s->pic_not_valid)
    graph_update_port(s, s->ports[s->view_port]);

  if (inverse) {
    port_t *port = s->ports[s->view_port];
    int foo = port->bgcolor;
    port->bgcolor = port->color;
    port->color = foo;
  }
}

void
_k_dyn_view_list_prepare_change(state_t *s)
     /* Removes all views in anticipation of a new window or text */
{
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
}

void
_k_restore_view_list_backgrounds(state_t *s, view_object_t *list, int list_nr)
     /* Restores the view backgrounds of list_nr members of list */
{
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
}

void
_k_view_list_free_backgrounds(state_t *s, view_object_t *list, int list_nr)
     /* Frees all backgrounds from the list without restoring; called by DrawPic */
{
  int i;

  SCIkdebug(SCIkMEM, "Freeing bglist: %d entries\n", list_nr);
  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
    int handle = list[i].underBits;

    if (handle)
      kfree(s, handle);

    PUT_HEAP(list[i].underBitsp, list[i].underBits = 0);
  }
}

void
_k_save_view_list_backgrounds(state_t *s, view_object_t *list, int list_nr)
     /* Stores the view backgrounds of list_nr members of list in their underBits selectors */
{
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
}

void
_k_view_list_dispose_loop(state_t *s, heap_ptr list_addr, view_object_t *list, int list_nr,
			  int funct_nr, int argc, int argp)
     /* disposes all list members flagged for disposal; funct_nr is the invoking kfunction */
{
  int i;

  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
      if (UGET_HEAP(list[i].signalp) & _K_VIEW_SIG_FLAG_DISPOSE_ME) {

	if (list[i].underBitsp) { /* Is there a bg picture left to clean? */
	  word mem_handle = list[i].underBits = GET_HEAP(list[i].underBitsp);

	  if (mem_handle) {
	    kfree(s, mem_handle);
	    PUT_HEAP(list[i].underBitsp, list[i].underBits = 0);
	  }
	}

	if (invoke_selector(INV_SEL(list[i].obj, delete, 1), 0))
	  SCIkwarn(SCIkWARNING, "Object at %04x requested deletion, but does not have"
		   " a delete funcselector\n", list[i].obj);
      }
  }
}


void
_k_invoke_view_list(state_t *s, heap_ptr list, int funct_nr, int argc, int argp)
     /* Invokes all elements of a view list. funct_nr is the number of the calling funcion. */
{
  heap_ptr node = GET_HEAP(list + LIST_LAST_NODE);

  while (node) {
    heap_ptr obj = UGET_HEAP(node + LIST_NODE_VALUE); /* The object we're using */

    if (!(GET_SELECTOR(obj, signal) & _K_VIEW_SIG_FLAG_FROZEN)) {
      word ubitsnew, ubitsold = GET_SELECTOR(obj, underBits);
      invoke_selector(INV_SEL(obj, doit, 1), 0); /* Call obj::doit() if neccessary */
      ubitsnew = GET_SELECTOR(obj, underBits);

      if (ubitsold == 0xffff)
	fprintf(stderr,"Obj %04x: Old underBits were 0xffff\n", obj);
      if (ubitsnew != ubitsold)
	fprintf(stderr,"Obj %04x: underBits(old, new) = (%04x, %04x)\n", obj, ubitsold, ubitsnew);
    }

    node = UGET_HEAP(node + LIST_PREVIOUS_NODE);
  }

}


static int _cmp_view_object(const void *obj1, const void *obj2) /* Used for qsort() later on */
{
  return (((view_object_t *)obj1)->y) - (((view_object_t *)obj2)->y);
}

#define _K_MAKE_VIEW_LIST_CYCLE 1
#define _K_MAKE_VIEW_LIST_CALC_PRIORITY 2

view_object_t *
_k_make_view_list(state_t *s, heap_ptr list, int *list_nr, int options, int funct_nr, int argc, int argp)
     /* Creates a view_list from a node list in heap space. Returns the list, stores the
     ** number of list entries in *list_nr. Calls doit for each entry if cycle is set.
     ** argc, argp, funct_nr should be the same as in the calling kernel function.
     */
{
  heap_ptr node;
  view_object_t *retval;
  int i;

  if (options & _K_MAKE_VIEW_LIST_CYCLE)
    _k_invoke_view_list(s, list, funct_nr, argc, argp); /* Invoke all objects if requested */

  node = GET_HEAP(list + LIST_LAST_NODE);
  *list_nr = 0;

  SCIkdebug(SCIkGRAPHICS, "Making list from %04x\n", list);

  if (GET_HEAP(list - 2) != 0x6) { /* heap size check */
    SCIkwarn(SCIkWARNING, "Attempt to draw non-list at %04x\n", list);
    return NULL; /* Return an empty list */
  }

  while (node) {
    (*list_nr)++;
    node = UGET_HEAP(node + LIST_PREVIOUS_NODE);
  } /* Counting the nodes */

  if (!(*list_nr))
    return NULL; /* Empty */

  retval = g_malloc(*list_nr * (sizeof(view_object_t))); /* Allocate the list */

  node = UGET_HEAP(list + LIST_LAST_NODE); /* Start over */

  i = 0;
  while (node) {
    short oldloop, oldcel;
    heap_ptr obj = GET_HEAP(node + LIST_NODE_VALUE); /* The object we're using */
    int view_nr = GET_SELECTOR(obj, view);
    resource_t *viewres = findResource(sci_view, view_nr);

    SCIkdebug(SCIkGRAPHICS, " - Adding %04x\n", obj);

    if (i == (*list_nr)) {
      (*list_nr)++;
      retval = g_realloc(retval, *list_nr * sizeof(view_object_t));
      /* This has been reported to happen */
    }

    retval[i].obj = obj;

    retval[i].x = GET_SELECTOR(obj, x);
    retval[i].y = GET_SELECTOR(obj, y) - GET_SELECTOR(obj, z);

    retval[i].nsLeft = GET_SELECTOR(obj, nsLeft);
    retval[i].nsRight = GET_SELECTOR(obj, nsRight);
    retval[i].nsTop = GET_SELECTOR(obj, nsTop);
    retval[i].nsBottom = GET_SELECTOR(obj, nsBottom);

    retval[i].loop = oldloop = GET_SELECTOR(obj, loop);
    retval[i].cel = oldcel = GET_SELECTOR(obj, cel);

    if (!viewres) {
      SCIkwarn(SCIkERROR, "Attempt to draw invalid view.%03d!\n", view_nr);
      retval[i].view = NULL;
      retval[i].view_nr = 0;
    } else {
      retval[i].view = viewres->data;
      retval[i].view_nr = view_nr;

      /* Clip loop and cel, write back if neccessary */
      oldloop = retval[i].loop;
      oldcel = retval[i].cel;
      _k_clip_loop_cel(&(retval[i].loop), &(retval[i].cel), retval[i].view);

      if (oldloop != retval[i].loop)
	PUT_SELECTOR(retval[i].obj, loop, retval[i].loop);

      if (oldcel != retval[i].cel)
	PUT_SELECTOR(retval[i].obj, cel, retval[i].cel);
    
    }

    if (lookup_selector(s, obj, s->selector_map.underBits, &(retval[i].underBitsp))
	!= SELECTOR_VARIABLE) {
      retval[i].underBitsp = 0;
      SCIkdebug(SCIkGRAPHICS, "Object at %04x has no underBits\n", obj);
    } else retval[i].underBits = GET_HEAP(retval[i].underBitsp);

    if (lookup_selector(s, obj, s->selector_map.signal, &(retval[i].signalp))
	!= SELECTOR_VARIABLE) {
      retval[i].signalp = 0;
      SCIkdebug(SCIkGRAPHICS, "Object at %04x has no signal selector\n", obj);
    }

    if ((options & _K_MAKE_VIEW_LIST_CALC_PRIORITY)
	&& !(UGET_HEAP(retval[i].signalp) & _K_VIEW_SIG_FLAG_FIX_PRI_ON)) { /* Calculate priority */
      int _priority, y = retval[i].y;
      _priority = VIEW_PRIORITY(y);

      PUT_SELECTOR(obj, priority, _priority);

      retval[i].priority = _priority;
    } else /* DON'T calculate the priority */
      retval[i].priority = GET_SELECTOR(obj, priority);

    s->pic_not_valid++; /* There ought to be some kind of check here... */

    i++; /* Next object in the list */

    node = UGET_HEAP(node + LIST_PREVIOUS_NODE); /* Next node */
  }

  qsort(retval, *list_nr, sizeof(view_object_t), _cmp_view_object);

  return retval;
}


void
_k_view_list_dispose(state_t *s, view_object_t **list_ptr, int *list_nr_ptr)
     /* Unallocates all list element data, frees the list */
{
  view_object_t *list = *list_ptr;
  if (!(*list_nr_ptr))
    return; /* Nothing to do :-( */

  _k_view_list_free_backgrounds(s, list, *list_nr_ptr);

  g_free (list);
  *list_ptr = NULL;
  *list_nr_ptr = 0;
}


/* Flags for _k_draw_view_list */
/* Whether some magic with the base object's "signal" selector should be done: */
#define _K_DRAW_VIEW_LIST_USE_SIGNAL 1
/* This flag draws all views with the "DISPOSE_ME" flag set: */ 
#define _K_DRAW_VIEW_LIST_DISPOSEABLE 2
/* Use this one to draw all views with "DISPOSE_ME" NOT set: */
#define _K_DRAW_VIEW_LIST_NONDISPOSEABLE 4

void
_k_draw_view_list(state_t *s, view_object_t *list, int list_nr, int flags)
     /* Draws list_nr members of list to s->pic. */
{
  int i;
  int argc = 0, argp = 0, funct_nr = -1; /* Kludges to work around INV_SEL dependancies */

  if (s->view_port != s->dyn_view_port)
    return; /* Return if the pictures are meant for a different port */

  for (i = 0; i < list_nr; i++) 
    if (list[i].obj) {
    word signal = (flags & _K_DRAW_VIEW_LIST_USE_SIGNAL)? GET_HEAP(list[i].signalp) : 0;

    if (!(flags & _K_DRAW_VIEW_LIST_USE_SIGNAL)
	|| ((flags & _K_DRAW_VIEW_LIST_DISPOSEABLE) && (signal & _K_VIEW_SIG_FLAG_DISPOSE_ME))
	|| ((flags & _K_DRAW_VIEW_LIST_NONDISPOSEABLE) && !(signal & _K_VIEW_SIG_FLAG_DISPOSE_ME))) {

      /* Now, if we either don't use signal OR if signal allows us to draw, do so: */
      if (!(flags & _K_DRAW_VIEW_LIST_USE_SIGNAL) || (!(signal & _K_VIEW_SIG_FLAG_NO_UPDATE) &&
						      !(signal & _K_VIEW_SIG_FLAG_HIDDEN))) {

	_k_clip_view(list[i].view, &(list[i].loop), &(list[i].cel));

	_k_set_now_seen(s, list[i].obj);

	if (lookup_selector(s, list[i].obj, s->selector_map.baseSetter, NULL) == SELECTOR_METHOD)
	  invoke_selector(INV_SEL(list[i].obj, baseSetter, 1), 0); /* SCI-implemented base setter */
	else
	  _k_base_setter(s, list[i].obj);

	SCIkdebug(SCIkGRAPHICS, "Drawing obj %04x with signal %04x\n", list[i].obj, signal);

	draw_view0(s->pic, s->ports[s->view_port],
		   list[i].x, list[i].y, list[i].priority, list[i].loop, list[i].cel,
		   GRAPHICS_VIEW_CENTER_BASE | GRAPHICS_VIEW_USE_ADJUSTMENT, list[i].view);
      }

      if (flags & _K_DRAW_VIEW_LIST_USE_SIGNAL) {
	signal &= ~(_K_VIEW_SIG_FLAG_UPDATE_ENDED | _K_VIEW_SIG_FLAG_UPDATING |
		    _K_VIEW_SIG_FLAG_NO_UPDATE | _K_VIEW_SIG_FLAG_UNKNOWN_6);
	/* Clear all of those flags */

	PUT_HEAP(list[i].signalp, signal); /* Write the changes back */
      } else continue;

      if (signal & 0 & _K_VIEW_SIG_FLAG_IGNORE_ACTOR)
	continue; /* I assume that this is used for PicViews as well, so no use_signal check */
      else if (signal & 0 & _K_VIEW_SIG_FLAG_UNKNOWN_5) { /* Yep, the continue doesn't really make sense. It's for clarification. */
	int brTop = GET_SELECTOR(list[i].obj, brTop);
	int brBottom = GET_SELECTOR(list[i].obj, brBottom);
	int brLeft = GET_SELECTOR(list[i].obj, brLeft);
	int brRight = GET_SELECTOR(list[i].obj, brRight);
	int yl, y = brTop;
	int priority_band_start = PRIORITY_BAND_FIRST(list[i].priority);
	/* Get start of priority band */

	if (priority_band_start > brTop)
	  y = priority_band_start;

	yl = abs(y - brBottom);

	fill_box(s->pic, brLeft, y + 10,
		 brRight - brLeft, yl, 0xf, 2);
	/* Fill the control map with solidity */

      } /* NOT Ignoring the actor */
    } /* ...if we're drawing disposeables and this one is disposeable, or if we're drawing non-
      ** disposeables and this one isn't disposeable */
    } /* for (i = 0; i < list_nr; i++) */

}

void
_k_dyn_view_list_accept_change(state_t *s)
     /* Restores all views after backupping their new bgs */
{
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

}


void
kAddToPic(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr list = PARAM(0);
  CHECK_THIS_KERNEL_FUNCTION;

  if (s->pic_views_nr) {
    g_free(s->pic_views);
    s->pic_views = NULL;
  }

  if (!list)
    return;

  SCIkdebug(SCIkGRAPHICS, "Preparing list...\n");
  s->pic_views = _k_make_view_list(s, list, &(s->pic_views_nr), 0, funct_nr, argc, argp);
  /* Store pic views for later re-use */

  SCIkdebug(SCIkGRAPHICS, "Drawing list...\n");
  _k_draw_view_list(s, s->pic_views, s->pic_views_nr, _K_DRAW_VIEW_LIST_NONDISPOSEABLE | _K_DRAW_VIEW_LIST_DISPOSEABLE);
  /* Draw relative to the bottom center */
  SCIkdebug(SCIkGRAPHICS, "Returning.\n");
}


void
kGetPort(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = s->view_port;
}


void
kSetPort(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  unsigned int newport = PARAM(0);

  CHECK_THIS_KERNEL_FUNCTION;

  graph_update_port(s, s->ports[s->view_port]); /* Update the port we're leaving */

  if ((newport >= MAX_PORTS) || (s->ports[newport] == NULL)) {
    SCIkwarn(SCIkERROR, "Invalid port %04x requested\n", newport);
    return;
  }

  s->view_port = newport;
}


void
kDrawCel(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *view = findResource(sci_view, PARAM(0));
  int loop = PARAM(1);
  int cel = PARAM(2);
  int x = PARAM(3);
  int y = PARAM(4);
  int priority = PARAM_OR_ALT(5, -1);


  CHECK_THIS_KERNEL_FUNCTION;

  if (!view) {
    SCIkwarn(SCIkERROR, "Attempt to draw non-existing view.%03n\n", PARAM(0));
    return;
  }
  _k_clip_view(view->data, &loop, &cel);

  SCIkdebug(SCIkGRAPHICS, "DrawCel((%d,%d), (view.%d, %d, %d), p=%d)\n", x, y, PARAM(0), loop,
	    cel, priority);

  draw_view0(s->pic, s->ports[s->view_port], x, y, priority, loop, cel, 0, view->data);
}



void
kDisposeWindow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  unsigned int goner = PARAM(0);

  if ((goner >= MAX_PORTS) || (goner < 3) ||(s->ports[goner] == NULL)) {
    SCIkwarn(SCIkERROR, "Removal of invalid window %04x requested\n", goner);
    return;
  }

  if (goner == s->view_port) /* Are we killing the active port? */
    s->view_port = s->ports[goner]->predecessor; /* Set predecessor as active port if so */

  _k_dyn_view_list_prepare_change(s);
  graph_restore_box(s, s->ports[goner]->bg_handle);

  _k_dyn_view_list_accept_change(s);

  graph_update_port(s, s->ports[goner]);

  g_free(s->ports[goner]);
  s->ports[goner] = NULL; /* Mark as free */
}


void
kNewWindow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  unsigned int window = 3;
  port_t *wnd;
  int xlo, ylo;

  CHECK_THIS_KERNEL_FUNCTION;

  while ((window < MAX_PORTS) && (s->ports[window]))
    ++window;

  if (window == MAX_PORTS) {
    KERNEL_OOPS("Out of window/port handles in kNewWindow! Increase MAX_PORTS in engine.h\n");
    return;
  }

  graph_update_port(s, s->ports[s->view_port]); /* Update the port we're leaving */

  wnd = g_malloc0(sizeof(port_t));

  wnd->ymin = PARAM(0) + 10;
  wnd->xmin = PARAM(1);
  wnd->ymax = PARAM(2) + 10; /*  +10 because of the menu bar- SCI scripts don't count it */
  wnd->xmax = PARAM(3) + 1;
  wnd->title = PARAM(4);
  wnd->flags = PARAM(5);
  wnd->priority = PARAM(6);
  wnd->color = PARAM_OR_ALT(7, 0);
  wnd->bgcolor = PARAM_OR_ALT(8, 15);
  wnd->font = s->titlebar_port.font; /* Default to 'system' font */
  wnd->font_nr = s->titlebar_port.font_nr;
  wnd->predecessor = s->view_port;

  wnd->alignment = ALIGN_TEXT_LEFT; /* FIXME?? */

  if (wnd->priority == -1)
    wnd->priority = 16; /* Max priority + 1*/

  s->ports[window] = wnd;

  xlo = wnd->xmin;
  ylo = wnd->ymin - ((wnd->flags & WINDOW_FLAG_TITLE)? 10 : 0);
  /* Windows with a title bar get positioned in a way ignoring the title bar. */

  if (!(wnd->flags&WINDOW_FLAG_DONTDRAW))
  {
    _k_dyn_view_list_prepare_change(s);

    wnd->bg_handle = graph_save_box(s, xlo, ylo, wnd->xmax - xlo + 1, wnd->ymax - ylo + 1, 3);

    draw_window(s->pic, s->ports[window], wnd->bgcolor, wnd->priority,
	        s->heap + wnd->title, s->titlebar_port.font , wnd->flags); /* Draw window */

    _k_dyn_view_list_accept_change(s);

  }

  /* Now sanitize the port values */
  if (wnd->xmin < 0)
    wnd->xmin = 0;
  if (wnd->xmax > 319)
    wnd->xmax = 319;
  if (wnd->ymin < 10)
    wnd->ymin = 10;
  if (wnd->ymax > 199)
    wnd->ymax = 199;

  if (!(wnd->flags&WINDOW_FLAG_DONTDRAW))
    graph_update_port(s, wnd); /* Update viewscreen */

  s->view_port = window; /* Set active port */

  s->acc = window;
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


void
kAnimate(state_t *s, int funct_nr, int argc, heap_ptr argp)
     /* Animations are supposed to take a maximum of s->animation_delay milliseconds. */
{
  int i, remaining_checkers;
  char checkers[32 * 19];
  heap_ptr cast_list = UPARAM_OR_ALT(0, 0);
  int cycle = UPARAM_OR_ALT(1, 0);
  int open_animation = 0;

  CHECK_THIS_KERNEL_FUNCTION;

  process_sound_events(s); /* Take care of incoming events (kAnimate is called semi-regularly) */

  if (!cast_list)
    _k_view_list_dispose(s, &(s->dyn_views), &(s->dyn_views_nr)); /* Clear dynviews list */

  if (s->dyn_views_nr) {
    g_free(s->dyn_views);
    s->dyn_views_nr = 0; /* No more dynamic views */
    s->dyn_views = NULL;
  }

  if (cast_list) {
    s->dyn_view_port = s->view_port; /* The list is valid for view_port */

    s->dyn_views = _k_make_view_list(s, cast_list, &(s->dyn_views_nr), (cycle? _K_MAKE_VIEW_LIST_CYCLE : 0)
				     | _K_MAKE_VIEW_LIST_CALC_PRIORITY, funct_nr, argc, argp);
    /* Initialize pictures- Steps 3-9 in Lars' V 0.1 list */

    SCIkdebug(SCIkGRAPHICS, "Handling PicViews:\n");
    if ((s->pic_not_valid) && (s->pic_views_nr))
      _k_draw_view_list(s, s->pic_views, s->pic_views_nr, 0); /* Step 10 */
    SCIkdebug(SCIkGRAPHICS, "Handling Dyn:\n");

    _k_restore_view_list_backgrounds(s, s->dyn_views, s->dyn_views_nr);
    _k_draw_view_list(s, s->dyn_views, s->dyn_views_nr, _K_DRAW_VIEW_LIST_DISPOSEABLE | _K_DRAW_VIEW_LIST_USE_SIGNAL);
    _k_save_view_list_backgrounds(s, s->dyn_views, s->dyn_views_nr);
    _k_draw_view_list(s, s->dyn_views, s->dyn_views_nr, _K_DRAW_VIEW_LIST_NONDISPOSEABLE | _K_DRAW_VIEW_LIST_USE_SIGNAL); /* Step 11 */
    /* Step 15 */
    _k_view_list_dispose_loop(s, cast_list, s->dyn_views, s->dyn_views_nr, funct_nr, argc, argp);
  } /* if (cast_list) */

  open_animation = (s->pic_is_new) && (s->pic_not_valid);

  if (open_animation) {

    SCIkdebug(SCIkGRAPHICS, "Animating pic opening type %x\n", s->pic_animate);

    switch(s->pic_animate) {
    case K_ANIMATE_BORDER_CLOSE_H_CENTER_OPEN_H :

      for (i = 0; i < 160; i++) {
	graph_clear_box(s, i, 10, 1, 190, 0);
	graph_clear_box(s, 319-i, 10, 1, 190, 0);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_CENTER_OPEN_H :

      for (i = 159; i >= 0; i--) {
	graph_update_box(s, i, 10, 1, 190);
	graph_update_box(s, 319-i, 10, 1, 190);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_BORDER_CLOSE_V_CENTER_OPEN_V :

      for (i = 0; i < 95; i++) {
	graph_clear_box(s, 0, i + 10, 320, 1, 0);
	graph_clear_box(s, 0, 199 - i, 320, 1, 0);
	(*s->gfx_driver->Wait)(s, 2 * s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_CENTER_OPEN_V :

      for (i = 94; i >= 0; i--) {
	graph_update_box(s, 0, i + 10, 320, 1);
	graph_update_box(s, 0, 199 - i, 320, 1);
	(*s->gfx_driver->Wait)(s, 2 * s->animation_delay);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_LEFT_CLOSE_RIGHT_OPEN :

      for(i = 0; i < 320; i++) {
	graph_clear_box(s, i, 10, 1, 190, 0);
	(*s->gfx_driver->Wait)(s, s->animation_delay / 2);
	process_sound_events(s);
      }

    case K_ANIMATE_RIGHT_OPEN :
      for(i = 319; i >= 0; i--) {
	graph_update_box(s, i, 10, 1, 190);
	(*s->gfx_driver->Wait)(s, s->animation_delay / 2);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_RIGHT_CLOSE_LEFT_OPEN :

      for(i = 319; i >= 0; i--) {
	graph_clear_box(s, i, 10, 1, 190, 0);
	(*s->gfx_driver->Wait)(s, s->animation_delay / 2);
	process_sound_events(s);
      }

    case K_ANIMATE_LEFT_OPEN :

      for(i = 0; i < 320; i++) {
	graph_update_box(s, i, 10, 1, 190);
	(*s->gfx_driver->Wait)(s, s->animation_delay / 2);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_TOP_CLOSE_BOTTOM_OPEN :

      for (i = 10; i < 200; i++) {
	graph_clear_box(s, 0, i, 320, 1, 0);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_BOTTOM_OPEN :

      for (i = 199; i >= 10; i--) {
	graph_update_box(s, 0, i, 320, 1);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_BOTTOM_CLOSE_TOP_OPEN :

      for (i = 199; i >= 10; i--) {
	graph_clear_box(s, 0, i, 320, 1, 0);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_TOP_OPEN :

      for (i = 10; i < 200; i++) {
	graph_update_box(s, 0, i, 320, 1);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_CENTER_CLOSE_F_BORDER_OPEN_F :

      for (i = 31; i >= 0; i--) {
	int height = i * 3;
	int width = i * 5;

	graph_clear_box(s, width, 10 + height, 5, 190 - 2*height, 0);
	graph_clear_box(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height, 0);

	graph_clear_box(s, width, 10 + height, 320 - 2*width, 3, 0);
	graph_clear_box(s, width, 200 - 3 - height, 320 - 2*width, 3, 0);

	(*s->gfx_driver->Wait)(s, 4 * s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_BORDER_OPEN_F :

      for (i = 0; i < 32; i++) {
	int height = i * 3;
	int width = i * 5;

	graph_update_box(s, width, 10 + height, 5, 190 - 2*height);
	graph_update_box(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height);

	graph_update_box(s, width, 10 + height, 320 - 2*width, 3);
	graph_update_box(s, width, 200 - 3 - height, 320 - 2*width, 3);

	(*s->gfx_driver->Wait)(s, 4 * s->animation_delay);
	process_sound_events(s);
      }

      break;


    case K_ANIMATE_BORDER_CLOSE_F_CENTER_OPEN_F :

      for (i = 0; i < 32; i++) {
	int height = i * 3;
	int width = i * 5;

	graph_clear_box(s, width, 10 + height, 5, 190 - 2*height, 0);
	graph_clear_box(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height, 0);

	graph_clear_box(s, width, 10 + height, 320 - 2*width, 4, 0);
	graph_clear_box(s, width, 200 - 4 - height, 320 - 2*width, 4, 0);

	(*s->gfx_driver->Wait)(s, 7 * s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_CENTER_OPEN_F :

      for (i = 31; i >= 0; i--) {
	int height = i * 3;
	int width = i * 5;

	graph_update_box(s, width, 10 + height, 5, 190 - 2*height);
	graph_update_box(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height);

	graph_update_box(s, width, 10 + height, 320 - 2 * width, 3);
	graph_update_box(s, width, 200 - 3 - height, 320 - 2 * width, 3);

	(*s->gfx_driver->Wait)(s, 7 * s->animation_delay);
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

	graph_clear_box(s, x * 10, 10 + y * 10, 10, 10, 0);
        if (remaining_checkers & 1)
	  s->gfx_driver->Wait(s, s->animation_delay / 4);

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

	graph_update_box(s, x * 10, 10 + y * 10, 10, 10);

	if (remaining_checkers & 1)
	  s->gfx_driver->Wait(s, s->animation_delay / 4);

	--remaining_checkers;
	process_sound_events(s);
      }

      break;


    default:
      if (s->pic_animate != K_ANIMATE_OPEN_SIMPLE)
	SCIkwarn(SCIkWARNING, "Unknown opening animation 0x%02x\n", s->pic_animate);

      graph_update_box(s, 0, 10, 320, 190);
    }

    s->pic_is_new = 0;
    s->pic_not_valid = 0;
    return;

  } /* if ((cast_list == 0) && (!s->pic_not_valid)) */

  graph_update_box(s, 0, 10, 320, 190); /* Just update and return */

  s->pic_not_valid = 0;

}

void
kShakeScreen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int shakes = PARAM_OR_ALT(0, 1);
  int i;

  for (i = 0; i < shakes; i++) {
    (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_ALL, 0, -10,0,0);
    (*s->gfx_driver->Wait)(s, 15000);
    (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_ALL, 0, 10,0,0);
    (*s->gfx_driver->Wait)(s, 15000);
  }
  (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_ALL, 0,0,0,0);
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
  int width = -1;
  int temp;
  int save_under = 0;
  char *text;
  resource_t *font_resource;
  port_t *port = s->ports[s->view_port];
  port_t save;
  int update_immediately = 1;

  save=*port;
  port->alignment = ALIGN_TEXT_LEFT;

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
    graph_update_box(s, port->xmin, port->ymin,
		     port->xmax - port->xmin + 1, port->ymax - port->ymin + 1);
    SCIkdebug(SCIkGRAPHICS, "Refreshing display...\n");
  }

  *port=save;

}


