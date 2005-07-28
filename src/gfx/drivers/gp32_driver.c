/***************************************************************************
 gp32_driver.c Copyright (C) 2004 Walter van Niftrik


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

   Walter van Niftrik <w.f.b.w.v.niftrik@stud.tue.nl>

***************************************************************************/

#include <sci_memory.h>
#include <gfx_driver.h>
#include <gfx_tools.h>
#include <ctype.h>

#include "keyboard.h"
#include "wrap.h"

/* Event queue */

#define EVENT_BUFFER_SIZE 16
static volatile sci_event_t event_buffer[EVENT_BUFFER_SIZE];
static volatile int event_total;

#define CB_BUFFER_SIZE 256
static char cb_buf[CB_BUFFER_SIZE];
static int cb_buf_pos;

#define GP32_MODE_VKBD 0
#define GP32_MODE_KBD 1
#define GP32_MODE_MOUSE 2
#define GP32_MODE_NR 3

#define GP32_CB_MODE_NORMAL 0
#define GP32_CB_MODE_FN 1
#define GP32_CB_MODE_NR 2

#define GP32_MOUSE_LEFT (1 << 0)
#define GP32_MOUSE_RIGHT (1 << 1)

struct _gp32_state {
	/* 0 = static buffer, 1 = back buffer, 2 = front buffer */
	/* Visual maps */
	byte *visual[3];
	/* Priority maps */
	byte *priority[2];
	/* Line pitch of visual buffers */
	int line_pitch[3];

	/* Virtual mouse pointer */
	volatile int pointer_dx, pointer_dy;

	/* Input mode */
	int mode;
	/* Chatboard mode */
	int cb_mode;

	/* The current bucky state of the virtual keyboard */
	int vkbd_bucky;
	/* Virtual keyboard bitmap. */
	byte *vkbd_bmp;

	/* State of buttons */
	guint16 buttons;
	/* Button repeat timer */
	int timer;

	/* State of simulated mouse buttons */
	int mstate;
};

static struct _gp32_state *S;

#define GP32_OPTION_BLU_PLUS (1 << 0)
static int options;

/* Chatboard key maps. */
typedef struct cb_key_map {
	int key;
	short bucky;
	const char *map1;
	const char *map2;
} cb_key_map_t;

