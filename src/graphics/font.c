/***************************************************************************
 font.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

 History:

   990402 - created (CJR)

***************************************************************************/
/* Font and font box drawing. */
/* The font interpretation is based on the SCI Font Specification as put
** down by Rainer De Temple.
*/

#include <engine.h>
#include "graphics.h"
#include <console.h>

#ifdef _MSC_VER
#include <ctype.h>
#endif

static int maxchar; /* biggest available character number +1 (usually 0x80) */
static int lastwidth, lastheight;
static int maxheight;
static int lastx, lasty;
static int rowwidths[32];
static int rowheights[32];


int
get_font_height(byte *font)
{
  return getInt16(font + FONT_FONTSIZE_OFFSET);
}

int
get_font_entries_nr(byte *font)
{
  return getInt16(font + FONT_MAXCHAR_OFFSET);
}

int
get_text_width(char *text, byte *font)
/* Similar to getTextParams, but threadsafe and less informative */
{
  unsigned char foo;
  int maxwidth = 0;
  int localmaxwidth = 0;

  while ((foo = *text++)) {
    if ((foo == '\n') || (foo == 0x0d)) {
      if (localmaxwidth > maxwidth)
	maxwidth = localmaxwidth;
      localmaxwidth = 0;
    } else { /* foo != '\n' */
      guint16 quux = getInt16((guint8 *) font+6+(foo<<1));
      guint8 *foopos = font + quux;
      localmaxwidth += *foopos;
    }
  }

  if (localmaxwidth > maxwidth)
    return localmaxwidth;

  return maxwidth;
}


void
get_text_size(char *text, byte *font, int max_allowed_width, int *width, int *height)
/* Similar to get_text_width, but detects both width and height, at a specified maxwidth */
{
  unsigned char foo;
  int maxwidth = 0;
  int localmaxwidth = 0;
  int lineheight = getInt16(font + FONT_FONTSIZE_OFFSET);
  int maxheight = lineheight;
  int last_breakpoint = 0;
  int last_break_width = 0;

  if (max_allowed_width < 0)
    max_allowed_width = 32767;

  while ((foo = *text++)) {
    if ((foo == '\n') || (foo == 0x0d)) {

      if (foo == '\n' && *text)
	maxheight += lineheight;

      if (localmaxwidth > maxwidth)
	maxwidth = localmaxwidth;
      localmaxwidth = 0;

    } else { /* foo != '\n' */
      guint16 quux = getInt16((guint8 *) font+6+(foo<<1));
      guint8 *foopos = font + quux;

      localmaxwidth += *foopos;

      if (*text == ' ') {
	last_breakpoint = localmaxwidth;
	last_break_width = *foopos;
      }

      if (localmaxwidth > max_allowed_width) {
	maxheight += lineheight;
	
	if (last_breakpoint == 0) { /* Text block too long and without whitespace? */
	  last_breakpoint = localmaxwidth - *foopos;
	  last_break_width = 0;
	}

	if (last_breakpoint == 0) {
	  sciprintf("font.c: get_text_size: Warning: maxsize %04x too small for '%s'\n",
		    max_allowed_width, text);
	  return;
	}

	if (last_breakpoint > maxwidth)
	  maxwidth = last_breakpoint;
	localmaxwidth = localmaxwidth - last_breakpoint/* - last_break_width*/;

      }

    }
  }

  if (localmaxwidth > maxwidth)
    *width = localmaxwidth;
  else
    *width = maxwidth;

  *height = maxheight;
}


void getTextParams(char *text, char *font)
/* sets lastwidth and lastheight properly */
{
  unsigned char foo;
  int rowcounter = 0;
  int localmaxwidth=0, localmaxheight=0;

  lastheight = 0;
  maxchar = getInt16(font+2);

  while ((foo = *(text++))) {
    if (foo != 0x0d) {
      if (foo == '\n') {
	if (localmaxwidth > lastwidth) lastwidth = localmaxwidth;
	rowheights[rowcounter] = localmaxheight;
	rowwidths[rowcounter++] = localmaxwidth;
	localmaxwidth = 0;
	lastheight += localmaxheight + 1;
	localmaxheight = 0;
      } else if (foo < maxchar) {
	guint16 quux = getInt16((guint8 *) font+6+(foo<<1));
	guint8 *foopos = font + quux;
	localmaxwidth += *foopos;
	if (localmaxheight < *(foopos+1))
	  localmaxheight = *(foopos+1);
      }
    }
  }
  if (localmaxwidth > lastwidth)
    lastwidth = localmaxwidth;

  rowheights[rowcounter] = localmaxheight;
  rowwidths[rowcounter++] = localmaxwidth;
  lastheight += localmaxheight + 1;
}

