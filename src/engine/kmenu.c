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

#include <engine.h>
#include <sci_widgets.h>


void
kAddMenu(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  menubar_add_menu(s->menubar, s->heap + UPARAM(0), s->heap + UPARAM(1), s->titlebar_port->font_nr, s->heap);
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

  sciw_set_status_bar(s, s->titlebar_port, s->status_bar_text);

  gfxop_update(s->gfx_state);
}


void
kDrawMenuBar(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;

  if (!s->titlebar_port->font_nr) {
    SCIkwarn(SCIkERROR, "No titlebar font is set: %d\n", s->titlebar_port->font_nr);
  }

#warning fixme!
#if 0
  if (PARAM(0))
    menubar_draw(s->pic, &(s->titlebar_port), s->menubar, -1, s->titlebar_port->font_nr);
  else
    draw_titlebar(s->pic, 0);

  graph_update_box(s, 0, 0, 320, 10);
#endif
}


#define ABOUT_FREESCI_PAGES_NR 3

struct {
  char *title;
  char *body;
  int fgcolor, bgcolor;
} _about_freesci_pages[ABOUT_FREESCI_PAGES_NR] = {
  {"FreeSCI hackers and contributors",
   "Bas Zoetekouw\nMan pages and debian package management\n\n"
   "Carl Muckenhoupt\nSources to the SCI resource viewer tools that started it all\n\n"
   "Chris Kehler\n\n"
   "Christoph Reichenbach\nProject & Website maintenance, UN*X code\n\n"
   "Christopher T. Lansdown\nCVS maintenance, Alpha compatibility fixes\n\n"
   "Claudio Matsuoka\nCVS snapshots, daily builds, bug reports\n\n",
   0, 15},
  {"More FreeSCI hackers and contributors",
   "Dark Minister\nSCI research (bytecode and parser)"
   "Dmitry Jemerov\nPort to the Win32 platform, numerous bugfixes\n\n"
   "Francois-R Boyer\nMT-32 information and mapping code\n\n"
   "Lars Skovlund\nMost of the relevant documentation, several bugfixes\n\n"
   "Magnus Reftel\nHeap implementation, Python class viewer, bugfixes\n\n"
   "Paul David Doherty\nGame version information",
   0, 15},
  {"Still more FreeSCI hackers & contributors",
   "Petr Vyhnak\nThe deflate-like SCI1 decompression algorithm\n\n"
   "Rainer De Temple\nSCI research\n\n"
   "Ravi I.\nSCI0 sound resource specification\n\n"
   "Rickard Lind\nMT32->GM MIDI mapping magic, sound research\n\n"
   "Rink Springer\nPort to the DOS platform, several bug fixes\n\n"
   "Sergey Lapin\nPort of Carl's type 2 decompression code",
   0, 15}
};


void
about_freesci(state_t *s)
{
#warning fixme!
#if 0
  int page;
  port_t port;
  byte *bodyfont, *titlefont;
  resource_t *bodyfont_res = NULL;
  int i;

  port.alignment = ALIGN_TEXT_CENTER;
  port.gray_text = 0;
  port.x = 0;
  port.y = 0;
  titlefont = s->titlebar_port.font;

  i = 999;
  while (!bodyfont_res && (i > -1))
    bodyfont_res = findResource(sci_font, i);

  if (i == -1)
    return;

  port.font = bodyfont = bodyfont_res->data;

  for (page = 0; page < ABOUT_FREESCI_PAGES_NR; ++page) {
    sci_event_t event;
    int cont = 2;
    int width, height, width2;

    port.x = 1;
    port.y = 5;

    port.color = _about_freesci_pages[page].fgcolor;
    port.bgcolor = _about_freesci_pages[page].bgcolor;

    get_text_size(_about_freesci_pages[page].body, bodyfont, 300, &width, &height);
    width2 = get_text_width(_about_freesci_pages[page].title, titlefont);

    if (width2 > width)
      width = width2;

    port.xmin = 156 - width / 2;
    port.xmax = 164 + width / 2;
    port.ymin = 100 - height / 2;
    port.ymax = 108 + height / 2;

    port.bg_handle = graph_save_box(s, port.xmin, port.ymin - 10, port.xmax - port.xmin + 1,
				    port.ymax - port.ymin + 11, SCI_MAP_VISUAL | SCI_MAP_PRIORITY);

    draw_window(s->pic, &port, port.bgcolor, 16, _about_freesci_pages[page].title, titlefont,
		WINDOW_FLAG_TITLE);
    text_draw(s->pic, &port, _about_freesci_pages[page].body, port.xmax - port.xmin - 3);
    graph_update_box(s, port.xmin - 2, port.ymin - 11, port.xmax - port.xmin + 5,
		     port.ymax - port.ymin + 14);

    while (cont) {
      event = getEvent(s);

      if (event.type == SCI_EVT_MOUSE_RELEASE || event.type == SCI_EVT_MOUSE_PRESS)
	--cont;

      if (event.type == SCI_EVT_KEYBOARD)
	cont = 0;

      s->gfx_driver->Wait(s, 25000);
    }

    graph_restore_box(s, port.bg_handle);
    graph_update_box(s, port.xmin - 2, port.ymin - 11, port.xmax - port.xmin + 5,
		     port.ymax - port.ymin + 14);
    
  }
#endif
}


