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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else /* !HAVE_UNISTD_H */
#ifdef _MSC_VER
#include <io.h>
#else /* no unistd.h, no io.h? */
#error "Fix graphics.c to include your local equivalent of unistd.h"
#endif /* error */
#endif /* !HAVE_UNISTD_H */

#ifndef _DOS
#include "glib.h"
#endif
#include "stdio.h"
#include <stdarg.h>
#include <engine.h>
#include <graphics.h>

#ifdef HAVE_LIBGGI
#include "graphics_ggi.h"
#endif

#ifdef HAVE_DDRAW
#include "graphics_ddraw.h"
#endif

#ifdef HAVE_GLX
#include "graphics_glx.h"
#endif

#ifdef HAVE_DGFX
#include "graphics_dgfx.h"
#endif

#define DEBUG_DRAWPIC

void _drawpicmsg(const char* format, ...)
{
#ifdef DEBUG_DRAWPIC
  va_list va;

  va_start(va, format);
  vfprintf(stderr, format, va);
  va_end(va);
#endif
}


#ifdef DEBUG_DRAWPIC
#define DRAWPICMSG _drawpicmsg
#else
#define DRAWPICMSG 1 ? (void)0 : _drawpicmsg
#endif

/*** Graphics drivers ***/

gfx_driver_t *gfx_drivers[] =
{
#ifdef HAVE_LIBGGI
  &gfx_driver_libggi,
#endif
#ifdef HAVE_DDRAW
  &gfx_driver_ddraw,
#endif
#ifdef HAVE_GLX
  &gfx_driver_glx,
#endif
#ifdef HAVE_DGFX
  &gfx_driver_dgfx,
#endif
  NULL
};

int sci_color_mode = SCI_COLOR_DITHER;

gfx_driver_t *
graph_get_default_driver()
{
#ifdef HAVE_LIBGGI
  return &gfx_driver_libggi;
#else
#ifdef HAVE_DDRAW
  return &gfx_driver_ddraw;
#else
#ifdef HAVE_DGFX
  return &gfx_driver_dgfx;
#else
  return NULL;
#endif
#endif
#endif
}

picture_t
alloc_empty_picture(int resolution, int colordepth)
{
  int i;

  picture_t retval = malloc(sizeof(struct gfx_picture));

  switch (resolution) {

  case SCI_RESOLUTION_320X200:

    retval->xres = 320;
    retval->yres = 200;
    retval->xfact = 1;
    retval->yfact = 1;
    break;

  case SCI_RESOLUTION_640X400:

    retval->xres = 640;
    retval->yres = 400;
    retval->xfact = 2;
    retval->yfact = 2;
    break;

  default:
    return NULL;
  }

  retval->bytespp = colordepth;

  for (i=0; i<4; i++)
    retval->maps[i] = calloc(retval->xres, retval->yres * colordepth);
  retval->view = calloc(retval->xres, retval->yres * colordepth);

  retval->size = retval->xres * retval->yres * colordepth;
  retval->bytespl = retval->xres * colordepth;
  retval->view_size = retval->bytespl * retval->yfact * 190;
  retval->view_offs = retval->bytespl * retval->yfact * 10;

  return retval;
}

void free_picture(picture_t picture)
{
  int i;

  free(picture->view);

  for (i=0; i<4; i++)
    free(picture->maps[i]);

  free(picture);
}

void clear_picture(picture_t pic, int fgcol)
{
  int i;

  if (pic->bytespp == 1)
    fgcol |= fgcol << 4;

  memset(pic->maps[0] + pic->view_offs, fgcol, pic->view_size);

  for (i=1; i<4; i++)
    memset(pic->maps[i], 0, pic->size);
}

/***************************************************************************/
/* The following code is was taken directly from Carl Muckenhoupt's sde.c. */
/* It is used with permission and has been ported to plot to picture_ts    */
/* instead of directly to the graphics RAM.                                */
/* Also, it has been made thread-safe.                                     */
/***************************************************************************/

  int col1, col2, priority, special;
  unsigned int curx, cury;