static inline void
_draw_text0(picture_t dest, port_t *port, int x, int y, char *text, char *font,
	   char color, int with_newline);

void
draw_text0(picture_t dest, port_t *port, int x, int y, char *text, char *font,
	   char color)
{
  _draw_text0(dest, port, x, y, text, font, color, 1);
}

void
draw_text0_without_newline(picture_t dest, port_t *port, int x, int y, char *text, char *font,
			   char color)
{
  _draw_text0(dest, port, x, y, text, font, color, 0);
}


static inline void
_draw_text0(picture_t dest, port_t *port, int x, int y, char *text, char *font,
	   char color, int with_newline)
{
  unsigned char foo;
  short rowcounter = 0;
  short xhome = x + port->xmin;
  int priority = port->priority;

  x += port->xmin;
  y += port->ymin;

  getTextParams(text, font);

  while ((foo= *(text++))) {
    if (foo != 0x0d) {
      if (foo == '\n' && with_newline) {
	y += rowheights[rowcounter++] + 1;
	x = xhome + ((lastwidth - rowwidths[rowcounter]) >> 1);
      } else if (foo < maxchar) {
	int xc;
	int yc;
	int xl, yl;
	int quux = getInt16((guint8 *) font+6+(foo<<1));
	byte *foopos = font + quux;
	int pos = x+ (y*SCI_SCREEN_WIDTH);
	int bytes;

	xl = *foopos;
	yl = *(foopos+1);
	foopos += 2;
	bytes = (xl - 1) >> 3;
	if (bytes > 3)
	  fprintf(stderr,"%s L%d: Warning: Character %d occupies %d bytes\n", __FILE__, __LINE__, foo, bytes+1);

	for (; yl; --yl) {
	  int poshome = pos;
	  int bc = bytes;
	  unsigned long bitmask = *foopos++;
	  unsigned long cmpmask = 1 << ((bc*8) + 7);

	  while (bc--) {
	    bitmask <<= 8;
	    bitmask |= *foopos++;
	  }

	  for (xc = 0; xc < xl; xc++) {
	    if (bitmask & cmpmask)
	      dest->maps[0][pos] = color;
	    /*	    else if (bgcolor >= 0) ## no bgcolor... ##
		    dest->maps[0][pos] = bgcolor;*/

	    if (priority >= 0)
	      dest->maps[1][pos] = priority;

	    pos++;
	    cmpmask >>= 1;
	  }

	  pos = poshome + SCI_SCREEN_WIDTH;
	}

	x += xl;
      }
    }
  }
}


static int
_text_draw_line(picture_t dest, int x, int y, char *text, int textlen, char *font,
		int c1, int c2, int bgcolor, int priority)
     /* Draws one line of text and returns the maximum height encountered */
{
  int line_height = 0;
  char foo;
  int maxchar = getInt16(font + FONT_MAXCHAR_OFFSET);

  while ((foo= *(text++)) && (textlen--))
    if (foo < maxchar) {
	int xc;
	int yc;
	int xl, yl;
	int quux = getInt16((guint8 *) font+6+(foo<<1));
	byte *foopos = font + quux;
	int pos = x+ (y*SCI_SCREEN_WIDTH);
	int bytes;

	xl = *foopos;
	yl = *(foopos+1);
	foopos += 2;
	bytes = (xl - 1) >> 3;
	if (bytes > 3)
	  fprintf(stderr,"%s L%d: Warning: Character %d occupies %d bytes\n", __FILE__, __LINE__, foo, bytes+1);

	for (; yl; --yl) {
	  int poshome = pos;
	  int bc = bytes;
	  unsigned long bitmask = *foopos++;
	  unsigned long cmpmask = 1 << ((bc*8) + 7);

	  while (bc--) {
	    bitmask <<= 8;
	    bitmask |= *foopos++;
	  }

	  for (xc = 0; xc < xl; xc++) {
	    if (bitmask & cmpmask)
	      dest->maps[0][pos] = ((pos^yl) &1)? c1:c2;
	    else if (bgcolor >= 0)
	      dest->maps[0][pos] = bgcolor;
	    
	    if (priority >= 0)
	      dest->maps[1][pos] = priority;
	    pos++;
	    cmpmask >>= 1;
	  }

	  pos = poshome + SCI_SCREEN_WIDTH;
	}

	x += xl;
    }

  return line_height;
}


