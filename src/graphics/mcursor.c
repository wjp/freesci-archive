/***************************************************************************
 mcursor.c (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Draws SCI0 mouse cursors */

#include <graphics.h>


void drawMouseCursor0(picture_t target, int x, int y, guint8 *cursor)
{
  int xc, yc, xend, yend, rowstart, bordershifter;
  guint32 bitmask; /* black'n'white bitmasks */
  int yborder;

  if (cursor[3]) { /* Set hot spot to cursor center */
    x -= 8;
    y -= 8;
  }

  if (y < 0) {
    yborder = -y;
    y = 0;
  } else yborder = 0;

  xend = x + 16;

  if (x < 0) {
    bordershifter = -x;
    x = 0;
  } else bordershifter = 0;

  if (xend > 320) xend = 320;
  yend = ((y + 16) > 200)? 200-y : 16 - yborder;

  cursor += 4; /* get beyond the header */
  cursor += (yborder << 1);
  rowstart = y * 320;

  for (yc = 0; yc < yend; yc++) {
    bitmask = (cursor[yc << 1]) << 16 | (cursor[(yc << 1) + 1] << 24)
      | cursor[(yc << 1) + 32] | cursor[(yc << 1) + 33] << 8;
    /* Stores black/trans part in upper 16b, white part in lower 16b */

    bitmask <<= bordershifter; /* clip left border */

    for (xc = x; xc < xend; xc++) {
      if ((bitmask & 0x8000) || !(bitmask & 0x80000000)) {
	target[0][rowstart + xc] = ((bitmask & 0x8000)? 0xf : 0x0);
	target[1][rowstart + xc] = 255;
      }

      bitmask <<= 1;
    }

    rowstart += 320;
  }
}


void drawMouseCursor01(picture_t target, int x, int y, guint8 *cursor)
{
  int xc, yc, xend, yend, rowstart, bordershifter;
  guint32 bitmask; /* black'n'white bitmasks */
  int yborder;

  x -= getInt16(cursor); /* subtracts the HotSpot x */
  y -= getInt16(cursor + 2); /* same for y */

  if (y < 0) {
    yborder = -y;
    y = 0;
  } else yborder = 0;

  xend = x + 16;

  if (x < 0) {
    bordershifter = -x;
    x = 0;
  } else bordershifter = 0;

  if (xend > 320) xend = 320;
  yend = ((y + 16) > 200)? 200-y : 16 - yborder;

  cursor += 4; /* get beyond the header */
  cursor += (yborder << 1);
  rowstart = y * 320;

  for (yc = 0; yc < yend; yc++) {
    bitmask = (cursor[yc << 1]) << 16 | (cursor[(yc << 1) + 1] << 24)
      | cursor[(yc << 1) + 32] | cursor[(yc << 1) + 33] << 8;
    /* Stores black/trans part in upper 16b, white part in lower 16b */

    bitmask <<= bordershifter; /* clip left border */

    for (xc = x; xc < xend; xc++) {
      if ((bitmask & 0x8000) || !(bitmask & 0x80000000)) {
	target[0][rowstart + xc] = ((bitmask & 0x8000)? 0xf : 0x0)
	  & ((bitmask & 0x80000000)? 0x7 : 0xf);
	target[1][rowstart + xc] = 255;
      }

      bitmask <<= 1;
    }

    rowstart += 320;
  }
}

void drawMouseCursor(picture_t target, int x, int y, guint8 *cursor)
{
  if (x < 0) x = 0;
  else if (x > 319) x = 319;
  if (y < 0) y = 0;
  else if (y > 199) y = 199;
  /* The original Sierra cursors resided in this area */

  if (sci_version == SCI_VERSION_0)
    drawMouseCursor0(target, x, y, cursor);
  else
    drawMouseCursor01(target, x, y, cursor);

}

