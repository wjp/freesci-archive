/***************************************************************************
 graphics_ggi.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

   990402 - created from the remaining shards of "picture.c" which were not
            included in "graphics.c" (CJR)

***************************************************************************/
/* Like most of the graphics stuff, this is preliminary. There are some bad hacks
** in here, and they will be taken care of when this thing gets its long-
** deserved re-write.
*/


#include <config.h>
#ifdef HAVE_LIBGGI

#include <graphics_ggi.h>
#include <uinput.h>
#include <engine.h>
#include <math.h>
#include <sys/time.h>

ggi_pixel egacol[256];
char colors_uninitialized = 1;

ggi_visual_t sci_default_visual;
int sci_default_visual_size;

#define NULL_REC_SIZE 640
static byte _null_rec[NULL_REC_SIZE]; /* Initialized to zero */

extern int _sci_ggi_double_visual;
#if 0
extern ggi_visual_t _sci_ggi_last_visual;
#endif
sci_event_t _sci_ggi_input_handler(state_t *s);
void initInputGGI();
/* those are from input_ggi.c */


uint8 _sci_xfer[640 * 4]; /* Transfer buffer for GGI */


/*** Graphics driver ***/

gfx_driver_t gfx_driver_libggi = 
{
  "ggi",
  libggi_init,
  libggi_shutdown,
  libggi_redraw,
  NULL,
  libggi_wait,
  _sci_ggi_input_handler
};

void initColors(ggi_visual_t visual)
{
  int i;
  int map;
  int palette = 0; /* Using a palettized mode? */
  ggi_mode mode;
  ggi_color vcal[16];

  ggiGetMode(visual, &mode);
  palette = (mode.graphtype & GT_PALETTE);

  for (i=0; i<16; i++) {
    vcal[i].r = (i & 0x04) ? 0xaaaa : 0;
    vcal[i].g = (i & 0x02) ? 0xaaaa : 0;
    vcal[i].b = (i & 0x01) ? 0xaaaa : 0;
    if (i & 0x08) {
      vcal[i].r += 0x5555;
      vcal[i].g += 0x5555;
      vcal[i].b += 0x5555;
    }
    vcal[i].a = 0;
    if (i == 6) { /* Special exception for brown */
      vcal[i].g >>= 1;
    }
  }
  for (i=0; i< 256; i++) {
    ggi_color color;

    color.r = INTERCOL((vcal[i & 0xf].r), (vcal[i >> 4].r));
    color.g = INTERCOL((vcal[i & 0xf].g), (vcal[i >> 4].g));
    color.b = INTERCOL((vcal[i & 0xf].b), (vcal[i >> 4].b));
    if (palette) {
      egacol[i] = i;
      ggiSetPalette(visual, i, 1, &color);
    } else
      egacol[i] = ggiMapColor(visual, &color);
  }
}



ggi_visual_t openVisual()
{
  ggi_mode mode = {
    1, // 1 frame
    {320,200}, // resolution
    {320,200}, // virtual
    {0,0},     // size in mm
    GT_AUTO,   // color depth
    {GGI_AUTO,GGI_AUTO}}; // font size
  ggi_visual_t retval;

  if (!(retval = ggiOpen(NULL))) return NULL;

  if (ggiSetMode(retval, &mode)) {
    fprintf(stderr,"Evading to different graphics mode...\n");
    if (ggiSetMode(retval, &mode)) return NULL;
  }

  if (colors_uninitialized) {
    initColors(retval);
    colors_uninitialized = 0;
  }

  ggiSetEventMask(retval, emKey | emPointer);
  _sci_ggi_double_visual = 0;
  initInputGGI();

#if 0
  return _sci_ggi_last_visual = retval;
#endif
  return retval;
}



ggi_visual_t openDoubleVisual()
{
  ggi_mode mode = {
    1, // 1 frame
    {640, 400}, // resolution
    {640, 400}, // virtual
    {0, 0},     // size in mm
    GT_AUTO,   // color depth
    {GGI_AUTO, GGI_AUTO}}; // font size
  ggi_visual_t retval;

  if (!(retval = ggiOpen(NULL))) return NULL;

  if (ggiSetMode(retval, &mode)) {
    fprintf(stderr,"Evading to different graphics mode...\n");
    if (ggiSetMode(retval, &mode)) return NULL;
  }

  if (colors_uninitialized) {
    initColors(retval);
    colors_uninitialized = 0;
  }

  ggiSetEventMask(retval, emKey | emPointer);
  _sci_ggi_double_visual = 1;
  initInputGGI();

#if 0
  return _sci_ggi_last_visual = retval;
#endif
  return retval;
}


