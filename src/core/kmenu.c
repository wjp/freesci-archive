/***************************************************************************
 kmenu.c Copyright (C) 1999 Christoph Reichenbach


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

#include <kernel.h>



void
kAddMenu(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  menubar_add_menu(s->menubar, s->heap + UPARAM(0), s->heap + UPARAM(1), s->titlebar_port.font, s->heap);
}


void
kSetMenu(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int index = UPARAM(0);

  menubar_set_attribute(s, (index >> 8) - 1, (index & 0xff) - 1, PARAM(1), UPARAM(2));
}

void
kGetMenu(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int index = UPARAM(0);

  s->acc = menubar_get_attribute(s, (index >> 8) - 1, (index & 0xff) - 1, PARAM(1));
}


void
kDrawStatus(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr text = PARAM(0);

  if (text) {
    draw_titlebar(s->pic, 0xf);
    draw_text0(s->pic, &(s->titlebar_port), 1, 1,
	       ((char *)s->heap) + PARAM(0), s->titlebar_port.font, 0);
  } else
    draw_titlebar(s->pic, 0);

  graph_update_box(s, 0, 0, 320, 10);
}


void
kDrawMenuBar(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;

  if (!s->titlebar_port.font) {
    SCIkwarn(SCIkERROR, "No titlebar font is set: %d\n", s->titlebar_port.font_nr);
  }

  if (PARAM(0))
    menubar_draw(s->pic, &(s->titlebar_port) ,s->menubar, -1, s->titlebar_port.font);
  else
    draw_titlebar(s->pic, 0);

  graph_update_box(s, 0, 0, 320, 10);
}


void
kMenuSelect(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr event = UPARAM(0);
  int claimed = 0;
  int type = GET_SELECTOR(event, type);
  int message = GET_SELECTOR(event, message);
  int modifiers = GET_SELECTOR(event, modifiers);
  int menu_nr, item_nr;
  menu_item_t *item;

  /* Check whether we can claim the event directly as a keyboard event */
  if (type == SCI_EVT_KEYBOARD) {
    int menuc, itemc;

    for (menuc = 0; menuc < s->menubar->menus_nr; menuc++)
      for (itemc = 0; itemc < s->menubar->menus[menuc].items_nr; itemc++) {
	item = s->menubar->menus[menuc].items + itemc;
	if ((item->type == MENU_TYPE_NORMAL)
	    && (item->enabled)
	    && (item->key == message)
	    && ((modifiers & (SCI_EVM_CTRL | SCI_EVM_ALT)) == item->modifiers)) {
	  /* Claim the event */
	  claimed = 1;
	  menu_nr = menuc;
	  item_nr = itemc;
	}
      }
  }

  if (claimed) {
    PUT_SELECTOR(event, claimed, 1);
    s->acc = ((menu_nr + 1) << 8) | item_nr;
    /*    fprintf(stderr,"Claimed %04x\n", s->acc);*/
  }
  else s->acc = 0x0; /* Not claimed */
}
