/***************************************************************************
 graphics.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

   990327 - created as "picview.c" (CJR)
   990401 - turned into "graphics.c" (CJR)

***************************************************************************/

#include "unistd.h"
#include "glib.h"
#include "stdio.h"
#include "graphics.h"

#define DEBUG_DRAWPIC

#ifdef DEBUG_DRAWPIC
#define DRAWPICMSG(a...) fprintf(stderr, a...);
#else
#define DRAWPICMSG(a...)
#endif

int sci_color_mode = SCI_COLOR_DITHER;

picture_t allocEmptyPicture()
{
  picture_t retval = g_malloc(sizeof(gint8 *) * 4);
  int i;
  for (i=0; i<4; i++)
    retval[i] = g_malloc(64000);
  return retval;
}

void freePicture(picture_t picture)
{
  int i;
  for (i=0; i<4; i++)
    g_free(picture[i]);
  g_free(picture);
}

void copyPicture(picture_t dest, picture_t src)
{
  int i;
  for (i=0; i<4; i++)
    memcpy(dest[i], src[i], 64000);
}


void clearPicture(picture_t pic, int fgcol)
{
  int i;

  memset(pic[0], 0, 320 * 10); /* Title bar */
  memset(pic[0]+ (320 * 10), fgcol, 320 * 190);

  for (i=1; i<4; i++)
    memset(pic[i], 0, 64000);
}

/***************************************************************************/
/* The following code is was taken directly from Carl Muckenhoupt's sde.c. */
/* It is used with permission and has been ported to plot to picture_ts    */
/* instead of directly to the graphics ram.                                */
/***************************************************************************/

int curx, cury, col1 = 0, col2 = 0;
char priority = 0, special=0;
char drawenable;
picture_t buffers = 0;


inline void putpix(int x, int y, short color, short screen)
{
  int pos;

  pos = (y+10) * 320 + x;

  switch(screen) {
  case 0: buffers[0][pos] = color; break;
  case 1: buffers[1][pos] = priority; break;
  case 2: buffers[2][pos] = special; break;
  case 3: buffers[3][pos] |= drawenable; break;
  }
}

#ifdef SCI_GRAPHICS_ALLOW_256
#define GETPIX(x, y, screen)  (buffers[(screen)][(x)+((y)*320)] \
			       & ((sci_color_mode && (screen == 1))? \
			       0xf : 0xff))
#else /* !SCI_GRAPHICS_ALLOW_256 */
#define GETPIX(x, y, screen)  (buffers[(screen)][(x)+((y)*320)])
#endif /* !SCI_GRAPHICS_ALLOW_256 */

inline void plot(int x, int y)
{
  int pos;

  pos = (y+10) * 320 + x;

#ifdef SCI_GRAPHICS_ALLOW_256
  if (sci_color_mode == SCI_COLOR_INTERPOLATE) {
    if (drawenable & 1) buffers[0][pos] = ((col1-col2)&0xf)<<4 | col2;
  } else if (sci_color_mode == SCI_COLOR_DITHER256) {
    if (drawenable & 1) buffers[0][pos] = ((x^y)&1)? ((col1-col2)&0xf)<<4 | col2
			  : (((col2-col1)&0xf))<<4 | col1;
  } else
    if (drawenable & 1) buffers[0][pos] = ((x^y)&1)? col1 : col2;
#else /* !SCI_GRAPHICS_ALLOW_256 */
  if (drawenable & 1) buffers[0][pos] = ((x^y)&1)? col1 : col2;
#endif /* !SCI_GRAPHICS_ALLOW_256 */

  if (drawenable & 2) buffers[1][pos] = priority;
  if (drawenable & 4) buffers[2][pos] = special;
  buffers[3][pos] |= drawenable;

}