static inline int
_menu_go_down(state_t *s, int menu_nr, int item_nr)
{
  int seeker, max = s->menubar->menus[menu_nr].items_nr;
  seeker = item_nr + 1;

  while ((seeker < max) && !menubar_item_valid(s, menu_nr, seeker))
    ++seeker;

  if (seeker != max)
    return seeker;
  else return item_nr;
}


void
kMenuSelect(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
#warning fixme!
#if 0
  heap_ptr event = UPARAM(0);
  int claimed = 0;
  int type = GET_SELECTOR(event, type);
  int message = GET_SELECTOR(event, message);
  int modifiers = GET_SELECTOR(event, modifiers);
  int menu_nr = -1, item_nr;
  menu_item_t *item;
  int menu_mode = 0; /* Menu is active */
  int mouse_down = 0;

  /* Check whether we can claim the event directly as a keyboard or said event */
  if (type & (SCI_EVT_KEYBOARD | SCI_EVT_SAID)) {
    int menuc, itemc;

    if ((type == SCI_EVT_KEYBOARD)
	&& (message == SCI_K_ESC))
      menu_mode = 1;

    else if ((type == SCI_EVT_SAID) || message) { /* Don't claim 0 keyboard event */
      SCIkdebug(SCIkMENU,"Menu: Got %s event: %04x/%04x\n",
		((type == SCI_EVT_SAID)? "SAID":"KBD"), message, modifiers);
    
      for (menuc = 0; menuc < s->menubar->menus_nr; menuc++)
	for (itemc = 0; itemc < s->menubar->menus[menuc].items_nr; itemc++) {
	  item = s->menubar->menus[menuc].items + itemc;

	  SCIkdebug(SCIkMENU,"Menu: Checking against %s: %04x/%04x (type %d, %s)\n",
		    item->text? item->text : "--bar--", item->key, item->modifiers,
		    item->type, item->enabled? "enabled":"disabled");

	  if (((item->type == MENU_TYPE_NORMAL)
	       && (item->enabled))
	      && (((type == SCI_EVT_KEYBOARD) /* keyboard event */
		   && menubar_match_key(item, message, modifiers))
		  || ((type == SCI_EVT_SAID) /* Said event */
		      && (item->flags & MENU_ATTRIBUTE_FLAGS_SAID)
		      && (said(s, item->said, (s->debug_mode & (1 << SCIkPARSER_NR))) != SAID_NO_MATCH)
		      )
		  )
	      ) {
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
	    item_nr = _menu_go_down(s, menu_nr, -1);
	  }
	  break;

	case SCI_K_RIGHT:
	  if (menu_nr < (s->menubar->menus_nr - 1)) {
	    ++menu_nr;
	    item_nr = -1;
	    item_nr = _menu_go_down(s, menu_nr, -1);
	  }
	  break;

	case SCI_K_UP:
	  if (item_nr > -1) {

	    do { --item_nr; }
	    while ((item_nr > -1) && !menubar_item_valid(s, menu_nr, item_nr));
	  }
	  break;

	case SCI_K_DOWN: {
	  item_nr = _menu_go_down(s, menu_nr, item_nr);
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

    if (backupped_port > -1) {
      graph_restore_box(s, backupped_port);
      graph_update_port(s, &port);
    }

    status_bar_draw(s, s->status_bar_text);
    graph_update_port(s, &(s->titlebar_port));
  }

  if (claimed) {
    PUT_SELECTOR(event, claimed, 1);

    if (menu_nr > -1) {
      s->acc = ((menu_nr + 1) << 8) | (item_nr + 1);

      if (s->menubar->menus[menu_nr].items[item_nr].flags == MENU_FREESCI_BLATANT_PLUG)
	about_freesci(s);

    } else
      s->acc = 0;

    SCIkdebug(SCIkMENU, "Menu: Claim -> %04x\n", s->acc);
  }
  else s->acc = 0x0; /* Not claimed */
#endif
  s->acc = 0; /* *** FIXME!!! *** */
}
