/***************************************************************************
 graphics_dgfx.c Copyright (C) 1999, 2000 Rink Springer


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

    Rink Springer [RS] [rink@springer.cx]

***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_DGFX

#include <graphics_dgfx.h>
#include <uinput.h>
#include <engine.h>
#include <math.h>
#include <sys/time.h>
#include <kdebug.h>

#include <sci_dos.h>
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <stdlib.h>
#include <string.h>
#include <sys/movedata.h>
#include <sys/segments.h>

#define DGFX_KEY_EVENT_BUFSIZE  6

static int keyboard_data_start(void) { };

volatile char dgfx_keyevent[DGFX_KEY_EVENT_BUFSIZE];
volatile char dgfx_keyevent_marker;
volatile char dgfx_temp;

static int keyboard_data_end(void) { };

static int keyboard_handler_start(void) { };

int dgfx_shift_state;

/*
 * keyboard_handler()
 *
 * this will be the new keyboard irq! don't we all just LOVE DOS! <g>
 *
 */
keyboard_handler() {
  /* get the char */
  dgfx_temp=inportb(0x60);

  /* still buffer space left? */
  if(dgfx_keyevent_marker<DGFX_KEY_EVENT_BUFSIZE) {
    /* yup. increment the counter */
    dgfx_keyevent_marker++;

    /* and add the key */
    dgfx_keyevent[dgfx_keyevent_marker]=dgfx_temp;
  }

  /* signal eoi */
  outportb(0x20,0x20);
}

static int keyboard_handler_end(void) { };

gfx_driver_t gfx_driver_dgfx =
{
  "dgfx",
  dgfx_init,
  dgfx_shutdown,
  dgfx_redraw,
  NULL,
  dgfx_wait,
  dgfx_get_event
};

static _go32_dpmi_seginfo wrapper,old_kbd_vector;

char* dgfx_virtualmem;
guchar dgfx_nofrows,dgfx_oldmode,dgfx_mouse_installed,dgfx_mouse_buttonstat;

state_t* dgfx_state;

#define NULL_REC_SIZE 640
static byte _null_rec[NULL_REC_SIZE]; /* Initialized to zero */

void initColors() {
  int i;
  int map;
  struct DGFX_RGB {
    guchar r;
    guchar g;
    guchar b;
  };
  struct DGFX_RGB vcal[16];
  struct DGFX_RGB color;

  for (i=0; i<16; i++) {
    vcal[i].r = (i & 0x04) ? 0xaaaa : 0;
    vcal[i].g = (i & 0x02) ? 0xaaaa : 0;
    vcal[i].b = (i & 0x01) ? 0xaaaa : 0;
    if (i & 0x08) {
      vcal[i].r += 0x5555;
      vcal[i].g += 0x5555;
      vcal[i].b += 0x5555;
    }
    if (i == 6) { /* Special exception for brown */
      vcal[i].g >>= 1;
    }
  }

  for (i=0; i< 256; i++) {
    color.r = INTERCOL((vcal[i & 0xf].r), (vcal[i >> 4].r));
    color.g = INTERCOL((vcal[i & 0xf].g), (vcal[i >> 4].g));
    color.b = INTERCOL((vcal[i & 0xf].b), (vcal[i >> 4].b));

    outportb(0x3c8,i);             /* tell dac the color value */
    outportb(0x3c9,color.r);       /* tell dac the red value */
    outportb(0x3c9,color.g);       /* tell dac the green value */
    outportb(0x3c9,color.b);       /* tell dac the blue value */
  }
}

