/***************************************************************************
 uinput.h (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* unified input header file */

#ifndef _SCI_UINPUT_H
#define _SCI_UINPUT_H

#include <stdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#define SCI_INPUT_DEFAULT_CLOCKTIME 100000
#define SCI_INPUT_DEFAULT_REDRAWTIME 30000

typedef struct {
  int x;
  int y;
  short button;
} sci_mouseclick_t;


typedef struct {
  unsigned char type;
  char key;
} sci_event_t;

/* definitions for sci_keypress_t.type : */
#define SCI_EV_KEY 0
/* printable [key] */
#define SCI_EV_CTRL_KEY 1
/* CTRL-[key] */
#define SCI_EV_ALT_KEY 2
/* ALT-[key] */
#define SCI_EV_SPECIAL_KEY 4
/* One of the special keys described below */
#define SCI_EV_CLOCK 5
/* Loop timer overflow: The 'heart beat' of the game */
#define SCI_EV_REDRAW 6
/* Time to redraw the pointer */
#define SCI_EV_MOUSE_CLICK 7
/* Mouse click */
#define SCI_EV_NOEVENT 127
/* No event */

/* Special keys: */
#define SCI_K_ESC 0
#define SCI_K_END 1
#define SCI_K_DOWN 2
#define SCI_K_PGDOWN 3
#define SCI_K_LEFT 4
#define SCI_K_CENTER 5
#define SCI_K_RIGHT 6
#define SCI_K_HOME 7
#define SCI_K_UP 8
#define SCI_K_PGUP 9
#define SCI_K_INSERT 10
#define SCI_K_DELETE 11
#define SCI_K_F1 21
#define SCI_K_F2 22
#define SCI_K_F3 23
#define SCI_K_F4 24
#define SCI_K_F5 25
#define SCI_K_F6 26
#define SCI_K_F7 27
#define SCI_K_F8 28
#define SCI_K_F9 29
#define SCI_K_F10 30
#define SCI_K_PANEL 126
/* Ctrl-` */
#define SCI_K_DEBUG 127
/* Ctrl-PadMinus */

#define SCI_K_RETURN SCI_K_ENTER
#define SCI_K_CONSOLE SCI_K_PANEL



/*extern sci_event_t (*_sci_input_handler)(void);*/
/* The input handler for the main window */

extern int sci_pointer_x, sci_pointer_y;
/* The current coordinates of the mouse pointer */

extern int sci_have_pointer;
/* Is 0 if no pointing device is present */

extern long sci_clock_time;
/* Time (in microseconds) in between two 'heart beats' */
extern long sci_redraw_time;
/* Time (in usecs) until the next SCI_EV_REDRAW can be sent */



sci_event_t getEvent (struct _state *s);
/* Returns the next SCI_EV_* event
** Parameters: (struct state *) Current game state
** Returns   : (sci_event_t) The next event, which may be any of the
**             existing events.
*/


#endif /* _SCI_UINPUT_H */
