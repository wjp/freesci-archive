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
x_unmap_key(Display *display, int keycode)
{
  KeySym xkey = XKeycodeToKeysym(display, keycode, 0);

  switch (xkey) {
  case XK_Control_L:
  case XK_Control_R: x_buckystate &= ~SCI_EVM_CTRL; return 0;
  case XK_Alt_L:
  case XK_Alt_R: x_buckystate &= ~SCI_EVM_ALT; return 0;
  case XK_Shift_L: x_buckystate &= ~SCI_EVM_LSHIFT; return 0;
  case XK_Shift_R: x_buckystate &= ~SCI_EVM_RSHIFT; return 0;
  }

  return 0;
}


int
x_map_key(Display *display, int keycode)
{
  KeySym xkey = XKeycodeToKeysym(display, keycode, 0);

  if ((xkey >= 'A') && (xkey <= 'Z'))
    return xkey;
  if ((xkey >= 'a') && (xkey <= 'z'))
    return xkey;
  if ((xkey >= '0') && (xkey <= '9'))
    return xkey;

  switch (xkey) {
  case XK_BackSpace: return SCI_K_BACKSPACE;
  case XK_Tab: return 9;
  case XK_Escape: return SCI_K_ESC;
  case XK_Return:
  case XK_KP_Enter: return SCI_K_ENTER;

  case XK_KP_Decimal: return SCI_K_DELETE;
  case XK_KP_0:
  case XK_KP_Insert: return SCI_K_INSERT;
  case XK_KP_End:
  case XK_KP_1: return SCI_K_END;
  case XK_Down:
  case XK_KP_Down:
  case XK_KP_2: return SCI_K_DOWN;
  case XK_KP_Page_Down:
  case XK_KP_3: return SCI_K_PGDOWN;
  case XK_Left:
  case XK_KP_Left:
  case XK_KP_4: return SCI_K_LEFT;
  case XK_KP_5: return SCI_K_CENTER;
  case XK_Right:
  case XK_KP_Right:
  case XK_KP_6: return SCI_K_RIGHT;
  case XK_KP_Home:
  case XK_KP_7: return SCI_K_HOME;
  case XK_Up:
  case XK_KP_Up:
  case XK_KP_8: return SCI_K_UP;
  case XK_KP_Page_Up:
  case XK_KP_9: return SCI_K_PGUP;

  case XK_F1: return SCI_K_F1;
  case XK_F2: return SCI_K_F2;
  case XK_F3: return SCI_K_F3;
  case XK_F4: return SCI_K_F4;
  case XK_F5: return SCI_K_F5;
  case XK_F6: return SCI_K_F6;
  case XK_F7: return SCI_K_F7;
  case XK_F8: return SCI_K_F8;
  case XK_F9: return SCI_K_F9;
  case XK_F10: return SCI_K_F10;

  case XK_Control_L:
  case XK_Control_R: x_buckystate |= SCI_EVM_CTRL; return 0;
  case XK_Alt_L:
  case XK_Alt_R: x_buckystate |= SCI_EVM_ALT; return 0;
  case XK_Caps_Lock:
  case XK_Shift_Lock: x_buckystate ^= SCI_EVM_CAPSLOCK; return 0;
  case XK_Scroll_Lock: x_buckystate ^= SCI_EVM_SCRLOCK; return 0;
  case XK_Num_Lock: x_buckystate ^= SCI_EVM_NUMLOCK; return 0;
  case XK_Shift_L: x_buckystate |= SCI_EVM_LSHIFT; return 0;
  case XK_Shift_R: x_buckystate |= SCI_EVM_RSHIFT; return 0;

  case XK_KP_Add: return '+';
  case XK_KP_Divide: return '/';
  case XK_KP_Subtract: return '-';
  case XK_KP_Multiply: return '*';

  case ',':
  case '.':
  case '/':
  case '\\':
  case ';':
  case '\'':
  case '[':
  case ']':
  case '`':
  case '-':
  case '=':
  case '<':
  case ' ':
    return xkey;
  }

  sciprintf("Unknown X keysym: %04x\n", xkey);
  return 0;
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
  int attributes[] = { GLX_DOUBLEBUFFER,
		       GLX_RGBA,
		       GLX_STENCIL_SIZE, 8,
		       None };

  GLXContext context;
  XVisualInfo *xvisinfo;
  XSetWindowAttributes win_attr;
  int default_screen, num_aux_buffers;
  glx_state_t *x = sci_malloc(sizeof(glx_state_t));

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
    fprintf(stderr,"FSCI-GLX: Could not get a double-buffered RGBA visual!\n");
    free(x);
    return 1;
  }

  x->width = 320;
  x->height = 200;

  win_attr.colormap = XCreateColormap(x->glx_display, RootWindow(x->glx_display, default_screen),
				      xvisinfo->visual, AllocNone);
  win_attr.event_mask = PointerMotionMask | StructureNotifyMask | ButtonPressMask
    | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
  win_attr.background_pixel = win_attr.border_pixel = 0;

  x->glx_window = XCreateWindow(x->glx_display, RootWindow(x->glx_display, default_screen),
				0, 0, x->width, x->height, 0, xvisinfo->depth, InputOutput,
				xvisinfo->visual, (CWBackPixel | CWBorderPixel | CWColormap | CWEventMask),
				&win_attr);

  if (!x->glx_window) {
    fprintf(stderr,"FSCI-GLX: Could not create window!\n");
    free(x);
    return 1;
  }

  XSync(x->glx_display, False);
  x->glx_context = glXCreateContext(x->glx_display, xvisinfo, NULL, GL_TRUE);
  XSync(x->glx_display, False);

  XFree((char *) xvisinfo);

  s->graphics.glx_state = x;

  XStoreName(x->glx_display, x->glx_window, "FreeSCI on GLX");
  XDefineCursor(x->glx_display, x->glx_window, x_empty_cursor(x->glx_display, x->glx_window));

  XMapWindow(x->glx_display, x->glx_window);
  glXMakeCurrent(x->glx_display, x->glx_window, x->glx_context);

  glShadeModel(GL_FLAT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glOrtho(0.0, 1.0, 1.0, 0.0, 16.0, 0.0);
  glPixelZoom(1.0, 1.0);

  glx_init_colors();

  glClearStencil(0x0);

  glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
  glPixelMapusv(GL_PIXEL_MAP_S_TO_S, 256, glx_palette_s);
  glPixelMapusv(GL_PIXEL_MAP_I_TO_R, 256, glx_palette_r);
  glPixelMapusv(GL_PIXEL_MAP_I_TO_G, 256, glx_palette_g);
  glPixelMapusv(GL_PIXEL_MAP_I_TO_B, 256, glx_palette_b);

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


static void
graphics_draw_region_glx(Window target, Display *display,
			 byte *source,  int vis_width, int vis_height,
			 int x, int y, int xl, int yl,
			 mouse_pointer_t *pointer, int pointer_x, int pointer_y)
{
  int i;

  glPixelZoom(vis_width / 320.0, vis_height / -200.0);

  glDrawBuffer(GL_BACK);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 320);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, MAX(x,0));
  glPixelStorei(GL_UNPACK_SKIP_ROWS, MAX(y,0));

  glRasterPos2f(x / 320.0, y / 200.0);
  glDrawPixels(xl, yl, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, source);

  if (pointer) {

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0);

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
    glDisable(GL_STENCIL_TEST);
  }

  glFlush();
  glReadBuffer(GL_BACK);
  glDrawBuffer(GL_FRONT);

  glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

  glPixelZoom(1.0, 1.0);

  glRasterPos2f(x / 320.0, ((y+yl) * (1 / (200.0))));

  x = (x * vis_width) / 320;
  y = (y * vis_height) / 200;
  xl = (xl * vis_width) / 320;
  yl = (yl * vis_height) / 200;

  glCopyPixels(x, (vis_height-(y+yl)), xl, yl, GL_COLOR);

  glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

  glFlush();
}