#define LINEMACRO(startx, starty, deltalinear, deltanonlinear, linearvar, nonlinearvar, \
                  linearend, nonlinearstart, linearmod, nonlinearmod) \
   x = (startx); y = (starty); \
   incrNE = ((deltalinear) > 0)? (deltalinear) : -(deltalinear); \
   incrNE <<= 1; \
   deltanonlinear <<= 1; \
   incrE = ((deltanonlinear) > 0) ? -(deltanonlinear) : (deltanonlinear);  \
   d = nonlinearstart-1;  \
   while (linearvar != (linearend)) { \
     plot(x,y); \
     linearvar += linearmod; \
     if ((d+=incrE) < 0) { \
       d += incrNE; \
       nonlinearvar += nonlinearmod; \
     }; \
   }; \
   plot(x,y);

void ditherto(short x1, short y1)
{
  short temp, dx, dy, incrE, incrNE, d, x, y, finalx, finaly;
  short tc1 = col1, tc2 = col2, tx = curx, ty = cury;
  finalx = x1;
  finaly = y1;

  dx = (x1 > curx)? x1-curx : curx-x1;
  dy = (y1 > cury)? y1-cury : cury-y1;

  if (dx > dy) {
    if (x1 < curx) {
      if (y1 < cury) { /* llu == left-left-up */
	LINEMACRO(curx, cury, dx, dy, x, y, x1, dx, -1, -1);
      } else {         /* lld */
	LINEMACRO(curx, cury, dx, dy, x, y, x1, dx, -1, 1);
      }
    } else { /* x1 >= curx */
      if (y1 < cury) { /* rru */
	LINEMACRO(curx, cury, dx, dy, x, y, x1, dx, 1, -1);
      } else {         /* rrd */
	LINEMACRO(curx, cury, dx, dy, x, y, x1, dx, 1, 1);
      }
    }
  } else { /* dx <= dy */
    if (y1 < cury) {
      if (x1 < curx) { /* luu */
	LINEMACRO(curx, cury, dy, dx, y, x, y1, dy, -1, -1);
      } else {         /* ruu */
	LINEMACRO(curx, cury, dy, dx, y, x, y1, dy, -1, 1);
      }
    } else { /* y1 >= cury */
      if (x1 < curx) { /* ldd */
	LINEMACRO(curx, cury, dy, dx, y, x, y1, dy, 1, -1);
      } else {         /* rdd */
	LINEMACRO(curx, cury, dy, dx, y, x, y1, dy, 1, 1);
      }
    }
  }
  curx = finalx;
  cury = finaly;
  col2 = tc2;
  col1 = tc1;
}

#undef LINEMACRO()


int _fill_bgcol, _fill_bgcol1, _fill_bgcol2;

/* #define FILLBOUNDARY(fx, fy) (drawenable & GETPIX(fx, fy, 3) && \
			      !(drawenable&1 && GETPIX(fx, fy, 0)==15)) */
/*#define FILLBOUNDARY(fx, fy) ((drawenable & (GETPIX((fx), (fy), 3))) && \
                             !((drawenable & 1) && (GETPIX((fx), (fy), 0)==15) && \
			     ((col1 != 15) && (col2 != 15))))*/
#define FILLBOUNDARY(fx, fy) ((drawenable & (GETPIX((fx), (fy), 3))) && \
                             !((drawenable & 1) && (GETPIX((fx), (fy), 0)==15) && \
			       ((col1 != 15) && (col2 != 15))))


void fillhelp(gint16 xstart, gint16 xend, gint16 y, gint16 direction)
{
  gint16 register xright = xstart, xleft = xstart;

  y += direction;
  if (y >= 0 && y < 190) {
    if (!FILLBOUNDARY(xleft, y+10)) {
      while (xleft > 0 && !FILLBOUNDARY(xleft-1, y+10)) xleft--;
      for (xright = xleft; xright < 320 && !FILLBOUNDARY(xright, y+10);
	   xright++)
	plot(xright, y);
      if (xleft < xstart) fillhelp(xleft, xstart, y, -direction);
      fillhelp(xleft, xright-1, y, direction);
    }
    while (xright <= xend) {
      while (FILLBOUNDARY(xright, y+10)) {
	xright++;
	if (xright > xend) return;
      }
      xleft = xright;
      while (xright<320 && !FILLBOUNDARY(xright, y+10)) {
	plot(xright, y);
	xright++;
      }
      fillhelp(xleft, xright-1, y, direction);
    }
    xright--;
    if (xright > xend) fillhelp(xend, xright, y, -direction);
  }
}

