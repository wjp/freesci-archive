/***************************************************************************
 engine_graphics.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Graphics operations that are relative to a state_t */


#include <engine.h>
#include <graphics_png.h>

#define CLIP_COORDS_AND_SIZE(x, y, xl, yl) \
  if (x < 0) \
    { xl += x; x = 0; } \
  if (y < 0) \
    { yl += y; y = 0; } \
  if (x + xl > 320) \
    xl = 320 - x ; \
  if (y + yl > 200) \
    yl = 200 - y


typedef struct {
  int x, y, xl, yl;
  int layers;

  byte data[1];
} _graph_memrect_t;


void
graph_clear_box(struct _state *s, int x, int y, int xl, int yl, int color)
{
  int pos = y * SCI_SCREEN_WIDTH + x;
  int _yl;

  CLIP_COORDS_AND_SIZE(x, y, xl, yl);
  if ((xl < 0) || (yl < 0))
    return;

  _yl = yl;

  while (_yl--) {
    memset(s->pic->view + pos, color, xl);

    pos += SCI_SCREEN_WIDTH;
  }
  (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_BOX, x, y, xl, yl);
}

void
graph_update_box(struct _state *s, int x, int y, int xl, int yl)
{
  int pos = y * SCI_SCREEN_WIDTH + x;
  int i, _yl;

  CLIP_COORDS_AND_SIZE(x, y, xl, yl);
  if ((xl < 0) || (yl < 0))
    return;

  _yl = yl;

  while (_yl--) {
    memcpy(s->pic->view + pos, s->pic->maps[s->pic_visible_map] + pos, xl);
    pos += SCI_SCREEN_WIDTH;
  }

  (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_BOX, x, y, xl, yl);
}


void
graph_fill_box_custom(struct _state *s, int x, int y, int xl, int yl,
		      int color, int priority, int control, int layers)
{
  int pos = 0;
  int i, _yl = yl;

  if (priority < 0)
    priority = 16;

  if (x < 0) {
    xl += x;
    x = 0;
  }

  if (y < 0) {
    yl += y;
    y = 0;
  }

  pos = y * SCI_SCREEN_WIDTH + x;

  if (x + xl >= s->pic->xres)
    xl = s->pic->xres - x;

  if (y + yl >= s->pic->yres)
    yl = s->pic->yres - y;

  if ((xl <= 0) || (yl <= 0))
    return;

  while (_yl--) {
    if (layers & 1)
      memset(s->pic->maps[0] + pos, SCI_MAP_EGA_COLOR(s->pic, color), xl);
    if ((layers & 2) && (priority >= 0))
      memset(s->pic->maps[1] + pos, priority, xl);
    if (layers & 4)
      memset(s->pic->maps[2] + pos, control, xl);
    pos += SCI_SCREEN_WIDTH;
  }
}


int
graph_save_box(struct _state *s, int x, int y, int xl, int yl, int layers)
{
  int handle;
  _graph_memrect_t *box;
  byte *dest;
  int i, map;
  int pos;

  CLIP_COORDS_AND_SIZE(x, y, xl, yl);
  if ((xl < 0) || (yl < 0))
    return 0;

  handle = kalloc(s, HUNK_TYPE_GFXBUFFER, xl * yl * 3 +  5*(sizeof(int)));
  /* Three layers plus x, y, xl, yl, layers */

  box = (_graph_memrect_t *) kmem(s, handle);
  dest = &(box->data[0]);

  box->x = x;
  box->y = y;
  box->xl = xl;
  box->yl = yl;
  box->layers = layers;

  for (map = 0; map < 3; map++)
    if (layers & (1 << map)) {

      pos = x + y * SCI_SCREEN_WIDTH;

      for (i = yl; i > 0; i--) {
	memcpy(dest, s->pic->maps[map] + pos, xl);

	pos += SCI_SCREEN_WIDTH;
	dest += xl;

      }
    }

  return handle;
}

