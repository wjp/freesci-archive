/***************************************************************************
 menubar.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Management and drawing operations for the SCI0 menu bar */
/* I currently assume that the hotkey information used in the menu bar is NOT
** used for any actual actions on behalf of the interpreter.
*/

#include <resource.h>
#include <menubar.h>
#include <engine.h>


char *
malloc_cpy(char *source)
{
  char *tmp = malloc(strlen(source) + 1);

  strcpy(tmp, source);
  return tmp;
}

char *
malloc_ncpy(char *source, int length)
{
  int rlen = MIN(strlen(source), length) + 1;
  char *tmp = malloc(rlen);

  strncpy(tmp, source, rlen);
  tmp[rlen -1] = 0;

  return tmp;
}

menubar_t *
menubar_new()
{
  menubar_t *tmp = malloc(sizeof(menubar_t));
  tmp->menus_nr = 0;

  return tmp;
}

void
menubar_free(menubar_t *menubar)
{
  int i;

  for (i = 0; i < menubar->menus_nr; i++) {
    menu_t *menu = &(menubar->menus[i]);
    int j;

    for (j = 0; j < menu->items_nr; j++) {
      if (menu->items[j].keytext)
	free (menu->items[j].keytext);
      if (menu->items[j].text)
        free (menu->items[j].text);
    }

    free(menu->items);
    free(menu->title);
  }

  if (menubar->menus_nr)
    free (menubar->menus);

  free(menubar);
}


int
_menubar_add_menu_item(menu_t *menu, int type, char *left, char *right, byte *font, int key,
		       int modifiers, int tag, heap_ptr text_pos)
/* Returns the total text size, plus MENU_BOX_CENTER_PADDING if (right != NULL) */
{
  menu_item_t *item;
  int total_left_size;

  if (menu->items_nr == 0) {
    menu->items = (menu_item_t *) malloc(sizeof(menu_item_t));
    menu->items_nr = 1;
  } else menu->items = (menu_item_t *) realloc(menu->items, sizeof(menu_item_t) * ++(menu->items_nr));

  item = &(menu->items[menu->items_nr - 1]);

  if ((item->type = type) == MENU_TYPE_HBAR)
    return 0;

  /* else assume MENU_TYPE_NORMAL */
  item->text = left;
  if (right)
  {
    item->keytext = right;
    item->flags = MENU_ATTRIBUTE_FLAGS_KEY;
    item->key = key;
    item->modifiers = modifiers;
  } else {
    item->keytext=NULL;
    item->flags = 0;
  }

  if (right)
    total_left_size = MENU_BOX_CENTER_PADDING + (item->keytext_size = get_text_width(right, font));
  else total_left_size = item->keytext_size = 0;

  item->enabled = 1;
  item->tag = tag;
  item->text_pos = text_pos;

  return total_left_size + get_text_width(left, font);
}