/* Normal mode. */
static cb_key_map_t cb_normal_map[] = {
	{'1', 0, "1\",20\r", "0c1111111111111111\"\r"},
	{'2', 0, "2\",20\r", "1c222222222\"\r"},
	{'3', 0, "3\",20\r", "1c333333\"\r"},
	{'4', 0, "4\",20\r", "1c44444\"\r"},
	{'5', 0, "5\",20\r", "1c5555\"\r"},
	{'6', 0, "6\",20\r", "1c66666666\"\r"},
	{'7', 0, "7\",20\r", "1c777777\"\r"},
	{'8', 0, "8\",20\r", "1c888888\"\r"},
	{'9', 0, "9\",20\r", "1c99999\"\r"},
	{'0', 0, "0\",20\r", "1c0000000000000\"\r"},
	{SCI_K_ENTER, 0, "eee\"\r", "c\"\r"},
	{'1', SCI_EVM_LSHIFT, "1111\"\r", "0c1111\"\r"},
	{'2', SCI_EVM_LSHIFT, "000\"\r", "1c000\"\r"},
	{'3', SCI_EVM_LSHIFT, "#\"\r", "1c#\"\r"},
	{'4', SCI_EVM_LSHIFT, "0000000\"\r", "1c0000000\"\r"},
	{'5', SCI_EVM_LSHIFT, "000000\"\r", "1c000000\"\r"},
	{'/', SCI_EVM_LSHIFT, "111\"\r", "0c111\"\r"},
	{'7', SCI_EVM_LSHIFT, "00\"\r", "1c00\"\r"},
	{'8', SCI_EVM_LSHIFT, "##\"\r", "1c##\"\r"},
	{'9', SCI_EVM_LSHIFT, "11111111111111\"\r", "0c11111111111111\"\r"},
	{'0', SCI_EVM_LSHIFT, "111111111111111\"\r", "0c111111111111111\"\r"},
	{'q', 0, "77\"\r", "1c77\"\r"},
	{'w', 0, "9\"\r", "1c9\"\r"},
	{'e', 0, "33\"\r", "1c33\"\r"},
	{'r', 0, "777\"\r", "1c777\"\r"},
	{'t', 0, "8\"\r", "1c8\"\r"},
	{'y', 0, "999\"\r", "1c999\"\r"},
	{'u', 0, "88\"\r", "1c88\"\r"},
	{'i', 0, "444\"\r", "1c444\"\r"},
	{'o', 0, "666\"\r", "1c666\"\r"},
	{'p', 0, "7\"\r", "1c7\"\r"},
	{'a', 0, "2\"\r", "1c2\"\r"},
	{'s', 0, "7777\"\r", "1c7777\"\r"},
	{'d', 0, "3\"\r", "1c3\"\r"},
	{'f', 0, "333\"\r", "1c333\"\r"},
	{'g', 0, "4\"\r", "1c4\"\r"},
	{'h', 0, "44\"\r", "1c44\"\r"},
	{'j', 0, "5\"\r", "1c5\"\r"},
	{'k', 0, "55\"\r", "1c55\"\r"},
	{'l', 0, "555\"\r", "1c555\"\r"},
	{'z', 0, "9999\"\r", "1c9999\"\r"},
	{'x', 0, "99\"\r", "1c99\"\r"},
	{'c', 0, "222\"\r", "1c222\"\r"},
	{'v', 0, "888\"\r", "1c888\"\r"},
	{'b', 0, "22\"\r", "1c22\"\r"},
	{'n', 0, "66\"\r", "1c66\"\r"},
	{'m', 0, "6\"\r", "1c6\"\r"},
	{'.', 0, "111111\"\r", "0c111111\"\r"},
	{SCI_K_BACKSPACE, 0, "<\"\r", "<\"\r"},
	{'>', 0, ">\"\r", ">\"\r"},
	{' ', 0, "1\"\r", "0c1\"\r"},
	{'q', SCI_EVM_LSHIFT, "2222222\"\r", "1c2222222\"\r"},
	{'w', SCI_EVM_LSHIFT, "22222222\"\r", "1c22222222\"\r"},
	{'e', SCI_EVM_LSHIFT, "3333\"\r", "1c3333\"\r"},
	{'r', SCI_EVM_LSHIFT, "33333\"\r", "1c3333\"\r"},
	{'t', SCI_EVM_LSHIFT, "4444\"\r", "1c4444\"\r"},
	{'y', SCI_EVM_LSHIFT, "1111111\"\r", "0c1111111\"\r"},
	{'u', SCI_EVM_LSHIFT, "6666\"\r", "1c6666\"\r"},
	{'i', SCI_EVM_LSHIFT, "6666666\"\r", "1c6666666\"\r"},
	{'o', SCI_EVM_LSHIFT, "77777\"\r", "1c77777\"\r"},
	{'p', SCI_EVM_LSHIFT, "8888\"\r", "1c8888\"\r"},
	{'a', SCI_EVM_LSHIFT, "88888\"\r", "1c88888\"\r"},
	{'s', SCI_EVM_LSHIFT, "2222\"\r", "1c2222\"\r"},
	{'d', SCI_EVM_LSHIFT, "1111111111\"\r", "0c1111111111\"\r"},
	{'f', SCI_EVM_LSHIFT, "222222\"\r", "1c222222\"\r"},
	{'g', SCI_EVM_LSHIFT, "111111111\"\r", "0c111111111\"\r"},
	{'h', SCI_EVM_LSHIFT, "666666\"\r", "1c666666\"\r"},
	{'j', SCI_EVM_LSHIFT, "00000000\"\r", "1c00000000\"\r"},
	{'k', SCI_EVM_LSHIFT, "66666\"\r", "1c66666\"\r"},
	{'l', SCI_EVM_LSHIFT, "22222\"\r", "1c22222\"\r"},
	{'z', SCI_EVM_LSHIFT, "0\"\r", "1c0\"\r"},
	{'x', SCI_EVM_LSHIFT, "11\"\r", "0c11\"\r"},
	{'c', SCI_EVM_LSHIFT, "00000000000000000\"\r", "1c00000000000000000\"\r"},
	{'v', SCI_EVM_LSHIFT, "0000000000000000\"\r", "1c0000000000000000\"\r"},
	{'b', SCI_EVM_LSHIFT, "0000\"\r", "1c0000\"\r"},
	{'n', SCI_EVM_LSHIFT, "7777777\"\r", "1c7777777\"\r"},
	{'m', SCI_EVM_LSHIFT, "77777777\"\r", "1c77777777\"\r"},
	{',', 0, "11111\"\r", "0c11111\"\r"},
	{0, 0, 0}
};

/* Function mode. */
static cb_key_map_t cb_fn_map[] = {
	{SCI_K_F1, 0, "1\",20\r", "0c1111111111111111\"\r"},
	{SCI_K_F2, 0, "2\",20\r", "1c222222222\"\r"},
	{SCI_K_F3, 0, "3\",20\r", "1c333333\"\r"},
	{SCI_K_F4, 0, "4\",20\r", "1c44444\"\r"},
	{SCI_K_F5, 0, "5\",20\r", "1c5555\"\r"},
	{SCI_K_F6, 0, "6\",20\r", "1c66666666\"\r"},
	{SCI_K_F7, 0, "7\",20\r", "1c777777\"\r"},
	{SCI_K_F8, 0, "8\",20\r", "1c888888\"\r"},
	{SCI_K_F9, 0, "9\",20\r", "1c99999\"\r"},
	{SCI_K_F10, 0, "0\",20\r", "1c0000000000000\"\r"},
	{SCI_K_ENTER, 0, "eee\"\r", "c\"\r"},
	{SCI_K_ESC, 0, "33\"\r", "1c33\"\r"},
	{SCI_K_HOME, 0, "88\"\r", "1c88\"\r"},
	{SCI_K_UP, 0, "444\"\r", "1c444\"\r"},
	{SCI_K_PGUP, 0, "666\"\r", "1c666\"\r"},
	{'`', SCI_EVM_CTRL, "1111111111\"\r", "0c1111111111\"\r"},
	{SCI_K_LEFT, 0, "5\"\r", "1c5\"\r"},
	{SCI_K_RIGHT, 0, "55\"\r", "1c55\"\r"},
	{SCI_K_TAB, 0, "8\"\r", "1c8\"\r"},
	{SCI_K_END, 0, "66\"\r", "1c66\"\r"},
	{SCI_K_DOWN, 0, "6\"\r", "1c6\"\r"},
	{SCI_K_PGDOWN, 0, "111111\"\r", "0c111111\"\r"},
	{SCI_K_BACKSPACE, 0, "<\"\r", "<\"\r"},
	{'>', 0, ">\"\r", ">\"\r"},
	{SCI_K_TAB, SCI_EVM_LSHIFT, "4444\"\r", "1c4444\"\r"},
	{SCI_K_PLUS, 0, "0\"\r", "1c0\"\r"},
	{SCI_K_MINUS, 0, "11\"\r", "0c11\"\r"},
	{0, 0, 0}
};