void draw_text0_centered(picture_t dest, port_t *port, int x, int y, char *text, char *font,
			 char color)
{
  getTextParams(text, font);

  draw_text0(dest, port,
	     x + (((port->xmax - port->xmin - rowwidths[0]) >> 1)),
	     y, text, font, color);
}

void
text_draw(picture_t dest, port_t *port, char *text, int maxwidth)
{
  int x = port->x + port->xmin;
  int y = port->y + port->ymin;
  unsigned char foo;
  short xhome = x;
  int width = 0;
  char* last_text_base = text;
  int last_text_count = 0;
  int last_breakpoint = 0;
  int last_breakpoint_width = 0;
  int maxchar = getInt16(port->font + FONT_MAXCHAR_OFFSET);
  int line_height = getInt16(port->font + FONT_FONTSIZE_OFFSET);
  int priority = port->priority;
  /*  int bgcolor = port->bgcolor; */

  if (maxwidth < 0)
    maxwidth = port->xmax - port->x; /* Negative means unlimited */

  do {
    foo = *text++;
    if (foo < maxchar) {
      guint16 quux = getInt16((guint8 *) port->font+6+(foo<<1));
      guint8 *foopos = port->font + quux;

      int xl = *foopos; /* Get width */

      if ((width > maxwidth) || (foo == '\n') || (foo == 0x0d) || (foo == 0)) {

	if ((foo == '\n') || (foo == 0) || (foo == 0x0d)) { /* newline or end of text */

	  last_breakpoint = last_text_count;
	  last_breakpoint_width = width;

	  if ((foo == 0x0d) && (text[1]))
	    ++last_text_count;

	} else {
	  if (!last_breakpoint) /* No blank to break at? */
	    last_breakpoint = last_text_count;

	  if (!last_breakpoint) { /* Uh-oh... */
	    sciprintf("text_draw(): MaxWidth %d is too small for '%s'\n", maxwidth, last_text_base);
	    return;
	  }
	}

	if ((last_text_count) && (foo != 10)) /* Unless we only have a single char that's too wide */
	  text--; /* unget char */
	else last_text_count++;

	if (port->alignment == ALIGN_TEXT_RIGHT)
	  x += maxwidth - last_breakpoint_width;
	else if (port->alignment == ALIGN_TEXT_CENTER)
	  x += ((maxwidth - last_breakpoint_width) / 2);

	if (port->gray_text)
	  _text_draw_line(dest, x, y, last_text_base, last_breakpoint,
			  port->font, port->bgcolor | (port->color << 4),
			  port->color | (port->bgcolor << 4),
			  port->bgcolor | (port->bgcolor << 4),
			  priority);
		  else
	  _text_draw_line(dest, x, y, last_text_base, last_breakpoint,
			  port->font, port->color | (port->color << 4),
			  port->color | (port->color << 4),
			  port->bgcolor | (port->bgcolor << 4),
			  priority);

	y += line_height;

	text = last_text_base += last_breakpoint + 1 + ((foo == 0x0d) && text[1]); /* Step over 0x0d */

	last_breakpoint = width = last_text_count = 0;
	x = xhome;

      } else {  /* No reason to draw */

	if (foo == ' ') {
	  last_breakpoint = last_text_count;
	  last_breakpoint_width = width;
	}

	width += xl;

	last_text_count++;
      }
    }
  } while (foo);
}

