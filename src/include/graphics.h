/***************************************************************************
 graphics.h (C) 1999 Christoph Reichenbach, TU Darmstadt


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

 History:

   990401 - created (honestly!) (CJR)

***************************************************************************/

#ifndef _SCI_GRAPHICS_H_
#define _SCI_GRAPHICS_H_

#include <resource.h>
#include <vm.h>


#define SCI_GRAPHICS_ALLOW_256
/* Allows 256 color modes */

#define _DEBUG_VISUALS
/* Use this define only if multiple visuals are available for your
** target, e.g. under X.
*/

typedef guint8** picture_t;
/* Used for storing "picture" resources (the background images). These
** have four layers: the actual screen buffer, a priority buffer (essentially
** a simple z buffer), a 'special' buffer (defining, among other things,
** the walkable area), and a write buffer, which is used internally while
** creating the picture.
*/


typedef struct {
  gint16 ymin, xmin; /* Upper left corner */
  gint16 ymax, xmax; /* Lower right corner */
} port_t;


typedef struct {
  gint16 ymin, xmin; /* Upper left corner */
  gint16 ymax, xmax; /* Lower right corner */

  heap_ptr title; /* Window title (if applicable) */

  gint16 flags; /* Window flags. See below. */

  gint16 priority, bgcolor, color; /* Priority/color values as usual */
} window_t; /* Can be typecast safely to become a port_t */


typedef struct {
  int hot_x, hot_y; /* Hot spots */
  int size_x, size_y; /* Bitmap size */
  byte *bitmap; /* Actual bitmap data */
  int color_key; /* The color used for transparency in the bitmap */
} mouse_pointer_t;


/****************************** GRAPHICS CALLBACK FUNCTION ******************************/
/* The graphics callback function is used by some kernel functions to redraw all or part
** of the screen. Its parameters are the current state, a command, two coordinates, and
** two block sizes. Each graphics library implementation must provide such a callback
** function, which is usually set automatically during graphics initialization.
*/

#define GRAPHICS_CALLBACK_REDRAW_ALL 0
/* Callback command to redraw the entire screen */

#define GRAPHICS_CALLBACK_REDRAW_BOX 1
/* Callback command to redraw the specified part of the screen and the mouse pointer */

#define GRAPHICS_CALLBACK_REDRAW_POINTER 2
/* Callback command to update the mouse pointer */


/****************************************************************************************/


#define SCI_COLOR_DITHER 0
/* Standard mode */
#define SCI_COLOR_INTERPOLATE 1
/* Interpolate colors */
#define SCI_COLOR_DITHER256 2
/* Dither with 256 colors */


#define SCI_FILL_NORMAL 1
/* Fill with the selected color */
#define SCI_FILL_BLACK 0
/* Fill with black */



/* The following flags are applicable to windows in SCI0: */
#define WINDOW_FLAG_TRANSPARENT 0x01
/* Window doesn't get filled with background color */

#define WINDOW_FLAG_NOFRAME 0x02
/* No frame is drawn around the window onto wm_view */

#define WINDOW_FLAG_TITLE 0x04
/* Add titlebar to window (10 pixels high, framed, text is centered and written
** in white on dark gray
*/

#define WINDOW_FLAG_DONTDRAW 0x80
/* Don't draw anything */


#define SCI_SCREEN_WIDTH 320
#define SCI_SCREEN_HEIGHT 200


extern int sci_color_mode;
/* sci_color_interpolate forces 16 color background pictures to be drawn
** with 256 interpolated colors instead of 16 dithered colors
*/

/*** FUNCTION DECLARATIONS ***/

picture_t allocEmptyPicture();
/* Creates an initialized but empty picture buffer.
** Paramters: void
** Returns  : (picture_t) An empty picture.
*/

void freePicture(picture_t picture);
/* Deallocates all memory associated by the specified picture_t.
** Parameters: (picture_t) picture: The picture to deallocate.
** Returns   : (void)
*/

