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
  while ((i = g_vsnprintf(buf, bufsize-1, fmt, argp)) == -1) {
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

  switch (event->type) {
    
  case SCI_EVT_KEYBOARD :
    if (stl < SCI_CONSOLE_MAX_INPUT) {
      memmove(_inpbuf + con_cursor, _inpbuf + con_cursor-1,
	      SCI_CONSOLE_MAX_INPUT - con_cursor+1);
      _inpbuf[con_cursor] = event->data;
      con_cursor++;
    }
    _outputlookback = _outputbufpos;
    break;

#if 0
  case SCI_EVT_CTRL_KEY:
    switch (event->key) {

    case 'K':
      _inpbuf[con_cursor] = 0;
      break;

    case 'A':
      con_cursor = 1;
      break;

    case 'E':
      con_cursor = stl+1;
      break;

    case 'B':
      if (con_cursor > 1) con_cursor--;
      break;

    case 'P':
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

    case 'N':
    sci_evil_label_nextcommand:
      if ((z = _commandlookback + 1) == _commandbuflen + 1)
	z = 0;

      if (_commandlookback != _commandbufpos) {
	_commandlookback = z;

	memcpy(con_input, &(_commandbuf[_commandlookback]), SCI_CONSOLE_MAX_INPUT);
	con_cursor = strlen(con_input)+1;
      }
      break;

    case 'F':
      if (con_cursor <= stl) con_cursor++;
      break;

    case 'H':
      if (con_cursor > 1) {
	memmove(_inpbuf + con_cursor-1, _inpbuf + con_cursor,
		SCI_CONSOLE_MAX_INPUT - con_cursor);
	con_cursor--;
      }
      break;

    case 'D':
      if (con_cursor <= stl) {
	memmove(_inpbuf + con_cursor, _inpbuf + con_cursor +1,
		SCI_CONSOLE_MAX_INPUT - con_cursor- 1);
      }
      break;

    case 'M': /* Enter */
      memcpy(&(_commandbuf[z = _commandbufpos]), con_input, SCI_CONSOLE_MAX_INPUT);
      ++_commandbufpos;
      _commandlookback = _commandbufpos %= SCI_CONSOLE_INPUT_BUFFER;
      if (_commandbuflen < _commandbufpos)
	_commandbuflen = _commandbufpos;

      con_input[0] = '\0';
      con_cursor = 1;
      event->type = SCI_EV_NOEVENT;
      _outputlookback = _outputbufpos;
      return _commandbuf[z];

    }
    _outputlookback = _outputbufpos;
    break;

  case SCI_EV_SPECIAL_KEY:
    z = 0;
    switch (event->key) {

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

    if (z) break;
#endif
  default:
    return NULL;
  }
  event->type = SCI_EVT_ERROR;
  return NULL;
}


#endif /* SCI_CONSOLE */
