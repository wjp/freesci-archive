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


struct _picture {

  guint8 *view;    /* The currently visible picture */
  guint8 *maps[4]; /* Background picture: View, Priority, Control, and Auxiliary */

  int xres, yres;  /* Horizontal and vertical size of the maps */
  int bytespp;     /* Bytes per pixel */
  int bytespl;     /* Bytes per line */
  int view_offs;   /* Offset of the first pixel below the title bar */

  int size;        /* Total pic size (of one map) */
  int view_size;   /* Total pic size not counting the title bar */

  int ega_colors[16]; /* The 16 EGA colors- this must be filled in by the
		      ** graphics implementation.
		      */

  /* Translation stuff: */
  int xfact, yfact; /* Factors for calculating coordinates */
};

typedef struct _picture* picture_t;
/* Used for storing "picture" resources (the background images). These
** have four layers: the actual screen buffer, a priority buffer (essentially
** a simple z buffer), a 'special' buffer (defining, among other things,
** the walkable area), and a write buffer, which is used internally while
** creating the picture.
*/


typedef struct {
  gint16 ymin, xmin; /* Upper left corner */
  gint16 ymax, xmax; /* Lower right corner */

  heap_ptr title; /* Window title (if applicable) */

  gint16 flags; /* Window flags. See below. */
  int alignment; /* ALIGN_TEXT_* to determine how text should be displayed */

  int x,y; /* Drawing coordinates */
  gint16 priority, bgcolor, color; /* Priority/color values as usual */

  byte *font; /* Font data */
  byte gray_text; /* Set to 1 to "gray out" text */

} port_t;


typedef struct {
  int hot_x, hot_y; /* Hot spots */
  int size_x, size_y; /* Bitmap size */
  byte *bitmap; /* Actual bitmap data */
  int color_key; /* The color used for transparency in the bitmap */
} mouse_pointer_t;


/* The three maps */
#define SCI_MAP_VISUAL 0x1
#define SCI_MAP_PRIORITY 0x2
#define SCI_MAP_CONTROL 0x4


#define SCI_RESOLUTION_320X200 0
#define SCI_RESOLUTION_640X400 1
/* The two resolutions that should be supported */

#define SCI_COLORDEPTH_8BPP 1
#define SCI_COLORDEPTH_16BPP 2
/* The two color depths that should be supported */

#define SCI_MAP_EGA_COLOR(pic, col) (pic->bytespp == 1)? (col | (col << 4)) : pic->ega_colors[col]
/* Macro for color mapping */


/****************************** GRAPHICS CALLBACK FUNCTION ******************************/
/* The graphics callback function is used by some kernel functions to redraw all or part
** of the screen. Its parameters are the current state, a command, two coordinates, and
** two block sizes. Each graphics library implementation must provide such a callback
** function, which is usually set automatically during graphics initialization.
*/

#define GRAPHICS_CALLBACK_REDRAW_ALL 0
/* Callback command to redraw the entire screen */

#define GRAPHICS_CALLBACK_REDRAW_BOX 1
/* Callback command to redraw the specified part of the screen */

#define GRAPHICS_CALLBACK_REDRAW_POINTER 2
/* Callback command to update the mouse pointer */


/****************************************************************************************/


#define SCI_COLOR_DITHER 0
/* Standard mode */
#define SCI_COLOR_INTERPOLATE 1
/* Interpolate colors */
#define SCI_COLOR_DITHER256 2
/* Dither with 256 colors */


/* Text justifications */
#define ALIGN_TEXT_RIGHT -1
#define ALIGN_TEXT_LEFT 0
#define ALIGN_TEXT_CENTER 1

#define MAX_TEXT_WIDTH_MAGIC_VALUE 192
/* This is the real width of a text with a specified width of 192 */


#define FONT_FONTSIZE_OFFSET 4
/* Offset of the font size information in the font resource */

#define FONT_MAXCHAR_OFFSET 2
/* Offset of the maximum character value */

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

#define SELECTOR_STATE_SELECTABLE 1
#define SELECTOR_STATE_FRAMED 2
#define SELECTOR_STATE_DISABLED 4
#define SELECTOR_STATE_SELECTED 8


#define SCI_SCREEN_WIDTH 320
#define SCI_SCREEN_HEIGHT 200


extern int sci_color_mode;
/* sci_color_interpolate forces 16 color background pictures to be drawn
** with 256 interpolated colors instead of 16 dithered colors
*/

/*** FUNCTION DECLARATIONS ***/

picture_t alloc_empty_picture(int resolution, int colordepth);
/* Creates an initialized but empty picture buffer.
** Paramters: (int) resolution: One of the SCI_RESOLUTION macros to determine the internal resolution
**            (int) colordepth: The number of bytes per pixel
** Returns  : (picture_t) An empty picture or NULL if any of the parameters was invalid
*/

void free_picture(picture_t picture);
/* Deallocates all memory associated by the specified picture_t.
** Parameters: (picture_t) picture: The picture to deallocate.
** Returns   : (void)
*/

