/***************************************************************************
 input_ggi.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

***************************************************************************/
/* Input handler for libggi */

#include <config.h>
#ifdef HAVE_LIBGGI

#include <assert.h>
#include <uinput.h>
#include <ggi/ggi.h>
#include <ctype.h>


static struct {
  int keysym, keymod; /* Identifiers for the key/ key combo */
  unsigned char type; /* Type of the key */
  char key; /* The key to return */
} _remap_keys[] = {
  { GIIUC_Escape,       0,              SCI_EV_SPECIAL_KEY, SCI_K_ESC },
  { GIIK_P1,            0,              SCI_EV_SPECIAL_KEY, SCI_K_END },
  { GIIK_P2,            0,              SCI_EV_SPECIAL_KEY, SCI_K_DOWN },
  { GIIK_P3,            0,              SCI_EV_SPECIAL_KEY, SCI_K_PGDOWN },
  { GIIK_P4,            0,              SCI_EV_SPECIAL_KEY, SCI_K_LEFT },
  { GIIK_P5,            0,              SCI_EV_SPECIAL_KEY, SCI_K_CENTER },
  { GIIK_P6,            0,              SCI_EV_SPECIAL_KEY, SCI_K_RIGHT },
  { GIIK_P7,            0,              SCI_EV_SPECIAL_KEY, SCI_K_HOME },
  { GIIK_P8,            0,              SCI_EV_SPECIAL_KEY, SCI_K_UP },
  { GIIK_P9,            0,              SCI_EV_SPECIAL_KEY, SCI_K_PGUP },
  { GIIK_P0,            0,              SCI_EV_SPECIAL_KEY, SCI_K_INSERT },
  { GIIK_PDecimal,      0,              SCI_EV_SPECIAL_KEY, SCI_K_DELETE },
  { GIIK_End,           0,              SCI_EV_SPECIAL_KEY, SCI_K_END },
  { GIIK_Down,          0,              SCI_EV_SPECIAL_KEY, SCI_K_DOWN },
  { GIIK_PageDown,      0,              SCI_EV_SPECIAL_KEY, SCI_K_PGDOWN },
  { GIIK_Left,          0,              SCI_EV_SPECIAL_KEY, SCI_K_LEFT },
  { GIIK_Right,         0,              SCI_EV_SPECIAL_KEY, SCI_K_RIGHT },
  { GIIK_Home,          0,              SCI_EV_SPECIAL_KEY, SCI_K_HOME },
  { GIIK_Up,            0,              SCI_EV_SPECIAL_KEY, SCI_K_UP },
  { GIIK_PageUp,        0,              SCI_EV_SPECIAL_KEY, SCI_K_PGUP },
  { GIIK_Insert,        0,              SCI_EV_SPECIAL_KEY, SCI_K_INSERT },
  { GIIK_Delete,        0,              SCI_EV_SPECIAL_KEY, SCI_K_DELETE },
  { GIIK_F1,            0,              SCI_EV_SPECIAL_KEY, SCI_K_F1 },
  { GIIK_F2,            0,              SCI_EV_SPECIAL_KEY, SCI_K_F2 },
  { GIIK_F3,            0,              SCI_EV_SPECIAL_KEY, SCI_K_F3 },
  { GIIK_F4,            0,              SCI_EV_SPECIAL_KEY, SCI_K_F4 },
  { GIIK_F5,            0,              SCI_EV_SPECIAL_KEY, SCI_K_F5 },
  { GIIK_F6,            0,              SCI_EV_SPECIAL_KEY, SCI_K_F6 },
  { GIIK_F7,            0,              SCI_EV_SPECIAL_KEY, SCI_K_F7 },
  { GIIK_F8,            0,              SCI_EV_SPECIAL_KEY, SCI_K_F8 },
  { GIIK_F9,            0,              SCI_EV_SPECIAL_KEY, SCI_K_F9 },
  { GIIK_F10,           0,              SCI_EV_SPECIAL_KEY, SCI_K_F10 },
  { GIIUC_Grave,        GII_MOD_CTRL,   SCI_EV_SPECIAL_KEY, SCI_K_PANEL },
  { GIIK_PMinus,        GII_MOD_CTRL,   SCI_EV_SPECIAL_KEY, SCI_K_DEBUG },
  { GIIK_PMinus,        0,              SCI_EV_KEY,         '-' },
  { GIIK_PPlus,         0,              SCI_EV_KEY,         '+' },
  { GIIUC_BackSpace,    0,              SCI_EV_CTRL_KEY,    'H' },
  { GIIUC_Tab,          0,              SCI_EV_CTRL_KEY,    'I' },
  { GIIUC_Return,       0,              SCI_EV_CTRL_KEY,    'M' },
  { GIIK_Enter,         0,              SCI_EV_CTRL_KEY,    'M' },
  { GIIK_PEnter,        0,              SCI_EV_CTRL_KEY,    'M' },
  { GIIUC_1,            GII_MOD_SHIFT,  SCI_EV_KEY,         '!' },
  { GIIUC_2,            GII_MOD_SHIFT,  SCI_EV_KEY,         '@' },
  { GIIUC_3,            GII_MOD_SHIFT,  SCI_EV_KEY,         '#' },
  { GIIUC_4,            GII_MOD_SHIFT,  SCI_EV_KEY,         '$' },
  { GIIUC_5,            GII_MOD_SHIFT,  SCI_EV_KEY,         '%' },
  { GIIUC_6,            GII_MOD_SHIFT,  SCI_EV_KEY,         '^' },
  { GIIUC_7,            GII_MOD_SHIFT,  SCI_EV_KEY,         '&' },
  { GIIUC_8,            GII_MOD_SHIFT,  SCI_EV_KEY,         '*' },
  { GIIUC_9,            GII_MOD_SHIFT,  SCI_EV_KEY,         '(' },
  { GIIUC_0,            GII_MOD_SHIFT,  SCI_EV_KEY,         ')' },
  { GIIUC_Minus,        GII_MOD_SHIFT,  SCI_EV_KEY,         '_' },
  { GIIUC_Equal,        GII_MOD_SHIFT,  SCI_EV_KEY,         '+' },
  { GIIUC_BracketLeft,  GII_MOD_SHIFT,  SCI_EV_KEY,         '{' },
  { GIIUC_BracketRight, GII_MOD_SHIFT,  SCI_EV_KEY,         '}' },
  { GIIUC_Semicolon,    GII_MOD_SHIFT,  SCI_EV_KEY,         ':' },
  { GIIUC_Apostrophe,   GII_MOD_SHIFT,  SCI_EV_KEY,         '"' },
  { GIIUC_BackSlash,    GII_MOD_SHIFT,  SCI_EV_KEY,         '|' },
  { GIIUC_Comma,        GII_MOD_SHIFT,  SCI_EV_KEY,         '<' },
  { GIIUC_Period,       GII_MOD_SHIFT,  SCI_EV_KEY,         '>' },
  { GIIUC_Slash,        GII_MOD_SHIFT,  SCI_EV_KEY,         '?' },
  { 0,0,0,0 }
};