int
dgfx_init(state_t* s, picture_t pic) {
  union REGS r;

  dgfx_state=s;

  /* first allocate 64KB of temponary data */
  if((dgfx_virtualmem=g_malloc0(64000))==NULL) {
    /* this failed... say it's bad */
    return 1;
  }

  /* first get all old information */
  memset(&r,0,sizeof(union REGS));
  r.x.ax=0x1130;              /* video bios: get font information */
  r.h.bh=0;
  int86(0x10,&r,&r);
  dgfx_nofrows=r.h.dl;

  memset(&r,0,sizeof(union REGS));
  r.h.ah=0xf;                 /* video bios: get current video mode */
  int86(0x10,&r,&r);
  dgfx_oldmode=r.h.al;

  /* and visit the new video mode */
  memset(&r,0,sizeof(union REGS));
  r.x.ax=0x13;                /* bios: set video mode */
  int86(0x10,&r,&r);

  memset(_null_rec, 0, NULL_REC_SIZE);

  /* now initialize the mouse */
  dgfx_mouse_installed=0;

  memset(&r,0,sizeof(union REGS));
  r.x.ax=0x0;                     /* mouse driver: reset and return status */
  int86(0x33,&r,&r);
  /* check if driver installed */
  if (r.x.ax==0xffff) {
    /* it is! set the flag */
    dgfx_mouse_installed=1;
  }

  initColors();

  dgfx_mouse_buttonstat=0;
  dgfx_check_mouse_event(NULL);

  /* use the 'under water' console :-) */
  con_passthrough=0;

  /* initialize the keyboard event marker */
  dgfx_keyevent_marker=0;

  /* get the old keyboard vector */
  _go32_dpmi_get_protected_mode_interrupt_vector(0x9,&old_kbd_vector);

  /* get a wrapper to wrap to my interrupt handler */
  wrapper.pm_offset = (int)keyboard_handler;
  wrapper.pm_selector =_go32_my_cs();
  _go32_dpmi_allocate_iret_wrapper(&wrapper);

  /* lock our stuff */
  _go32_dpmi_lock_code(keyboard_handler_start,(long)keyboard_handler_end-(long)keyboard_handler_start);
  _go32_dpmi_lock_data((void *)&dgfx_keyevent,DGFX_KEY_EVENT_BUFSIZE);
  _go32_dpmi_lock_data((void *)&dgfx_keyevent_marker,sizeof(char));
  _go32_dpmi_lock_data((void *)&dgfx_temp,sizeof(char));

  /* and activate the new one */
  _go32_dpmi_set_protected_mode_interrupt_vector(0x9,&wrapper);

  dgfx_shift_state=0;
  
  return 0;
}

void
dgfx_shutdown(state_t *s) {
  union REGS r;

  /* unhook keyboard irq */
  _go32_dpmi_set_protected_mode_interrupt_vector(0x9,&old_kbd_vector);
  _go32_dpmi_free_iret_wrapper(&wrapper);

  /* free the extra buffer */
  if (dgfx_virtualmem!=NULL) {
    g_free(dgfx_virtualmem);
    dgfx_virtualmem=NULL;
  }

  memset(&r,0,sizeof(union REGS));
  r.x.ax=dgfx_oldmode;       /* bios: set video mode */
  int86(0x10,&r,&r);

  if (dgfx_nofrows!=0x18) {  /* was the user in 25 mode line? */
    /* no, so set the 8x8 font */
    memset(&r,0,sizeof(union REGS));
    r.x.ax=0x1112;           /* video bios: set 8x8 chars */
    r.h.bl=0;
    int86(0x10,&r,&r);
  }
}

void
dgfx_wait(state_t* s, long usec) {
  /*    usleep(usec); */
}

