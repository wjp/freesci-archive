/***************************************************************************
 con_io.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* SCI console */

#include <console.h>
#ifdef SCI_CONSOLE
#include <resource.h>
#include <string.h>
#include <assert.h>
#include <graphics.h>
#include <engine.h>

int con_row_counter = 0;
int con_visible_rows = 0;
int con_display_row = 0;
int con_cursor = 1;


/* Command buffer */
static char _commandbuf[SCI_CONSOLE_INPUT_BUFFER][SCI_CONSOLE_MAX_INPUT];
static int _commandbuflen = 0;
static int _commandbufpos = 0;
static int _commandlookback = 0;

char _inpbuf[SCI_CONSOLE_MAX_INPUT + 1] = "$\0";
char *con_input_line = _inpbuf + 1;


void *_xmalloc(size_t size)
{
  void *retval = (void *) sci_malloc(size);
  if (!retval) {
    fprintf(stderr,"Out of memory!\n");
    exit(1);
  }
  return retval;
}

static byte _con_gfx_initialized = 0;

void
con_init_gfx()
{
  if (!_con_gfx_initialized) {
    _con_gfx_initialized = 1;
    con_hook_int(&sci_color_mode, "color_mode", "SCI0 picture resource draw mode (0..2)");
    con_hook_int(&con_display_row, "con_display_row", "Number of rows to display for the"
		 "\n  onscreen console");
  }
}


char *
con_input(sci_event_t *event)
/* This function is hell. Feel free to clean it up. */
{
  int stl = strlen(con_input_line);
  int z,i; /* temp vars */

  if (con_visible_rows <= 0) return NULL;

  if (event->type == SCI_EVT_KEYBOARD) {

    if ((event->buckybits & (SCI_EVM_CTRL | SCI_EVM_ALT)) == 0) {
      if (((event->data >= '0') && (event->data < '@'))
	  || ((event->data >= 'a') && (event->data <= 'z'))
	  || (event->data == ' ')
	  || (event->data == '-')
	  || (event->data == '+')
	  ){
	if (stl < SCI_CONSOLE_MAX_INPUT) {
	  memmove(_inpbuf + con_cursor, _inpbuf + con_cursor-1,
		  SCI_CONSOLE_MAX_INPUT - con_cursor+1);

	  if (event->buckybits & (SCI_EVM_RSHIFT | SCI_EVM_LSHIFT | SCI_EVM_CAPSLOCK)) /* Shift? */
	    switch (event->data) {

	    case '1': _inpbuf[con_cursor] = '!'; break;
	    case '2': _inpbuf[con_cursor] = '@'; break;
	    case '3': _inpbuf[con_cursor] = '#'; break;
	    case '4': _inpbuf[con_cursor] = '$'; break;
	    case '5': _inpbuf[con_cursor] = '%'; break;
	    case '6': _inpbuf[con_cursor] = '^'; break;
	    case '7': _inpbuf[con_cursor] = '&'; break;
	    case '8': _inpbuf[con_cursor] = '*'; break;
	    case '9': _inpbuf[con_cursor] = '('; break;
	    case '0': _inpbuf[con_cursor] = ')'; break;
	    case '-': _inpbuf[con_cursor] = '_'; break;
	    case '=': _inpbuf[con_cursor] = '+'; break;
	    case ';': _inpbuf[con_cursor] = ':'; break;
	    case '\'': _inpbuf[con_cursor] = '"'; break;
	    case '\\': _inpbuf[con_cursor] = '|'; break;
	    case '[': _inpbuf[con_cursor] = '{'; break;
	    case ']': _inpbuf[con_cursor] = '}'; break;
	    case ',': _inpbuf[con_cursor] = '<'; break;
	    case '.': _inpbuf[con_cursor] = '>'; break;
	    case '/': _inpbuf[con_cursor] = '?'; break;

	    default:
	      _inpbuf[con_cursor] = toupper(event->data);

	    } else _inpbuf[con_cursor] = event->data;

	  con_cursor++;
	}
	con_outputlookback = con_outputbufpos;
      } else switch (event->data) {

	case SCI_K_ENTER: goto sci_evil_label_enter;

	case SCI_K_BACKSPACE: goto sci_evil_label_backspace;

	case SCI_K_LEFT:
	  if (con_cursor > 1) con_cursor--;
	  z = 1;
	  con_outputlookback = con_outputbufpos;
	  break;

	case SCI_K_UP:
	  goto sci_evil_label_prevcommand;

	case SCI_K_DOWN:
	  goto sci_evil_label_nextcommand;

	case SCI_K_RIGHT:
	  if (con_cursor <= stl) con_cursor++;
	  z = 1;
	  con_outputlookback = con_outputbufpos;
	  break;

	case SCI_K_PGUP:
	  for (i = 0; i < 3; i++) {
	    if ((z = con_outputlookback - 1) <= 0)
	      z += con_outputbuflen;
	    if (z == con_outputbufpos)
	      break;
	    con_outputlookback = z;
	  }
	  z = 1;
	  break;

	case SCI_K_PGDOWN:
	  for (i = 0; i < 3; i++) {
	    if ((z = con_outputlookback + 1) > con_outputbuflen)
	      z = 1;
	    if (con_outputlookback == con_outputbufpos)
	      break;
	    con_outputlookback = z;
	  }
	  z = 1;
	  break;

	case SCI_K_DELETE:
	  if (con_cursor <= stl) {
	    memmove(_inpbuf + con_cursor, _inpbuf + con_cursor +1,
		    SCI_CONSOLE_MAX_INPUT - con_cursor- 1);
	  }
	  con_outputlookback = con_outputbufpos;
	  z = 1;
	  break;
      }
    } else

      if (event->buckybits & SCI_EVM_CTRL) {
	switch (event->data) {

	case 'k':
	  _inpbuf[con_cursor] = 0;
	  break;

	case 'a':
	  con_cursor = 1;
	  break;

	case 'e':
	  con_cursor = stl+1;
	  break;

	case 'b':
	  if (con_cursor > 1) con_cursor--;
	  break;

	case 'p':
	sci_evil_label_prevcommand:
	if (_commandlookback == _commandbufpos)
	  memcpy(&(_commandbuf[_commandbufpos]), con_input_line, SCI_CONSOLE_MAX_INPUT);

	if ((z = _commandlookback - 1) == -1)
	  z = _commandbuflen;

	if (z != _commandbufpos) {
	  _commandlookback = z;

	  memcpy(con_input_line, &(_commandbuf[_commandlookback]), SCI_CONSOLE_MAX_INPUT);
	  con_cursor = strlen(con_input_line)+1;
	}
	break;

	case 'n':
	sci_evil_label_nextcommand:
	if ((z = _commandlookback + 1) == _commandbuflen + 1)
	  z = 0;

	if (_commandlookback != _commandbufpos) {
	  _commandlookback = z;

	  memcpy(con_input_line, &(_commandbuf[_commandlookback]), SCI_CONSOLE_MAX_INPUT);
	  con_cursor = strlen(con_input_line)+1;
	}
	break;

	case 'f':
	  if (con_cursor <= stl) con_cursor++;
	  break;

	case 'h':
	sci_evil_label_backspace:
	if (con_cursor > 1) {
	  memmove(_inpbuf + con_cursor-1, _inpbuf + con_cursor,
		  SCI_CONSOLE_MAX_INPUT - con_cursor);
	  con_cursor--;
	}
	break;

	case 'd':
	  if (con_cursor <= stl) {
	    memmove(_inpbuf + con_cursor, _inpbuf + con_cursor +1,
		    SCI_CONSOLE_MAX_INPUT - con_cursor- 1);
	  }
	  break;

	case 'm': /* Enter */
	sci_evil_label_enter:
	memcpy(&(_commandbuf[z = _commandbufpos]), con_input_line, SCI_CONSOLE_MAX_INPUT);
	++_commandbufpos;
	_commandlookback = _commandbufpos %= SCI_CONSOLE_INPUT_BUFFER;
	if (_commandbuflen < _commandbufpos)
	  _commandbuflen = _commandbufpos;

	con_input_line[0] = '\0';
	con_cursor = 1;
	con_outputlookback = con_outputbufpos;
	return _commandbuf[z];

	}
	con_outputlookback = con_outputbufpos;

      }
  }
  return NULL;
}