static cb_key_map_t *cb_key_maps[GP32_CB_MODE_NR] =
  {cb_normal_map, cb_fn_map};

static int
gp32_add_event(sci_event_t *event)
/* Adds an event to the end of the event queue.
** Parameters: (sci_event_t *) event: The event to add
** Returns   : (int) 1 on success, 0 on error
*/
{
	if (event_total >= EVENT_BUFFER_SIZE)
		/* Event buffer is full. */
		return 0;

	event_buffer[event_total++] = *event;
	return 1;
}

static void
gp32_copy_rect_buffer_rot(byte *src, byte *dest, int srcline, int destline,
  int bytespp, rect_t sr, point_t dp)
/* Copies a rectangle from one buffer to another, while rotating it 90
** degrees counter-clockwise.
** Parameters: (byte *) src: The source buffer
**             (byte *) dest: The destination buffer
**             (int) srcline: line pitch of src in bytes
**             (int) destline: line pitch of dest in bytes
**             (int) bytespp: number of bytes per pixel of src and dest
**             (rect_t) sr: Rectangle of src to copy
**             (point_t) dp: Left top corner in dest where copy should go
** Returns   : (void)
*/
{
	src += sr.y * srcline + sr.x * 2;
	dest += dp.x * destline + (destline - bytespp - dp.y * bytespp);
	int i;

	switch (bytespp) {
	case 1:	for (i = 0; i < sr.yl; i++) {
			int j;
			byte *d = dest;
			for (j = 0; j < sr.xl; j++) {
				*d = *(src + j);
				d += destline;
			}
			src += srcline;
			dest--;
		}
		break;
	case 2:	for (i = 0; i < sr.yl; i++) {
			int j;
			byte *d = dest;
			for (j = 0; j < sr.xl; j++) {
				*((guint16 *) d) = *(((guint16 *) src) + j);
				d += destline;
			}
			src += srcline;
			dest -= 2;
		}
	}
}

static void
draw_keyboard()
/* Draws an image of the virtual keyboard in its current state to the
** framebuffer.
** Parameters: (void)
** Returns   : (void)
*/
{
	rect_t sr;
	point_t dp;

	vkbd_draw();

	sr.x = 0;
	sr.xl = 320;
	sr.y = 0;
	sr.yl = 40;
	dp.x = 0;
	dp.y = 200;

	gp32_copy_rect_buffer_rot(S->vkbd_bmp, S->visual[2], 320 * 2,
	  240 * 2, 2, sr, dp);
}

static void
hide_keyboard()
/* Hides the virtual keyboard.
** Parameters: (void)
** Returns   : (void)
*/
{
	int row;
	gp_setFramebuffer((u16 *) FRAMEBUFFER + 20, 1);
	for (row = 0; row < 320; row++)
		memset(S->visual[2] + 240 * row * 2, 0, 80);
}

static void
show_keyboard()
/* Displays the virtual keyboard.
** Parameters: (void)
** Returns   : (void)
*/
{
	gp_setFramebuffer((u16 *) FRAMEBUFFER, 1);
	draw_keyboard();
}

static void
input_mode(int mode, int cb_mode)
/* Updates framebuffer for input modes. For mode GP32_MODE_VKBD, the virtual
** keyboard will be drawn on the screen. Otherwise two characters will be
** drawn on the screen to indicate the current GP32 and Chatboard input
** modes.
** Parameters: (int) mode: GP32 input mode
**             (int) cb_mode: Chatboard input mode
** Returns   : (void)
*/
{
	if (mode == GP32_MODE_VKBD)
		show_keyboard();
	else if (mode == GP32_MODE_KBD) {
		hide_keyboard();
		gp_drawString(312, 212, 1, "K", 0xFFFE,
		  (unsigned short *) FRAMEBUFFER);
	}
	else if (mode == GP32_MODE_MOUSE) {
		hide_keyboard();
		gp_drawString(312, 212, 1, "M", 0xFFFE,
		  (unsigned short *) FRAMEBUFFER);
	}

	if (mode != GP32_MODE_VKBD) {
		if (cb_mode == GP32_CB_MODE_NORMAL)
			gp_drawString(0, 212, 1, "N", 0xFFFE,
			  (unsigned short *) FRAMEBUFFER);
		else
			gp_drawString(0, 212, 1, "F", 0xFFFE,
			  (unsigned short *) FRAMEBUFFER);
	}
}

