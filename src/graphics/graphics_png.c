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
#include <engine.h>
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
    for (i=0; i< 256; i++) {
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


int
png_save_buffer(picture_t pic, char *name,
		int xoffset, int yoffset, int width, int height,
		byte *data, int force_8bpp_special)
{
  FILE *fil;
  byte *dpic = data;
  png_structp structp;
  png_infop infop;
  png_color sci0_color_interpol[256];
  int bpp = pic->bytespp;
  int palette = (pic->bytespp == 1);
  int i;

  if (force_8bpp_special) /* Special treatment requested? */
    palette = bpp = 1;

  if (!data) return 1;
  if (!(fil = fopen(name, "wb"))) return 1;
  if (!(structp = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0,0,0))
      || !(infop = png_create_info_struct(structp))) {
    fclose(fil);
    return 1;
  }

  if (setjmp(structp->jmpbuf)) { /* PNG Error handling happens here */
    sciprintf("Error while writing to PNG to %s\n", name);
    fclose(fil);
    return 1;
  }

  png_init_io(structp, fil);

  if (force_8bpp_special) { /* Gradient palette */
    for (i=0; i< 256; i++) {
      sci0_color_interpol[i].red =
	sci0_color_interpol[i].green =
	sci0_color_interpol[i].blue = i;
    }
  } else {
      for (i=0; i< 256; i++) {
	sci0_color_interpol[i].red = INTERCOL(sci0_png_palette[i & 0xf].red,
					      (sci0_png_palette[i >> 4].red));
	sci0_color_interpol[i].green = INTERCOL(sci0_png_palette[i & 0xf].green,
						(sci0_png_palette[i >> 4].green));
	sci0_color_interpol[i].blue = INTERCOL(sci0_png_palette[i & 0xf].blue,
					       (sci0_png_palette[i >> 4].blue));
      }
  }

  infop->width = width;
  infop->height = height;
  png_set_oFFs (structp, infop, xoffset, yoffset, PNG_OFFSET_PIXEL);

  infop->bit_depth = 8;

  if (palette) {
    infop->valid |= PNG_INFO_PLTE;
    infop->color_type = PNG_COLOR_TYPE_PALETTE;
    infop->num_palette = 256;
    infop->palette = sci0_color_interpol;
  } else /* No palette */
    infop->color_type = PNG_COLOR_TYPE_RGB;

  /* FIXME: Handle non-palettized savegames correctly */

  png_write_info(structp, infop);
  png_set_packing(structp);

  for (i=0; i<height; i++) {
    png_write_row(structp, dpic);
    dpic += width;
  }

  png_write_end(structp, infop);
  png_destroy_write_struct(&structp, NULL);

  fclose(fil);

  return 0;
}


byte *
png_load_buffer(picture_t pic, char *name,
		int *xoffset, int *yoffset, int *_width, int *_height,
		int *size, int force_8bpp_special)
{
  png_structp structp = NULL;
  png_infop infop;
  FILE *fil;
  char header[8];
  int i, bytespp, stepwidth;
  byte *buf, *bufpos = NULL;
  png_uint_32 _xoffset, _yoffset, width, height, units;
  png_color sci0_color_interpol[256];

  if (!(fil = fopen(name, "rb"))) {
    sciprintf("File opening failed while trying to access '%s'\n", name);
    perror("");
    return NULL;
  }

  if ((width = (fread(header, 1, 8, fil)) != 8) || (png_sig_cmp(header, 0, 8))) {
    if (width != 8)
      perror("While reading PNG header");
    return NULL;
  }
  if (!(structp = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0,0,0))
      || !(infop = png_create_info_struct(structp))) {

    if (structp)
      png_destroy_read_struct(&structp, NULL, NULL);

    fclose(fil);
    return NULL;
  }


  if (setjmp(structp->jmpbuf)) { /* Error reading/interpreting the PNG file? */
    sciprintf("Error while reading PNG from %s\n", name);

    if (buf)
      free(buf);

    png_destroy_read_struct(&structp, &infop, NULL);
    fclose(fil);

    return NULL;
  }

  png_init_io(structp, fil);
  png_set_sig_bytes(structp, 8); /* We have already read the first 8 bytes for validation */

  png_read_info(structp, infop);


  width = infop->width;
  height = infop->height;

  png_get_oFFs(structp, infop, &_xoffset, &_yoffset, &units);
  if (xoffset)
    *xoffset = _xoffset;
  if (yoffset)
    *yoffset = _yoffset;


  if (_width)
    *_width = width;

  if (_height)
    *_height = height;

  if (force_8bpp_special || (pic->bytespp == 1)) { /* Palette mode? */
    int i;

    bytespp = 1;

    if (force_8bpp_special) { /* Gradient palette */
      for (i=0; i< 256; i++) {
	sci0_color_interpol[i].red =
	  sci0_color_interpol[i].green =
	  sci0_color_interpol[i].blue = i;
      }
    } else {
      for (i=0; i< 256; i++) {
	sci0_color_interpol[i].red = INTERCOL(sci0_png_palette[i & 0xf].red,
					      (sci0_png_palette[i >> 4].red));
	sci0_color_interpol[i].green = INTERCOL(sci0_png_palette[i & 0xf].green,
						(sci0_png_palette[i >> 4].green));
	sci0_color_interpol[i].blue = INTERCOL(sci0_png_palette[i & 0xf].blue,
					       (sci0_png_palette[i >> 4].blue));
      }
    }
    /* Palette mode. Dither accordingly. */
    png_set_dither(structp, sci0_color_interpol, 256, 256, NULL, 0);

  } else {

    bytespp = pic->bytespp;

  }
  /* FIXME: Support arbitrary palettes as well */

  png_read_update_info(structp, infop);

  bufpos = buf = malloc(*size = (height * (stepwidth = width * bytespp)));

  for (i = 0; i < height; i++) {
    png_read_rows(structp, &bufpos, NULL, 1);
    bufpos += stepwidth;
  }

  png_read_end(structp, infop);
  png_destroy_read_struct(&structp, &infop, NULL);

  /* FIXME: Scale buffer if neccessary */

  return buf;
}


char *_freesci_file_maps[4] = {
  FREESCI_FILE_VISUAL_MAP,
  FREESCI_FILE_PRIORITY_MAP,
  FREESCI_FILE_CONTROL_MAP,
  FREESCI_FILE_AUXILIARY_MAP
};


int
png_save_pic(picture_t pic)
{
  int i;

  for (i = 0; i < 4; i++) {
    if (png_save_buffer(pic, _freesci_file_maps[i], 0, 0,
			(i)? 320 : pic->xres,
			(i)? 200 : pic->yres,
			pic->maps[i], i))
      return 1;
  }
  return 0;
}

int
png_load_pic(picture_t pic)
{
  byte *temp_maps[4];
  int width, height, size;
  int i;

  for (i = 0; i < 4; i++)
    if (!(temp_maps[i] = png_load_buffer(pic, _freesci_file_maps[i],
					 NULL, NULL, 
					 (i)? NULL : &width, (i)? NULL : &height, &size,
					 i)))
      {
	int j;

	for (j = 0; j < i; j++)
	  free(temp_maps[j]);
	return 1;
      }

  for (i = 0; i < 4; i++) {
    free(pic->maps[i]); /* Free the old maps ... */
    pic->maps[i] = temp_maps[i]; /* ... and set the new ones */
  }

  return 0;
}


#endif /* HAVE_LIBPNG */
