/***************************************************************************
 graphics_glx.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

 History:

   990402 - created from the remaining shards of "picture.c" which were not
            included in "graphics.c" (CJR)

***************************************************************************/
/* Like most of the graphics stuff, this is preliminary. There are some bad hacks
** in here, and they will be taken care of when this thing gets its long-
** deserved re-write.
** This stuff is roughly based on public domain code by Brian Paul.
*/


#include <config.h>
#ifdef HAVE_GLX

#include <uinput.h>
#include <engine.h>
#include <math.h>
#include <sys/time.h>
#include <X11/keysym.h>

#include <graphics_glx.h>

/*** Graphics driver ***/

int glx_init(state_t *s, picture_t pic);
void glx_shutdown(state_t *s);
void glx_redraw(state_t *s, int command, int x, int y, int xl, int yl);
void glx_wait(state_t *s, long usec);
sci_event_t glx_input_handler(state_t *s);

static unsigned short glx_palette_r[256];
static unsigned short glx_palette_g[256];
static unsigned short glx_palette_b[256];
static unsigned short glx_palette_a[256]; /* Alpha */
static unsigned short glx_palette_s[256]; /* Stencil */

int x_buckystate = SCI_EVM_INSERT; /* "bucky bits */

gfx_driver_t gfx_driver_glx = 
{
  "glx",
  glx_init,
  glx_shutdown,
  glx_redraw,
  NULL,
  glx_wait,
  glx_input_handler
};


int
x_map_key(int xkey)
{
  if ((xkey >= 'A') && (xkey <= 'Z'))
    return xkey;
  if ((xkey >= 'a') && (xkey <= 'z'))
    return xkey - 'a' + 'A';
  if ((xkey >= '0') && (xkey <= '9'))
    return xkey;

  switch (xkey) {
  case XK_Control_L:
  case XK_Control_R: x_buckystate ^= SCI_EVM_CTRL;
  case XK_Alt_L:
  case XK_Alt_R: x_buckystate ^= SCI_EVM_ALT;
  case XK_Caps_Lock:
  case XK_Shift_Lock: x_buckystate ^= SCI_EVM_CAPSLOCK;
  case XK_Scroll_Lock: x_buckystate ^= SCI_EVM_SCRLOCK;
  case XK_Num_Lock: x_buckystate ^= SCI_EVM_NUMLOCK;
  }
}

Cursor
x_empty_cursor(Display *display, Drawable drawable) /* Generates an empty X cursor */
{
  byte cursor_data[] = {0};
  XColor black = {0,0,0};
  Pixmap cursor_map;

  Cursor retval;

  cursor_map = XCreateBitmapFromData(display, drawable, cursor_data, 1, 1);

  retval = XCreatePixmapCursor(display, cursor_map, cursor_map, &black, &black, 0, 0);

  XFreePixmap(display, cursor_map);

  return retval;
}


int
glx_init_colors()
{
  int i;
  unsigned short temp_r[16];
  unsigned short temp_g[16];
  unsigned short temp_b[16];

  memset(glx_palette_a, sizeof(unsigned short) * 256, 0);

  for (i=0; i<16; i++) {

    temp_r[i] = (i & 0x04) ? 0xaaaa : 0;
    temp_g[i] = (i & 0x02) ? 0xaaaa : 0;
    temp_b[i] = (i & 0x01) ? 0xaaaa : 0;
    if (i & 0x08) {
      temp_r[i] += 0x5555;
      temp_g[i] += 0x5555;
      temp_b[i] += 0x5555;
    }

    if (i == 6) { /* Special exception for brown */
      temp_g[i] >>= 1;
    }
  }
  for (i=0; i< 256; i++) {

    glx_palette_r[i] = INTERCOL((temp_r[i & 0xf]), (temp_r[i >> 4]));
    glx_palette_g[i] = INTERCOL((temp_g[i & 0xf]), (temp_g[i >> 4]));
    glx_palette_b[i] = INTERCOL((temp_b[i & 0xf]), (temp_b[i >> 4]));
    glx_palette_s[i] = i;
  }
}

