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

  if (s->status_bar_text)
    free(s->status_bar_text);

  s->status_bar_text = NULL;

  if (text)
    s->status_bar_text = strdup(s->heap + text);

  status_bar_draw(s, s->status_bar_text);

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
  int menu_nr = -1, item_nr;
  menu_item_t *item;
  int menu_mode = 0; /* Menu is active */
  int mouse_down = 0;

  /* Check whether we can claim the event directly as a keyboard event */
  if (type == SCI_EVT_KEYBOARD) {
    int menuc, itemc;

    if (message == SCI_K_ESC)
      menu_mode = 1;

    else {
      SCIkdebug(SCIkMENU,"Menu: Got KBD event: %04x/%04x\n", message, modifiers);
    
      for (menuc = 0; menuc < s->menubar->menus_nr; menuc++)
	for (itemc = 0; itemc < s->menubar->menus[menuc].items_nr; itemc++) {
	  item = s->menubar->menus[menuc].items + itemc;

	  SCIkdebug(SCIkMENU,"Menu: Checking against %s: %04x/%04x (type %d, %s)\n",
		    item->text? item->text : "--bar--", item->key, item->modifiers,
		    item->type, item->enabled? "enabled":"disabled");

	  /* FIXME: Add Said() check */
	  if ((item->type == MENU_TYPE_NORMAL)
	      && (item->enabled)
	      && (item->key == message)
	      && ((modifiers & (SCI_EVM_CTRL | SCI_EVM_ALT)) == item->modifiers)) {
	    /* Claim the event */
	    SCIkdebug(SCIkMENU,"Menu: Event CLAIMED for %d/%d\n", menuc, itemc);
	    claimed = 1;
	    menu_nr = menuc;
	    item_nr = itemc;
	  }
	}
    }
  }

  if ((type == SCI_EVT_MOUSE_PRESS) && (s->pointer_y < 10)) {
    menu_mode = 1;
    mouse_down = 1;
  }

  if (menu_mode) {
    int backupped_port = -1;
    int old_item;
    int old_menu;
    port_t port;
    port.font = s->titlebar_port.font;

    item_nr = -1;

    /* Default to menu 0, unless the mouse was used to generate this effect */
    if (mouse_down)
      menubar_map_pointer(s, &menu_nr, &item_nr, &port);
    else
      menu_nr = 0;

    menubar_draw(s->pic, &(s->titlebar_port), s->menubar, menu_nr, s->titlebar_port.font);
    graph_update_port(s, &(s->titlebar_port));

    old_item = -1;
    old_menu = -1;

    while (menu_mode) {
      sci_event_t event = getEvent(s);

      claimed = 0;

      switch (event.type) {
      case SCI_EVT_KEYBOARD:
	switch (event.data) {

	case SCI_K_ESC:
	  menu_mode = 0;
	  break;

	case SCI_K_ENTER:
	  menu_mode = 0;
	  if ((item_nr >= 0) && (menu_nr >= 0))
	    claimed = 1;
	  break;

	case SCI_K_LEFT:
	  if (menu_nr > 0) {
	    --menu_nr;
	    item_nr = -1;
	  }
	  break;

	case SCI_K_RIGHT:
	  if (menu_nr < (s->menubar->menus_nr - 1)) {
	    ++menu_nr;
	    item_nr = -1;
	  }
	  break;

	case SCI_K_UP:
	  if (item_nr > -1) {

	    do { --item_nr; }
	    while ((item_nr > -1) && !menubar_item_valid(s, menu_nr, item_nr));
	  }
	  break;

	case SCI_K_DOWN: {
	  int seeker, max = s->menubar->menus[menu_nr].items_nr;
	  seeker = item_nr + 1;

	  while ((seeker < max) && !menubar_item_valid(s, menu_nr, seeker))
	    ++seeker;

	  if (seeker != max)
	    item_nr = seeker;
	}
	break;

	}
	break;

      case SCI_EVT_MOUSE_RELEASE:
	menu_mode = (s->pointer_y < 10);
	claimed = !menu_mode && !menubar_map_pointer(s, &menu_nr, &item_nr, &port);
	mouse_down = 0;
	break;

      case SCI_EVT_MOUSE_PRESS:
	mouse_down = 1;
	break;

      case SCI_EVT_NONE:
	s->gfx_driver->Wait(s, 2500); /* Wait 2.5 ms if no event took place */
	break;
      }

      if (mouse_down)
	menubar_map_pointer(s, &menu_nr, &item_nr, &port);

      /* Remove the active menu item, if neccessary */
      if ((backupped_port > -1) && item_nr != old_item)
	menubar_draw_item(s, &port, old_menu, old_item, 0);

      if (((item_nr > -1) && (backupped_port == -1)) || (menu_nr != old_menu)) { /* Update menu */

	if (backupped_port > -1) {
	  graph_restore_box(s, backupped_port);
	  graph_update_port(s, &port);
	}

	menubar_draw(s->pic, &(s->titlebar_port), s->menubar, menu_nr, s->titlebar_port.font);
	graph_update_port(s, &(s->titlebar_port));

	if (mouse_down || item_nr >= 0) {
	  backupped_port = menubar_draw_menu(s, menu_nr, &port);
	  old_item = -42; /* Make sure that the port is redrawn */
	}
	else
	  backupped_port = -1;
      } /* ...if the menu changed. */

      /* Update the menu entry, if neccessary */
      if ((backupped_port > -1) && item_nr != old_item) {
	menubar_draw_item(s, &port, menu_nr, item_nr, 1);
	graph_update_port(s, &port);
      }

      old_item = item_nr;
      old_menu = menu_nr;

    } /* while (menu_mode) */

    if (backupped_port > -1)
      graph_restore_box(s, backupped_port);

    status_bar_draw(s, s->status_bar_text);
    graph_update_port(s, &(s->titlebar_port));
  }

  if (claimed) {
    PUT_SELECTOR(event, claimed, 1);

    if (menu_nr > -1)
      s->acc = ((menu_nr + 1) << 8) | (item_nr + 1);
    else
      s->acc = 0;

    SCIkdebug(SCIkMENU, "Menu: Claim -> %04x\n", s->acc);
  }
  else s->acc = 0x0; /* Not claimed */
}
