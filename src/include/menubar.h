/***************************************************************************
 menubar.h Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Header for SCI0 menu bar management */

#ifndef _SCI_MENUBAR_H_
#define _SCI_MENUBAR_H_

#include <graphics.h>

struct _state;

#define MENU_FREESCI_BLATANT_PLUG 0xfff0
/* This adds an "About FreeSCI" menu option to the first menu */


#define MENU_HBAR_STRING "!--"
/* The string used in SCI to determine an empty menu line */

#define MENU_BORDER_SIZE 0
/* The number of pixels added to the left and right to the text of a menu on the menu bar */

#define MENU_LEFT_BORDER 5
/* The number of pixels added to the left of the first menu */

#define MENU_BOX_CENTER_PADDING 5
/* Number of pixels to leave in between the left and the right centered text content in boxes
** that use right centered content
*/


#define MENU_TYPE_NORMAL 0
#define MENU_TYPE_HBAR 1 /* Horizontal bar */


#define MENU_ATTRIBUTE_SAID 0x6d
#define MENU_ATTRIBUTE_TEXT 0x6e
#define MENU_ATTRIBUTE_KEY 0x6f
#define MENU_ATTRIBUTE_ENABLED 0x70
#define MENU_ATTRIBUTE_TAG 0x71

/* Those flags determine whether the corresponding menu_item_t entries are valid */
#define MENU_ATTRIBUTE_FLAGS_KEY 0x01
#define MENU_ATTRIBUTE_FLAGS_SAID 0x02

typedef struct {
  int type; /* Normal or hbar */
  char *keytext; /* right-centered part of the text (the key) */
  int keytext_size; /* Width of the right-centered text */

  int flags;
  byte said[8]; /* Said spec for this item */
  heap_ptr said_pos;
  char *text;
  heap_ptr text_pos;
  int modifiers, key; /* Hotkey for this item */
  int enabled;
  int tag;

} menu_item_t;


typedef struct {
  char *title;

  int title_width; /* Width of the title in pixels */
  int width; /* Pixel width of the menu window */

  int items_nr; /* Window height equals to intems_nr * 10 */
  menu_item_t *items; /* Actual entries into the menu */

} menu_t;



typedef struct {

  int menus_nr;
  menu_t *menus; /* The actual menus */

} menubar_t;




/********** function definitions *********/

char *
malloc_cpy(char *source);
/* Copies a string into a newly allocated memory part
** Parameters: (char *) source: The source string
** Returns   : (char *) The resulting copy, allocated with malloc().
** To free this string, use the standard free() command.
*/


char *
malloc_ncpy(char *source, int length);
/* Copies a string into a newly allocated memory part, up to a certain length.
** Parameters: (char *) source: The source string
**             (int) length: The maximum length of the string (not conting a trailing \0).
** Returns   : (char *) The resulting copy, allocated with malloc().
** To free this string, use the standard free() command.
*/


menubar_t *
menubar_new();
/* Creates a new menubar struct
** Parameters: (void)
** Returns   : (menubar_t *) A pointer to the new menubar entity
** To free the entity, call menubar_free.
*/


void
menubar_free(menubar_t *menubar);
/* Frees all memory associated with a menubar
** Parameters: (menubar_t *) menubar: The menubar to free
** Returns   : (void)
*/


void
menubar_add_menu(menubar_t *menubar, char *title, char *entries, byte *font, byte *heapbase);
/* Adds a menu to the menubar.
** Parameters: (menubar_t *) menubar: The menubar to operate on
**             (char *) title: The menu title
**             (char *) entries: A string of menu entries
**             (byte *) font: The font which is to be used for drawing
**             (byte *) heapbase: Base address of the heap 'entries' is read from
** Returns   : (void)
** The menu entries use the following special characters:
** '`' : Right justify the following part
** ':' : End of this entry
** '#' : Function key (replaced by 'F')
** '^' : Control key (replaced by \002, which looks like "CTRL")
** and the special string "--!", which represents a horizontal bar in the menu.
**
** If MENU_FREESCI_BLATANT_PLUG is defined, an additional option "About FreeSCI" will be
** added if this was the first menu to be added to the menu bar.
*/


int
menubar_set_attribute(struct _state *s, int menu, int item, int attribute, int value);
/* Sets the (currently unidentified) foo and bar values.
** Parameters: (state_t *) s: The current state
**             (int) menu: The menu number to edit
**             (int) item: The menu item to change
**             (int) attribute: The attribute to modify
**             (int) value: The value the attribute should be set to
** Returns   : (int) 0 on success, 1 if either menu or item were invalid
*/


int
menubar_get_attribute(struct _state *s, int menu, int item, int attribute);
/* Sets the (currently unidentified) foo and bar values.
** Parameters: (state_t *) s: The current state
**             (int) menu: The menu number
**             (int) item: The menu item to read
**             (int) attribute: The attribute to read from
** Returns   : (int) The attribute value, or -1 on error
*/

void
menubar_draw(picture_t pic, port_t *port, menubar_t *menubar, int activated, byte *font);
/* Draws the menu bar
** Parameters: (picture_t *) pic: The picture to draw to
**             (port_t *) port: The port to draw into
**             (menubar_t *) menubar: The menu bar to draw
**             (int) activated: The number of the menu option to activate
**             (byte *) font: The font to draw with
** Returns   : (void)
** Use an illegal value for "activated" (like -1) in order not to activate any
** entry.
*/

#endif /* !_SCI_MENUBAR_H_ */