void closeVisual(ggi_visual_t visual)
{
  ggiClose(visual);

  /* Nothing more ATM... */
}

void
graphics_draw_region_ggi(ggi_visual_t vis, byte *data,
			 int x, int y, int xl, int yl,
			 mouse_pointer_t *pointer, int pointer_x, int pointer_y)
{
  int xc, yc, pos = 0;
  ggi_mode mode;
  int bytelen;
  int index, counter;
  int xend, yend;
  int pointer_end_x, pointer_end_y;
  int pointer_x_affected = 0; /* set to 1 if the pointer has to be drawn at all */

  int write_bytelen = 4;      /* Byte length for writing */
  ggi_pixel *_egacol;         /* Points to egacol, plus offset */

  ggiGetMode(vis, &mode);

  if (mode.visible.x >= 640) { /* double sized? */
    /* FIXME */
    /*    call graphics_draw_region_ggi_double */
    return;
  }
  
  bytelen = GT_SIZE(mode.graphtype) >> 3; /* min of 8 bpp */

  /* The following lines should help the compiler in determining the
  ** possible values for write_bytelen
  */
  if (bytelen == 2) /* 15 or 16 bpp */
    write_bytelen = 2;
  else if (bytelen == 1) /* 8 bpp */
    write_bytelen = 1;

#ifdef WORDS_BIGENDIAN
  /* The following hack shifts the offset of the array we get our color values from
  ** on bigendian machines, so that the correct data is transmitted. E.g., if ggi_pixel
  ** is 32 bits/bigendian (sparc) and the display is 16 bpp, the color value in one
  ** specified pixel might look like this:
  **    0x 00 00 ff ff
  ** Since we're 16 bit, only the last to bytes should to be copied, so we shift our offset
  ** two bytes to the right. Or to wherever the LSB is.
  */
  _egacol = (ggi_pixel *)(((char *) &egacol[0]) + 4 - write_bytelen);

#else /* !WORDS_BIGENDIAN */

  _egacol = egacol;

#endif /* !WORDS_BIGENDIAN */



  if (x < 0) {
    xl += x;
    x = 0;
  }

  if (y < 0) {
    yl += y;
    y = 0;
  }

  xend = x + xl + 1;
  yend = y + yl + 1;

  if (xend > SCI_SCREEN_WIDTH)
    xend = SCI_SCREEN_WIDTH;

  if (yend > SCI_SCREEN_HEIGHT)
    yend = SCI_SCREEN_HEIGHT;

  if (pointer) {
    pointer_x -= pointer->hot_x;
    pointer_y -= pointer->hot_y; /* Adjust hot spot */
    pointer_end_x = pointer_x + pointer->size_x;
    pointer_end_y = pointer_y + pointer->size_y;

    if ((pointer_x >= x) && (pointer_x < xend))
      pointer_x_affected = 1; /* Pointer might have to be drawn */

    if ((pointer_end_x >= x) && (pointer_end_x < xend))
      pointer_x_affected = 1; /* Pointer might have to be drawn */

    if ((pointer_x <= x) && (pointer_end_x >= xend))
      pointer_x_affected = 1;

  } /* if (pointer) */

  for (yc = y; yc < yend; yc++) {
    int pointer_row;
    counter = yc * SCI_SCREEN_WIDTH + x; /* Screen Position */
    index = -bytelen;

    if ((yc < pointer_y) || (yc >= pointer_end_y))
      pointer_row = -1;
    else if (pointer) pointer_row = (yc - pointer_y) * pointer->size_y;

    if ((!pointer_x_affected) || (pointer_row == -1))
      for (xc=x; xc<xend; xc++)
	memcpy(&(_sci_xfer[index += bytelen]), &(_egacol[data[counter++]]), write_bytelen);
    else { /* Check for mouse pointer */

      xc = x;
      for (; xc < pointer_x; xc++)
	memcpy(&(_sci_xfer[index += bytelen]), &(_egacol[data[counter++]]), write_bytelen);
      
      for (; (xc < pointer_end_x) && (xc < xend); xc++) {
	int colval = pointer->bitmap[pointer_row + xc - pointer_x];
	if (colval == pointer->color_key)
	  colval = data[counter];

	memcpy(&(_sci_xfer[index += bytelen]), &(_egacol[colval]), write_bytelen);

	counter++;
      }

      for (; xc < xend; xc++)
	memcpy(&(_sci_xfer[index += bytelen]), &(_egacol[data[counter++]]), write_bytelen);
      
    }

    ggiPutHLine(vis, x, yc, xl, _sci_xfer);
  } /* for (yc... ) */

}


