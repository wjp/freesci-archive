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


struct _state;

#define SCI_INPUT_DEFAULT_CLOCKTIME 100000
#define SCI_INPUT_DEFAULT_REDRAWTIME 30000


typedef struct {
  int type;
  int data;
  int buckybits;
} sci_event_t;

/*Values for type*/
#define SCI_EVT_NONE            0
#define SCI_EVT_MOUSE_PRESS     (1<<0)
#define SCI_EVT_MOUSE_RELEASE   (1<<1)
#define SCI_EVT_KEYBOARD        (1<<2)
#define SCI_EVT_JOYSTICK        (1<<6)
#define SCI_EVT_SAID            (1<<7)
/*Fake values for other events*/
#define SCI_EVT_ERROR           (1<<10)


/* Keycodes of special keys: */
#define SCI_K_ESC 27
#define SCI_K_BACKSPACE 8
#define SCI_K_ENTER 13
#define SCI_K_END 79
#define SCI_K_DOWN 80
#define SCI_K_PGDOWN 81
#define SCI_K_LEFT 75
#define SCI_K_CENTER 53 /*Note: this is the same as '5'. Dunno what should be done here...*/
#define SCI_K_RIGHT 77
#define SCI_K_HOME 71
#define SCI_K_UP 72
#define SCI_K_PGUP 73
#define SCI_K_INSERT 82
#define SCI_K_DELETE 83
#define SCI_K_F1 59
#define SCI_K_F2 60
#define SCI_K_F3 61
#define SCI_K_F4 62
#define SCI_K_F5 63
#define SCI_K_F6 64
#define SCI_K_F7 65
#define SCI_K_F8 66
#define SCI_K_F9 67
#define SCI_K_F10 68

/*Values for buckybits */
#define SCI_EVM_RSHIFT          (1<<0)
#define SCI_EVM_LSHIFT          (1<<1)
#define SCI_EVM_CTRL            (1<<2)
#define SCI_EVM_ALT             (1<<3)
#define SCI_EVM_SCRLOCK         (1<<4)
#define SCI_EVM_NUMLOCK         (1<<5)
#define SCI_EVM_CAPSLOCK        (1<<6)
#define SCI_EVM_INSERT          (1<<7)


extern long sci_clock_time;
/* Time (in microseconds) in between two 'heart beats' */
extern long sci_redraw_time;
/* Time (in usecs) until the next SCI_EV_REDRAW can be sent */

#endif /* _SCI_UINPUT_H */