void
menubar_add_menu(menubar_t *menubar, char *title, char *entries, byte *font, byte *heapbase)
{
  int add_freesci = 0;
  menu_t *menu;
  char tracker;
  char *left = NULL, *right, *left_origin;
  int string_len = 0;
  int tag, c_width, max_width = 0;
  char *_heapbase = (char *) heapbase;

  if (menubar->menus_nr == 0) {
#ifdef MENU_FREESCI_BLATANT_PLUG
    add_freesci = 1;
#endif
    menubar->menus = malloc(sizeof(menu_t));
    menubar->menus_nr = 1;
  } else menubar->menus = realloc(menubar->menus, ++(menubar->menus_nr) * sizeof (menu_t));

  menu = &(menubar->menus[menubar->menus_nr-1]);
  menu->items_nr = 0;
  menu->title = malloc_cpy(title);

  menu->title_width = get_text_width(menu->title, font);

  do {
    tracker = *entries++;

    if (!left) { /* Left string not finished? */

      if ((tracker == ':') || (tracker == 0)) { /* End of entry */
	int entrytype = MENU_TYPE_NORMAL;

	left = malloc_ncpy(entries - string_len - 1, string_len);

	if (strcmp(left, MENU_HBAR_STRING) == 0) {
	  entrytype = MENU_TYPE_HBAR; /* Horizontal bar */
	}

	c_width = _menubar_add_menu_item(menu, entrytype, left, NULL, font, 0, 0, 0,
					 (entries - _heapbase) - string_len - 1);
	if (c_width > max_width)
	  max_width = c_width;

	string_len = 0;
	left = NULL; /* Start over */

      } else if (tracker == '`') { /* Start of right string */

	left = malloc_ncpy(left_origin = (entries - string_len - 1), string_len);
	string_len = 0; /* Continue with the right string */

      } 
      else string_len++; /* Nothing special */

    } else { /* Left string finished => working on right string */
      if ((tracker == ':') || (tracker == 0)) { /* End of entry */
	int key, modifiers = 0;

	right = malloc_ncpy(entries - string_len - 1, string_len);

	if (right[0] == '#') {
	  right[0] = 'F'; /* Function key */

	  key = SCI_K_F1 + (right[1] - '1');

	  if (right[1] == '0')
	    key = 10; /* F10 */

	  if (right[2]=='=') {
	    tag = atoi(right + 3);
	    right[2] = 0;
	  } else tag=0;	  
	} 
	else {

	  if (right[0] == '^') {
	    right[0] = '^'; /* Control key - there must be a replacement... */
	    key = right[1];
	    modifiers = SCI_EVM_CTRL;

	    if ((key >= 'a') && (key <= 'z'))
	      right[1] = key - 'a' + 'A';

	    if (right[2]=='=') {
	      tag = atoi(right+3);
	      right[2] = 0;
	    } else tag=0;	  

	  }
	  else {
	    key = right[0];
	    if ((key >= 'a') && (key <= 'z'))
	      right[0] = key - 'a' + 'A';

	    if (right[1]=='=') {
	      tag=atoi(right+2);
	      right[1] = 0;
	    } else tag=0;	  
	  }

	  if ((key >= 'A') && (key <= 'Z'))
	    key = key - 'A' + 'a'; /* Lowercase the key */
	}


	c_width = _menubar_add_menu_item(menu, MENU_TYPE_NORMAL, left, right, font, key,
					 modifiers, tag, left_origin - _heapbase);
	if (c_width > max_width)
	  max_width = c_width;

        string_len = 0;
        left = NULL;  /* Start over */

      } else string_len++; /* continuing entry */
    } /* right string finished */

  } while (tracker);

#ifdef MENU_FREESCI_BLATANT_PLUG
  if (add_freesci) {
      
    char *freesci_text = strdup ("About FreeSCI");
    c_width = _menubar_add_menu_item(menu, MENU_TYPE_NORMAL, freesci_text, NULL, font, 0, 0, 0, 0);
    if (c_width > max_width)
      max_width = c_width;

    menu->items[menu->items_nr-1].flags = MENU_FREESCI_BLATANT_PLUG;
  }
#endif /* MENU_FREESCI_BLATANT_PLUG */

  menu->width = max_width;
}