void
graphics_draw_region_dgfx(byte *data, int x, int y, int xl, int yl,
             mouse_pointer_t *pointer, int pointer_x, int pointer_y)
{
  int xc, yc, pos = 0;
  int bytelen;
  int index, counter;
  int xend, yend;
  int pointer_end_x, pointer_end_y;
  int pointer_x_affected = 0; /* set to 1 if the pointer has to be drawn at all */
  int a,b;
  guchar c;

  if (x < 0) {
    xl += x;
    x = 0;
  }

  if (y < 0) {
    yl += y;
    y = 0;
  }

  xend = x + xl;
  yend = y + yl;

  if (xend > SCI_SCREEN_WIDTH)
    xend = SCI_SCREEN_WIDTH;

  if (yend > SCI_SCREEN_HEIGHT)
    yend = SCI_SCREEN_HEIGHT;

  if (pointer) {
    pointer_x -= pointer->hot_x;
    pointer_y -= pointer->hot_y; /* Adjust hot spot */
    pointer_end_x = pointer_x + pointer->size_x;
    pointer_end_y = pointer_y + pointer->size_y;

    if ((pointer_x >= x) && (pointer_x < xend))
      pointer_x_affected = 1; /* Pointer might have to be drawn */

    if ((pointer_end_x >= x) && (pointer_end_x < xend))
      pointer_x_affected = 1; /* Pointer might have to be drawn */

    if ((pointer_x <= x) && (pointer_end_x >= xend))
      pointer_x_affected = 1;

  } /* if (pointer) */

  for (yc = y; yc < yend; yc++) {
    for (xc = x; xc < xend; xc++) {
      dgfx_virtualmem[yc * 320 + xc] = data[yc * 320 + xc];
    }
  } /* for (yc... ) */

  if (pointer_x_affected) {
    a=0;b=0;
    for(yc = pointer_y; yc < (pointer_y + pointer->size_y - 1); yc++) {
        for(xc = pointer_x; xc < (pointer_x + pointer->size_x); xc++) {
            c=pointer->bitmap[(a/8) * pointer->size_y+b];
            if((c!=pointer->color_key)&&(xc<SCI_SCREEN_WIDTH)&&(yc<SCI_SCREEN_HEIGHT)) {
               dgfx_virtualmem[yc * 320 + xc] = c;
            }
            b++;
        }
        a++;
    }
  }

  /* move the data buffer to the video memory */
  dosmemput(dgfx_virtualmem,64000,0xa0000);
}

