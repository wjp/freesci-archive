/***************************************************************************
 window.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Provides SCI0 window drawing functions */

#include <engine.h>

void
draw_box(picture_t dest, int x, int y, int xl, int yl, int color, int priority)
{
  int c; /* counter */
  int startpos, pos;

  if (x<0) x = 0;
  if (y<10) y = 10;
  if (x+xl>319) xl = 319-x;
  if (y+yl>199) yl = 199-y;
  if (xl<1) return;
  if (yl<1) return;

  startpos = pos = x + y*320;
  startpos -= 321;

  for (c=0; c<yl; c++) {
    memset(dest->maps[0]+pos,color,xl);
    if (priority >= 0)
      memset(dest->maps[1]+pos,priority,xl);
    pos += 320;
  }

}

void
fill_box(picture_t dest, int x, int y, int xl, int yl, int value, int map)
{
  int pos;
  int width;
  int lines;

  if (x<0) x = 0;
  if (y<10) y = 10;
  if (x+xl>319) xl = 319-x;
  if (y+yl>199) yl = 199-yl;
  if (xl<1) return;
  if (yl<1) return;

  width = dest->bytespp * xl;
  pos = (y * dest->yfact * dest->bytespl) + (x * dest->xfact * dest->bytespp);
  lines = yl * dest->yfact;

  for (; lines; lines--) {
    memset(dest->maps[map] + pos, value, width);
    pos += dest->bytespl;
  }
}


void
draw_frame(picture_t dest, int x, int y, int xl, int yl, int color, int priority,
	   int stipple)
{
  int c; /* counter */
  int i;
  int startpos, pos;

  color = SCI_MAP_EGA_COLOR(dest, color);

  if (x<0) x = 0;
  if (y<10) y = 10;
  if (x+xl>319) xl = 319-x;
  if (y+yl>199) yl = 199-yl;
  if (xl<1) return;
  if (yl<1) return;

  startpos = pos = x + y*320;
  startpos -= 321;

  for (c=0; c<yl; c++) {

    if ((c == 0) || (c == yl-1)) {

      if (!stipple)
	memset(dest->maps[0]+pos,color,xl);
      else
	for (i = pos + 1; i < pos+xl; i += 2)
	  *(dest->maps[0]+i) = color;

      if (priority >= 0)
	memset(dest->maps[1]+pos,priority,xl);

    } else {

      if (!stipple || (c & 0x1))
	*(dest->maps[0]+pos) = color;
      if (!stipple || (c & 0x1))
	*(dest->maps[0]+pos+xl-1) = color;

      if (priority >= 0) {
	*(dest->maps[1]+pos) = priority;
	*(dest->maps[1]+pos+xl-1) = priority;
      }

    }
    pos += 320;
  }

}

void
draw_titlebar(picture_t dest, int color)
{
  int i;

  color = SCI_MAP_EGA_COLOR(dest, color);

  for (i = 0; i < 10; i++) {
    memset(dest->maps[0] + i * 320, (i == 9)? 0 : color, 320);
    memset(dest->maps[1] + i * 320, 10, 320); /* Priority for the menubar */
  }
}


void
draw_titlebar_section(picture_t dest, int start, int length, int color)
{
  int i;

  if (start > 319)
    return;

  if (start + length > 319)
    length = 320 - start;

  color = SCI_MAP_EGA_COLOR(dest, color);

  for (i = 0; i < 10; i++) {
    memset(dest->maps[0] + i * 320 + start, (i == 9)? 0 : color, length);
    memset(dest->maps[1] + i * 320 + start, 10, length); /* Priority for the menubar */
  }
}


void draw_window(picture_t dest, port_t *port, int color, int priority,
		 char *title, guint8 *titlefont, int flags)
{
  int x = port->xmin;
  int y = port->ymin;
  int xl = port->xmax - x + 1;
  int yl = port->ymax - y + 1;
  port_t headerport;

  if (!(flags & WINDOW_FLAG_NOFRAME)) {
    ++x;
    ++y;
    xl -=3;
    yl -=3;
  } /* Adjust centerpiece */

  color = SCI_MAP_EGA_COLOR(dest, color);

  if (!(flags & WINDOW_FLAG_DONTDRAW)) {

    if (flags & WINDOW_FLAG_TITLE) {
      memcpy(&headerport, port, sizeof (port_t)); /* Create a header */
      headerport.ymax = y;
      headerport.ymin -= 10;

      draw_window(dest, &headerport, 0x88, priority, NULL, NULL,
		 flags & (WINDOW_FLAG_TRANSPARENT | WINDOW_FLAG_NOFRAME)); /* Draw header */

      draw_text0_centered(dest, &headerport, 0, 1, title, titlefont, 0xff); /* Draw header text */

    }

    if (!(flags & WINDOW_FLAG_NOFRAME)) {

      int xdrawpos = (x < 1)? 0 : x - 1;
      int xdrawlen = (x + xl + 2> 319)? 319 - x : xl + 2;
      int xshadelen = (x + xl + 3 > 319)? 319 - x : xl + 1;
      int ydrawpos = (y < 11)? 10 : y - 1;
      int ydrawend = (port->ymax > 199)? 199 : port->ymax;
      int pos, cn;

      if (y >= 10) {
	memset(&(dest->maps[0][(y-1)*320 + xdrawpos]), 0, xdrawlen);
	memset(&(dest->maps[1][(y-1)*320 + xdrawpos]), priority, xdrawlen);
      }

      if ((port->ymax) < 201) {
	memset(&(dest->maps[0][(port->ymax-1)*320 + xdrawpos]), 0, xdrawlen + 1);
	memset(&(dest->maps[1][(port->ymax-1)*320 + xdrawpos]), priority, xdrawlen + 1);
      }

      if ((port->ymax) < 200) {
	memset(&(dest->maps[0][(port->ymax)*320 + xdrawpos+1]), 0, xshadelen + 1);
	memset(&(dest->maps[1][(port->ymax)*320 + xdrawpos+1]), priority, xshadelen + 1);
      }

      pos = ydrawpos * 320 + xdrawpos;

      for (cn = ydrawpos; cn < ydrawend; cn++) {
	if (x >= 0) {
	  dest->maps[0][pos] = 0;
	  dest->maps[1][pos] = priority;
	}
	if (port->xmax < 319) {
	  dest->maps[0][pos + xdrawlen - 1] = 0;
	  dest->maps[1][pos + xdrawlen - 1] = priority;
	}
	if ((cn > ydrawpos) && (port->xmax < 318)) {
	  dest->maps[0][pos + xdrawlen] = 0;
	  dest->maps[1][pos + xdrawlen] = priority;
	}

	pos += 320;
      }
    }

  }

  if (!(flags & WINDOW_FLAG_TRANSPARENT))
    draw_box(dest, x, y, xl, yl, color, priority); /* draw box last to overwrite header */

}