void ditherfill(gint16 x, gint16 y)
{
  gint16 register xstart, xend, old_drawenable = drawenable;

  if (GETPIX(x, y+2, 1) == priority) drawenable &= ~2;
  if (GETPIX(x, y+2, 2) == special) drawenable &= ~4;

  if (FILLBOUNDARY(x, y+10)) return;
  for (xstart = x; xstart >= 0 && !FILLBOUNDARY(xstart, y+10); xstart--)
    plot(xstart, y);
  xstart++;
  for (xend = x+1; xend < 320 && !FILLBOUNDARY(xend, y+10); xend++)
    plot(xend, y);
  xend--;
  fillhelp(xstart, xend, y, 1);
  fillhelp(xstart, xend, y, -1);
  drawenable = old_drawenable;
}

unsigned char patcode, patnum;
void plotpattern(int x, int y)
{
  static gint8 circles[][30] = { /* bitmaps for circle patterns */
    {0x80},
    {0x4e, 0x40},
    {0x73, 0xef, 0xbe, 0x70},
    {0x38, 0x7c, 0xfe, 0xfe, 0xfe, 0x7c, 0x38, 0x00},
    {0x1c, 0x1f, 0xcf, 0xfb, 0xfe, 0xff, 0xbf, 0xef, 0xf9, 0xfc, 0x1c},
    {0x0e, 0x03, 0xf8, 0x7f, 0xc7, 0xfc, 0xff, 0xef, 0xfe, 0xff, 0xe7,
           0xfc, 0x7f, 0xc3, 0xf8, 0x1f, 0x00},
    {0x0f, 0x80, 0xff, 0x87, 0xff, 0x1f, 0xfc, 0xff, 0xfb, 0xff, 0xef,
	   0xff, 0xbf, 0xfe, 0xff, 0xf9, 0xff, 0xc7, 0xff, 0x0f, 0xf8,
	   0x0f, 0x80},
    {0x07, 0xc0, 0x1f, 0xf0, 0x3f, 0xf8, 0x7f, 0xfc, 0x7f, 0xfc, 0xff,
	   0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0x7f,
	   0xfc, 0x7f, 0xfc, 0x3f, 0xf8, 0x1f, 0xf0, 0x07, 0xc0}};
  static guint8 junq[32] = { /* random-looking fill pattern */
    0x20, 0x94, 0x02, 0x24, 0x90, 0x82, 0xa4, 0xa2, 0x82, 0x09, 0x0a, 0x22,
    0x12, 0x10, 0x42, 0x14, 0x91, 0x4a, 0x91, 0x11, 0x08, 0x12, 0x25, 0x10,
    0x22, 0xa8, 0x14, 0x24, 0x00, 0x50, 0x24, 0x04};
  static guint8 junqindex[128] = { /* starting points for junq fill */
    0x00, 0x18, 0x30, 0xc4, 0xdc, 0x65, 0xeb, 0x48,
    0x60, 0xbd, 0x89, 0x05, 0x0a, 0xf4, 0x7d, 0x7d,
    0x85, 0xb0, 0x8e, 0x95, 0x1f, 0x22, 0x0d, 0xdf,
    0x2a, 0x78, 0xd5, 0x73, 0x1c, 0xb4, 0x40, 0xa1,
    0xb9, 0x3c, 0xca, 0x58, 0x92, 0x34, 0xcc, 0xce,
    0xd7, 0x42, 0x90, 0x0f, 0x8b, 0x7f, 0x32, 0xed,
    0x5c, 0x9d, 0xc8, 0x99, 0xad, 0x4e, 0x56, 0xa6,
    0xf7, 0x68, 0xb7, 0x25, 0x82, 0x37, 0x3a, 0x51,
    0x69, 0x26, 0x38, 0x52, 0x9e, 0x9a, 0x4f, 0xa7,
    0x43, 0x10, 0x80, 0xee, 0x3d, 0x59, 0x35, 0xcf,
    0x79, 0x74, 0xb5, 0xa2, 0xb1, 0x96, 0x23, 0xe0,
    0xbe, 0x05, 0xf5, 0x6e, 0x19, 0xc5, 0x66, 0x49,
    0xf0, 0xd1, 0x54, 0xa9, 0x70, 0x4b, 0xa4, 0xe2,
    0xe6, 0xe5, 0xab, 0xe4, 0xd2, 0xaa, 0x4c, 0xe3,
    0x06, 0x6f, 0xc6, 0x4a, 0xa4, 0x75, 0x97, 0xe1
    };
  int k, l, size;
  guint8 junqbit = junqindex[patnum];
  size = (patcode&7);
  if (x<size) x=size;
  else if (x>320-size) x=320-size;
  if (y<size) y = size;
  else if (y>=190-size) y=189-size;
  if (patcode & 0x10) { /* rectangle */
    for (l=y-size; l<=y+size; l++) for (k=x-size; k<=x+size+1; k++) {
      if (patcode & 0x20) {
	if ((junq[junqbit>>3] >> (7-(junqbit & 7))) & 1) plot(k, l);
	junqbit++;
	if (junqbit == 0xff) junqbit=0;
      }
      else plot(k, l);
    }
  }
  else { /* circle */
    int circlebit = 0;
    for (l=y-size; l<=y+size; l++) for (k=x-size; k<=x+size+1; k++) {
      if ((circles[patcode&7][circlebit>>3] >> (7-(circlebit & 7))) & 1) {
	if (patcode & 0x20) {
	  if ((junq[junqbit>>3] >> (7-(junqbit & 7))) & 1) plot(k, l);
	  junqbit++;
	  if (junqbit == 0xff) junqbit=0;
	}
	else plot(k, l);
      }
      circlebit++;
    }
  }
}