inline void putpix(picture_t buffers, int x, int y, short color, short screen,
		   int col1, int col2, int priority, int special, char drawenable)
{
  int pos;

  pos = (y+10) * 320 + x;

  switch(screen) {
  case 0: buffers->maps[0][pos] = color | (color << 4); break;
  case 1: buffers->maps[1][pos] = priority; break;
  case 2: buffers->maps[2][pos] = special; break;
  case 3: buffers->maps[3][pos] |= drawenable; break;
  }
}

#define GETPIX(x, y, screen)  (buffers->maps[(screen)][(x)+((y)*320)] \
			       & ((sci_color_mode && (screen == 1))? \
			       0xf : 0xff))

inline void plot(picture_t buffers, int x, int y,
		 int col1, int col2, int priority, int special,  char drawenable)
{
  int pos;

  pos = (y+10) * 320 + x;

  if (sci_color_mode == SCI_COLOR_INTERPOLATE) {
    if (drawenable & 1) buffers->maps[0][pos] = (col1 << 4) | col2;
  } else if (sci_color_mode == SCI_COLOR_DITHER256) {
    if (drawenable & 1) buffers->maps[0][pos] = ((x^y)&1)? (col1 << 4) | col2
			  : (col2 << 4) | col1;
  } else
    if (drawenable & 1) buffers->maps[0][pos] = ((x^y)&1)? (col1 | (col1 << 4)) : (col2 | (col2 << 4));

  if (drawenable & 2) buffers->maps[1][pos] = priority;
  if (drawenable & 4) buffers->maps[2][pos] = special;
  buffers->maps[3][pos] |= drawenable;

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
     plot(buffers, x,y, col1, col2, priority, special, drawenable); \
     linearvar += linearmod; \
     if ((d+=incrE) < 0) { \
       d += incrNE; \
       nonlinearvar += nonlinearmod; \
     }; \
   }; \
   plot(buffers, x,y, col1, col2, priority, special, drawenable);

void dither_line(picture_t buffers, int curx, int cury, short x1, short y1,
		 int col1, int col2, int priority, int special, char drawenable)
{
  short dx, dy, incrE, incrNE, d, x, y, finalx, finaly;
  short tc1 = col1, tc2 = col2;
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

#define FILLBOUNDARY(fx, fy) ((drawenable & (GETPIX((fx), (fy), 3))) && \
                             !((drawenable & 1) && (GETPIX((fx), (fy), 0)==0xff) && \
			       ((col1 != 0xf) && (col2 != 0xf)))) \


void fillhelp(picture_t buffers, gint16 xstart, gint16 xend, gint16 y, gint16 direction,
	      int col1, int col2, int priority, int special, char drawenable)
{
  gint16 register xright = xstart, xleft = xstart;

  y += direction;
  if (y >= 0 && y < 190) {
    if (!FILLBOUNDARY(xleft, y+10)) {
      while (xleft > 0 && !FILLBOUNDARY(xleft-1, y+10)) xleft--;
      for (xright = xleft; xright < 320 && !FILLBOUNDARY(xright, y+10);
	   xright++)
	plot(buffers, xright, y, col1, col2, priority, special, drawenable);
      if (xleft < xstart) fillhelp(buffers, xleft, xstart, y, -direction,
				   col1, col2, priority, special, drawenable);
      fillhelp(buffers, xleft, xright-1, y, direction,
	       col1, col2, priority, special, drawenable);
    }
    while (xright <= xend) {
      while (FILLBOUNDARY(xright, y+10)) {
	xright++;
	if (xright > xend) return;
      }
      xleft = xright;
      while (xright<320 && !FILLBOUNDARY(xright, y+10)) {
	plot(buffers, xright, y, col1, col2, priority, special, drawenable);
	xright++;
      }
      fillhelp(buffers, xleft, xright-1, y, direction,
	       col1, col2, priority, special, drawenable);
    }
    xright--;
    if (xright > xend)
      fillhelp(buffers, xend, xright, y, -direction,
	       col1, col2, priority, special, drawenable);
  }
}