void
libggi_redraw(struct _state *s, int command, int x, int y, int xl, int yl)
{
  ggi_visual_t vis = s->graphics.ggi_visual;
  int mp_x, mp_y, mp_size_x, mp_size_y;

  if (s->mouse_pointer) {
    mp_x = s->pointer_x - s->mouse_pointer->hot_x;
    mp_y = s->pointer_y - s->mouse_pointer->hot_y;
    mp_size_x = s->mouse_pointer->size_x;
    mp_size_y = s->mouse_pointer->size_y;
  } else { /* No mouse pointer */
    mp_x = s->pointer_x;
    mp_y = s->pointer_y;
    mp_size_x = mp_size_y = 0;
  }


  switch (command) {
  case GRAPHICS_CALLBACK_REDRAW_ALL:
    if (y == 0)
      graphics_draw_region_ggi(vis, s->pic->view,
			       0, 0, 320, 200,
			       s->mouse_pointer, s->pointer_x, s->pointer_y);
    else {
      int lines_to_clear, line;
      int first_line_to_clear;
      int i;

      if (y < 0) {
	graphics_draw_region_ggi(vis, s->pic->view + (320 * y),
				 0, -y, 320, 200+y,
				 s->mouse_pointer, s->pointer_x, s->pointer_y);
	lines_to_clear = -y;
	first_line_to_clear = 0;
      } else { /* y > 0 */
	graphics_draw_region_ggi(vis, s->pic->view + (320 * y),
				 0, 0, 320, 200-y,
				 s->mouse_pointer, s->pointer_x, s->pointer_y);
	lines_to_clear = y;
	first_line_to_clear = 200 - y;
      }

      line = first_line_to_clear;

      for (i = 0; i < lines_to_clear; i++) {
	graphics_draw_region_ggi(vis, &(_null_rec[0]) - (line*320), /* Adjust for y coordinate */
				 0, line, 320, 1,
				 s->mouse_pointer, s->pointer_x, s->pointer_y);
	++line;
      }
    }
    break;
  case GRAPHICS_CALLBACK_REDRAW_BOX:
    graphics_draw_region_ggi(vis, s->pic->view, /* Draw box */
			     x, y, xl, yl,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    break;
  case GRAPHICS_CALLBACK_REDRAW_POINTER:
    graphics_draw_region_ggi(vis, s->pic->view, /* Draw new pointer */
			     mp_x, mp_y, mp_size_x, mp_size_y,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    graphics_draw_region_ggi(vis, s->pic->view, /* Remove old pointer */
			     s->last_pointer_x,s->last_pointer_y,
			     s->last_pointer_size_x, s->last_pointer_size_y,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    break;
  default:
    fprintf(stderr,"graphics_callback_ggi: Invalid command %d\n", command);
  }

  s->last_pointer_size_x = mp_size_x;
  s->last_pointer_size_y = mp_size_y;
  s->last_pointer_x = mp_x;
  s->last_pointer_y = mp_y; /* Update mouse pointer status */

}



int
libggi_init(state_t *s, picture_t pic)
{
  ggiInit();
  s->graphics.ggi_visual = openVisual();
  memset(_null_rec, 0, NULL_REC_SIZE);
  return 0;
}

void
libggi_shutdown(state_t *s)
{
  ggiClose(s->graphics.ggi_visual);
  ggiExit();
}

void
libggi_wait(state_t* s, long usec)
{
  struct timeval tv = {0, usec};

  while(tv.tv_usec>0)
    {
      if(ggiEventPoll(s->graphics.ggi_visual, emPtrMove, &tv))
	{
	  ggi_event e;
	  ggiEventRead(s->graphics.ggi_visual, &e, emPtrMove);
	  switch(e.any.type)
	    {
	    case evPtrRelative:
	      {
		s->pointer_x+=e.pmove.x;
		s->pointer_y+=e.pmove.y;
	      } break;
	    case evPtrAbsolute:
	      {
		s->pointer_x=e.pmove.x;
		s->pointer_y=e.pmove.y;
		if (_sci_ggi_double_visual) {
		  s->pointer_x >>= 1;
		  s->pointer_y >>= 1;
		}
	      } break;
	    }
	  s->gfx_driver->Redraw(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0, 0,
				0, 0);
	}
    } 
}

#endif /* HAVE_LIBGGI */
