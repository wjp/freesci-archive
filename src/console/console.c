/***************************************************************************
 console.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

int con_display_row = 0;
int con_row_counter = 0;
int con_visible_rows = 0;
int con_cursor = 1;
int con_passthrough = 0;
FILE *con_file;

char _commandbuf[SCI_CONSOLE_INPUT_BUFFER][SCI_CONSOLE_MAX_INPUT];
char _outputbuf[SCI_CONSOLE_OUTPUT_BUFFER][SCI_CONSOLE_LINE_WIDTH];
int _outputbufpos = 0; /* buffer line */
int _outputbufcolumn = 0;
int _outputbuflen = 0;
int _outputlookback = 0;
int _commandbuflen = 0;
int _commandbufpos = 0;
int _commandlookback = 0;

char _inpbuf[SCI_CONSOLE_MAX_INPUT + 1] = "$\0";
char *con_input = _inpbuf + 1;


void *_xmalloc(size_t size)
{
  void *retval = (void *) malloc(size);
  if (!retval) {
    fprintf(stderr,"Out of memory!\n");
    exit(1);
  }
  return retval;
}


void sciprintf(char *fmt, ...)
{
  va_list argp;
  size_t bufsize = 256;
  int i;
  char *buf = (char *) _xmalloc(bufsize);
  char *mbuf;

  va_start(argp, fmt);
  while ((i = g_vsnprintf(buf, bufsize-1, fmt, argp)) == -1 || (i > bufsize - 1)) {
    /* while we're out of space... */
    va_end(argp);
    va_start(argp, fmt); /* reset argp */

    free(buf);
    buf = (char *) _xmalloc(bufsize <<= 1);
  }
  va_end(argp);

  if (con_passthrough)
    printf("%s",buf);
  if (con_file)
    fprintf(con_file, "%s", buf);

  mbuf = buf;

  i = strlen(mbuf);

  while (i > 0) {
    char *seekerpt = strchr(mbuf, '\n');
    int seeker = seekerpt? seekerpt - mbuf : SCI_CONSOLE_LINE_WIDTH - _outputbufcolumn;
    if (seeker >= SCI_CONSOLE_LINE_WIDTH - _outputbufcolumn)
      seeker = (i > SCI_CONSOLE_LINE_WIDTH - _outputbufcolumn)?
	SCI_CONSOLE_LINE_WIDTH - _outputbufcolumn : i;

    memcpy(_outputbuf[_outputbufpos] + _outputbufcolumn, mbuf, seeker);
    if (seeker + _outputbufcolumn < SCI_CONSOLE_LINE_WIDTH)
      _outputbuf[_outputbufpos][seeker + _outputbufcolumn] = 0;

    _outputbufcolumn += seeker;

    if (_outputbufcolumn == SCI_CONSOLE_LINE_WIDTH) {
      _outputbufpos++;
      _outputbufcolumn = 0;
    }

    if (seekerpt && (seeker == seekerpt - mbuf)) {
      _outputbufcolumn = 0;
      _outputbufpos++;
      mbuf++;
      i--;
    }

    mbuf += seeker;
    i -= seeker;

    if (_outputbufpos >= SCI_CONSOLE_OUTPUT_BUFFER) {
      _outputbufpos -= SCI_CONSOLE_OUTPUT_BUFFER;
      _outputbuflen = SCI_CONSOLE_OUTPUT_BUFFER;
    }
  }
  if (_outputbuflen < _outputbufpos)
    _outputbuflen = _outputbufpos;

  _outputlookback = _outputbufpos;
  free(buf);
}



