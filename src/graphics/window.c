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

#include <resource.h>
#include <graphics.h>

void drawBox(picture_t dest, short x, short y, short xl, short yl, char color, char priority)
{
  int c; /* counter */
  int startpos, pos;

  if (x<0) x = 0;
  if (y<10) y = 10;
  if (x+xl>319) xl = 319-x;
  if (y+yl>199) yl = 199-yl;
  if (xl<1) return;
  if (yl<1) return;

  startpos = pos = x + y*320;
  startpos -= 321;

  for (c=0; c<yl; c++) {
    memset(dest[0]+pos,color,xl);
    if (priority >= 0)
      memset(dest[1]+pos,priority,xl);
    pos += 320;
  }

}



void drawWindow(picture_t dest, port_t *port, char color, char priority,
		char *title, guint8 *titlefont, gint16 flags)
{
  int x = port->xmin;
  int y = port->ymin;
  int xl = port->xmax - x + 1;
  int yl = port->ymax - y + 1;
  port_t headerport;

  if (!(flags & WINDOW_FLAG_DONTDRAW)) {

    if (flags & WINDOW_FLAG_TITLE) {
      memcpy(&headerport, port, sizeof (port_t)); /* Create a header */
      headerport.ymax = y - 1;
      headerport.ymin -= 10;

      drawWindow(dest, &headerport, 8, priority, NULL, NULL,
		 flags & (WINDOW_FLAG_TRANSPARENT | WINDOW_FLAG_NOFRAME)); /* Draw header */

      drawTextCentered0(dest, &headerport, 0, 0, title, titlefont, 15); /* Draw header text */

    }

    if (!(flags & WINDOW_FLAG_NOFRAME)) {
      int xdrawpos = (x < 1)? 0 : x - 1;
      int xdrawlen = (x + xl + 2> 319)? 318 - x : xl + 2;
      int xshadelen = (x + xl + 3 > 319)? 317 - x : xl + 2;
      int ydrawpos = (y < 11)? 10 : y - 1;
      int ydrawend = (port->ymax + 2 > 199)? 198 - port->ymax : port->ymax + 2;
      int pos, cn;

      if (y > 10) {
	memset(&(dest[0][(y-1)*320 + xdrawpos]), 0, xdrawlen);
	memset(&(dest[1][(y-1)*320 + xdrawpos]), priority, xdrawlen);
      }

      if ((port->ymax) < 199) {
	memset(&(dest[0][(port->ymax+1)*320 + xdrawpos]), 0, xdrawlen);
	memset(&(dest[1][(port->ymax+1)*320 + xdrawpos]), priority, xdrawlen);
      }

      if ((port->ymax) < 198) {
	memset(&(dest[0][(port->ymax+2)*320 + xdrawpos+1]), 0, xshadelen);
	memset(&(dest[1][(port->ymax+2)*320 + xdrawpos+1]), priority, xshadelen);
      }

      pos = ydrawpos * 320 + xdrawpos;

      for (cn = ydrawpos; cn < ydrawend; cn++) {
	if (x >= 0) {
	  dest[0][pos] = 0;
	  dest[1][pos] = priority;
	}
	if (port->xmax < 319) {
	  dest[0][pos + xdrawlen - 1] = 0;
	  dest[1][pos + xdrawlen - 1] = priority;
	}
	if ((cn > ydrawpos) && (port->xmax < 318)) {
	  dest[0][pos + xdrawlen] = 0;
	  dest[1][pos + xdrawlen] = priority;
	}

	pos += 320;
      }
    }

  }

  if (!(flags & WINDOW_FLAG_TRANSPARENT))
    drawBox(dest, x, y, xl, yl, color, priority); /* draw box last to overwrite header */

}