int drawView0(picture_t dest, port_t *port, int xp, int yp, short _priority,
	      short group, short index, guint8 *data)
{
  gint8 *dataptr,*lookup;
  gint16 nloops, ncells, loop, cell;
  int x, y, maxx, maxy, minx;
  guint8 transparency;
  guint8 reverse;
  gint16 *test;
  gint16 absx = xp, absy = 10 + yp, clipmaxx = 319, clipmaxy = 199;

  if (port) {
    clipmaxy = port->ymax - port->ymin;
    clipmaxx = port->xmax - port->xmin + 1;
  }

  drawenable = 1;
  if (data == 0) return -3;
  nloops = getInt16(data);
  if (group > nloops) return -1;
  if (group < 1) return -1;
  if (index < 1) return -2;
  loop = group;
  cell = index;
  buffers = dest;
  {
    int blindtop, blindleft, blindright; /* zones outside of the picture */
    int homepos;
    int i;

    lookup = (data + (getInt16(data+6+(loop<<1))));
    ncells = getInt16(lookup);
    if (cell > ncells) return -2;
    lookup+=2;
    dataptr = data+getInt16(lookup + (cell<<1));
    maxx = getInt16(dataptr);
    maxy = getInt16(dataptr+2);

    minx = x = (xp < 0) ? 0 : xp;
    y = yp;
    dataptr += 7;
    transparency = dataptr[-1];
    reverse = (getInt16(data+2)>>(loop-1)) & 1;

    if (y < 0) { /* clipping the ceiling */
      blindtop = (-y)*maxx;
      y = 0;
    } else blindtop = 0;

    maxx += x;
    maxy += y;
    if (maxy > clipmaxy) maxy = clipmaxy;

    if (maxx < 0) return 0;
    if (y > clipmaxy) return 0;
    if (maxy < 0) return 0;

    /* clipping the sides: */
    if (xp < 0) {
      blindleft = x - xp;
      maxx -= blindleft;
      xp = 0;
    } else blindleft = 0;

    blindright = maxx-(clipmaxx + 1);

    if (maxx > (clipmaxx + 1)) maxx = (clipmaxx + 1);

    if (reverse) { /* mirror picture along the y axis */
      int j = blindleft;
      blindleft = blindright;
      blindright = j;
    } 

    {
      guint8 color, rep;

      /* Clip upper boundary */
      do {
	dataptr++;
	rep = (*dataptr >> 4) & 0xf;
	if (!rep) return 0;
	if (rep >= blindtop) { 
	  rep -= blindtop;
	  blindtop = 0;
	} else blindtop -= rep;
      } while (blindtop > 0);

      homepos = port->xmin + (reverse? maxx-1 : x) + (y + port->ymin) * 320;

      /* Clip left boundary (right boundary if reverse */
      while (y < maxy) {
	int pos;
	int i = blindleft;
	while (i > 0) {
	  if (rep >= i) {
	    rep -= i;
	    i = 0;
	  } else {
	    i -= rep;
	    dataptr++;
	    if (!(rep = (*dataptr >> 4) & 0xf)) return 0;
	  }
	}
	color = *dataptr & 0xf;

	pos = homepos;
	homepos += 320;

	if (reverse) {
	  while (x++ < maxx) { /* reversed */
	    if (rep == 0) {
	      dataptr++;
	      color = *dataptr & 0xf;
	      if (!(rep = (*dataptr >> 4) & 0xf)) return 0;
	    }

	    if ((color != transparency) && (dest[1][pos] <= _priority)) {
	      dest[0][pos]= color;
	      dest[1][pos]= _priority;
	    }

	    rep--;
	    pos--;
	  }
	} else { /* normal drawing */
	  while (x++ < maxx) { /* reversed */
	    if (rep == 0) {
	      dataptr++;
	      color = *dataptr & 0xf;
	      if (!(rep = (*dataptr >> 4) & 0xf)) return 0;
	    }

	    if ((color != transparency) && (dest[1][pos] <= _priority)) {
	      dest[0][pos]= color;
	      dest[1][pos]= _priority;
	    }

	    rep--;
	    pos++;
	  }
	}

	/* clip right boundary (left boundary if reversed) */
	i = blindright;
	while (i > 0) {
	  if (rep >= i) {
	    rep -= i;
	    i = 0;
	  } else {
	    i -= rep;
	    dataptr++;
	    if (!(rep = (*dataptr >> 4) & 0xf)) return 0;
	  }
	}
	x = minx;
	y++;
      }
    }
  }

  fflush(NULL);
  return 0;
}

