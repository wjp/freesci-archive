/***************************************************************************
 kevent.c Copyright (C) 1999 Christoph Reichenbach


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

int stop_on_event;

#define SCANCODE_ROWS_NR 3

struct {
	int offset;
	char *keys;
} scancode_rows[SCANCODE_ROWS_NR] = {
	{0x10, "QWERTYUIOP[]"},
	{0x1e, "ASDFGHJKL;'\\"},
	{0x2c, "ZXCVBNM,./"}
};

int
scancode(int ch) /* Calculates a PC keyboard scancode from a character */
{
	int row;
	int c = toupper(ch);

	for (row = 0; row < SCANCODE_ROWS_NR; row++) {
		char *keys = scancode_rows[row].keys;
		int offset = scancode_rows[row].offset;

		while (*keys) {
			if (*keys == c)
				return offset << 8;

			offset++;
			keys++;
		}
	}

	return ch; /* not found */
}

static int
sci_toupper(int c)
{
	char shifted_numbers[] = ")!@#$%^&*(";
	c = toupper(c);

	if (c >= 'A' && c <= 'Z')
		return c;

	if (c >= '0' && c <= '9')
		return shifted_numbers[c-'0'];

	switch (c) {
	case SCI_K_TAB: return SCI_K_SHIFT_TAB;
	case '`': return '~';
	case '-': return '_';
	case '=': return '+';
	case ';': return ':';
	case '\'': return '"';
	case '\\': return '|';
	case ',': return '<';
	case '.': return '>';
	case '/': return '?';
	default: return c; /* No match */
	}
}

void
kGetEvent(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int mask = UPARAM(0);
	heap_ptr obj = UPARAM(1);
	sci_event_t e;
	int oldx, oldy;
	CHECK_THIS_KERNEL_FUNCTION;

	if (s->kernel_opt_flags & KERNEL_OPT_FLAG_GOT_2NDEVENT) {
		/* Penalty time- too many requests to this function without
		** waiting!  */
		gfxop_usleep(s->gfx_state, 1000000 / 60);
	}
  
	/*If there's a simkey pending, and the game wants a keyboard event, use the
	 *simkey instead of a normal event*/
	if (_kdebug_cheap_event_hack && (mask & SCI_EVT_KEYBOARD)) {
		PUT_SELECTOR(obj, type, SCI_EVT_KEYBOARD); /*Keyboard event*/
		s->acc=1;
		PUT_SELECTOR(obj, message, _kdebug_cheap_event_hack);
		PUT_SELECTOR(obj, modifiers, SCI_EVM_NUMLOCK); /*Numlock on*/
		PUT_SELECTOR(obj, x, s->gfx_state->pointer_pos.x);
		PUT_SELECTOR(obj, y, s->gfx_state->pointer_pos.y);
		_kdebug_cheap_event_hack = 0;
		return;
	}
  
	oldx=s->gfx_state->pointer_pos.x;
	oldy=s->gfx_state->pointer_pos.y;
	e = gfxop_get_event(s->gfx_state);

	s->parser_event = 0; /* Invalidate parser event */

	PUT_SELECTOR(obj, x, s->gfx_state->pointer_pos.x);
	PUT_SELECTOR(obj, y, s->gfx_state->pointer_pos.y);
	/*  gfxop_set_pointer_position(s->gfx_state, gfx_point(s->gfx_state->pointer_pos.x, s->gfx_state->pointer_pos.y)); */


	if (e.type)
		s->kernel_opt_flags &= ~(KERNEL_OPT_FLAG_GOT_EVENT
					 | KERNEL_OPT_FLAG_GOT_2NDEVENT);
	else {
		if (s->kernel_opt_flags & KERNEL_OPT_FLAG_GOT_EVENT)
			s->kernel_opt_flags |= KERNEL_OPT_FLAG_GOT_2NDEVENT;
		else
			s->kernel_opt_flags |= KERNEL_OPT_FLAG_GOT_EVENT;
	}
  
	switch(e.type)
		{
		case SCI_EVT_KEYBOARD: {

			if ((e.buckybits & SCI_EVM_LSHIFT) && (e.buckybits & SCI_EVM_RSHIFT)
			    && (e.data == '-')) {

				sciprintf("Debug mode activated\n");

				script_debug_flag = 1; /* Enter debug mode */
				_debug_seeking = _debug_step_running = 0;
				s->onscreen_console = 0;

			} else if ((e.buckybits & SCI_EVM_CTRL) && (e.data == '`')) {

				script_debug_flag = 1; /* Enter debug mode */
				_debug_seeking = _debug_step_running = 0;
				s->onscreen_console = 1;

			} else if ((e.buckybits & SCI_EVM_CTRL) && (e.data == '1')) {

				if (s->visual)
					s->visual->print(GFXW(s->visual), 0);

			} else {
				if (e.buckybits & SCI_EVM_ALT)
					e.data = scancode(e.data); /* Scancodify if appropriate */

				if ((e.buckybits & (SCI_EVM_RSHIFT | SCI_EVM_LSHIFT)) && !(e.buckybits & SCI_EVM_CAPSLOCK))
					e.data = sci_toupper(e.data);
				if (!(e.buckybits & (SCI_EVM_RSHIFT | SCI_EVM_LSHIFT)) && (e.buckybits & SCI_EVM_CAPSLOCK))
					e.data = sci_toupper(e.data);

				PUT_SELECTOR(obj, type, SCI_EVT_KEYBOARD); /*Keyboard event*/
				s->acc=1;
				PUT_SELECTOR(obj, message, e.data);
				PUT_SELECTOR(obj, modifiers, e.buckybits);

			}
		} break;
		case SCI_EVT_MOUSE_RELEASE:
		case SCI_EVT_MOUSE_PRESS: {
			int extra_bits=0;
			if(mask&e.type)
				{
					switch(e.data) {
					case 2: extra_bits=SCI_EVM_LSHIFT|SCI_EVM_RSHIFT; break;
					case 3: extra_bits=SCI_EVM_CTRL;
					default:break;
					}

					PUT_SELECTOR(obj, type, e.type);
					PUT_SELECTOR(obj, message, 1);
					PUT_SELECTOR(obj, modifiers, e.buckybits|extra_bits);
					s->acc=1;
				}
			return;
		} break;
		default: {
			s->acc = 0; /* Unknown or no event */
		}
		}
    
	if ((s->acc)&&(stop_on_event)) {
		stop_on_event = 0;
		script_debug_flag = 1;
	}  
}

