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
#include <engine.h>

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

static int buckybits;
static int pending;

sci_event_t _sci_ggi_input_handler(state_t *s)
{
  sci_event_t event_redraw, event_loop;
  struct timeval temptime = {0,0};
  struct timeval curtime;

  event_loop.type = SCI_EVT_CLOCK;
  event_redraw.type = SCI_EVT_REDRAW;

  if(pending!=-1)
  {
  	sci_event_t r;
  	r.type=SCI_EVT_KEYBOARD;
  	r.data=pending;
  	pending=-1;
  	r.buckybits=buckybits;
  	return r;
  }

  while (1) {

    if (ggiEventPoll(_sci_ggi_last_visual, emAll, &temptime)) {
      ggi_event event;
      sci_event_t retval;
      
      ggiEventRead(_sci_ggi_last_visual, &event, emAll);
      switch (event.any.type) {
      case evKeyPress:
      case evKeyRepeat:
	retval.type = SCI_EVT_KEYBOARD;
        retval.data=-1;
        switch(event.key.label)
	  {
	  case GIIK_P4:
	  case GIIK_Left: retval.data=0; pending=SCI_K_LEFT;; break;
	  case GIIK_P6:
	  case GIIK_Right: retval.data=0; pending=SCI_K_RIGHT; break;
	  case GIIK_P8:
	  case GIIK_Up: retval.data=0; pending=SCI_K_UP; break;
	  case GIIK_P2:
	  case GIIK_Down: retval.data=0; pending=SCI_K_DOWN; break;
	  case GIIK_P7:
	  case GIIK_Home: retval.data=0; pending=SCI_K_HOME; break;
	  case GIIK_P1:
	  case GIIK_End: retval.data=0; pending=SCI_K_END; break;
	  case GIIK_P9:
	  case GIIK_PageUp: retval.data=0; pending=SCI_K_PGUP; break;
	  case GIIK_P3:
	  case GIIK_PageDown: retval.data=0; pending=SCI_K_PGDOWN; break;
	  case GIIK_P5: retval.data=SCI_K_CENTER; break;

	  case GIIK_ShiftL: buckybits^=SCI_EVM_LSHIFT; break;
	  case GIIK_ShiftR: buckybits^=SCI_EVM_RSHIFT; break;
	  case GIIK_CtrlR:
	  case GIIK_CtrlL: buckybits^=SCI_EVM_CTRL; break;
	  case GIIK_AltL:
	  case GIIK_AltR:
	  case GIIK_MetaL:
	  case GIIK_MetaR: buckybits^=SCI_EVM_ALT; break;
	  case GIIK_CapsLock: buckybits^=SCI_EVM_CAPSLOCK; break;
	  case GIIK_NumLock: buckybits^=SCI_EVM_NUMLOCK; break;
	  case GIIK_ScrollLock: buckybits^=SCI_EVM_SCRLOCK; break;
	  case GIIK_Insert: buckybits^=SCI_EVM_INSERT; break;
	  case GIIK_PEnter:
	  case GIIK_Enter: retval.data='\r'; break;
	  case GIIUC_Tab: retval.data='\t'; break;
	  case GIIUC_Space: retval.data=' '; break;
	  case GIIUC_BackSpace: retval.data=SCI_K_BACKSPACE; break;

	    /*FIXME: Add all special keys in a sane way*/
	  default:
	    {
	      if(event.key.sym>='a' && event.key.sym<='z')
		retval.data=event.key.sym-'a'+97;
	      if(event.key.sym>='A' && event.key.sym<='Z')
		retval.data=event.key.sym-'A'+97;
	      if(event.key.sym>='0' && event.key.sym<='9')
		retval.data=event.key.sym-'0'+48;
	    }
	  }
        if(retval.data==-1) continue;
        retval.buckybits=buckybits;
        return retval;
	
      case evPtrButtonPress:
        retval.type = SCI_EVT_MOUSE_PRESS;
        retval.data = event.pbutton.button;
        retval.buckybits=buckybits;
        return retval;
	
      case evPtrButtonRelease:
        retval.type = SCI_EVT_MOUSE_RELEASE;
        retval.data = event.pbutton.button;
        retval.buckybits=buckybits;
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
  buckybits=0;
  pending=-1;
  /* reset timers, leave them at current time to send redraw events ASAP */

}










/***************************************************************************/
/* GII STUFF                                                               */
/***************************************************************************/


gii_input_t _sci_gii_input_device;



sci_event_t _sci_gii_input_handler(state_t* s)
{
  sci_event_t event_redraw, event_loop;
  struct timeval temptime = {0,0};
  struct timeval curtime;

  event_loop.type = SCI_EVT_CLOCK;
  event_redraw.type = SCI_EVT_REDRAW;

  while (1) {

    if (giiEventPoll(_sci_gii_input_device, emAll, &temptime)) {
      gii_event event;
      sci_event_t retval;
      
      giiEventRead(_sci_gii_input_device, &event, emAll);
      switch (event.any.type) {
      case evKeyPress:
      case evKeyRepeat:
	retval.type = SCI_EVT_KEYBOARD;
        retval.data=-1;
        switch(event.key.label)
	  {
	  case GIIK_ShiftL: buckybits^=SCI_EVM_LSHIFT; break;
	  case GIIK_ShiftR: buckybits^=SCI_EVM_RSHIFT; break;
	  case GIIK_CtrlR:
	  case GIIK_CtrlL: buckybits^=SCI_EVM_CTRL; break;
	  case GIIK_AltL:
	  case GIIK_AltR:
	  case GIIK_MetaL:
	  case GIIK_MetaR: buckybits^=SCI_EVM_ALT; break;
	  case GIIK_CapsLock: buckybits^=SCI_EVM_CAPSLOCK; break;
	  case GIIK_NumLock: buckybits^=SCI_EVM_NUMLOCK; break;
	  case GIIK_ScrollLock: buckybits^=SCI_EVM_SCRLOCK; break;
	  case GIIK_Insert: buckybits^=SCI_EVM_INSERT; break;
	  case GIIK_Enter: retval.data='\r'; break;
	  case GIIUC_Tab: retval.data='\t'; break;
	  case GIIK_Left: retval.data=75;
	  case GIIK_Right: retval.data=77;
	    /*FIXME: Add all special keys in a sane way*/
	  default:
	    {
	      if(event.key.sym>='a' && event.key.sym<='z')
		retval.data=event.key.sym-'a'+97;
	      if(event.key.sym>='A' && event.key.sym<='Z')
		retval.data=event.key.sym-'A'+97;
	      if(event.key.sym>='0' && event.key.sym<='9')
		retval.data=event.key.sym-'0'+48;
	    }
	  }
        if(retval.data==-1) continue;
        retval.buckybits=buckybits;
        return retval;
	
      case evPtrButtonPress:
        retval.type = SCI_EVT_MOUSE_PRESS;
        retval.buckybits=buckybits;
        retval.data = event.pbutton.button;
        return retval;
	
      case evPtrButtonRelease:
        retval.type = SCI_EVT_MOUSE_RELEASE;
        retval.buckybits=buckybits;
        retval.data = event.pbutton.button;
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
#if 0  
  if (_sci_input_handler) return 1; /* Input handler has already been installed! */
#endif

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

#if 0
  _sci_input_handler = &_sci_gii_input_handler;
#endif

  return 0;
}

#endif /* HAVE_LIBGGI */

