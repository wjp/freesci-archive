/***************************************************************************
 input_ggi.c Copyright (C) 1999 Christoph Reichenbach


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
#if 0
ggi_visual_t _sci_ggi_last_visual;
#endif

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
  struct timeval temptime = {0,0};
  struct timeval curtime;
  
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

    if (ggiEventPoll(s->graphics.ggi_visual, emAll, &temptime)) {
      ggi_event event;
      sci_event_t retval;
      
      ggiEventRead(s->graphics.ggi_visual, &event, emAll);
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

	  case GIIUC_Minus:
	  case GIIK_PMinus: retval.data = '-'; break;
	  case GIIUC_Plus:
	  case GIIK_PPlus: retval.data = '+'; break;

	  case GIIUC_Grave: retval.data = '`'; break;

	  case GIIK_ShiftL: buckybits |= SCI_EVM_LSHIFT; break;
	  case GIIK_ShiftR: buckybits |= SCI_EVM_RSHIFT; break;
	  case GIIK_CtrlR:
	  case GIIK_CtrlL: buckybits |= SCI_EVM_CTRL; break;
	  case GIIK_AltL:
	  case GIIK_AltR:
	  case GIIK_MetaL:
	  case GIIK_MetaR: buckybits |= SCI_EVM_ALT; break;
	  case GIIK_CapsLock: buckybits |= SCI_EVM_CAPSLOCK; break;
	  case GIIK_NumLock: buckybits^=SCI_EVM_NUMLOCK; break;
	  case GIIK_ScrollLock: buckybits^=SCI_EVM_SCRLOCK; break;
	  case GIIK_Insert: buckybits^=SCI_EVM_INSERT; break;
	  case GIIK_PEnter:
	  case GIIK_Enter: retval.data='\r'; break;
	  case GIIUC_Tab: retval.data='\t'; break;
	  case GIIUC_Space: retval.data=' '; break;
	  case GIIUC_BackSpace: retval.data=SCI_K_BACKSPACE; break;
	  case GIIK_F1: retval.data = SCI_K_F1; break;
	  case GIIK_F2: retval.data = SCI_K_F2; break;
	  case GIIK_F3: retval.data = SCI_K_F3; break;
	  case GIIK_F4: retval.data = SCI_K_F4; break;
	  case GIIK_F5: retval.data = SCI_K_F5; break;
	  case GIIK_F6: retval.data = SCI_K_F6; break;
	  case GIIK_F7: retval.data = SCI_K_F7; break;
	  case GIIK_F8: retval.data = SCI_K_F8; break;
	  case GIIK_F9: retval.data = SCI_K_F9; break;
	  case GIIK_F10: retval.data = SCI_K_F10; break;
	  case GIIUC_Escape: retval.data = SCI_K_ESC; break;

	    /*FIXME: Add all special keys in a sane way*/
	  default:
	    {
	      if(event.key.label>='a' && event.key.label<='z')
		retval.data=event.key.label-'a'+97;
	      if(event.key.label>='A' && event.key.label<='Z')
		retval.data=event.key.label-'A'+97;
	      if(event.key.label>='0' && event.key.label<='9')
		retval.data=event.key.label-'0'+48;
	    }
	  }
        if(retval.data==-1) continue;
        retval.buckybits=buckybits;

        return retval;

      case evKeyRelease:
        switch(event.key.label)
	  {
	  case GIIK_ShiftL: buckybits &= ~SCI_EVM_LSHIFT; break;
	  case GIIK_ShiftR: buckybits &= ~SCI_EVM_RSHIFT; break;
	  case GIIK_CtrlR:
	  case GIIK_CtrlL: buckybits &= ~SCI_EVM_CTRL; break;
	  case GIIK_AltL:
	  case GIIK_AltR:
	  case GIIK_MetaL:
	  case GIIK_MetaR: buckybits &= ~SCI_EVM_ALT; break;
	  }
	continue;
	
      case evPtrButtonPress:
        retval.type = SCI_EVT_MOUSE_PRESS;
        retval.data = event.pbutton.button;
        retval.buckybits=buckybits;

	if (event.pbutton.button == GII_PBUTTON_LEFT)
	  retval.buckybits |= SCI_EVM_CTRL;

	if (event.pbutton.button == GII_PBUTTON_RIGHT)
	  retval.buckybits |= SCI_EVM_LSHIFT | SCI_EVM_RSHIFT;
        return retval;
	
      case evPtrButtonRelease:
        retval.type = SCI_EVT_MOUSE_RELEASE;
        retval.data = event.pbutton.button;
        retval.buckybits=buckybits;

	if (event.pbutton.button == GII_PBUTTON_LEFT)
	  retval.buckybits |= SCI_EVM_CTRL;

	if (event.pbutton.button == GII_PBUTTON_RIGHT)
	  retval.buckybits |= SCI_EVM_LSHIFT | SCI_EVM_RSHIFT;

        return retval; 
      
      case evPtrAbsolute:
	s->pointer_x = event.pmove.x;
	s->pointer_y = event.pmove.y;
	if (_sci_ggi_double_visual) {
	  s->pointer_x >>= 1;
	  s->pointer_y >>= 1;
	}
	continue;
	
      case evPtrRelative:
	s->pointer_x += event.pmove.x;
	s->pointer_y += event.pmove.y;
	/* FIXME: This may make the pointer too fast on high res! */
	continue;
      }
    } else {
      sci_event_t retval;

      retval.type = SCI_EVT_NONE; /* Nothing happened */
      return retval;
    }

  }
}

void initInputGGI()
{
  gettimeofday(&_sci_ggi_redraw_loopt, NULL);
  _sci_ggi_loopt = _sci_ggi_redraw_loopt;
  buckybits = SCI_EVM_INSERT; /* Start up in "insert" mode */
  pending = -1;
  /* reset timers, leave them at current time to send redraw events ASAP */

}


#endif /* HAVE_LIBGGI */