void
kMapKeyToDir(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = UPARAM(0);

	if (GET_SELECTOR(obj, type) == SCI_EVT_KEYBOARD) { /* Keyboard */
		int mover = -1;
		switch (GET_SELECTOR(obj, message)) {
		case SCI_K_HOME: mover = 8; break;
		case SCI_K_UP: mover = 1; break;
		case SCI_K_PGUP: mover = 2; break;
		case SCI_K_LEFT: mover = 7; break;
		case SCI_K_CENTER:
		case 76: mover = 0; break;
		case SCI_K_RIGHT: mover = 3; break;
		case SCI_K_END: mover = 6; break;
		case SCI_K_DOWN: mover = 5; break;
		case SCI_K_PGDOWN: mover = 4; break;
		default: break;
		}

		if (mover >= 0) {
			PUT_SELECTOR(obj, type, SCI_EVT_JOYSTICK);
			PUT_SELECTOR(obj, message, mover);
			s->acc = 1;
		} else s->acc = 0;
	}
}


void
kGlobalToLocal(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = UPARAM_OR_ALT(0, 0);

	if (obj) {
		int x = GET_SELECTOR(obj, x);
		int y = GET_SELECTOR(obj, y);

		PUT_SELECTOR(obj, x, x - s->port->zone.x);
		PUT_SELECTOR(obj, y, y - s->port->zone.y);
	}
}


void
kLocalToGlobal(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr obj = UPARAM_OR_ALT(0, 0);

	if (obj) {
		int x = GET_SELECTOR(obj, x);
		int y = GET_SELECTOR(obj, y);

		PUT_SELECTOR(obj, x, x + s->port->zone.x);
		PUT_SELECTOR(obj, y, y + s->port->zone.y);
	}
}

void /* Not implemented */
kJoystick(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	CHECK_THIS_KERNEL_FUNCTION;
	SCIkdebug(SCIkSTUB, "Unimplemented syscall 'Joystick()'\n", funct_nr);
	s->acc = 0;
}