int
glx_init(state_t *s, picture_t pic)
{
  int attributes[] = { GLX_AUX_BUFFERS, 1, /* One aux buffer used for double buffering */
		       GLX_RGBA,
		       GLX_STENCIL_SIZE, 8,
		       None };

  GLXContext context;
  XVisualInfo *xvisinfo;
  int default_screen, num_aux_buffers;
  glx_state_t *x = malloc(sizeof(glx_state_t));

  s->graphics.glx_state = NULL;

  x->glx_display = XOpenDisplay(NULL);

  if (!x->glx_display) {
    fprintf(stderr,"FSCI-GLX: Could not open X connection!\n");
    free(x);
    return 1;
  }

  default_screen = DefaultScreen(x->glx_display);

  xvisinfo = glXChooseVisual(x->glx_display, default_screen, attributes);
  if (!xvisinfo) {
    fprintf(stderr,"FSCI-GLX: Could not get an RGB visual with one AUX buffer!\n");
    free(x);
    return 1;
  }

  glXGetConfig(x->glx_display, xvisinfo, GLX_AUX_BUFFERS, &num_aux_buffers);

  if (!num_aux_buffers) {
    fprintf(stderr,"This glx implementation does not provide auxiliary buffers.\n"
	    "Aborting...\n");
    exit (1);
  }

  x->width = 640;
  x->height = 400;

  x->glx_window = XCreateSimpleWindow(x->glx_display, RootWindow(x->glx_display, default_screen),
				      0, 0, x->width, x->height, 0, 0, 0);

  if (!x->glx_window) {
    fprintf(stderr,"FSCI-GLX: Could not create window!\n");
    free(x);
    return 1;
  }

  x->glx_context = glXCreateContext(x->glx_display, xvisinfo, NULL, 1);

  s->graphics.glx_state = x;

  XStoreName(x->glx_display, x->glx_window, "FreeSCI on GLX");
  XDefineCursor(x->glx_display, x->glx_window, x_empty_cursor(x->glx_display, x->glx_window));

  XSelectInput(x->glx_display, x->glx_window,
	       PointerMotionMask | StructureNotifyMask | ButtonPressMask
	       | ButtonReleaseMask | KeyPressMask);
  XMapWindow(x->glx_display, x->glx_window);

  glXMakeCurrent(x->glx_display, x->glx_window, x->glx_context);

  glShadeModel(GL_FLAT);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glOrtho(0.0, 1.0, 1.0, 0.0, 16.0, 0.0);
  glPixelZoom(2.0, -2.0);

  glx_init_colors();

  glClearStencil(0x0);
  glEnable(GL_STENCIL_TEST);

  glStencilFunc(GL_ALWAYS, 0, 0);

  glPixelTransferi(GL_MAP_COLOR, TRUE);
  glPixelMapusv(GL_PIXEL_MAP_S_TO_S, 256, glx_palette_s);
  glPixelMapusv(GL_PIXEL_MAP_I_TO_R, 256, glx_palette_r);
  glPixelMapusv(GL_PIXEL_MAP_I_TO_G, 256, glx_palette_g);
  glPixelMapusv(GL_PIXEL_MAP_I_TO_B, 256, glx_palette_b);

  {
    int i;
    glGetIntegerv(GL_AUX_BUFFERS, &i);
    printf("Using the GLX display target with %d aux buffers\n", i);
  }

  return 0;
}

void
glx_shutdown(state_t *s)
{
  glx_state_t *x = s->graphics.glx_state;

  if (x) {
    fprintf(stderr, "Shutting down GLX target.\n");
    glXDestroyContext(x->glx_display, x->glx_context);
    XDestroyWindow(x->glx_display, x->glx_window);
    XCloseDisplay(x->glx_display);
  }
}


