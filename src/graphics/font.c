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

#include "graphics.h"

short maxchar; /* biggest available character number +1 (usually 0x80) */
short lastwidth, lastheight;
short maxheight;
short lastx, lasty;
short rowwidths[32];
short rowheights[32];


int
get_text_width(char *text, byte *font)
/* Similar to getTextParams, but threadsafe and less informative */
{
  unsigned char foo;
  int maxwidth = 0;
  int localmaxwidth = 0;

  while (foo = *text++) {
    if (foo == '\n') {
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


void getTextParams(char *text, char *font)
/* sets lastwidth and lastheight properly */
{
  unsigned char foo;
  int rowcounter = 0;
  int localmaxwidth=0, localmaxheight=0;

  lastheight = 0;
  maxchar = getInt16(font+2);

  while ((foo = *(text++))) {
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
      if (localmaxheight < *(foopos+1)) localmaxheight = *(foopos+1);
    }
  }
  if (localmaxwidth > lastwidth) lastwidth = localmaxwidth;
  rowheights[rowcounter] = localmaxheight;
  rowwidths[rowcounter++] = localmaxwidth;
  lastheight += localmaxheight + 1;
}


void drawTextCentered0(picture_t dest, port_t *port, int x, int y, char *text, char *font,
		  char color)
{
  getTextParams(text, font);
  drawText0(dest, port,
	    x + ((port->xmax - port->xmin - rowwidths[0]) >> 1),
	    y, text, font, color);
}

void drawText0(picture_t dest, port_t *port, int x, int y, char *text, char *font,
		  char color)
{
  unsigned char foo;
  short rowcounter = 0;
  short xhome = x + port->xmin;

  x += port->xmin;
  y += port->ymin;

  getTextParams(text, font);
  x += (lastwidth - rowwidths[0]) >> 1;

  while ((foo= *(text++))) {
    if (foo == '\n') {
      y += rowheights[rowcounter++] + 1;
      x = xhome + ((lastwidth - rowwidths[rowcounter]) >> 1);
    } else if (foo < maxchar) {
      short xc;
      short yc;
      unsigned char xl, yl;
      guint16 quux = getInt16((guint8 *) font+6+(foo<<1));

      guint8 *foopos = font + quux;
      int pos = x+ (y*320);

      xl = *foopos;
      yl = *(foopos+1);
      foopos += 2;
      if (xl < 9) /* 8 bit */
	for (yc = 0; yc < yl; yc++) {
	  int poshome = pos;
	  guint8 bitmask = *(foopos++);
	  for (xc = 0; xc < xl; xc++) {
	    if (bitmask & 0x80) dest[0][pos] = color;
	    pos++;
	    bitmask <<= 1;
	  }
	  pos = poshome + 320;
	}
      else { /* 16 bit */
	for (yc = 0; yc < yl; yc++) {
	  int poshome = pos;
	  guint16 bitmask = *(foopos+1) | *foopos << 8;
	  /* interestingly, this bitmask is big-endian */
	  foopos += 2;
	  for (xc = 0; xc < xl; xc++) {
	    if (bitmask & 0x8000) dest[0][pos] = color;
	    bitmask <<= 1;
	    pos++;
	  }
	  pos = poshome + 320;
	}
      }
      x += xl;
    }
  }
}