void
graph_restore_box(struct _state *s, int handle)
{
  _graph_memrect_t *box;
  byte *src;
  int map, pos;
  int i;

  if (!handle)
    return; /* Assume that the caller knew that this wouldn't work */

  box = (_graph_memrect_t *) kmem(s, handle);

  /*#ifdef SCI_GRAPHICS_DEBUG_IMAGE_REPOSITORY
    draw_frame(s->pic, box->x, box->y, box->xl, box->yl, 0xee, -1);
      graph_update_box(s, box->x, box->y, box->xl, box->yl);
    fprintf(stderr,"Restoring (%d, %d), size (%d, %d)\n", box->x, box->y, box->xl, box->yl);
  #endif */


  if (!box) {
    sciprintf("graph_restore_box: Warning: Could not restore handle %04x\n", handle);
    return;
  }

  src = &(box->data[0]);

  for (map = 0; map < 3; map++)
    if (box->layers & (1 << map)) {

      pos = box->x + box->y * SCI_SCREEN_WIDTH;

      for (i = box->yl; i > 0; i--) {
	memcpy(s->pic->maps[map] + pos, src, box->xl);

	pos += SCI_SCREEN_WIDTH;
	src += box->xl;
	
      }
    }

  kfree(s, handle);
}

int
view0_backup_background(state_t *s, int x, int y, int loop, int cel, byte *data)
{
  int loops_nr = getInt16(data);
  int lookup, cels_nr;
  int addr;
  int width, height;
  int reverse = (getInt16(data+2)>>loop) & 1;

  port_t *port = s->ports[s->view_port];

  if ((loop >= loops_nr) || (loop < 0))
    return -1;

  lookup = getInt16(data+8+(loop<<1));
  cels_nr = getInt16(data + lookup);

  if ((cel < 0) || (cel >= cels_nr))
    return -1;

  lookup += 4 + (cel << 1);

  addr = getInt16(data + lookup);

  width = getInt16(data + addr);
  height = getInt16(data + addr + 2);

  /* x,y are relative to the bottom center, so: */
  y -= height;
  x -= width / 2;

  y++; /* Magic */

  /* Now add the relative picture offsets: */
  if (reverse)
    x -= ((gint8 *)data)[addr + 4];
  else
    x += ((gint8 *)data)[addr + 4];

  y += (gint8)data[addr + 5];

  /* Move to port-relative position */
  x += port->xmin;
  y += port->ymin;

  /* Clip: */
  if (x < port->xmin) {
    width += (x - port->xmin);
    x = port->xmin;
  }
  if (y < port->ymin) {
    height += (y - port->ymin);
    y = port->ymin;
  }
  if (y + height >= port->ymax)
    height = 1 + port->ymax - y;
  if (x + width >= port->xmax)
    width = 1 + port->xmax - x;

  if ((width <= 0) || (height <= 0))
    return 0; /* What a waste of time! */

    return graph_save_box(s, x, y, width, height, 0x7); /* Store all layers, return handle */
}



void
graph_fill_port(struct _state *s, port_t *port, int color)
{
  int pos = port->ymin * SCI_SCREEN_WIDTH + port->xmin;
  int _yl = port->ymax - port->ymin + 1;

  if (color < 0)
    return;

  if (s->pic->bytespp == 1)
    color |= color << 4; /* Extend color */

  while (_yl--) {
    memset(s->pic->maps[0] + pos, color, port->xmax - port->xmin + 1);
    pos += SCI_SCREEN_WIDTH;
  }
}

void
graph_update_port(struct _state *s, port_t *port)
{

  graph_update_box(s, port->xmin-1, port->ymin - ((port->flags & WINDOW_FLAG_TITLE)? 10 : 1),
		   port->xmax - port->xmin + 3,
		   port->ymax - port->ymin + ((port->flags & WINDOW_FLAG_TITLE)? 12 : 2));

  (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0,0,0,0);
}