void drawPicture0(picture_t dest, int flags, int defaultPalette, guint8 *data)
{
  gint8 *ptr;
  static gint8 startcolors[40] =
    {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
     0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0x88,
     0x88, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x88,
     0x88, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
     0x08, 0x91, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x88};
  gint8 colors[4][40], priorities[4][40];
  guint8 code;
  short x, y;


  buffers = dest;
  ptr = data;
  if (data == 0) return;
  priority = 0;
  col1 = col2 = 0;
  drawenable = 3;
  patcode = patnum = 0;
  for (x=0; x<4; x++) for (y=0; y<40; y++) {
    colors[x][y] = startcolors[y];
    priorities[x][y] = y & 0x0f;
  }
  while ((code = *(ptr++)) != 0xff) {

    switch (code) {
      case 0xf0: /* set color */
	code = *(ptr++);
	code = colors[code/40 + defaultPalette][code%40];
	col1 = (code >> 4) & 0x0f;
	col2 = code & 0x0f;
	drawenable |= 1;
	break;
      case 0xf1: /* turn off color draw */
	drawenable &= ~1;
	break;
      case 0xf2: /* set priority */
	code = *(ptr++);
	priority = priorities[code/40][code%40];
	drawenable |= 2;
	break;
      case 0xf3: /* turn off priority draw */
	drawenable &= ~2;
	break;
      case 0xf4: /* short-relative pattern */
	if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	plotpattern(x, y);
	ptr += 3;
	while (*((guint8 *)ptr) < 0xf0) {
	  if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	  if (*ptr & 0x80) x -= ((*ptr & 0x70) >> 4);
	  else x += ((*ptr & 0x70) >> 4);
	  if (*ptr & 0x08) y -= (*ptr & 0x07);
	  else y += (*ptr & 0x07);
	  plotpattern(x, y);
	  ptr++;
	}
	break;
      case 0xf5: /* draw relative lines */
	curx = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	cury = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	ptr += 3;
	while (*((guint8 *)ptr) < 0xf0) {
	  if (ptr[0] & 0x80) y = cury - (ptr[0] & 0x7f);
	  else y = cury + ptr[0];
	  x = curx+ptr[1];
	  ditherto(x, y);
	  ptr += 2;
	}
	break;
      case 0xf6: /* draw long lines */
	curx = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	cury = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	while (*((guint8 *)ptr) < 0xf0) {
	  x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	  y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	  ditherto(x, y);
	  ptr += 3;
	}
	break;
      case 0xf7: /* draw short lines */
	curx = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	cury = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	ptr += 3;
	while (*((guint8 *)ptr) < 0xf0) {
	  if (*ptr & 0x80) x = curx - ((*ptr & 0x70) >> 4);
	  else x = curx + ((*ptr & 0x70) >> 4);
	  if (*ptr & 0x08) y = cury - (*ptr & 0x07);
	  else y = cury + (*ptr & 0x07);
	  ditherto(x, y);
	  ptr++;
	}
	break;
      case 0xf8: /* fill */
	{
	  int oldc1, oldc2;

	  if (!(flags & 1)) { /* Fill in black only? */
	    oldc1 = col1;
	    oldc2 = col2;
	    col1 = col2 = 0;
	  }

	  while (*((guint8 *)ptr) < 0xf0) {
	    x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	    y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	    ditherfill(x, y);
	    ptr += 3;
	  }

	  if (!(flags & 1)) { /* Reset old colors */
	    col1 = oldc1;
	    col2 = oldc2;
	  }
	}
	break;
      case 0xf9: /* change pattern */
	patcode = *(ptr++) & 0x37;
	break;
      case 0xfa: /* absolute pattern */
	while (*((guint8 *)ptr) < 0xf0) {
	  if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	  x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	  y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	  plotpattern(x, y);
	  ptr += 3;
	}
	break;
      case 0xfb: /* set floor */
	code = *(ptr++);
	special = priorities[code/40][code%40];
	drawenable |= 4;
	break;
      case 0xfc: /* disable floor draw */
	drawenable &= ~4;
	break;
      case 0xfd: /* relative pattern */
	if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	curx = (*ptr & 0xf0) << 4;
	cury = (*ptr & 0x0f) << 8;
	x = curx | (0xff & *(ptr+1));
	y = cury | (0xff & *(ptr+2));
	plotpattern(x, y);
	ptr += 3;
	while (*((guint8 *)ptr) < 0xf0) {
	  if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	  if (ptr[0] & 0x80) y -= (ptr[0] & 0x7f);
	  else y += ptr[0];
	  x += ptr[1];
	  plotpattern(x, y);
	  ptr += 2;
	}
	break;
      case 0xfe: /* define palette */
	code = *(ptr++);
	switch (code) {
	  case 0:
	    while ((code = *ptr) < 0xf0) {
	      colors[code/40][code%40] = *(ptr+1);
	      ptr += 2;
	    }
	    break;
	  case 1:
	    x = *(ptr++);
	    for (y=0; y<40; y++) colors[x][y] = *(ptr++);
	    break;
	  case 2:
	    x = *(ptr++);
	    for (y=0; y<40; y++) priorities[x][y] = *(ptr++);
	    break;
	  case 5:
	    ptr++;
	  default:
	    fprintf(stderr,"Unknown palette %02x", code);
	    break;
	}
	break;
      default: ;
    }
  }
}

#undef GETPIX(x, y, screen)

/***************************************************************************/
/* Carl's code ends here.                                                  */
/***************************************************************************/