char *
consoleInput(sci_event_t *event)
/* This function is hell. Feel free to clean it up. */
{
  int stl = strlen(con_input);
  int z,i; /* temp vars */

  if (con_visible_rows <= 0) return NULL;

  if (event->type == SCI_EVT_KEYBOARD) {
    
    if ((event->buckybits & (SCI_EVM_NUMLOCK | SCI_EVM_CTRL | SCI_EVM_ALT)) == 0) {
      if (event->data >= 32) {
	if (stl < SCI_CONSOLE_MAX_INPUT) {
	  memmove(_inpbuf + con_cursor, _inpbuf + con_cursor-1,
		  SCI_CONSOLE_MAX_INPUT - con_cursor+1);

	  if (event->buckybits & (SCI_EVM_RSHIFT | SCI_EVM_LSHIFT | SCI_EVM_CAPSLOCK)) /* Shift? */
	    switch (event->data) {

	    case '-': _inpbuf[con_cursor] = '_'; break;

	    default:
	      _inpbuf[con_cursor] = toupper(event->data);

	    } else _inpbuf[con_cursor] = event->data;  

	  con_cursor++;
	}
	_outputlookback = _outputbufpos;
      } else switch (event->data) {

	case SCI_K_ENTER: goto sci_evil_label_enter;

	case SCI_K_BACKSPACE: goto sci_evil_label_backspace;

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
	  memcpy(&(_commandbuf[_commandbufpos]), con_input, SCI_CONSOLE_MAX_INPUT);

	if ((z = _commandlookback - 1) == -1)
	  z = _commandbuflen;

	if (z != _commandbufpos) {
	  _commandlookback = z;

	  memcpy(con_input, &(_commandbuf[_commandlookback]), SCI_CONSOLE_MAX_INPUT);
	  con_cursor = strlen(con_input)+1;
	}
	break;

	case 'n':
	sci_evil_label_nextcommand:
	if ((z = _commandlookback + 1) == _commandbuflen + 1)
	  z = 0;

	if (_commandlookback != _commandbufpos) {
	  _commandlookback = z;

	  memcpy(con_input, &(_commandbuf[_commandlookback]), SCI_CONSOLE_MAX_INPUT);
	  con_cursor = strlen(con_input)+1;
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
	memcpy(&(_commandbuf[z = _commandbufpos]), con_input, SCI_CONSOLE_MAX_INPUT);
	++_commandbufpos;
	_commandlookback = _commandbufpos %= SCI_CONSOLE_INPUT_BUFFER;
	if (_commandbuflen < _commandbufpos)
	  _commandbuflen = _commandbufpos;

	con_input[0] = '\0';
	con_cursor = 1;
	_outputlookback = _outputbufpos;
	return _commandbuf[z];

	}
	_outputlookback = _outputbufpos;

      } else if (event->buckybits & SCI_EVM_NUMLOCK)
	switch (event->data) {

	case SCI_K_LEFT:
	  if (con_cursor > 1) con_cursor--;
	  z = 1;
	  _outputlookback = _outputbufpos;
	  break;

	case SCI_K_UP:
	  goto sci_evil_label_prevcommand;

	case SCI_K_DOWN:
      goto sci_evil_label_nextcommand;

	case SCI_K_RIGHT:
	  if (con_cursor <= stl) con_cursor++;
	  z = 1;
	  _outputlookback = _outputbufpos;
	  break;

	case SCI_K_PGUP:
	  for (i = 0; i < 3; i++) {
	    if ((z = _outputlookback - 1) <= 0)
	      z += _outputbuflen;
	    if (z == _outputbufpos)
	      break;
	    _outputlookback = z;
	  }
	  z = 1;
	  break;

	case SCI_K_PGDOWN:
	  for (i = 0; i < 3; i++) {
	    if ((z = _outputlookback + 1) > _outputbuflen)
	      z = 1;
	    if (_outputlookback == _outputbufpos)
	      break;
	    _outputlookback = z;
	  }
	  z = 1;
	  break;

	case SCI_K_DELETE:
	  if (con_cursor <= stl) {
	    memmove(_inpbuf + con_cursor, _inpbuf + con_cursor +1,
		    SCI_CONSOLE_MAX_INPUT - con_cursor- 1);
	  }
	  _outputlookback = _outputbufpos;
	  z = 1;
	  break;

	}
  }
  return NULL;
}


byte *
con_backup_screen(state_t *s)
{
  byte *retval = malloc(s->pic->size);

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
  int blen = (con_visible_rows << 3) + 3;
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
  nextline = _outputlookback;

  if (pos >= 0) /* Draw the cursor */
    for (i=0; i < 7; i++)
      memset(&(target[pos += 320]), SCI_MAP_EGA_COLOR(s->pic, SCI_CONSOLE_CURSORCOLOR), 7);

  pos = outplines;
  while (pos--) { /* Draw messages */
    nextline = (nextline - 1);
    if (nextline < 0)
      nextline += SCI_CONSOLE_OUTPUT_BUFFER;
    if (nextline == _outputbufpos) break;
    drawString(s->pic, s->pic_visible_map, pos, SCI_CONSOLE_LINE_WIDTH,
	       _outputbuf[nextline], SCI_MAP_EGA_COLOR(s->pic, SCI_CONSOLE_FGCOLOR));
  }

  pos = outplines;

  if (con_visible_rows > inplines)
    i = 0;
  else { /* Draw input text */
    i = SCI_CONSOLE_LINE_WIDTH * (inplines - con_visible_rows);
    inplines = con_visible_rows;
  }
  while (inplines--) {
    drawString(s->pic, s->pic_visible_map, pos++, SCI_CONSOLE_LINE_WIDTH,
	       _inpbuf + i, SCI_MAP_EGA_COLOR(s->pic, SCI_CONSOLE_INPUTCOLOR));
    i += SCI_CONSOLE_LINE_WIDTH;
  }

  graph_update_box(s, 0, 0, 320, 200);
}

#endif /* SCI_CONSOLE */
