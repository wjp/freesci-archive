/***************************************************************************
 graphics_png.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Provides facilities for writing pictures and seperate views to .png files */


#include <config.h>
#ifdef HAVE_LIBPNG

#include <graphics_png.h>
#include <math.h>

static png_color sci0_png_palette[16] = {
  {0x00, 0x00, 0x00}, {0x00, 0x00, 0xaa}, {0x00, 0xaa, 0x00}, {0x00, 0xaa, 0xaa},
  {0xaa, 0x00, 0x00}, {0xaa, 0x00, 0xaa}, {0xaa, 0x55, 0x00}, {0xaa, 0xaa, 0xaa},
  {0x55, 0x55, 0x55}, {0x55, 0x55, 0xff}, {0x55, 0xff, 0x55}, {0x55, 0xff, 0xff},
  {0xff, 0x55, 0x55}, {0xff, 0x55, 0xff}, {0xff, 0xff, 0x55}, {0xff, 0xff, 0xff}};

int
write_pic_png(char *filename, guint8 *pic)
{
  FILE *fil;
  png_structp structp;
  png_infop infop;
  png_color sci0_color_interpol[256];
  int i;

  if (!pic) return 1;
  if (!(fil = fopen(filename, "wb"))) return 2;
  if (!(structp = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0,0,0))
      || !(infop = png_create_info_struct(structp))) {
    fclose(fil);
    return 3;
  }

  if (setjmp(structp->jmpbuf)) { /* PNG Error handling happens here */
    fclose(fil);
    return 4;
  }

  png_init_io(structp, fil);

  if (sci_color_mode) { /* Interpolate the palette */
    int i;
    for (i=0; i< 255; i++) {
      sci0_color_interpol[i].red = INTERCOL(sci0_png_palette[i & 0xf].red,
					    (sci0_png_palette[i >> 4].red));
      sci0_color_interpol[i].green = INTERCOL(sci0_png_palette[i & 0xf].green,
					    (sci0_png_palette[i >> 4].green));
      sci0_color_interpol[i].blue = INTERCOL(sci0_png_palette[i & 0xf].blue,
					    (sci0_png_palette[i >> 4].blue));
    }
  }

  infop->width = 320;
  infop->height = 200;
  infop->valid |= PNG_INFO_PLTE;
  infop->color_type = PNG_COLOR_TYPE_PALETTE;
  infop->bit_depth = (sci_color_mode)? 8 : 4;
  infop->num_palette = (sci_color_mode)? 256 : 16;
  infop->palette = (sci_color_mode)? sci0_color_interpol
    : sci0_png_palette;

  png_write_info(structp, infop);
  png_set_packing(structp);

  for (i=0; i<200; i++) {
    png_write_row(structp, pic);
    pic += 320;
  }

  png_write_end(structp, infop);
  png_destroy_write_struct(&structp, NULL);

  fclose(fil);

  return 0;
}


#endif /* HAVE_LIBPNG */
