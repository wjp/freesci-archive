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

#include <config.h>
#ifdef HAVE_LIBGGI

#include <graphics_ggi.h>
#include <uinput.h>
#include <engine.h>

#ifdef SCI_GRAPHICS_ALLOW_256
ggi_pixel egacol[256];
#else /* !SCI_GRAPHICS_ALLOW_256 */
ggi_pixel egacol[16];
#endif /* !SCI_GRAPHICS_ALLOW_256 */
char colors_uninitialized = 1;

ggi_visual_t sci_default_visual;
int sci_default_visual_size;


extern int _sci_ggi_double_visual;
extern ggi_visual_t _sci_ggi_last_visual;
sci_event_t _sci_ggi_input_handler();
void initInputGGI();
/* those are from input_ggi.c */


uint8 _sci_xfer[640 * 4]; /* Transfer buffer for GGI */


#ifdef SCI_GRAPHICS_ALLOW_256
void initColors(ggi_visual_t visual)
{
  int i;
  ggi_color vcal[16];

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
    color.r = (vcal[i & 0xf].r / 5)*3
      + (vcal[((i & 0xf)+(i >> 4)) & 0xf].r / 5)*2;
    color.g = (vcal[i & 0xf].g / 5)*3
      + (vcal[((i & 0xf)+(i >> 4)) & 0xf].g / 5)*2;
    color.b = (vcal[i & 0xf].b / 5)*3
      + (vcal[((i & 0xf)+(i >> 4)) & 0xf].b / 5)*2;
    egacol[i] = ggiMapColor(visual, &color);
  }
}
#else /* !SCI_GRAPHICS_ALLOW_256 */
void initColors(ggi_visual_t visual)
{
  int i;

  for (i=0; i<16; i++) {
    ggi_color vcal;
    vcal.r = (i & 0x04) ? 0xaaaa : 0;
    vcal.g = (i & 0x02) ? 0xaaaa : 0;
    vcal.b = (i & 0x01) ? 0xaaaa : 0;
    if (i & 0x08) {
      vcal.r += 0x5555;
      vcal.g += 0x5555;
      vcal.b += 0x5555;
    }
    vcal.a = 0;
    if (i == 6) { /* Special exception for brown */
      vcal.g >>= 1;
    }
    egacol[i] = ggiMapColor(visual, &vcal);
  }
}
#endif /* !SCI_GRAPHICS_ALLOW_256 */





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
    fprintf(stderr,"Evading to different mode...\n");
    if (ggiSetMode(retval, &mode)) return NULL;
  }

  if (colors_uninitialized) {
    initColors(retval);
    colors_uninitialized = 0;
  }

  ggiSetEventMask(retval, emKey | emPointer);
  _sci_ggi_double_visual = 0;
  initInputGGI();

  return _sci_ggi_last_visual = retval;
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
    fprintf(stderr,"Evading to different mode...\n");
    if (ggiSetMode(retval, &mode)) return NULL;
  }

  if (colors_uninitialized) {
    initColors(retval);
    colors_uninitialized = 0;
  }

  ggiSetEventMask(retval, emKey | emPointer);
  _sci_ggi_double_visual = 1;
  initInputGGI();

  return _sci_ggi_last_visual = retval;
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

  ggiGetMode(vis, &mode);

  if (mode.visible.x >= 640) { /* double sized? */
    /* FIXME */
    /*    call graphics_draw_region_ggi_double */
    return;
  }
  
  bytelen = GT_SIZE(mode.graphtype) >> 3; /* min of 8 bpp */

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
	*(uint32 *)&(_sci_xfer[index += bytelen]) = *(uint32 *)&(egacol[data[counter++]]);
    else { /* Check for mouse pointer */

      xc = x;
      for (; xc < pointer_x; xc++)
	*(uint32 *)&(_sci_xfer[index += bytelen]) = *(uint32 *)&(egacol[data[counter++]]);
      
      for (; (xc < pointer_end_x) && (xc < xend); xc++) {
	int colval = pointer->bitmap[pointer_row + xc - pointer_x];
	if (colval == pointer->color_key)
	  colval = data[counter];

	*(uint32 *)&(_sci_xfer[index += bytelen]) = *(uint32 *)&(egacol[colval]);

	counter++;
      }

      for (; xc < xend; xc++)
	*(uint32 *)&(_sci_xfer[index += bytelen]) = *(uint32 *)&(egacol[data[counter++]]);
      
    }

    ggiPutHLine(vis, x, yc, xl, _sci_xfer);
  } /* for (yc... ) */

}


void
graphics_callback_ggi(struct _state *s, int command, int x, int y, int xl, int yl)
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
    graphics_draw_region_ggi(vis, s->pic[s->pic_layer],
			     0, 0, 319, 199,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    break;
  case GRAPHICS_CALLBACK_REDRAW_BOX:
    graphics_draw_region_ggi(vis, s->pic[s->pic_layer], /* Draw box */
			     x, y, xl, yl,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    break;
  case GRAPHICS_CALLBACK_REDRAW_POINTER:
    graphics_draw_region_ggi(vis, s->pic[s->pic_layer], /* Draw new pointer */
			     mp_x, mp_y, mp_size_x, mp_size_y,
			     s->mouse_pointer, s->pointer_x, s->pointer_y);
    graphics_draw_region_ggi(vis, s->pic[s->pic_layer], /* Remove old pointer */
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


void displayPicture(ggi_visual_t visual, picture_t pic, short layer)
{
  int x,y,pos = 0;
  ggi_mode mode;
  int bytelen;
  int index;
  int counter = 0;

  ggiGetMode(visual, &mode);

  if (mode.visible.x >= 640) { /* You want a double visual */
    displayPictureDouble(visual, pic, layer);
    return;
  }

  bytelen = GT_SIZE(mode.graphtype) >> 3; /* at least 8 bits! */

  for (y=0; y<200; y++) {
    index = -bytelen;

    for (x=0; x<320; x++)
      *(uint32 *)&(_sci_xfer[index += bytelen]) = *(uint32 *)&(egacol[pic[layer][counter++]]);

    ggiPutHLine(visual, 0, y, 320, _sci_xfer);
  }
}


void displayPictureDouble(ggi_visual_t visual, picture_t pic, short layer)
{
  int x,y,pos = 0;
  ggi_mode mode;
  int bytelen;
  int index;
  int counter = 0;

  ggiGetMode(visual, &mode);

  bytelen = GT_SIZE(mode.graphtype) >> 3; /* at least 8 bits! */

  for (y=0; y<400; y+=2) {
    uint32 bvar;

    index = -bytelen;

    for (x=0; x<320; x++) {
      bvar = *(uint32 *)&(egacol[pic[layer][counter++]]);
      *(uint32 *)&(_sci_xfer[index += bytelen]) = bvar;
      *(uint32 *)&(_sci_xfer[index += bytelen]) = bvar;
    }

    ggiPutHLine(visual, 0, y, 640, _sci_xfer);
    ggiPutHLine(visual, 0, y+1, 640, _sci_xfer);
  }
}

int
open_visual_ggi(state_t *s)
{
  s->graphics.ggi_visual = openVisual();
  return (!(s->graphics_callback = graphics_callback_ggi));
}

void
close_visual_ggi(state_t *s)
{
  ggiClose(s->graphics.ggi_visual);
}
  

#endif /* HAVE_LIBGGI */