static void
input_handler(void) __attribute__ ((interrupt ("IRQ")));

static void
input_handler(void)
/* Checks buttons and adds events to the event queue.
** Parameters: (void)
** Returns   : (void)
*/
{
	sci_event_t event;
	guint16 buttons = gp_getButton();
	if (S->timer > 0)
		S->timer--;
	if ((buttons != S->buttons) || !S->timer) {
		/* If we're repeating a key use a lower timer. */
		if (!S->timer)
			S->timer = 8;
		else
			S->timer = 25;

		S->buttons = buttons;

		if (buttons & BUTTON_START) {
			S->mode++;
			if (S->mode == GP32_MODE_NR)
				S->mode = 0;
			input_mode(S->mode, S->cb_mode);
		}
		if (S->mode == GP32_MODE_VKBD) {
			event.type = SCI_EVT_KEYBOARD;
			event.buckybits = S->vkbd_bucky;
			event.data = 0;
			if (buttons & BUTTON_RIGHT)
				vkbd_handle_input(KBD_RIGHT);
			if (buttons & BUTTON_LEFT)
				vkbd_handle_input(KBD_LEFT);
			if (buttons & BUTTON_UP)
				vkbd_handle_input(KBD_UP);
			if (buttons & BUTTON_DOWN)
				vkbd_handle_input(KBD_DOWN);
			if (buttons & BUTTON_A) {
				int vkbd_key;
				if (vkbd_get_key(&vkbd_key, &S->vkbd_bucky)) {
					event.buckybits = S->vkbd_bucky;
					event.data = vkbd_key;
				}
			}
			else if (buttons & BUTTON_B)
				event.data = SCI_K_BACKSPACE;
			else if (buttons & BUTTON_R)
				event.data = SCI_K_ENTER;
			else if (buttons & BUTTON_L)
				event.data = SCI_K_ESC;
			if (event.data)
				gp32_add_event(&event);
			draw_keyboard();
		}
		else if (S->mode == GP32_MODE_KBD) {
			event.type = SCI_EVT_KEYBOARD;
			event.buckybits = 0;
			event.data = 0;
			if (buttons & BUTTON_RIGHT)
				event.data = SCI_K_RIGHT;
			else if (buttons & BUTTON_LEFT)
				event.data = SCI_K_LEFT;
			else if (buttons & BUTTON_UP)
				event.data = SCI_K_UP;
			else if (buttons & BUTTON_DOWN)
				event.data = SCI_K_DOWN;
			else if ((buttons & BUTTON_A) || (buttons & BUTTON_R))
				event.data = SCI_K_ENTER;
			else if ((buttons & BUTTON_B) || (buttons & BUTTON_L))
				event.data = SCI_K_ESC;
			if (event.data)
				gp32_add_event(&event);
		}
	}
	if (S->mode == GP32_MODE_MOUSE) {
		event.buckybits = 0;
		int step = (buttons & BUTTON_L ? 2 : 1);
		if (buttons & BUTTON_RIGHT)
			S->pointer_dx += step;
		if (buttons & BUTTON_LEFT)
			S->pointer_dx -= step;
		if (buttons & BUTTON_UP)
			S->pointer_dy -= step;
		if (buttons & BUTTON_DOWN)
			S->pointer_dy += step;

		if ((buttons & BUTTON_A)
		  && !(S->mstate & GP32_MOUSE_LEFT)) {
			event.type = SCI_EVT_MOUSE_PRESS;
			event.data = 1;
			gp32_add_event(&event);
			S->mstate |= GP32_MOUSE_LEFT;
		}
		if ((buttons & BUTTON_B)
		  && !(S->mstate & GP32_MOUSE_RIGHT)) {
			event.type = SCI_EVT_MOUSE_PRESS;
			event.data = 2;
			gp32_add_event(&event);
			S->mstate |= GP32_MOUSE_RIGHT;
		}
		if (!(buttons & BUTTON_A)
		  && (S->mstate & GP32_MOUSE_LEFT)) {
			event.type = SCI_EVT_MOUSE_RELEASE;
			event.data = 1;
			gp32_add_event(&event);
			S->mstate &= ~GP32_MOUSE_LEFT;
		}
		if (!(buttons & BUTTON_B)
		  && (S->mstate & GP32_MOUSE_RIGHT)) {
			event.type = SCI_EVT_MOUSE_RELEASE;
			event.data = 2;
			gp32_add_event(&event);
			S->mstate &= ~GP32_MOUSE_RIGHT;
		}
	}
}

static void
install_input_handler()
/* Installs the timer-driven input handler.
** Parameters: (void)
** Returns   : (void)
*/
{
	int pclk;

	pclk = gp_getPCLK();
	pclk /= 16;
	pclk /= 256;
	/* We want to check for input 60 times per second. */
	pclk /= 60;

	gp_disableIRQ();
	/* Set prescaler for timer 2, 3 and 4 to 256. */
	rTCFG0 |= (0xFF << 8);
	/* Set timer 3 divider to 16. */
	rTCFG1 |= (0x03 << 12);
	rTCNTB3 = pclk;
	/* Load timer 3 counter and start with auto-reload. */
	rTCON |= (1 << 17);
	rTCON = (rTCON ^ (1 << 17)) | (1 << 19) | (1 << 16);
	gp_installSWIIRQ(13, input_handler);
	gp_enableIRQ();
}