struct timeval _sci_ggi_redraw_loopt, _sci_ggi_loopt;
/* timer variables */

int _sci_ggi_double_visual;
ggi_visual_t _sci_ggi_last_visual;


#define SCI_TIMEVAL_ADD(timev, addusec)  \
  { timev.tv_usec += addusec;              \
  while (timev.tv_usec >= 1000000L) {    \
    timev.tv_usec -= 1000000L;           \
    timev.tv_sec++;                      \
  }}

#define SCI_TIMEVAL_LATER(later, earlier) \
  ((later.tv_sec == earlier.tv_sec)? (later.tv_usec >= earlier.tv_usec) \
    : (later.tv_sec >= earlier.tv_sec))

sci_event_t _sci_ggi_input_handler()
{
  sci_event_t event_redraw, event_loop;
  struct timeval temptime = {0,0};
  struct timeval curtime;

  event_loop.type = SCI_EV_CLOCK;
  event_redraw.type = SCI_EV_REDRAW;

  while (1) {

    if (ggiEventPoll(_sci_ggi_last_visual, emAll, &temptime)) {
      int mapseeker;
      ggi_event event;
      sci_event_t retval;
      
      ggiEventRead(_sci_ggi_last_visual, &event, emAll);
      switch (event.any.type) {
      case evKeyPress:
      case evKeyRepeat:
	mapseeker = 0;
	while (_remap_keys[mapseeker].keysym) {
	  if ((event.key.label == _remap_keys[mapseeker].keysym)
	      && ((event.key.modifiers & _remap_keys[mapseeker].keymod)
		  == _remap_keys[mapseeker].keymod)) {
	    retval.type = _remap_keys[mapseeker].type;
	    retval.key = _remap_keys[mapseeker].key;
	    return retval;
	  }
	  mapseeker++;
	}

	if (event.key.sym > 127) continue;
	if ((event.key.sym < 31) && !(event.key.modifiers & GII_MOD_CTRL)) continue;
	retval.type = SCI_EV_KEY;
	retval.key = event.key.label;

	//	fprintf(stderr,"%04x\n",event.key.modifiers);
	if (GII_MOD_ALT & event.key.modifiers)
	  retval.type = SCI_EV_ALT_KEY;
	else
	  if (GII_MOD_CTRL & event.key.modifiers)
	    retval.type = SCI_EV_CTRL_KEY;
	  else
	    if (!(GII_MOD_SHIFT & event.key.modifiers))
	      retval.key = event.key.sym;

	return retval;

      case evPtrButtonPress:
	retval.type = SCI_EV_MOUSE_CLICK;
	retval.key = event.pbutton.button;
	return retval;
      
      case evPtrAbsolute:
      sci_pointer_x = event.pmove.x;
      sci_pointer_y = event.pmove.y;
      if (_sci_ggi_double_visual) {
	sci_pointer_x >>= 1;
	sci_pointer_y >>= 1;
      }
      continue;

      case evPtrRelative:
      sci_pointer_x += event.pmove.x;
      sci_pointer_y += event.pmove.y;
      /* FIXME: This may make the pointer too fast on high res! */
      continue;
      }
    }


    gettimeofday(&curtime, NULL);

    if (SCI_TIMEVAL_LATER(curtime, _sci_ggi_loopt)) { /* time to send a loop event */

      if (SCI_TIMEVAL_LATER(_sci_ggi_loopt, _sci_ggi_redraw_loopt)) {
	/* wait- redraw_loop has to be sent before that */
	while (SCI_TIMEVAL_LATER(curtime, _sci_ggi_redraw_loopt))
	  SCI_TIMEVAL_ADD(_sci_ggi_redraw_loopt, sci_redraw_time);
	/* to prevent event overkills */
	return event_redraw;
      }

      /* else */
      while (SCI_TIMEVAL_LATER(curtime, _sci_ggi_loopt)) {
	SCI_TIMEVAL_ADD(_sci_ggi_loopt, sci_clock_time);
      }
      return event_loop;

    }

    if (SCI_TIMEVAL_LATER(curtime, _sci_ggi_redraw_loopt)) {
      /* wait- redraw_loop has to be sent before that */
      while (SCI_TIMEVAL_LATER(curtime, _sci_ggi_redraw_loopt))
	SCI_TIMEVAL_ADD(_sci_ggi_redraw_loopt, sci_redraw_time);
      /* to prevent event overkills */
      return event_redraw;
    }


    if (SCI_TIMEVAL_LATER(_sci_ggi_loopt, _sci_ggi_redraw_loopt))
      temptime = _sci_ggi_redraw_loopt;
    else
      temptime = _sci_ggi_loopt;

    temptime.tv_sec -= curtime.tv_sec;
    if ((temptime.tv_usec -= curtime.tv_usec) <= 0) {
      temptime.tv_sec--;
      temptime.tv_usec += 1000000;
    }

    assert(temptime.tv_sec >= 0);
    assert(temptime.tv_usec >= 0);
    assert(temptime.tv_usec < 1000000);
  }
}