void
dgfx_redraw(struct _state *s, int command, int x, int y, int xl, int yl) {
  int mp_x, mp_y, mp_size_x, mp_size_y;

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
    if (y == 0)
      graphics_draw_region_dgfx(s->pic->view, 0, 0, 320, 200,
                                s->mouse_pointer, s->pointer_x, s->pointer_y);
    else {
      int lines_to_clear, line;
      int first_line_to_clear;
      int i;

      if (y < 0) {
    graphics_draw_region_dgfx(s->pic->view + (320 * y),
                            0, -y, 320, 200+y,
				 s->mouse_pointer, s->pointer_x, s->pointer_y);
	lines_to_clear = -y;
	first_line_to_clear = 0;
      } else { /* y > 0 */
    graphics_draw_region_dgfx(s->pic->view + (320 * y),
				 0, 0, 320, 200-y,
				 s->mouse_pointer, s->pointer_x, s->pointer_y);
	lines_to_clear = y;
	first_line_to_clear = 200 - y;
      }

      line = first_line_to_clear;

      for (i = 0; i < lines_to_clear; i++) {
    graphics_draw_region_dgfx(&(_null_rec[0]) - (line*320), /* Adjust for y coordinate */
				 0, line, 320, 1,
				 s->mouse_pointer, s->pointer_x, s->pointer_y);
	++line;
      }
    }
    break;
  case GRAPHICS_CALLBACK_REDRAW_BOX:
    graphics_draw_region_dgfx(s->pic->view, /* Draw box */
			     x, y, xl, yl,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    break;
  case GRAPHICS_CALLBACK_REDRAW_POINTER:
    graphics_draw_region_dgfx(s->pic->view, /* Draw new pointer */
			     mp_x, mp_y, mp_size_x, mp_size_y,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    graphics_draw_region_dgfx(s->pic->view, /* Remove old pointer */
			     s->last_pointer_x,s->last_pointer_y,
			     s->last_pointer_size_x, s->last_pointer_size_y,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    break;
  default:
    fprintf(stderr,"graphics_callback_dgfx: Invalid command %d\n", command);
  }

  s->last_pointer_size_x = mp_size_x;
  s->last_pointer_size_y = mp_size_y;
  s->last_pointer_x = mp_x;
  s->last_pointer_y = mp_y; /* Update mouse pointer status */
}

gboolean
dgfx_check_mouse_event(sci_event_t *evt) {
  guint32 mx,my,bs,oldbs;
  union REGS r;

  /* if no mouse, say no */
  if(!dgfx_mouse_installed) return FALSE;

  /* poll mouse */
  memset(&r,0,sizeof(union REGS));
  r.x.ax=0x3;                     /* mouse driver: return position and button status */
  int86(0x33,&r,&r);
  mx=r.x.cx>>1;                   /* store x position (must be divided by 2) */
  my=r.x.dx;                      /* store y position */
  bs=r.h.bl;

  /* if the coords are diffenent, tell SCI that */
  if((mx!=dgfx_state->pointer_x)||(my!=dgfx_state->pointer_y)) {
    /* update coords */
    dgfx_state->pointer_x=mx;dgfx_state->pointer_y=my;

    dgfx_state->gfx_driver->Redraw (dgfx_state, GRAPHICS_CALLBACK_REDRAW_POINTER, 0, 0, 0, 0);
  }

  /* if the buttons have not changed, say no */
  if(bs==dgfx_mouse_buttonstat) return FALSE;

  /* update button status */
  oldbs=dgfx_mouse_buttonstat;
  dgfx_mouse_buttonstat=bs;

  /* if no pointer specified, leave */
  if(evt==NULL) return TRUE;

  /* if the left mouse button was off and is now on, do that */
  if((!(oldbs&1))&&(bs&1)) {
    evt->type = SCI_EVT_MOUSE_PRESS;
    evt->data = 1;
    evt->buckybits = 0;

    /* it's ok */
    return TRUE;
  }

  /* if the right mouse button was off and is now on, do that */
  if((!(oldbs&2))&&(bs&2)) {
    evt->type = SCI_EVT_MOUSE_PRESS;
    evt->data = 2;
    evt->buckybits = 0;

    /* it's ok */
    return TRUE;
  }

  /* if the left mouse button was on and is now off, do that */
  if(((oldbs&1))&&(!(bs&1))) {
    evt->type = SCI_EVT_MOUSE_RELEASE;
    evt->data = 1;
    evt->buckybits = 0;

    /* it's ok */
    return TRUE;
  }

  /* if the right mouse button was on and is now off, do that */
  if(((oldbs&2))&&(!(bs&2))) {
    evt->type = SCI_EVT_MOUSE_RELEASE;
    evt->data = 2;
    evt->buckybits = 0;

    /* it's ok */
    return TRUE;
  }

  /* say no */
  return FALSE;
}

char
dgfx_scancode_to_ascii(unsigned char scancode,char* make_break) {
    char i,bits;

    if(scancode>0x7f) {
        scancode=scancode-0x80;
        *make_break=1;
    } else {
        *make_break=0;
    }

    /* first of all, check special state keys */
    switch(scancode) {
        case DGFX_KEY_LSHIFT: if (*make_break==0) {
                                  dgfx_shift_state |= 3;
                              } else {
                                  dgfx_shift_state &= ~3;
                              }
                              return 0;
        case DGFX_KEY_RSHIFT: if (*make_break==0) {
                                  dgfx_shift_state |= 3;
                              } else {
                                  dgfx_shift_state &= ~3;
                              }
                              return 0;
          case DGFX_KEY_CTRL: if (*make_break==0) {
                                  dgfx_shift_state |= 4;
                              } else {
                                  dgfx_shift_state &= ~4;
                              }
                              return 0;
    }

    i=0; bits=0;
    switch(scancode) {
        /* alfabet */
        case DGFX_KEY_A: i='a'; break;
        case DGFX_KEY_B: i='b'; break;
        case DGFX_KEY_C: i='c'; break;
        case DGFX_KEY_D: i='d'; break;
        case DGFX_KEY_E: i='e'; break;
        case DGFX_KEY_F: i='f'; break;
        case DGFX_KEY_G: i='g'; break;
        case DGFX_KEY_H: i='h'; break;
        case DGFX_KEY_I: i='i'; break;
        case DGFX_KEY_J: i='j'; break;
        case DGFX_KEY_K: i='k'; break;
        case DGFX_KEY_L: i='l'; break;
        case DGFX_KEY_M: i='m'; break;
        case DGFX_KEY_N: i='n'; break;
        case DGFX_KEY_O: i='o'; break;
        case DGFX_KEY_P: i='p'; break;
        case DGFX_KEY_Q: i='q'; break;
        case DGFX_KEY_R: i='r'; break;
        case DGFX_KEY_S: i='s'; break;
        case DGFX_KEY_T: i='t'; break;
        case DGFX_KEY_U: i='u'; break;
        case DGFX_KEY_V: i='v'; break;
        case DGFX_KEY_W: i='w'; break;
        case DGFX_KEY_X: i='x'; break;
        case DGFX_KEY_Y: i='y'; break;
        case DGFX_KEY_Z: i='z'; break;
    }
    /* did this resolve? */
    if(i) {
        /* return it */
        return i;
    }
    switch(scancode) {
        /* numbers */
        case DGFX_KEY_0: i='0'; break;
        case DGFX_KEY_1: i='1'; break;
        case DGFX_KEY_2: i='2'; break;
        case DGFX_KEY_3: i='3'; break;
        case DGFX_KEY_4: i='4'; break;
        case DGFX_KEY_5: i='5'; break;
        case DGFX_KEY_6: i='6'; break;
        case DGFX_KEY_7: i='7'; break;
        case DGFX_KEY_8: i='8'; break;
        case DGFX_KEY_9: i='9'; break;
    }
    /* did this resolve? */
    if(i) {
        /* yeah, check shift state */
        if (dgfx_shift_state) {
            /* shift is active. make it uppercase */
            switch(i) {
                case '0': return ')';
                case '1': return '!';
                case '2': return '@';
                case '3': return '#';
                case '4': return '$';
                case '5': return '%';
                case '6': return '^';
                case '7': return '&';
                case '8': return '*';
                case '9': return '(';
            }
        }
        /* return the char */
        return i;
    }
    switch(scancode) {
        /* special chars */
        case DGFX_KEY_MINUS: i='-'; break;
        case DGFX_KEY_EQUAL: i='='; break;
     case DGFX_KEY_LBRACKET: i='['; break;
     case DGFX_KEY_RBRACKET: i=']'; break;
        case DGFX_KEY_COLON: i=';'; break;
        case DGFX_KEY_TILDE: i='`'; break;
        case DGFX_KEY_COMMA: i=','; break;
          case DGFX_KEY_DOT: i='.'; break;
        case DGFX_KEY_SLASH: i='/'; break;
        case DGFX_KEY_SPACE: i=' '; break;
        case DGFX_KEY_ENTER: i=13; break;
        case DGFX_KEY_KPSUB: i='-'; break;
       case DGFX_KEY_BSPACE: i=8; break;
          case DGFX_KEY_TAB: i=9; break;
    }
    if(i) {
        return i;
    }
    /* we can't resolve this. return zero */
    return 0;
}

gboolean
dgfx_check_key_event(sci_event_t* ev) {
    char bits;
    char ch;
    char breakcode;

    /* events there? */
    if (dgfx_keyevent_marker>0) {
        /* yup. convert them! */
        bits=0;
        ch=dgfx_scancode_to_ascii(dgfx_keyevent[dgfx_keyevent_marker],&breakcode);
        if(dgfx_shift_state&1) bits|=SCI_EVM_LSHIFT;
        if(dgfx_shift_state&2) bits|=SCI_EVM_RSHIFT;
        if(dgfx_shift_state&4) bits|=SCI_EVM_CTRL;

        ev->type = SCI_EVT_KEYBOARD;
        ev->data = ch;
        ev->buckybits = bits;

        /* whenever F12 is hit, the game will die instantly (emergency exit) */
        if(dgfx_keyevent[dgfx_keyevent_marker]==DGFX_KEY_F12) {
            dgfx_shutdown(NULL);
            exit(1);
        }

        /* strip it off */
        dgfx_keyevent_marker--;

        /* say we got one! */
        return TRUE;
    }

    return FALSE;
}

sci_event_t
dgfx_get_event(state_t *s) {
  sci_event_t event;

  /* check mouse events */
  if(dgfx_check_mouse_event(&event)==TRUE) {
    /* something happened! */
    return event;
  }

  /* check keyboard events */
  if(dgfx_check_key_event(&event)==TRUE) {
    /* something happened! */
    return event;
  }

  event.data = 0;
  event.type = SCI_EVT_NONE;
  event.buckybits = 0;

  return event;
}

#endif /* HAVE_DGFX */