static void
remove_input_handler()
/* Removes the timer-driven input handler.
** Parameters: (void)
** Returns   : (void)
*/
{
	gp_disableIRQ();
	/* Turn timer 3 off. */
	rTCON = BITCLEAR(rTCON, 16);
	gp_removeSWIIRQ(13, input_handler);
	gp_enableIRQ();
}

static void
chatboard_send(const char *msg)
/* Sends a string to the Chatboard.
** Parameters: (const char *) msg: The string to send.
** Returns   : (void)
*/
{
	int i = 0;

	while (i < strlen(msg)) {
		if (((rUFSTAT0 >> 4) & 0x0f) < 16)
			rUTXH0 = msg[i++];
	}
}

static int
convert_key(const char *key, int map, short *bucky)
/* Converts a Chatboard AT-string to a FreeSCI keycode.
** Parameters: (const char *) key: The AT-string to convert.
**             (int) map: Index of the key map to use.
**             (short *) bucky: Return argument for buckybits.
** Returns   : (int) Converted FreeSCI keycode on success, 0 on error.
*/
{
	int i = 0;
	int len = strlen(key);

	if (len <= 9)
		return 0;

	/* Verify start of key string. */
	if (strncmp(key, "AT+CKPD=\"", 9))
		return 0;

	/* Search for key string. */
	while (cb_key_maps[map][i].key) {
		if (!strncmp(cb_key_maps[map][i].map1, key + 9, len - 9))
			break;
		if (!strncmp(cb_key_maps[map][i].map2, key + 9, len - 9))
			break;
		i++;
	}

	*bucky = cb_key_maps[map][i].bucky;
	return cb_key_maps[map][i].key;
}

static void
chatboard_receive(void) __attribute__ ((interrupt ("IRQ")));

static void
chatboard_receive(void)
/* Handles Chatboard input and adds events to the event queue.
** Parameters: (void)
** Returns   : (void)
*/
{
	int chars, i;

	if (rUFSTAT0 & 0x100)
		/* Buffer full? */
		chars = 16;
	else
		/* Number of characters available */
		chars = rUFSTAT0 & 0x0F;

	for(i = 0; i < chars; i++) {
		char c = rURXH0;
		if (cb_buf_pos < CB_BUFFER_SIZE - 1)
			cb_buf[cb_buf_pos++] = c;
		if (c == 13) {
			sci_event_t event;

			cb_buf[cb_buf_pos] = '\0';
			chatboard_send("OK\15\12");

			event.type = SCI_EVT_KEYBOARD;
			event.data = convert_key(cb_buf, S->cb_mode,
			  &event.buckybits);

			if (event.data == '>') {
				S->cb_mode = (S->cb_mode + 1) % GP32_CB_MODE_NR;
				input_mode(S->mode, S->cb_mode);
			}
			else if (event.data > 0)
				gp32_add_event(&event);

			cb_buf_pos = 0;
		}
	}
}

static void
chatboard_init()
/* Initializes the Chatboard and installs the IRQ handler.
** Parameters: (void)
** Returns   : (void)
*/
{
	int PCLK = gp_getPCLK();

	cb_buf_pos = 0;

	rCLKCON |= 0x100; // Controls PCLK into UART0 block

	/* No IRDA, 8N1 */
	rULCON0 = 3;

	/* Enable timeout IRQ, set IRQ/polling mode */
	rUCON0 = (1 << 7) | (1 << 2) | (1 << 0);

	/* Set receive FIFO to 16 bytes, transmit FIFA to 0 bytes and reset
	FIFO's
	*/
	rUFCON0 = (3 << 4) | (1 << 2) | (1 << 1) | (1 << 0);

	/* Set baud rate divider */
	rUBRDIV0 = (int) (PCLK / (9600 * 16)) - 1;

	/* Install interrupt routine */
	gp_disableIRQ();
	gp_installSWIIRQ(23, chatboard_receive);
	gp_enableIRQ();
}

static void
chatboard_exit()
/* Removes the Chatboard IRQ handler.
** Parameters: (void)
** Returns   : (void)
*/
{
	/* Remove interrupt routine */
	gp_disableIRQ();
	gp_removeSWIIRQ(23, chatboard_receive);
	gp_enableIRQ();
}

static guint32
get_color(struct _gfx_driver *drv, gfx_color_t col)
/* Converts a color as described in col to it's representation in memory.
** Parameters: (_gfx_driver *) drv: The driver to use
**             (gfx_color_t) color: The color to convert
** Returns   : (guint32) the color's representation in memory
*/
{
	guint32 retval;
	guint32 temp;

	retval = 0;

	temp = col.visual.r;
	temp |= temp << 8;
	temp |= temp << 16;
	retval |= (temp >> drv->mode->red_shift) & (drv->mode->red_mask);
	temp = col.visual.g;
	temp |= temp << 8;
	temp |= temp << 16;
	retval |= (temp >> drv->mode->green_shift) & (drv->mode->green_mask);
	temp = col.visual.b;
	temp |= temp << 8;
	temp |= temp << 16;
	retval |= (temp >> drv->mode->blue_shift) & (drv->mode->blue_mask);

	return retval;
}

