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

#define SCI_COLOR_DITHER 0
/* Standard mode */
#define SCI_COLOR_INTERPOLATE 1
/* Interpolate colors */
#define SCI_COLOR_DITHER256 2
/* Dither with 256 colors */

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

void drawPicture0(picture_t dest, guint8* data);
/* Draws a picture resource to a picture_t buffer.
** Parameters: (picture_t) dest: The initialized picture buffer to draw to.
**             (guint8*) data: The data to draw (usually resource_t.data).
** Remember that this function is much slower than copyPicture, so you should
** store a backup copy of the drawn picture if you want to use it to display
** some animation.
*/

void clearPicture(picture_t pic);
/* Clears a picture
** Parameters: (picture_t) pic: The picture to clear
** Returns   : (void)
** Clearing a picture means that buffers 1-3 are set to 0, while buffer 0
** is set to zero for the first ten pixel rows and to 15 for the remainder.
** This must be called before a drawView0(), unless the picture is intended
** to be drawn ontop of an already existing picture.
*/

int drawView0(picture_t dest, int x, int y, short priority,
	      short group, short index, guint8 *data);
/* Draws a specified element of a view resource to a picture.
** Parameters: (picture_t) dest: The picture_t to draw to.
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

void drawBox0(picture_t dest, short x, short y, short xl, short yl, char color);
/* Draws a shaded box.
** Parameters: (picture_t) dest: The picture_t to draw to.
**             (short) x,y: The coordinates to draw to.
**             (short) xl,yl: The width and height of the box.
**             (char) color: The color to draw with.
** Returns   : (void)
** The box will then be drawn accoding to the provided parameters, outlined
** in black (thus increasing the size) and provided with a shadow (one pixel
** thick, black and towards the lower right).
*/

void drawTextboxxy0(picture_t dest, short x, short y, char *text, char *font,
		     char color);
void drawTextboxy0(picture_t dest, short y, char *text, char *font, char color);
void drawTextbox0(picture_t dest, char* text, char *font, char color);
/* Draws a shaded box according to width and height calculated from the
** provided text and font.
** Parameters: (picture_t) dest: The picture_t to draw to.
**             (short) x,y: The coordinates to draw to.
**             (char *) text: The text.
**             (char *) font: The font to calculate with.
**             (char) color: The color to draw the box in.
** Returns   : (void)
** This will only draw the BOX, not the actual text.
** If either the x coordinate or both x and y coordinates are not provided,
** the box will be centered.
*/

void drawTextxy0(picture_t dest, short x, short y, char *text, char *font,
		  char color);
void drawText0(picture_t dest, char *text, char *font, char color);
/* Draws text in a specific font and color.
** Parameters: (picture_t) dest: The picture_t to draw to.
**             (short) x,y: The coordinates to draw to.
**             (char *) text: The text to draw.
**             (char *) font: The font to draw the text in.
**             (char) color: The color to use for drawing.
** Returns   : (void)
** This will only draw the TEXT, without any surrounding box.
** If no x,y coordinates are provided, the coordinates of the last drawTextBox()
** call are assumed.
*/


void drawMouseCursor(picture_t target, int x, int y, guint8 *cursor);
/* Draws a mouse cursor
** Parameters: target: The picture_t to draw to
**             (x,y): The coordinates to draw to (are clipped to valid values)
**             cursor: The cursor data to draw
** This function currently uses SCI0 cursor drawing for everything.
*/

#endif