void
graph_draw_selector_button(struct _state *s, port_t *port, int state,
			   int x, int y, int xl, int yl,
			   char *text, byte *font)
{
  graph_fill_box_custom(s, x + port->xmin, y + port->ymin, xl, yl,
			port->bgcolor, -1, -1, 1); /* Clear button background */
  graph_draw_selector_text(s, port, state,
			   x, y, xl, yl, text, font, ALIGN_TEXT_CENTER);

  if ((state & SELECTOR_STATE_SELECTABLE) && (state & SELECTOR_STATE_SELECTED))
    draw_frame(s->pic, port->xmin + x, port->ymin + y -1,
	       xl - 1, yl - 1, port->color, port->priority);
  else
    draw_frame(s->pic, port->xmin + x, port->ymin + y -1,
	       xl - 1, yl - 1, port->bgcolor, port->priority);
}


void
graph_draw_selector_text(struct _state *s, port_t *port, int state,
			 int x, int y, int xl, int yl,
			 char *text, byte *font, int alignment)
{
  port_t oldport;

  memcpy(&oldport, port, sizeof(oldport)); /* Backup old port data */

  port->x = x;
  port->y = y;
  port->font = font;
  port->gray_text = state & SELECTOR_STATE_DISABLED;
  port->alignment = alignment;

  text_draw(s->pic, port, text, xl);

  if (state & SELECTOR_STATE_FRAMED)
    draw_frame(s->pic, port->xmin + x-1, port->ymin + y-2,
	       xl + 1, yl + 1, port->color, port->priority);

  memcpy(port, &oldport, sizeof(oldport)); /* Restore old port data */
}


void
graph_draw_selector_edit(struct _state *s, port_t *port, int state,
			 int x, int y, int xl, int yl, int cursor,
			 char *text, byte *font)
{
  char *temp = malloc(strlen(text) +2);
  int textwidth;
  int textheight;

  graph_fill_box_custom(s, x + port->xmin, y + port->ymin, xl, yl,
			port->bgcolor, -1, -1, 1); /* Clear box background */


  graph_draw_selector_text(s, port, state,
			   x, y - 1, xl, yl, text, font, ALIGN_TEXT_LEFT);

  if (time(NULL) & 1) { /* Blink cursor in 1s intervals */
    strncpy(temp, text, cursor);
    temp[cursor] = 0;

    get_text_size(temp, font, -1, &textwidth, &textheight);

    if (cursor == strlen(text)) /* At end of text block? */
      graph_fill_box_custom(s, x + port->xmin + textwidth, y+port->ymin,
			    1, textheight, port->color, -1, -1, 1);
    /* Draw thin line */
    else { /* Single character */
      int charwidth;
      int oldcol = port->color;
      int oldbgcol = port->bgcolor;
      byte *oldfont = port->font;

      temp[0] = text[cursor];
      temp[1] = 0; /* Isolate the "blinking" char */

      charwidth = get_text_width(temp, font);
      graph_fill_box_custom(s, x + port->xmin + textwidth, y + port->ymin,
			    charwidth, textheight, port->color, -1, -1, 1);


      port->font = font;
      port->color = oldbgcol;
      port->bgcolor = -1;
      port->x += x + textwidth;
      port->y += y;
      text_draw(s->pic, port, temp, xl);
      port->x -= x + textwidth;
      port->y -= y;
      port->color = oldcol;
      port->bgcolor = oldbgcol;
      port->font = oldfont;
    }


  }

  free(temp);
}


void
graph_draw_selector_icon(struct _state *s, port_t *port, int state,
			 int x, int y, int xl, int yl,
			 byte *data, int loop, int cel)
{
  draw_view0(s->pic, port, x, y, 16, loop, cel, GRAPHICS_VIEW_USE_ADJUSTMENT, data);

  if (state & SELECTOR_STATE_FRAMED)
    draw_frame(s->pic, port->xmin + x-1, port->ymin + y-2,
	       xl + 1, yl + 1, port->color, port->priority);

}


void
graph_draw_selector_control(struct _state *s, port_t *port, int state,
			 int x, int y, int xl, int yl)
{
  /* Draw outer frame: */
  draw_frame(s->pic, port->xmin+x, port->ymin+y, xl, yl, port->color, port->priority);

  /* Draw inner frame: */
  draw_frame(s->pic, port->xmin+x, port->ymin+y + 10, xl, yl - 20, port->color, port->priority);
}


