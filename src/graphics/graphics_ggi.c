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

  

#endif /* HAVE_LIBGGI */
