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
      if (menu->items[j].right)
	free (menu->items[j].right);
      if (menu->items[j].left)
        free (menu->items[j].left);
    }

    free(menu->items);
    free(menu->title);
  }

  if (menubar->menus_nr)
    free (menubar->menus);

  free(menubar);
}


int
_menubar_add_menu_item(menu_t *menu, int type, char *left, char *right, byte *font)
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
  item->left = (char *) malloc (strlen (left)+1);
  strcpy (item->left, left);
  if (right)
  {
    item->right = (char *) malloc (strlen (right)+1);
    strcpy (item->right, right);
  }
  else item->right=NULL;

  if (right)
    total_left_size = MENU_BOX_CENTER_PADDING + (item->rightsize = get_text_width(right, font));
  else total_left_size = item->rightsize = 0;

  item->foo = item->bar = 0; /* Set those anomalities to zero until I know what they mean */

  return total_left_size + get_text_width(left, font);
}

void
menubar_add_menu(menubar_t *menubar, char *title, char *entries, byte *font)
{
  int add_freesci = 0;
  menu_t *menu;
  char tracker;
  char *left = 0, *right;
  int string_len = 0;
  int c_width, max_width = 0;

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

	if (strcmp(left, MENU_HBAR_STRING) == 0)
	  entrytype = MENU_TYPE_HBAR; /* Horizontal bar */

	c_width = _menubar_add_menu_item(menu, entrytype, left, NULL, font);
	if (c_width > max_width)
	  max_width = c_width;

	string_len = 0;
	left = NULL; /* Start over */

      } else if (tracker == '`') { /* Start of right string */

	left = malloc_ncpy(entries - string_len - 1, string_len);
	string_len = 0; /* Continue with the right string */

      } string_len++; /* Nothing special */

    } else { /* Left string finished => working on right string */
      if ((tracker == ':') || (tracker == 0)) { /* End of entry */

	right = malloc_ncpy(entries - string_len - 1, MIN(2, string_len));
	/* Some entries have strange stuff added to the end */

	if (right[0] == '#')
	  right[0] = 'F'; /* Function key */

	if (right[0] == '^')
	  right[0] = 2; /* Control key */

	c_width = _menubar_add_menu_item(menu, MENU_TYPE_NORMAL, left, right, font);
	if (c_width > max_width)
	  max_width = c_width;

      } else string_len++; /* continuing entry */
      if (string_len);
    } /* right string finished */

  } while (tracker);

#ifdef MENU_FREESCI_BLATANT_PLUG
  if (add_freesci) {
      
    c_width = _menubar_add_menu_item(menu, MENU_TYPE_NORMAL, "About FreeSCI", NULL, font);
    if (c_width > max_width)
      max_width = c_width;

    menu->items[menu->items_nr-1].foo = MENU_FREESCI_BLATANT_PLUG;
  }
#endif /* MENU_FREESCI_BLATANT_PLUG */

  menu->width = max_width;
}


int
menubar_set_foobar(menubar_t *menubar, int menu, int item, int foo, int bar)
{
  if ((menu < 0) || (item < 0))
    return 1;

  if ((menu < menubar->menus_nr) || (item < menubar->menus[menu].items_nr))
    return 1;

  menubar->menus[menu].items[item].foo = foo;
  menubar->menus[menu].items[item].bar = bar;

  return 0;
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

    draw_text0(pic, port, text_x, 1, menubar->menus[nr].title, font, (nr == activated)? 0xf:0);

    x += xlength;
  }
}