int
menubar_set_attribute(state_t *s, int menu_nr, int item_nr, int attribute, int value)
{
  menubar_t *menubar = s->menubar;
  menu_item_t *item;

  if ((menu_nr < 0) || (item_nr < 0))
    return 1;

  if ((menu_nr < menubar->menus_nr) || (item_nr < menubar->menus[menu_nr].items_nr))
    return 1;

  item = menubar->menus[menu_nr].items + item_nr;

  switch (attribute) {

  case MENU_ATTRIBUTE_SAID:
    if (value) {

      item->said_pos = value;
      memcpy(item->said, s->heap + value, 8); /* Copy Said spec */
      item->flags |= MENU_ATTRIBUTE_SAID;

    } else
      item->flags &= ~MENU_ATTRIBUTE_SAID;

    break;

  case MENU_ATTRIBUTE_TEXT:
    free(item->text);
    assert(value);
    item->text = strdup(s->heap + value);
    item->text_pos = value;
    break;

  case MENU_ATTRIBUTE_KEY:
    if (item->keytext)
      free(item->keytext);

    if (value) {

      item->key = value;
      item->modifiers = 0;
      item->keytext = malloc(2);
      item->keytext[0] = value;
      item->keytext[1] = 0;
      item->flags |= MENU_ATTRIBUTE_KEY;
      if ((item->key >= 'A') && (item->key <= 'Z'))
	item->key = item->key - 'A' + 'a'; /* Lowercase the key */


    } else {

      item->keytext = NULL;
      item->flags &= ~MENU_ATTRIBUTE_KEY;

    }

  case MENU_ATTRIBUTE_ENABLED:
    item->enabled = value;
    break;

  case MENU_ATTRIBUTE_TAG:
    item->tag = value;
    break;

  default:
    sciprintf("Attempt to set invalid attribute of menu %d, item %d: 0x%04x\n",
	      menu_nr, item_nr, attribute);
    return 1;
  }

  return 0;
}

int
menubar_get_attribute(state_t *s, int menu_nr, int item_nr, int attribute)
{
  menubar_t *menubar = s->menubar;
  menu_item_t *item;

  if ((menu_nr < 0) || (item_nr < 0))
    return -1;

  if ((menu_nr < menubar->menus_nr) || (item_nr < menubar->menus[menu_nr].items_nr))
    return -1;

  item = menubar->menus[menu_nr].items + item_nr;

  switch (attribute) {
  case MENU_ATTRIBUTE_SAID:
    return item->said_pos;

  case MENU_ATTRIBUTE_TEXT:
    return item->text_pos;

  case MENU_ATTRIBUTE_KEY:
    return item->key;

  case MENU_ATTRIBUTE_ENABLED:
    return item->enabled;

  case MENU_ATTRIBUTE_TAG:
    return item->tag;

  default:
    sciprintf("Attempt to read invalid attribute from menu %d, item %d: 0x%04x\n",
	      menu_nr, item_nr, attribute);
    return -1;
  }
}


void
menubar_draw(picture_t pic, port_t *port, menubar_t *menubar, int activated, byte *font)
{
  int x = MENU_LEFT_BORDER;
  int nr = 0; /* Menu number */

  draw_titlebar(pic, 0xf);

  for (; nr < menubar->menus_nr; nr++) {
    int text_x = x + MENU_BORDER_SIZE;
    int xlength = MENU_BORDER_SIZE * 2 + menubar->menus[nr].title_width;

    if (nr == activated)
      draw_titlebar_section(pic, x, xlength, 0);

    draw_text0(pic, port, text_x, 1, menubar->menus[nr].title, font, (nr == activated)? 0xff : 0x00);

    x += xlength;
  }
}


void
status_bar_draw(state_t *s, char *text)
{
  if (text) {
    draw_titlebar(s->pic, 0xf);
    draw_text0(s->pic, &(s->titlebar_port), 1, 1,
	       text, s->titlebar_port.font, 0);
  } else
    draw_titlebar(s->pic, 0);
}

int
menubar_draw_menu(struct _state *s, int menu_nr, port_t *menu_port)
{
  int backup_handle;
  int x = MENU_LEFT_BORDER, xl, yl, i;
  menubar_t *menubar = s->menubar;
  menu_t *menu = menubar->menus + menu_nr;

  if (menu_nr >= menubar->menus_nr)
    return -1;

  for (i = 0; i < menu_nr; i++)
    x += MENU_BORDER_SIZE * 2 + menubar->menus[i].title_width;

  xl = menu->width;
  yl = menu->items_nr * 10; /* Rather trivial ATM */

  backup_handle = graph_save_box(s, x - 1, 10, xl + 5, yl + 1, SCI_MAP_VISUAL);
  draw_frame(s->pic, x - 1, 10, xl + 5, yl + 1, 0x00, -1);
  draw_box(s->pic, x, 10, xl + 3, yl, 0xff, -1);

  menu_port->ymin = 10;
  menu_port->xmin = x;
  menu_port->ymax = 10 + yl;
  menu_port->xmax = x + xl + 3;

  for (i = 0; i < menu->items_nr; i++)
    menubar_draw_item(s, menu_port, menu_nr, i, 0);

  return backup_handle;
}