void initInputGGI()
{
  gettimeofday(&_sci_ggi_redraw_loopt, NULL);
  _sci_ggi_loopt = _sci_ggi_redraw_loopt;
  /* reset timers, leave them at current time to send redraw events ASAP */

  _sci_input_handler = &_sci_ggi_input_handler;

}










/***************************************************************************/
/* GII STUFF                                                               */
/***************************************************************************/


gii_input_t _sci_gii_input_device;



sci_event_t _sci_gii_input_handler()
{
  sci_event_t event_redraw, event_loop;
  struct timeval temptime = {0,0};
  struct timeval curtime;

  event_loop.type = SCI_EV_CLOCK;
  event_redraw.type = SCI_EV_REDRAW;

  while (1) {

    if (giiEventPoll(_sci_gii_input_device, emAll, &temptime)) {
      int mapseeker;
      gii_event event;
      sci_event_t retval;
      
      giiEventRead(_sci_gii_input_device, &event, emAll);
      switch (event.any.type) {
      case evKeyPress:
      case evKeyRepeat:
	mapseeker = 0;
	while (_remap_keys[mapseeker].keysym) {
	  if ((event.key.label == _remap_keys[mapseeker].keysym)
	      && ((event.key.modifiers & _remap_keys[mapseeker].keymod)
		  == _remap_keys[mapseeker].keymod)) {
	    retval.type = _remap_keys[mapseeker].type;
	    retval.key = _remap_keys[mapseeker].key;
	    return retval;
	  }
	  mapseeker++;
	}

	if (event.key.sym > 127) continue;
	if ((event.key.sym < 31) && !(event.key.modifiers & GII_MOD_CTRL)) continue;
	retval.type = SCI_EV_KEY;
	retval.key = event.key.label;

	//	fprintf(stderr,"%04x\n",event.key.modifiers);
	if (GII_MOD_ALT & event.key.modifiers)
	  retval.type = SCI_EV_ALT_KEY;
	else
	  if (GII_MOD_CTRL & event.key.modifiers)
	    retval.type = SCI_EV_CTRL_KEY;
	  else
	    if (!(GII_MOD_SHIFT & event.key.modifiers))
	      retval.key = event.key.sym;

	return retval;

      case evPtrButtonPress:
	retval.type = SCI_EV_MOUSE_CLICK;
	retval.key = event.pbutton.button;
	return retval;
      
      case evPtrAbsolute:
      sci_pointer_x = event.pmove.x;
      sci_pointer_y = event.pmove.y;
      if (_sci_ggi_double_visual) {
	sci_pointer_x >>= 1;
	sci_pointer_y >>= 1;
      }
      continue;

      case evPtrRelative:
      sci_pointer_x += event.pmove.x;
      sci_pointer_y += event.pmove.y;
      /* FIXME: This may make the pointer too fast on high res! */
      continue;
      }
    }


    gettimeofday(&curtime, NULL);

    if (SCI_TIMEVAL_LATER(curtime, _sci_ggi_loopt)) { /* time to send a loop event */

      if (SCI_TIMEVAL_LATER(_sci_ggi_loopt, _sci_ggi_redraw_loopt)) {
	/* wait- redraw_loop has to be sent before that */
	while (SCI_TIMEVAL_LATER(curtime, _sci_ggi_redraw_loopt))
	  SCI_TIMEVAL_ADD(_sci_ggi_redraw_loopt, sci_redraw_time);
	/* to prevent event overkills */
	return event_redraw;
      }

      /* else */
      while (SCI_TIMEVAL_LATER(curtime, _sci_ggi_loopt)) {
	SCI_TIMEVAL_ADD(_sci_ggi_loopt, sci_clock_time);
      }
      return event_loop;

    }

    if (SCI_TIMEVAL_LATER(curtime, _sci_ggi_redraw_loopt)) {
      /* wait- redraw_loop has to be sent before that */
      while (SCI_TIMEVAL_LATER(curtime, _sci_ggi_redraw_loopt))
	SCI_TIMEVAL_ADD(_sci_ggi_redraw_loopt, sci_redraw_time);
      /* to prevent event overkills */
      return event_redraw;
    }


    if (SCI_TIMEVAL_LATER(_sci_ggi_loopt, _sci_ggi_redraw_loopt))
      temptime = _sci_ggi_redraw_loopt;
    else
      temptime = _sci_ggi_loopt;

    temptime.tv_sec -= curtime.tv_sec;
    if ((temptime.tv_usec -= curtime.tv_usec) <= 0) {
      temptime.tv_sec--;
      temptime.tv_usec += 1000000;
    }

    assert(temptime.tv_sec >= 0);
    assert(temptime.tv_usec >= 0);
    assert(temptime.tv_usec < 1000000);
  }
}



void _sci_gii_input_cleanup()
{
  fprintf(stderr,"SCI Input cleanup\n");
  if (giiClose(_sci_gii_input_device)) {
    fprintf(stderr,"GII input device could not be closed!\n");
  }
  if (giiExit())
    fprintf(stderr,"GII cleanup failed!\n");
}

int initInputGII()
{
  if (_sci_input_handler) return 1; /* Input handler has already been installed! */

  if (giiInit()) return 1;

  if ((_sci_gii_input_device = giiOpen("input-stdin", "n")) == NULL) {
    fprintf(stderr,"Failed to open input-stdin with GII\n");
    /* Well, in this case we probably don't have a console anyway, but what the hell... */
    giiExit();
    return 1;
  }

  atexit(&(_sci_gii_input_cleanup));

  gettimeofday(&_sci_ggi_redraw_loopt, NULL);
  _sci_ggi_loopt = _sci_ggi_redraw_loopt;
  /* reset timers, leave them at current time to send redraw events ASAP */

  _sci_input_handler = &_sci_gii_input_handler;

  return 0;
}

#endif /* HAVE_LIBGGI */