static void
gp32_draw_line_buffer(byte *buf, int line, int bytespp, int x1,
  int y1, int x2, int y2, guint32 color)
/* Draws a line in a buffer.
** This function was taken from sdl_driver.c with small modifications
** Parameters: (byte *) buf: The buffer to draw in
**             (int) line: line pitch of buf in bytes
**             (int) bytespp: number of bytes per pixel of buf
**             (int) x1, y1, x2, y2: The line to draw: (x1,y1)-(x2,y2).
**             (guint32) color: The color to draw with
** Returns   : (void)
*/
{
	int pixx, pixy;
	int x,y;
	int dx,dy;
	int sx,sy;
	int swaptmp;
	guint8 *pixel;

	dx = x2 - x1;
	dy = y2 - y1;
	sx = (dx >= 0) ? 1 : -1;
	sy = (dy >= 0) ? 1 : -1;

	dx = sx * dx + 1;
	dy = sy * dy + 1;
	pixx = bytespp;
	pixy = line;
	pixel = ((guint8*) buf) + pixx * x1 + pixy * y1;
	pixx *= sx;
	pixy *= sy;
	if (dx < dy) {
		swaptmp = dx; dx = dy; dy = swaptmp;
		swaptmp = pixx; pixx = pixy; pixy = swaptmp;
	}

	x = 0;
	y = 0;
	switch(bytespp) {
		case 1:
			for(; x < dx; x++, pixel += pixx) {
				*pixel = color;
				y += dy;
				if (y >= dx) {
					y -= dx; pixel += pixy;
				}
			}
			break;
		case 2:
			for (; x < dx; x++, pixel += pixx) {
				*(guint16*)pixel = color;
				y += dy;
				if (y >= dx) {
					y -= dx;
					pixel += pixy;
				}
			}
	}

}

static void
gp32_draw_filled_rect_buffer(byte *buf, int line, int bytespp, rect_t rect,
  guint32 color)
/* Draws a filled rectangle in a buffer.
** Parameters: (byte *) buf: The buffer to draw in
**             (int) line: line pitch of buf in bytes
**             (int) bytespp: number of bytes per pixel of buf
**             (rect_t) rect: The rectangle to fill
**             (guint32) color: The fill color
** Returns   : (void)
*/
{
	buf += rect.y * line + rect.x * bytespp;
	int i;

	switch (bytespp) {
		case 1:	for (i = 0; i < rect.yl; i++) {
				memset(buf, color, rect.xl);
				buf += line;
			}
			break;
		case 2:	for (i = 0; i < rect.yl; i++) {
				int j;
				for (j = 0; j < rect.xl; j++)
					((guint16 *) buf)[j] = color;
				buf += line;
			}
	}
}

static void
gp32_copy_rect_buffer(byte *src, byte *dest, int srcline, int destline,
  int bytespp, rect_t sr, point_t dp)
/* Copies a rectangle from one buffer to another.
** Parameters: (byte *) src: The source buffer
**             (byte *) dest: The destination buffer
**             (int) srcline: line pitch of src in bytes
**             (int) destline: line pitch of dest in bytes
**             (int) bytespp: number of bytes per pixel of src and dest
**             (rect_t) sr: Rectangle of src to copy
**             (point_t) dp: Left top corner in dest where copy should go
** Returns   : (void)
*/
{
	src += sr.y*srcline + sr.x*bytespp;
	dest += dp.y*destline + dp.x*bytespp;
	int i;

	switch (bytespp) {
		case 1:	for (i = 0; i < sr.yl; i++) {
				memcpy(dest, src, sr.xl);
				src += srcline;
				dest += destline;
			}
			break;
		case 2:	for (i = 0; i < sr.yl; i++) {
				int j;
				for (j = 0; j < sr.xl; j++)
					((guint16 *) dest)[j] =
					  ((guint16 *) src)[j];
				src += srcline;
				dest += destline;
			}
	}
}

static int
gp32_set_parameter(struct _gfx_driver *drv, char *attribute, char *value)
{
	if (!stricmp(attribute, "blu_plus")) {
		if (string_truep(value)) {
			options |= GP32_OPTION_BLU_PLUS;
			return GFX_OK;
		}
		else {
			options &= ~GP32_OPTION_BLU_PLUS;
			return GFX_OK;
		}
	}
	sciprintf("Fatal error: Attribute '%s' does not exist\n", attribute);
	return GFX_FATAL;
}