void draw_pic0(picture_t dest, int flags, int defaultPalette, guint8* data);
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

void clear_picture(picture_t pic, int fgcol);
/* Clears a picture
** Parameters: (picture_t) pic: The picture to clear
**             (int) fgcol: The foreground color to use for screen 0
** Returns   : (void)
** Clearing a picture means that buffers 1-3 are set to 0, while buffer 0
** is set to zero for the first ten pixel rows and to 15 for the remainder.
** This must be called before a drawView0(), unless the picture is intended
** to be drawn ontop of an already existing picture.
*/

int draw_view0(picture_t dest, port_t *port, int x, int y, short priority,
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
**             (byte *) data: The view data
** Returns   : (int) The width, or -1 if loop or cel were invalid.
*/


int
view0_cel_height(int loop, int cel, byte *data);
/* Returns the height of an SCI0 view cell.
** Parameters: (int) loop: The loop to examine
**             (int) cel: The cell
**             (byte *) data: The view data
** Returns   : (int) The height, or -1 if loop or cel were invalid.
*/


int
view0_loop_count(byte *data);
/* Retreives the number of loops from a view
** Parameters: (byte *) data: The view data
** Returns   : (int) The number of loops in that view
*/


int
view0_cel_count(int loop, byte *data);
/* Retreives the number of cels from a loop of a view
** Parameters: (int) loop: The loop in question
**             (byte *) data: The view data
** Returns   : (int) The number of cels in that loop
*/


void draw_box(picture_t dest, short x, short y, short xl, short yl, char color, char priority);
void draw_frame(picture_t dest, short x, short y, short xl, short yl, char color, char priority);
/* Draws a simple box.
** Parameters: (picture_t) dest: The picture_t to draw to.
**             (short) x,y: The coordinates to draw to.
**             (short) xl,yl: The width and height of the box.
**             (char) color: The color to draw with.
**             (char) priority: The priority to fill the box with (it still overwrites anything)
** Returns   : (void)
** The box does not come with any fancy shading. Use drawWindow to do this.
** draw_frame does the same as draw_box, except that it doesn't fill the box.
*/


void
fill_box(picture_t dest, int x, int y, int xl, int yl, int value, int map);
/* Fills a box in the picture on the specified map with the specified value
** Parameters: (picture_t) dest: The picture_t to write to
**             (int) x,y: The upper left point of the box
**             (int) xl,yl: The box's horizontal and vertical extension
**             (int) value: The value to fill it with
**             (int) map: The map on which the value is to be filled in
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

void
get_text_size(char *text, byte *font, int max_allowed_width, int *width, int *height);
/* Determines the width and height of the specified text
** Parameters: (char *) text: The text to check
**             (byte *) font: The font to check it with
**             (int) max_allowed_width: The maximum width to allow for a single text line
**             (int *) width, height: Pointers to the variables which the result is stored in
** Returns   : (void)
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


void draw_window(picture_t dest, port_t *port, char color, char priority,
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


void draw_text0(picture_t dest, port_t *port, int x, int y, char *text, char *font, char color);
void draw_text0_centered(picture_t dest, port_t *port, int x, int y,
			 char *text, char *font, char color);
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


void
text_draw(picture_t dest, port_t *port, char *text, int maxwidth);
/* Draws text according to the parameters specified.
** Parameters: (picture_t) dest: The picture_t to draw to
**             (port_t *) port: Pointer to the port containing color/font/alignment infos
**             (char *) text: The text to draw
**             (int) maxwidth: Maximum pixel width per line; or <0 for "unlimited"
** Returns   : (void)
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


/***************************************************************/
/* Implementation-independant graphics operations for state_ts */
/***************************************************************/

void
graph_clear_box(struct _state *s, int x, int y, int xl, int yl, int color);
/* Fills the specified box with the specified color and updates the screen
** Parameters: (state_t *) s: The state_t to operate on
**             (int) x,y: Upper left hand coordinates of the affected box
**             (int) xl,yl: Size of the affected box
**             (int) color: The color to use
** Returns   : (void)
** This function invokes the callback command for the affected box ONLY.
** The mouse pointer must be re-drawn separately.
** No clipping is performed.
*/

void
graph_update_box(struct _state *s, int x, int y, int xl, int yl);
/* Copies the specified box from bgpic to pic and updates the screen
** Parameters: (state_t *) s: The state_t to operate on
**             (int) x,y: Upper left hand coordinates of the affected box
**             (int) xl,yl: Size of the affected box
** Returns   : (void)
** This function invokes the callback command for the affected box ONLY.
** The mouse pointer must be re-drawn separately
** No clipping is performed.
*/

void
graph_update_port(struct _state *s, port_t *port);
/* Updates a port
** Paramters: (state_t *) s: The state_t to operate on
**            (port_t *) port: The port to update
** Returns  : (void);
** This function uses graph_update_box and then updates the mouse pointer.
*/

int
graph_save_box(struct _state *s, int x, int y, int xl, int yl, int layers);
/* Saves a box in graphics space (s->pic) into memory
** Parameters: (state_t *) s: The state_t to operate on
**             (int) x,y : The upper left hand coordinates of the box to save
**             (int) xl, yl : The width and height of the box
**             (int) layers: Bit 0: Store visual map?
**                           Bit 1: Store priority map?
**                           Bit 2: Store special map?
** Returns   : (int) A "kernel memory" handle to be used with graph_restore_box
*/

void
graph_restore_box(struct _state *s, int handle);
/* Restores a box in graphics space associated with the specified handle and updates the screen.
** Parameters: (state_t *) s: The state_t to operate on
**             (int) handle: The handle of the box to restore
** Returns     (void)
*/

int
view0_backup_background(struct _state *s, int x, int y, int loop, int cel, byte *data);
/* Backs up the background that would be overwritten if the specified picture was drawn
** Parameters: (state_t *) s: The state_t to operate on
**             (int) x,y: The intended position to draw the view to
**             (int) loop, cel: The picture inside the view resource
**             (byte *) data: The view resource
** Returns   : (int): A handle that may be used with graph_restore_box
** This function allocates kernel memory (via graph_save_box()/kalloc()) to store the view. All
** three relevant layers are saved.
*/

void
graph_fill_port(struct _state *s, port_t *port, int color);
/* Fills the port with color, unless color < 0
** Parameters: (state_t *) s: The state to operate on
**             (port_t *) port: The port to use
**             (int) color: The color to draw with
** Returns   : (void)
*/

void
graph_draw_selector_button(struct _state *s, port_t *port, int state,
			   int x, int y, int xl, int yl,
			   char *text, byte *font);
/* Draws a selector button.
** Parameters: (state_t *) s: The state to operate on
**             (port_t *) port: The port to use
**             (int) state: The selector state to use; a combination of the SELECTOR_STATE_* flags
**             (int) x,y: The upper left corner of the selector
**             (int) xl,yl: Height and width of the selector in question
**             (char *) text: The text on the button
**             (byte *) font: Pointer to the font to use
** Returns   : (void)
*/

void
graph_draw_selector_text(struct _state *s, port_t *port, int state,
			 int x, int y, int xl, int yl,
			 char *text, byte *font, int alignment);
/* Draws a text selector.
** Parameters: (state_t *) s: The state to operate on
**             (port_t *) port: The port to use
**             (int) state: The selector state to use; a combination of the SELECTOR_STATE_* flags
**             (int) x,y: The upper left corner of the selector
**             (int) xl,yl: Height and width of the selector in question
**             (char *) text: The text
**             (byte *) font: Pointer to the font to use
**             (int) alignment: One of ALIGN_TEXT_*
** Returns   : (void)
*/


void
graph_draw_selector_edit(struct _state *s, port_t *port, int state,
			 int x, int y, int xl, int yl,
			 char *text, byte *font);
/* Draws an edit frame selector.
** Parameters: (state_t *) s: The state to operate on
**             (port_t *) port: The port to use
**             (int) state: The selector state to use; a combination of the SELECTOR_STATE_* flags
**             (int) x,y: The upper left corner of the selector
**             (int) xl,yl: Height and width of the selector in question
**             (char *) text: The text inside the edit box
**             (byte *) font: Pointer to the font to use
** Returns   : (void)
*/


void
graph_draw_selector_icon(struct _state *s, port_t *port, int state,
			 int x, int y, int xl, int yl,
			 byte *data, int loop, int cel);
/* Draws an iconic (bitmapped) selector..
** Parameters: (state_t *) s: The state to operate on
**             (port_t *) port: The port to use
**             (int) state: The selector state to use; a combination of the SELECTOR_STATE_* flags
**             (int) x,y: The upper left corner of the selector
**             (int) xl,yl: Height and width of the selector in question
**             (byte *) data: The view resource from which the data should be taken
**             (int) loop, cel: Loop and cell inside the view resource
** Returns   : (void)
*/


void
graph_draw_selector_control(struct _state *s, port_t *port, int state,
			    int x, int y, int xl, int yl);
/* Draws a control selector (a "scollbox").
** Parameters: (state_t *) s: The state to operate on
**             (port_t *) port: The port to use
**             (int) state: The selector state to use; a combination of the SELECTOR_STATE_* flags
**             (int) x,y: The upper left corner of the selector
**             (int) xl,yl: Height and width of the selector in question
** Returns   : (void)
*/


word
graph_on_control(struct _state *s, int x, int y, int xl, int yl, int map);
/* Checks the picture maps for certain pixels and returns them in a bitmask
** Parameters: (state_t *) s: The state_t to operate on
**             (int) x,y: The upper left coordinates of the box to check
**             (int) xl, yl: Horizontal and vertical extension of the box
**             (int) map: The maps to check
** Returns   : (word): A bitmask where each bit is set if and only if the color value equivalent to
**                     the bit's position was found in the specified rectangle on the specified maps.
** This function performs clipping.
** Example: using it on an area of a control map where only color values 15,2  and 0 are used would
** result in 0x8005 being returned (i.e. bits 15, 2, and 0 would be set).
*/

#endif