void ditherfill(picture_t buffers, gint16 x, gint16 y,
		int col1, int col2, int priority, int special, char drawenable)
{
  gint16 xstart, xend, old_drawenable = drawenable;

  /*  if (GETPIX(x, y+2, 1) == priority) drawenable &= ~2; */ /* FIXME ??? */
  if (GETPIX(x, y+2, 2) == special) drawenable &= ~4;

  if (FILLBOUNDARY(x, y+10)) return;
  for (xstart = x; xstart >= 0 && !FILLBOUNDARY(xstart, y+10); xstart--)
    plot(buffers, xstart, y, col1, col2, priority, special, drawenable);
  xstart++;
  for (xend = x+1; xend < 320 && !FILLBOUNDARY(xend, y+10); xend++)
    plot(buffers, xend, y,
	 col1, col2, priority, special, drawenable);
  xend--;
  fillhelp(buffers, xstart, xend, y, 1,
	   col1, col2, priority, special, drawenable);
  fillhelp(buffers, xstart, xend, y, -1,
	   col1, col2, priority, special, drawenable);
  drawenable = old_drawenable;
}

unsigned char patcode, patnum;
void plotpattern(picture_t buffers, int x, int y,
		 int col1, int col2, int priority, int special, char drawenable)
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
	if ((junq[junqbit>>3] >> (7-(junqbit & 7))) & 1) plot(buffers, k, l, col1, col2,
							      priority, special, drawenable);
	junqbit++;
	if (junqbit == 0xff) junqbit=0;
      }
      else plot(buffers, k, l, col1, col2, priority, special, drawenable);
    }
  }
  else { /* circle */
    int circlebit = 0;
    for (l=y-size; l<=y+size; l++) for (k=x-size; k<=x+size+1; k++) {
      if ((circles[patcode&7][circlebit>>3] >> (7-(circlebit & 7))) & 1) {
	if (patcode & 0x20) {
	  if ((junq[junqbit>>3] >> (7-(junqbit & 7))) & 1) plot(buffers, k, l,	col1, col2,
								priority, special,  drawenable);
	  junqbit++;
	  if (junqbit == 0xff) junqbit=0;
	}
	else plot(buffers, k, l, col1, col2, priority, special, drawenable);
      }
      circlebit++;
    }
  }
}

int
view0_base_modify(int loop, int cel, byte *data, int *xvar, int *yvar)
{
  int loops_nr = getInt16(data);
  int lookup, cels_nr;
  int addr;
  int reversed = (getInt16(data + 2) >> loop) & 1;
  int xoffs, yoffs;

  if ((loop >= loops_nr) || (loop < 0))
    return -1;

  lookup = getInt16(data+8+(loop<<1));
  cels_nr = getInt16(data + lookup);

  if ((cel < 0) || (cel >= cels_nr))
    return -1;

  lookup += 4 + (cel << 1);

  addr = getInt16(data + lookup);

  xoffs = (gint8) data[addr + 4];
  yoffs = (gint8) data[addr + 5];

  if (reversed)
    *xvar += xoffs;
  else
    *xvar -= xoffs;

  *yvar -= yoffs;

  return 0;
}

int
view0_cel_width(int loop, int cel, byte *data)
{
  int loops_nr = getInt16(data);
  int lookup, cels_nr;
  int addr;

  if ((loop >= loops_nr) || (loop < 0))
    return -1;

  lookup = getInt16(data+8+(loop<<1));
  cels_nr = getInt16(data + lookup);

  if ((cel < 0) || (cel >= cels_nr))
    return -1;

  lookup += 4 + (cel << 1);

  addr = getInt16(data + lookup);

  return getInt16(data + addr);
}

int
view0_cel_height(int loop, int cel, byte *data)
{
  int loops_nr = getInt16(data);
  int lookup, cels_nr;
  int addr;

  if ((loop >= loops_nr) || (loop < 0))
    return -1;

  lookup = getInt16(data+8+(loop<<1));
  cels_nr = getInt16(data + lookup);

  if ((cel < 0) || (cel >= cels_nr))
    return -1;

  lookup += 4 + (cel << 1);

  addr = getInt16(data + lookup);

  return getInt16(data + addr + 2);
}



int
view0_loop_count(byte *data)
{
  return getInt16(data);
}

int
view0_cel_count(int loop, byte *data)
{
  int loops_nr = getInt16(data);
  int lookup;

  if ((loop >= loops_nr) || (loop < 0))
    return 0;

  lookup = getInt16(data+8+(loop<<1));
  return getInt16(data + lookup);
}