void copyPicture(picture_t dest, picture_t src);
/* Copies the content of a picture.
** Parameters: (picture_t) dest: An INITIALIZED picture which the data is to
**                                be copied to.
**             (picture_t) src: The picture which should be duplicated.
** Returns   : (void)
*/

void drawPicture0(picture_t dest, int flags, int defaultPalette, guint8* data);
/* Draws a picture resource to a picture_t buffer.
** Parameters: (picture_t) dest: The initialized picture buffer to draw to.
**             (int) flags: The picture flags. Currently, only bit 0 is used;
**                          with bit0, pictures are filled normally, with !bit0,
**                          fill commands are executed by filling in black.
**             (int) defaultPalette: The default palette to use for drawing
**                          (used to distinguish between day and night in some
**                          games)
**             (guint8*) data: The data to draw (usually resource_t.data).
** Remember that this function is much slower than copyPicture, so you should
** store a backup copy of the drawn picture if you want to use it to display
** some animation.
*/

void clearPicture(picture_t pic, int fgcol);
/* Clears a picture
** Parameters: (picture_t) pic: The picture to clear
**             (int) fgcol: The foreground color to use for screen 0
** Returns   : (void)
** Clearing a picture means that buffers 1-3 are set to 0, while buffer 0
** is set to zero for the first ten pixel rows and to 15 for the remainder.
** This must be called before a drawView0(), unless the picture is intended
** to be drawn ontop of an already existing picture.
*/

int drawView0(picture_t dest, port_t *port, int x, int y, short priority,
	      short group, short index, guint8 *data);
/* Draws a specified element of a view resource to a picture.
** Parameters: (picture_t) dest: The picture_t to draw to.
**             (port_t *) port: The viewport to draw to (NULL for the WM port)
**             (int) x,y: The position to draw to (clipping is performed).
**             (short) priority: The image's priority in the picture. Images
**                               with a higher priority cannot be overdrawn
**                               by pictures with a lower priority. Sensible
**                               values range from 0 to 15.
**             (short) group: A view resource consists of one or more groups of
**                            images, which usually form an animation. Use this
**                            value to determine the group.
**             (short) index: The picture index inside the specified group.
**             (guint8*) data: The data to draw (usually resource_t.data).
** Returns   : (int) 0 on success, -1 if the specified group could not be
**             found, or -2 if the index inside the group is invalid.
*/

int
view0_cel_width(int loop, int cel, byte *data);
/* Returns the width of an SCI0 view cell.
** Parameters: (int) loop: The loop to examine
**             (int) cel: The cell
**             (byte *): The view data
** Returns   : (int) The width, or -1 if loop or cel were invalid.
*/


int
view0_cel_height(int loop, int cel, byte *data);
/* Returns the height of an SCI0 view cell.
** Parameters: (int) loop: The loop to examine
**             (int) cel: The cell
**             (byte *): The view data
** Returns   : (int) The height, or -1 if loop or cel were invalid.
*/


void drawBox(picture_t dest, short x, short y, short xl, short yl, char color, char priority);
/* Draws a simple box.
** Parameters: (picture_t) dest: The picture_t to draw to.
**             (short) x,y: The coordinates to draw to.
**             (short) xl,yl: The width and height of the box.
**             (char) color: The color to draw with.
**             (char) priority: The priority to fill the box with (it still overwrites anything)
** Returns   : (void)
** The box does not come with any fancy shading. Use drawWindow to do this.
*/


void dither_line(picture_t dest, int x1, int y1, short x2, short y2,
		 int col1, int col2, int priority, int special, char drawenable);
/* Draws a line.
** Parameters: (picture_t) dest: The picture_t to draw to
**             (int) x1, y1: Start position
**             (int) x2, y2: End position
**             (int) col1, col2: The two colors used for foreground dithering
**             (int) priority: The priority to draw with
**             (int) special: The special color to draw with
**             (char) drawenable: Bit 0: Enable visual map drawing
**                                Bit 1: Enable priority map drawing
**                                Bit 2: Enable special map drawing
** Returns   : (void)
*/