void
glx_redraw(struct _state *s, int command, int x, int y, int xl, int yl)
{
  Window target = s->graphics.glx_state->glx_window;
  Display *display = s->graphics.glx_state->glx_display;
  int mp_x, mp_y, mp_size_x, mp_size_y;
  int wsize_x = s->graphics.glx_state->width;
  int wsize_y = s->graphics.glx_state->height;

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
    graphics_draw_region_glx(target, display, s->pic->view, wsize_x, wsize_y,
			     0, 0, 320, 200,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    break;
  case GRAPHICS_CALLBACK_REDRAW_BOX:
    graphics_draw_region_glx(target, display, s->pic->view, wsize_x, wsize_y, /* Draw box */
			     x, y, xl, yl,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    break;
  case GRAPHICS_CALLBACK_REDRAW_POINTER:
    graphics_draw_region_glx(target, display, s->pic->view, wsize_x, wsize_y, /* Draw new pointer */
    			     mp_x, mp_y, mp_size_x, mp_size_y,
    			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    graphics_draw_region_glx(target, display, s->pic->view, wsize_x, wsize_y, /* Remove old pointer */
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
	sci_event->data = x_map_key(display, event.xkey.keycode);

	if (sci_event->data)
	  return;

	break;
      }

      case KeyRelease:
	x_unmap_key(display, event.xkey.keycode);
	break;

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
	glMatrixMode(GL_PROJECTION);

	*width = event.xconfigure.width;
	*height = event.xconfigure.height;

	glViewport(0,0,*width, *height);

	glLoadIdentity();
	/*	glOrtho(0.0, 1.0, 1.0, 0.0, 16.0, 0.0);*/

	glPixelZoom(*width / 320.0, *height / -200.0);
	s->gfx_driver->Redraw(s, GRAPHICS_CALLBACK_REDRAW_ALL, 0, 0, 0, 0);
      }
      break;

      default:
	fprintf(stderr,"_glx_get_event(): Received unhandled event %04x\n", event.type);
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
		 | ButtonReleaseMask | KeyPressMask | KeyReleaseMask,
		 0, &input);

  return input;
}

#endif /* HAVE_LIBGGI */
