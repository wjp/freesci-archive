/***************************************************************************
 sci_widgets.h Copyright (C) 2000 Christoph Reichenbach


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

    Christoph Reichenbach (CR) <jameson@linuxgames.com>

***************************************************************************/
/* SCI-specific widget handling */

#ifndef _SCI_WIDGETS_H_
#define _SCI_WIDGETS_H_

#include <engine.h>

/* The following flags are applicable to windows in SCI0: */
#define WINDOW_FLAG_TRANSPARENT 0x01


#define WINDOW_FLAG_NOFRAME 0x02
/* No frame is drawn around the window onto wm_view */

#define WINDOW_FLAG_TITLE 0x04
/* Add titlebar to window (10 pixels high, framed, text is centered and written
** in white on dark gray
*/

#define WINDOW_FLAG_DONTDRAW 0x80
/* Don't draw anything */



void
sciw_set_status_bar(state_t *s, gfxw_port_t *status_bar, char *text);
/* Sets the contents of a port used as status bar
** Parmeters: (state_t *) s: The affected game state
**            (gfxw_port_t *) status_bar: The status bar port
**            (char *) text: The text to draw
** Returns  : (void)
*/

gfxw_port_t *
sciw_new_window(state_t *s, rect_t area, int font, gfx_color_t color, gfx_color_t bgcolor,
		int title_font, gfx_color_t title_color, gfx_color_t title_bg_color,
		char *title, int flags);
/* Creates a new SCI style window
** Parameters: (state_t *) s: The affected game state
**             (rect_t) area: The screen area to frame (not including a potential window title)
**             (int) font: Default font number to use
**             (gfx_color_t) color: The foreground color to use for drawing
**             (gfx_color_t) bgcolor: The background color to use
**             (int) title_font: The font to use for the title bar (if any)
**             (gfx_color_t) title_color: Color to use for the title bar text
**             (gfx_color_t) title_bg_color: Color to use for the title bar background
**             (char *) title: The text to write into the title bar
**             (int) flags: Any ORred combination of window flags
** Returns   : (gfxw_port_t *) A newly allocated port with the requested characteristics
*/

#endif /* _!SCI_WIDGETS_H_ */


