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


typedef struct {
  int x, y, xl, yl;
  int layers;

  byte data[1];
} _graph_memrect_t;


void
graph_clear_box(struct _state *s, int x, int y, int xl, int yl, int color)
{
  int pos = y * SCI_SCREEN_WIDTH + x;
  int _yl = yl;

  while (_yl--) {
    memset(s->pic->view + pos, color, xl);

    pos += SCI_SCREEN_WIDTH;
  }

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_BOX, x, y, xl, yl);
}


void
graph_update_box(struct _state *s, int x, int y, int xl, int yl)
{
  int pos = y * SCI_SCREEN_WIDTH + x;
  int i, _yl = yl;

  while (_yl--) {
    memcpy(s->pic->view + pos, s->pic->maps[s->pic_visible_map] + pos, xl);
    pos += SCI_SCREEN_WIDTH;
  }

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_BOX, x, y, xl, yl);
}

int
graph_save_box(struct _state *s, int x, int y, int xl, int yl, int layers)
{
  int handle;
  _graph_memrect_t *box;
  byte *dest;
  int i, map;
  int pos;

  if (x < 0) {
    xl += x;
    x = 0;
  }

  if (y < 0) {
    yl += y;
    y = 0;
  }

  if (x + xl > 320)
    xl = 320 - x;

  if (y + yl > 200)
    yl = 200 - y;

  if ((xl < 0) || (yl < 0))
    return 0;

  handle = kalloc(s, xl * yl * 3 +  5*(sizeof(int))); /* Three layers plus x, y, xl, yl, layers */
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

  /*#ifdef SCI_GRAPHICS_DEBUG_IMAGE_REPOSITORY */
  /*  draw_frame(s->pic, box->x, box->y, box->xl, box->yl, 0xee, -1);
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

  graph_update_box(s, port->xmin-1, port->ymin - ((port->flags & WINDOW_FLAG_TITLE)? 11 : 1),
		   port->xmax - port->xmin + 4,
		   port->ymax - port->ymin + ((port->flags & WINDOW_FLAG_TITLE)? 14 : 4));

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0,0,0,0);
}





void
graph_draw_selector_button(struct _state *s, port_t *port, int state,
			   int x, int y, int xl, int yl,
			   char *text, byte *font)
{
  graph_draw_selector_text(s, port, state, x, y, xl, yl, text, font, ALIGN_TEXT_CENTER);

  if ((state & SELECTOR_STATE_SELECTABLE) && (state & SELECTOR_STATE_SELECTED))
    draw_frame(s->pic, port->xmin + x, port->ymin + y,
	       xl - 1, yl - 1, port->color, port->priority);

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
    draw_frame(s->pic, port->xmin + x-1, port->ymin + y-1,
	       xl + 1, yl + 1, port->color, port->priority);

  memcpy(port, &oldport, sizeof(oldport)); /* Restore old port data */
}


void
graph_draw_selector_edit(struct _state *s, port_t *port, int state,
			 int x, int y, int xl, int yl,
			 char *text, byte *font)
{
  graph_draw_selector_text(s, port, state, x, y, xl, yl, text, font, ALIGN_TEXT_LEFT);
}


void
graph_draw_selector_icon(struct _state *s, port_t *port, int state,
			 int x, int y, int xl, int yl,
			 byte *data, int loop, int cel)
{
  draw_view0(s->pic, port, x, y, 16, loop, cel, data);
}


void
graph_draw_selector_control(struct _state *s, port_t *port, int state,
			 int x, int y, int xl, int yl)
{
  /* Draw outer frame: */
  draw_frame(s->pic, x, y, xl, yl, port->color, port->priority);

  /* Draw inner frame: */
  draw_frame(s->pic, x, y + 10, xl, yl - 20, port->color, port->priority);
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
    xl = port->xmin - x + 1;
  if (y + yl > port->ymax)
    yl = port->ymin - y + 1;

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