void
menubar_draw_item(struct _state *s, port_t *port, int menu_nr, int item_nr, int active)
{
  menubar_t *menubar = s->menubar;
  menu_item_t *item;
  int y = item_nr * 10;

  if (((menu_nr < 0) || (item_nr < 0))
      || ((menu_nr >= menubar->menus_nr) || (item_nr >= menubar->menus[menu_nr].items_nr)))
    return;

  item = menubar->menus[menu_nr].items + item_nr;

  switch (item->type) {
  case MENU_TYPE_NORMAL: {

    graph_fill_box_custom(s, port->xmin, y + 10, port->xmax - port->xmin, 10,
			  active? 0x00 : 0xff, -1, -1, SCI_MAP_VISUAL);
    
    port->alignment = ALIGN_TEXT_LEFT;
    port->bgcolor = -1;
    port->gray_text = !item->enabled;

    port->y = y + 1;
    port->x = 1;
    port->color = active? 0xff : 0x00;
    text_draw(s->pic, port, item->text, port->xmax - port->xmin - 3);

    if (item->keytext) {
      port->alignment = ALIGN_TEXT_RIGHT;
      text_draw(s->pic, port, item->keytext, port->xmax - port->xmin - 3);
    }
  }
  break;

  case MENU_TYPE_HBAR: {
    dither_line(s->pic, port->xmin, y + 5, port->xmax - 1, y + 5,
		0xff, 0x00, -1, -1, SCI_MAP_VISUAL);
  }
  break;
  }

  item = menubar->menus[menu_nr].items + item_nr;
}

int
menubar_item_valid(state_t *s, int menu_nr, int item_nr)
{
  menubar_t *menubar = s->menubar;
  menu_item_t *item;

  if ((menu_nr < 0) || (item_nr < 0))
    return 0;

  if ((menu_nr >= menubar->menus_nr) || (item_nr >= menubar->menus[menu_nr].items_nr))
    return 0;

  item = menubar->menus[menu_nr].items + item_nr;

  if ((item->type == MENU_TYPE_NORMAL)
      && item->enabled)
    return 1;

  return 0; /* May not be selected */
}


int
menubar_map_pointer(state_t *s, int *menu_nr, int *item_nr, port_t *port)
{
  menubar_t *menubar = s->menubar;
  menu_t *menu;

  if (s->pointer_y <= 10) { /* Re-evaulate menu */
    int x = MENU_LEFT_BORDER;
    int i;

    for (i = 0; i < menubar->menus_nr; i++) {
      int newx = x + MENU_BORDER_SIZE * 2 + menubar->menus[i].title_width;

      if (s->pointer_x < x)
	return 0;

      if (s->pointer_x < newx) {
	*menu_nr = i;
	*item_nr = -1;
      }

      x = newx;
    }

    return 0;

  } else {
    int row = (s->pointer_y / 10) - 1;

    if ((*menu_nr < 0) || (*menu_nr >= menubar->menus_nr))
      return 1; /* No menu */
    else
      menu = menubar->menus + *menu_nr; /* Menu is valid, assume that it's popped up */

    if (menu->items_nr <= row)
      return 1;

    if ((s->pointer_x < port->xmin) || (s->pointer_x > port->xmax))
      return 1;

    if (menubar_item_valid(s, *menu_nr, row))
      *item_nr = row; /* Only modify if we'll be hitting a valid element */

    return 0;
  }

}