void
graphics_draw_region_glx(Window target, Display *display,
			 byte *source,
			 int x, int y, int xl, int yl,
			 mouse_pointer_t *pointer, int pointer_x, int pointer_y)
{
  int i;

  glDrawBuffer(GL_AUX0);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 320);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, MAX(x,0));
  glPixelStorei(GL_UNPACK_SKIP_ROWS, MAX(y,0));

  glRasterPos2f(x / 320.0, y / 200.0);
  glDrawPixels(xl, yl, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, source);

  if (pointer) {

    glPixelStorei(GL_UNPACK_ROW_LENGTH, pointer->size_x);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    glRasterPos2f((pointer_x - pointer->hot_x) / 320.0, (pointer_y - pointer->hot_y) / 200.0);

    /* Copy the mouse pointer to the stencil buffer */
    glStencilFunc(GL_ALWAYS, 1, 0x1);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glDrawPixels(pointer->size_x, pointer->size_y, GL_STENCIL_INDEX,
		 GL_UNSIGNED_BYTE, pointer->bitmap);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);

    /* Now make sure that the "color_key" is ignored while drawing */
    glStencilFunc(GL_NOTEQUAL, pointer->color_key, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); /* might give some additional performance */

    /* Draw pointer */
    glDrawPixels(pointer->size_x, pointer->size_y, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, pointer->bitmap);

    glStencilFunc(GL_ALWAYS, 0, 0);
  }

  glFlush();

  glDrawBuffer(GL_FRONT);
  glReadBuffer(GL_AUX0);

  glCopyPixels(x, y, xl, yl, GL_COLOR);
}


