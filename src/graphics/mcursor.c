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
/* Draws and calculates SCI01? mouse pointers. */

#include <graphics.h>


void
map_mouse_cursor(byte *target, byte *data, int colors[4])
/* Maps SCI0/01 mouse cursors from source to the 8bpp target according to the specified colors */
{
  int xc, yc;

  for (yc = 0; yc < 16; yc++) {
    guint32 bitmask = (data[yc << 1]) << 16 | (data[(yc << 1) + 1] << 24)
      | data[(yc << 1) + 32] | data[(yc << 1) + 33] << 8;
    /* Stores black/trans part in upper 16b, white part in lower 16b */

    for (xc = 0; xc < 16; xc++) {
      int colindex = ((!(bitmask & 0x8000)) << 1) | !(bitmask & 0x80000000);

      target[(yc << 4) + xc] = colors[colindex];
      bitmask <<= 1;
    }
  }
}


void
calc_mouse_cursor0(mouse_pointer_t *pointer, byte* data)
{
  int colors[4] = {0xff, 0xff, 1, 0};

  if (data[3]) { /* Centered hot spot */
    pointer->hot_x = 8;
    pointer->hot_y = 8;
  } else {
    pointer->hot_x = 0;
    pointer->hot_y = 0;
  }

  pointer->size_x = pointer->size_y = 16; /* Fixed size */

  pointer->bitmap = malloc(16 * 16);

  data += 4; /* Get over the header */

  map_mouse_cursor(pointer->bitmap, data, colors);

  pointer->color_key = 1; /* Used for transparency here */
}

void
calc_mouse_cursor01(mouse_pointer_t *pointer, byte* data)
{
  int colors[4] = {0x77, 0xff, 1, 0};

  pointer->hot_x = getInt16(data);
  pointer->hot_y = getInt16(data + 2);

  pointer->size_x = pointer->size_y = 16; /* Fixed size */

  pointer->bitmap = malloc(16 * 16);

  data += 4; /* Get over the header */

  map_mouse_cursor(pointer->bitmap, data, colors);

  pointer->color_key = 1; /* Used for transparency here */
}



mouse_pointer_t *
calc_mouse_cursor(byte *data)
{
  mouse_pointer_t *pointer = malloc(sizeof(mouse_pointer_t));

  switch (sci_version) {
  case SCI_VERSION_0: calc_mouse_cursor0(pointer, data); break;
  case SCI_VERSION_01: calc_mouse_cursor01(pointer, data); break;
  default:
    free(pointer);
    fprintf(stderr,"Warning: Mouse pointers not supported for this SCI version!\n");
    return NULL;
  }

  return pointer;
}


void
free_mouse_cursor(mouse_pointer_t *pointer)
{
  if (!pointer)
    return;
  free(pointer->bitmap);
  free(pointer);
}