static int
gp32_init_specific(struct _gfx_driver *drv, int xfact, int yfact,
  int bytespp)
{
	int i;
	int rmask = 0, gmask = 0, bmask = 0, rshift = 0, gshift = 0;
	int bshift = 0;

	sciprintf("Initialising video mode\n");

	drv->state = NULL;
	if (!S)
		S = sci_malloc(sizeof(struct _gp32_state));
	if (!S)
		return GFX_FATAL;

	if (xfact != 1 || yfact != 1 || bytespp != 2) {
		sciprintf("Error: buffers with scale factors (%d,%d) and "
		  " bpp=%d are not supported\n", xfact, yfact, bytespp);
		return GFX_ERROR;
	}
	for (i = 0; i < 2; i++) {
		if (!(S->priority[i] = sci_malloc(320 * 200)) ||
		  !(S->visual[i] = sci_malloc(320 * 200 * 2))) {
			sciprintf("Error: Could not reserve memory for "
			  "buffer\n");
			return GFX_ERROR;
		}
	}

	for (i = 0; i < 2; i++) {
		S->line_pitch[i] = 320 * 2;
		memset(S->visual[i], 0, 320 * 200 * 2);
		memset(S->priority[i], 0, 320 * 200);
	}

	rmask = 0xF800;
	gmask = 0x07C0;
	bmask = 0x003E;
	rshift = 16;
	gshift = 21;
	bshift = 26;

	if (options & GP32_OPTION_BLU_PLUS)
		gp_initFramebufferBP((u16 *) FRAMEBUFFER, 16, 50);
	else
		gp_initFramebuffer((u16 *) FRAMEBUFFER, 16, 50);
	S->visual[2] = (byte *) FRAMEBUFFER;
	S->line_pitch[2] = 240 * 2;
	/* We clear 40 extra bytes which are displayed when virtual
	** keyboard is hidden.
	*/
	memset(S->visual[2], 0, 320 * 240 * 2 + 40);

	drv->mode = gfx_new_mode(xfact, yfact, bytespp, rmask, gmask, bmask,
	  0, rshift, gshift, bshift, 0, 0, 0);

	S->pointer_dx = 0;
	S->pointer_dy = 0;
	S->mstate = 0;
	S->vkbd_bucky = 0;
	S->vkbd_bmp = sci_malloc(320 * 40 * 2);
	vkbd_init((guint16 *) S->vkbd_bmp, 320);
	S->mode = GP32_MODE_KBD;
	S->cb_mode = GP32_CB_MODE_NORMAL;
	input_mode(S->mode, S->cb_mode);

	event_total = 0;

	chatboard_init();

	install_input_handler();

	sciprintf("Video mode initialisation completed succesfully\n");

	return GFX_OK;
}

static int
gp32_init(struct _gfx_driver *drv)
{
	gp_setCPUspeed(133);
	if (gp32_init_specific(drv, 1, 1, 2) != GFX_OK) return GFX_FATAL;

	return GFX_OK;
}

static void
gp32_exit(struct _gfx_driver *drv)
{
	remove_input_handler();

	chatboard_exit();

	if (S) {
		sciprintf("Freeing graphics buffers\n");
		sci_free(S->visual[0]);
		sci_free(S->visual[1]);
		sci_free(S->priority[0]);
		sci_free(S->priority[1]);
		sci_free(S);
		S = NULL;
	}

	gp_setFramebuffer((u16 *) FRAMEBUFFER, 1);
	gp_clearFramebuffer16((unsigned short *) FRAMEBUFFER, 0xFFFF);
	gp_drawString(68, 116, 23, "Loading, please wait...", 0x0000,
		(unsigned short *) FRAMEBUFFER);
}

	/*** Drawing operations ***/

static int
gp32_draw_line(struct _gfx_driver *drv, rect_t line, gfx_color_t color,
	      gfx_line_mode_t line_mode, gfx_line_style_t line_style)
{
	guint32 scolor;
	int xfact = 1;
	int yfact = 1;
	int xsize = 320;
	int ysize = 200;

	int xc, yc;
	int x1, y1, x2, y2;

	scolor = get_color(drv, color);

	x1 = line.x;
	y1 = line.y;

	for (xc = 0; xc < xfact; xc++)
		for (yc = 0; yc < yfact; yc++) {
			x1 = line.x + xc;
			y1 = line.y + yc;
			x2 = x1 + line.xl;
			y2 = y1 + line.yl;

			if (x1 < 0) x1 = 0;
			if (x2 < 0) x2 = 0;
			if (y1 < 0) y1 = 0;
			if (y2 < 0) y2 = 0;

			if (x1 > xsize) x1 = xsize;
			if (x2 >= xsize) x2 = xsize - 1;
			if (y1 > ysize) y1 = ysize;
			if (y2 >= ysize) y2 = ysize - 1;

			if (color.mask & GFX_MASK_VISUAL)
				gp32_draw_line_buffer(S->visual[1],
				  320*2, 2, x1, y1, x2, y2,
				  get_color(drv, color));

			if (color.mask & GFX_MASK_PRIORITY)
				gp32_draw_line_buffer(S->priority[1], 320,
				  1, x1, y1, x2, y2, color.priority);
		}

	return GFX_OK;
}

static int
gp32_draw_filled_rect(struct _gfx_driver *drv, rect_t rect,
  gfx_color_t color1, gfx_color_t color2, gfx_rectangle_fill_t shade_mode)
{
	if (color1.mask & GFX_MASK_VISUAL)
		gp32_draw_filled_rect_buffer(S->visual[1], S->line_pitch[1],
		  2, rect, get_color(drv, color1));

	if (color1.mask & GFX_MASK_PRIORITY)
		gp32_draw_filled_rect_buffer(S->priority[1], 320, 1, rect,
		  color1.priority);

	return GFX_OK;
}

	/*** Pixmap operations ***/