int
get_text_width(char *text, byte *font);
/* Returns the width of the specified text in the specified font.
** Parameters: (char *) text: The text to check
**             (byte *) font: The font to check it with
** Returns   : (int) The width of the longest line of the text in the specified font.
** The '\n' character is treated as usual (newline), so that the provided text may
** contain of several lines of text. In this case, the length of the longest line is
** returned.
*/


void draw_titlebar(picture_t dest, int color);
/* Fills the title bar with the specified color
** Parameters: (picture_t) dest: The picture_t to draw to
**             (int) color: The color which the titlebar should be filled with (usually 0 or 0xf)
** Returns   : (void)
*/

void
draw_titlebar_section(picture_t dest, int start, int length, int color);
/* Fills a section of the title bar with the specified color
** Parameters: (picture_t) dest: The picture_t to draw to
**             (int) start: The x coordinate to start at
**             (int) length: The length of the block to draw
**             (int) color: The color which the titlebar should be filled with (usually 0 or 0xf)
** Returns   : (void)
*/


void drawWindow(picture_t dest, port_t *port, char color, char priority,
		char *title, guint8 *titlefont, gint16 flags);
/* Draws a window with the appropriate facilities
** Parameters: (picture_t) dest: The picture_t to draw to
**             (port_t *) port: The port to draw to
**             (short) color: The background color of the window
**             (short) priority: The window's priority
**             (char *) title: The title to draw (see flags)
**             (char *) titlefont: The font which the title should be drawn with
**             (gint16) flags: The window flags (see the beginning of this header file)
** This function will draw a window; it is very similar to the kernel call NewWindow() in its
** functionality.
*/


void drawText0(picture_t dest, port_t *port, int x, int y, char *text, char *font, char color);
void drawTextCentered0(picture_t dest, port_t *port, int x, int y, char *text, char *font, char color);
/* Draws text in a specific font and color.
** Parameters: (picture_t) dest: The picture_t to draw to.
**             (port_t *) port: The port to draw to.
**             (int) x, y: The coordinates (relative to the port) to draw to.
**             (char *) text: The text to draw.
**             (char *) font: The font to draw the text in.
**             (char) color: The color to use for drawing.
** Returns   : (void)
** This will only draw the supplied text, without any surrounding box.
** drawTextCentered will, in addition, center the text to the supplied port.
*/


void drawMouseCursor(picture_t target, int x, int y, guint8 *cursor);
/* DEPRECATED
** Draws a mouse cursor
** Parameters: target: The picture_t to draw to
**             (x,y): The coordinates to draw to (are clipped to valid values)
**             cursor: The cursor data to draw
** This function currently uses SCI0 cursor drawing for everything.
*/


mouse_pointer_t *
calc_mouse_cursor(byte *data);
/* Calculates mouse cursor data
** Parameters: (byte *) data: The resource data from which it should be calculated
** Returns   : (mouse_pointer_t *) A pointer to a valid mouse_pointer_t structure
** The mouse_pointer structure is allocated with malloc(). To unallocate it, free_mouse_cursor()
** below can be used.
*/

void
free_mouse_cursor(mouse_pointer_t *pointer);
/* Frees space allocated by a mouse_pointer_t structure
** Parameters: (mouse_pointer_t *) pointer: The pointer to deallocate
** Returns   : (void)
** A simple function to free all memory allocated with the specified pointer.
*/

int
open_visual_ggi(struct _state *s);
/* Opens a ggi visual and sets all callbacks to ggi-specific functions
** Parameter: (state_t *) s: Pointer to the affected state_t
** Returns  : (int) 0 on success, 1 otherwise
*/

void
close_visual_ggi(struct _state *s);
/* Closes a ggi visual on a state_t
** Parameter: (state_t *) s: Pointer to the affected state_t
** Returns  : (void)
*/

#endif