void
glx_redraw(struct _state *s, int command, int x, int y, int xl, int yl)
{
  Window target = s->graphics.glx_state->glx_window;
  Display *display = s->graphics.glx_state->glx_display;
  int mp_x, mp_y, mp_size_x, mp_size_y;

  if (!target) {
    fprintf(stderr,"FSCI-GLX: No GLX target available- internal error!\n");
    exit(-1);
  }

  if (s->mouse_pointer) {
    mp_x = s->pointer_x - s->mouse_pointer->hot_x;
    mp_y = s->pointer_y - s->mouse_pointer->hot_y;
    mp_size_x = s->mouse_pointer->size_x;
    mp_size_y = s->mouse_pointer->size_y;
  } else { /* No mouse pointer */
    mp_x = s->pointer_x;
    mp_y = s->pointer_y;
    mp_size_x = mp_size_y = 0;
  }


  switch (command) {
  case GRAPHICS_CALLBACK_REDRAW_ALL:

    if (y) fprintf(stderr,"Starting to shake %d\n", y);
    glOrtho(0.0, 1.0, 1.0 - (y / 200.0), -(y/200.0), 16.0, 0.0);
    if (y) fprintf(stderr,"Stopping shake\n", y);
    graphics_draw_region_glx(target, display, s->pic->view,
			     0, 0, 320, 200,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    break;
  case GRAPHICS_CALLBACK_REDRAW_BOX:
    graphics_draw_region_glx(target, display, s->pic->view, /* Draw box */
			     x, y, xl, yl,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    break;
  case GRAPHICS_CALLBACK_REDRAW_POINTER:
    graphics_draw_region_glx(target, display, s->pic->view, /* Draw new pointer */
    			     mp_x, mp_y, mp_size_x, mp_size_y,
    			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    graphics_draw_region_glx(target, display, s->pic->view, /* Remove old pointer */
			     s->last_pointer_x,s->last_pointer_y,
			     s->last_pointer_size_x, s->last_pointer_size_y,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    break;
  default:
    fprintf(stderr,"glx_redraw: Invalid command %d\n", command);
  }

  s->last_pointer_size_x = mp_size_x;
  s->last_pointer_size_y = mp_size_y;
  s->last_pointer_x = mp_x;
  s->last_pointer_y = mp_y; /* Update mouse pointer status */

}

void
_glx_get_event(state_t *s, int eventmask, long wait_usec, sci_event_t *sci_event)
{
  XEvent event;
  Window window = s->graphics.glx_state->glx_window;
  Display *display = s->graphics.glx_state->glx_display;
  unsigned int *width = &(s->graphics.glx_state->width);
  unsigned int *height = &(s->graphics.glx_state->height);
  struct timeval ctime, timeout_time, sleep_time;
  int usecs_to_sleep;

  gettimeofday(&timeout_time, NULL);
  timeout_time.tv_usec += wait_usec;

  /* Calculate wait time */
  timeout_time.tv_sec += (timeout_time.tv_usec / 1000000);
  timeout_time.tv_usec %= 1000000;

  do {
    int redraw_pointer_request = 0;

    while (XCheckWindowEvent(display, window, eventmask, &event)) {
      switch (event.type) {

      case KeyPress: {
	sci_event->type = SCI_EVT_KEYBOARD;
	sci_event->buckybits = x_buckystate;
	sci_event->data = x_map_key(event.xkey.keycode);

	if (sci_event->data)
	  return;

	break;
      }

      case ButtonPress: {
	sci_event->type = SCI_EVT_MOUSE_PRESS;
	sci_event->buckybits = event.xkey.state;
	sci_event->data = 0;
	return;
      }

      case ButtonRelease: {
	sci_event->type = SCI_EVT_MOUSE_RELEASE;
	sci_event->buckybits = event.xkey.state;
	sci_event->data = 0;
	return;
      }

      case MotionNotify: {

	if (*width)
	  s->pointer_x = event.xmotion.x * 320 / *width;
	if (*height)
	  s->pointer_y = event.xmotion.y * 200 / *height;

	if (s->pointer_x != s->last_pointer_x
	    || s->pointer_y != s->last_pointer_y)
	  redraw_pointer_request = 1;
      }
      break;

      case ConfigureNotify: {
	glLoadIdentity();

	*width = event.xconfigure.width;
	*height = event.xconfigure.height;

	glPixelZoom(*width / 320.0, *height / -200.0);
	s->gfx_driver->Redraw(s, GRAPHICS_CALLBACK_REDRAW_ALL, 0, 0, 0, 0);
      }
      break;

      default:
	fprintf(stderr,"%s: Received unhandled event %04x\n", __FUNCTION__, event.type);
      }

    }

    gettimeofday(&ctime, NULL);

    usecs_to_sleep = (timeout_time.tv_sec > ctime.tv_sec)? 1000000 : 0;
    usecs_to_sleep += timeout_time.tv_usec - ctime.tv_usec;
    if (ctime.tv_sec > timeout_time.tv_sec) usecs_to_sleep = -1;


    if (usecs_to_sleep > 0) {

      if (usecs_to_sleep > 10000)
	usecs_to_sleep = 10000; /* Sleep for a maximum of 10 ms */

      sleep_time.tv_usec = usecs_to_sleep;
      sleep_time.tv_sec = 0;

      select(0, NULL, NULL, NULL, &sleep_time); /* Sleep. */
    }

    if (redraw_pointer_request)
      s->gfx_driver->Redraw(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0, 0, 0, 0);

  } while (usecs_to_sleep >= 0);

  if (sci_event)
    sci_event->type = SCI_EVT_NONE; /* No event. */
}


void
glx_wait(state_t* s, long usec)
{
  _glx_get_event(s, PointerMotionMask | StructureNotifyMask, usec, NULL);
}

sci_event_t
glx_input_handler(state_t *s)
{
  sci_event_t input;

  _glx_get_event(s,
		 PointerMotionMask | StructureNotifyMask | ButtonPressMask
		 | ButtonReleaseMask | KeyPressMask,
		 0, &input);

  return input;
}

#endif /* HAVE_LIBGGI */