word
graph_on_control(state_t *s, int x, int y, int xl, int yl, int _map)
{
  word retval = 0;
  port_t *port = s->ports[s->view_port];
  int map;

  if (x < port->xmin) {
    xl += x - port->xmin;
    x = port->xmin;
  }
  if (y < port->ymin) {
    yl += y - port->ymin;
    y = port->ymin;
  }
  if (x + xl > port->xmax)
    xl = port->xmax + 1 - x;
  if (y + yl > port->ymax)
    yl = port->ymax + 1 - y;

  if ((xl <= 0) || (yl <= 0))
    return 0;

  for (map = 0; map < 3; map++)
    if (_map & (1 << map)) {
      int startindex = (s->pic->bytespl * y) + x * s->pic->bytespp;
      int i;

      while (yl--) {
	for (i = 0; i < xl; i++)
	  retval |= (1 << ((s->pic->maps[map][startindex + i]) & 0xf));

	startindex += s->pic->bytespl;
      }
    }

  return retval;
}


int
graph_png_save_box(state_t *s, byte *mem)
{
  char filename[20];
  int layer, i;
  int handle = 0;
  byte *data;

  _graph_memrect_t *box = (_graph_memrect_t *) mem;

  if (!box)
    return -1;

  data = &(box->data[0]);

  i = 1;
  while (!handle) { /* Find an empty handle */
    int exists = 0;

    for (layer = 1; layer < 5; layer <<= 1) {
      FILE *fh;

      sprintf(filename, "buffer_%d.%d", i, layer);
      if (fh = fopen(filename, "r")) {
	fclose(fh);
	exists = 1;
      }
    }

    if (!exists)
      handle = i;
    else ++i;
  }

  for (layer = 1; layer < 5; layer <<= 1) {
    sprintf(filename, "buffer_%d.%d", handle, layer);
    unlink(filename); /* Kill it just in case... */

    if (layer & box->layers) {
      if (png_save_buffer(s->pic, filename, box->x, box->y, /* Try to save */
			  box->xl, box->yl, data, (layer != 1)))
	return -1;
      if (layer == 1) { /* If on layer 1, we're working on an actual picture */
	data += (box->xl * box->yl * s->pic->xfact * s->pic->yfact * s->pic->bytespp);
      } else
	data += (box->xl * box->yl); /* Else it's a simple byte map */
    }
  }

  return handle;
}


byte *
graph_png_load_box(state_t *s, int handle, int *alloc_size)
{
  _graph_memrect_t *box;
  int x, y, xl, yl, i;
  char filename[20];
  int layer;
  int layers = 0;
  byte *data[3] = {NULL, NULL, NULL};
  byte *datap;
  int sizes[3] = {0, 0, 0};
  int totalsize = 0;

  for (i = 0; i < 3; i++) {
    FILE *tempfile;

    layer = 1 << i;

    sprintf(filename, "buffer_%d.%d", handle, layer);
    if (tempfile = fopen(filename, "r")) {

      fclose(tempfile);

      if (data[i] = png_load_buffer(s->pic, filename, &x, &y, /* Try to load */
				    &xl, &yl, &(sizes[i]), (layer != 1))) {
	layers |= layer;
	totalsize += sizes[i];
      }
    }
  }

  if (!layers) /* Nothing read */
    return NULL;

  box = (_graph_memrect_t *) malloc(*alloc_size = (sizeof(int) * 5 + totalsize));

  box->x = x;
  box->y = y;
  box->xl = xl;
  box->yl = yl;
  box->layers = layers;

  datap = &(box->data[0]);
  for (i = 0; i < 3; i++) /* Concatenate all graphics information */
    if (data[i]) {
      memcpy(datap, data[i], sizes[i]);
      datap += sizes[i];

      free(data[i]);
    }

  return (byte *) box;
}


/*
  ((defun foo (bar baz)
  (cond ((endp baz) '(anywhere))
  (t (append (list (first bar)) (cons (car baz) (foo (rest bar) (cdr baz)))))
  ))
  (foo (nthcdr 3 (foo '(at divine emacs) '(however looks evaluate))) '(like can lisp))
 */