byte *
con_backup_screen(state_t *s)
{
  byte *retval = sci_malloc(s->pic->size);

  return memcpy(retval, s->pic->maps[s->pic_visible_map], s->pic->size);
}


void
con_restore_screen(state_t *s, byte *backup)
{
  memcpy(s->pic->maps[s->pic_visible_map], backup, s->pic->size);
  graph_update_box(s, 0, 0, 320, 200);
  free(backup);
}


void
con_draw(state_t *s, byte *backup)
{
  int inplines = 1 + strlen(_inpbuf) / SCI_CONSOLE_LINE_WIDTH;
  int outplines = con_visible_rows - inplines;
  int nextline, i;
  int xc, yc, pos;
  byte *target = s->pic->maps[s->pic_visible_map];

  pos = 0;
  for (yc = 0; yc < con_visible_rows * 8 + 3; yc++) /* Add background */
    for (xc = 0; xc < 320; xc++) {
      target[pos] = (backup[pos] & 0xf0) | SCI_CONSOLE_BGCOLOR;
      pos++;
    }

  /* Draw bar */
  memset(target + 320 * (con_visible_rows * 8 + 1),
	 SCI_MAP_EGA_COLOR(s->pic, SCI_CONSOLE_BORDERCOLOR), 320);

  pos = (con_cursor % SCI_CONSOLE_LINE_WIDTH) << 3; /* X offset */
  pos += ((con_cursor / SCI_CONSOLE_LINE_WIDTH) + outplines) * 8*320; /* Y offset */

  if (outplines < 0)
    outplines = 0;
  nextline = con_outputlookback;

  if (pos >= 0) /* Draw the cursor */
    for (i=0; i < 7; i++)
      memset(&(target[pos += 320]), SCI_MAP_EGA_COLOR(s->pic, SCI_CONSOLE_CURSORCOLOR), 7);

  pos = outplines;
  while (pos--) { /* Draw messages */
    nextline = (nextline - 1);
    if (nextline < 0)
      nextline += SCI_CONSOLE_OUTPUT_BUFFER;
    if (nextline == con_outputbufpos) break;
    con_draw_string(s->pic, s->pic_visible_map, pos, SCI_CONSOLE_LINE_WIDTH,
	       con_outputbuf[nextline], SCI_MAP_EGA_COLOR(s->pic, SCI_CONSOLE_FGCOLOR));
  }

  pos = outplines;

  if (con_visible_rows > inplines)
    i = 0;
  else { /* Draw input text */
    i = SCI_CONSOLE_LINE_WIDTH * (inplines - con_visible_rows);
    inplines = con_visible_rows;
  }
  while (inplines--) {
    con_draw_string(s->pic, s->pic_visible_map, pos++, SCI_CONSOLE_LINE_WIDTH,
		    _inpbuf + i, SCI_MAP_EGA_COLOR(s->pic, SCI_CONSOLE_INPUTCOLOR));
    i += SCI_CONSOLE_LINE_WIDTH;
  }

  graph_update_box(s, 0, 0, 320, 200);
}

#endif /* SCI_CONSOLE */