int draw_view0(picture_t dest, port_t *port, int xp, int yp, short _priority,
	      short group, short index, int mode, guint8 *data)
{
  gint8 *dataptr,*lookup;
  gint16 nloops, ncells, loop, cell;
  int x, y, maxx, maxy, minx;
  guint8 transparency;
  guint8 reverse;
  int clipmaxx = 319, clipmaxy = 199;

  group++; index++; /* Both start at 0, but we need them to start at 1 */

  yp++; /* Magic */

  if (port) {
    clipmaxy = port->ymax - port->ymin;
    clipmaxx = port->xmax - port->xmin + 1;
  }

  if (data == 0) return -3;
  nloops = getInt16(data);
  if (group > nloops) return -1;
  if (group < 1) return -1;
  if (index < 1) return -2;
  loop = group;
  cell = index;
  {
    int blindtop, blindleft, blindright; /* zones outside of the picture */
    int homepos;
    int xoffs, yoffs;

    lookup = (data + (getInt16(data+6+(loop<<1))));
    ncells = getInt16(lookup);
    if (cell > ncells) return -2;
    lookup+=2;
    dataptr = data+getInt16(lookup + (cell<<1));
    maxx = getInt16(dataptr);
    maxy = getInt16(dataptr+2);

    xoffs = (gint8) dataptr[4];
    yoffs = (gint8) dataptr[5]; /* These two bytes contain relative offsets */

    dataptr += 7;
    transparency = dataptr[-1];
    reverse = (getInt16(data+2)>>(loop-1)) & 1;

    if (mode & GRAPHICS_VIEW_USE_ADJUSTMENT) {
      if (reverse)
	xp -= xoffs;
      else
	xp += xoffs;
      yp += yoffs;
    }

    if (mode & GRAPHICS_VIEW_CENTER_BASE) {
      xp -= maxx/2;
      yp -= maxy;
    }

    minx = x = (xp < 0) ? 0 : xp;
    y = yp;

    if (y < 0) { /* clipping the ceiling */
      maxy += y;
      blindtop = (-y)*maxx;
      y = 0;
    } else blindtop = 0;

    maxx += x;
    maxy += y;

    if (maxy > clipmaxy + 1) maxy = clipmaxy + 1;

    if (maxx < 0) return 0;
    if (y > clipmaxy) return 0;
    if (maxy < 0) return 0;
    if (x >= clipmaxx) return 0;

    /* clipping the sides: */
    if (xp < 0) {
      blindleft = x - xp;
      maxx -= blindleft;
      xp = 0;
    } else blindleft = 0;

    blindright = maxx-clipmaxx;

    if (maxx > clipmaxx) maxx = clipmaxx;

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

      /* Clip left boundary (right boundary if reversed */
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
	color = SCI_MAP_EGA_COLOR(dest, color);

	pos = homepos;
	homepos += 320;

	if (reverse) {
	  while (x++ < maxx) { /* reversed */
	    if (rep == 0) {
	      dataptr++;
	      color = *dataptr & 0xf;
	      color = SCI_MAP_EGA_COLOR(dest, color); /* Prepare for the FreeSCI palette */
	      if (!(rep = (*dataptr >> 4) & 0xf)) return 0;
	    }

	    if (((color & 0xf) != transparency) && ((dest->maps[1][pos] <= _priority) || (_priority < 0))) {
	      dest->maps[0][pos]= color;
	      if (_priority >= 0)
		dest->maps[1][pos]= _priority;
	    }

	    rep--;
	    pos--;
	  }
	} else { /* normal drawing */
	  while (x++ < maxx) { /* reversed */
	    if (rep == 0) {
	      dataptr++;
	      color = *dataptr & 0xf;
	      color = SCI_MAP_EGA_COLOR(dest, color); /* FreeSCI palette preparation */
	      if (!(rep = (*dataptr >> 4) & 0xf)) return 0;
	    }

	    if (((color & 0xf) != transparency) && ((dest->maps[1][pos] <= _priority) || (_priority < 0))) {
	      dest->maps[0][pos]= color;
	      if (_priority >= 0)
		dest->maps[1][pos]= _priority;
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

  #ifndef _DOS
     /* Where is this for? -- Rink */
     fflush(NULL);
  #endif
  return 0;
}

#define DEBUG_DRAWPIC

void draw_pic0(picture_t dest, int flags, int defaultPalette, guint8 *data)
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
  char drawenable = 3;


  ptr = data;
  if (data == 0) return;
  priority = 0;
  col1 = col2 = 0;
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
	priority = code & 0xf;
	drawenable |= 2;
	break;
      case 0xf3: /* turn off priority draw */
	drawenable &= ~2;
	break;
      case 0xf4: /* short-relative pattern */
	if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	plotpattern(dest, x, y, col1, col2, priority, special, drawenable);
	ptr += 3;
	while (*((guint8 *)ptr) < 0xf0) {
	  if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	  if (*ptr & 0x80) x -= ((*ptr & 0x70) >> 4);
	  else x += ((*ptr & 0x70) >> 4);
	  if (*ptr & 0x08) y -= (*ptr & 0x07);
	  else y += (*ptr & 0x07);
	  plotpattern(dest, x, y, col1, col2, priority, special, drawenable);
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
	  dither_line(dest, curx, cury, x, y, col1, col2, priority, special, drawenable);
	  curx = x;
	  cury = y;
	  ptr += 2;
	}
	break;
      case 0xf6: /* draw long lines */
	curx = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	cury = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	while (*((guint8 *)ptr) < 0xf0) {
	  x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	  y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	  dither_line(dest, curx, cury, x, y, col1, col2, priority, special, drawenable);
	  curx = x;
	  cury = y;
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
	  dither_line(dest, curx, cury, x, y, col1, col2, priority, special, drawenable);
	  curx = x;
	  cury = y;
	  ptr++;
	}
	break;
      case 0xf8: /* fill */
	{
	  int oldc1 = col1, oldc2 = col2;

	  if (!(flags & 1)) { /* Fill in black only? */
	    oldc1 = col1;
	    oldc2 = col2;
	    col1 = col2 = 0;
	  }

	  while (*((guint8 *)ptr) < 0xf0) {
	    x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	    y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	    ditherfill(dest, x, y, col1, col2, priority, special, drawenable);
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
	  plotpattern(dest, x, y, col1, col2, priority, special, drawenable);
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
	plotpattern(dest, x, y, col1, col2, priority, special, drawenable);
	ptr += 3;
	while (*((guint8 *)ptr) < 0xf0) {
	  if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	  if (ptr[0] & 0x80) y -= (ptr[0] & 0x7f);
	  else y += ptr[0];
	  x += ptr[1];
	  plotpattern(dest, x, y, col1, col2, priority, special, drawenable);
	  ptr += 2;
	}
	break;
      case 0xfe: /* Misc. commands */
	code = *(ptr++);
	switch (code) {
	  case 0: /* Define single palette entries */
	    while ((code = *ptr) < 0xf0) {
	      colors[code/40][code%40] = *(ptr+1);
	      ptr += 2;
	    }
	    break;
	  case 1: /* Define entire table */
	    x = *(ptr++);
	    for (y=0; y<40; y++) colors[x][y] = *(ptr++);
	    break;
	  case 2:	/* Cases 2-6 only apply for monochrome displays - L.S. */
	    ptr+=41; 
	    break;
	  case 3:	
	    ptr++;
	    break;
	  case 4:
	    break;
	  case 5:
	    ptr++;
	    break;
	  case 6:
	    break;
/* The following cases are SCI01 only */	    	   
	  case 7: /* Draw an _embedded_ view in the picture */
          	x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
		y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
		ptr=ptr+5+getInt16(ptr); /* Past the embedded view */
		break;
	  case 8: /* Sets priority values for the pic */
	      ptr+=16;
	      break;
	      	        
	  default:
	    fprintf(stderr,"Unknown palette cmd %02x at %x\n", code, (int) ptr-(int) data);
	    break;
	}
	break;
      default: { printf("Unknown pic command: 0x%02x at %x\n", code, (int) ptr-(int) data); }
    }
  }
}

#undef GETPIX(x, y, screen)

/***************************************************************************/
/* Carl's code ends here.                                                  */
/***************************************************************************/