static int
gp32_register_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm)
{
	return GFX_ERROR;
}

static int
gp32_unregister_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm)
{
	return GFX_ERROR;
}

static int
gp32_draw_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm, int priority,
		rect_t src, rect_t dest, gfx_buffer_t buffer)
{
	int bufnr = (buffer == GFX_BUFFER_STATIC)? 0:1;

	return gfx_crossblit_pixmap(drv->mode, pxm, priority, src, dest,
	  S->visual[bufnr], S->line_pitch[bufnr], S->priority[bufnr], 320,
	  1, 0);
}

static int
gp32_grab_pixmap(struct _gfx_driver *drv, rect_t src, gfx_pixmap_t *pxm,
		gfx_map_mask_t map)
{
	switch (map) {
		case GFX_MASK_VISUAL:
			gp32_copy_rect_buffer(S->visual[1], pxm->data,
			  S->line_pitch[1], src.xl*2, 2, src,
			  gfx_point(0, 0));
			pxm->xl = src.xl;
			pxm->yl = src.yl;
			return GFX_OK;
		case GFX_MASK_PRIORITY:
			gp32_copy_rect_buffer(S->priority[1],
			  pxm->index_data, 320, src.xl, 1, src,
			  gfx_point(0, 0));
			pxm->index_xl = src.xl;
			pxm->index_yl = src.yl;
			return GFX_OK;
		default:
			sciprintf("Error: attempt to grab pixmap from "
			  "invalid map");
			return GFX_ERROR;
	}
}


	/*** Buffer operations ***/

static int
gp32_update(struct _gfx_driver *drv, rect_t src, point_t dest,
  gfx_buffer_t buffer)
{
	int tbufnr = (buffer == GFX_BUFFER_BACK)? 1:2;

	if (tbufnr == 1)
		gp32_copy_rect_buffer(S->visual[0], S->visual[1],
		  S->line_pitch[0], S->line_pitch[1], 2, src, dest);
	else
		gp32_copy_rect_buffer_rot(S->visual[1],
		  S->visual[2], S->line_pitch[1], 240 * 2, 2, src, dest);

	if ((tbufnr == 1) && (src.x == dest.x) && (src.y == dest.y))
		gp32_copy_rect_buffer(S->priority[0], S->priority[1], 320,
		  320, 1, src, dest);

	return GFX_OK;
}

static int
gp32_set_static_buffer(struct _gfx_driver *drv, gfx_pixmap_t *pic,
  gfx_pixmap_t *priority)
{
	memcpy(S->visual[0], pic->data, 320 * 200 * 2);
	memcpy(S->priority[0], priority->index_data, 320 * 200);

	return GFX_OK;
}


	/*** Palette operations ***/

static int
gp32_set_palette(struct _gfx_driver *drv, int index, byte red, byte green,
  byte blue)
{
	return GFX_ERROR;
}


	/*** Mouse pointer operations ***/

static int
gp32_set_pointer (struct _gfx_driver *drv, gfx_pixmap_t *pointer)
{
	return GFX_ERROR;
}


	/*** Event management ***/

static sci_event_t
gp32_get_event(struct _gfx_driver *drv)
{
	sci_event_t event;

	gp_disableIRQ();

	drv->pointer_x += S->pointer_dx;
	drv->pointer_y += S->pointer_dy;
	S->pointer_dx = 0;
	S->pointer_dy = 0;
	if (drv->pointer_x < 0) drv->pointer_x = 0;
	if (drv->pointer_x >= 320) drv->pointer_x = 320 - 1;
	if (drv->pointer_y < 0) drv->pointer_y = 0;
	if (drv->pointer_y >= 200) drv->pointer_y = 200 - 1;

	if (event_total > 0) {
		event = event_buffer[event_total - 1];
		event_total--;
		gp_enableIRQ();
		return event;
	}

	gp_enableIRQ();
	event.type = SCI_EVT_NONE;
	event.buckybits = 0;
	return event;
}

static int
gp32_usec_sleep(struct _gfx_driver *drv, long usecs)
{
	usleep(usecs);
	return GFX_OK;
}

gfx_driver_t
gfx_driver_gp32 = {
	"gp32",
	"0.1",
	SCI_GFX_DRIVER_MAGIC,
	SCI_GFX_DRIVER_VERSION,
	NULL,
	0,
	0,
	GFX_CAPABILITY_PIXMAP_GRABBING | GFX_CAPABILITY_MOUSE_SUPPORT,
	0,
	gp32_set_parameter,
	gp32_init_specific,
	gp32_init,
	gp32_exit,
	gp32_draw_line,
	gp32_draw_filled_rect,
	gp32_register_pixmap,
	gp32_unregister_pixmap,
	gp32_draw_pixmap,
	gp32_grab_pixmap,
	gp32_update,
	gp32_set_static_buffer,
	gp32_set_pointer,
	gp32_set_palette,
	gp32_get_event,
	gp32_usec_sleep,
	NULL
};
