/***************************************************************************
 graphics_ggi.h Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

   990402 - created (CJR)

***************************************************************************/
/* Graphics for GGI */

#ifndef _SCI_GRAPHICS_GGI_H_
#define _SCI_GRAPHICS_GGI_H_

#include <config.h>
#ifdef HAVE_LIBGGI

#include "graphics.h"
#include "ggi/ggi.h"

#define SCI_VISUAL_NORMAL 0
#define SCI_VISUAL_DOUBLE 1

extern ggi_visual_t sci_default_visual;
/* The default visual. This isn't used by the library itself. */
extern int sci_default_visual_size;
/* The default visual's size. Not used by the library itself */


ggi_visual_t
openVisual();
/* Creates a 320x200 GGI visual.
** Parameters: (void)
** Returns   : (ggi_visual_t) The visual, or NULL if opening failed.
** This will also initialize the internal EGA color lookup table, if this has
** not already been done. Beware: If you are displaying to several visuals
** with a different color layout, this will only work properly for the first
** one.
** It will also install an input handler for the visual, if no input handler
** has been added yet.
*/

ggi_visual_t
openDoubleVisual();
/* Creates a 640x400 GGI visual.
** Parameters: (void)
** Returns   : (ggi_visual_t) The visual, or NULL if opening failed.
** This will also initialize the internal EGA color lookup table, if this has
** not already been done. Beware: If you are displaying to several visuals
** with a different color layout, this will only work properly for the first
** one.
** It will also install an input handler for the visual, if no input handler
** has been added yet.
*/

void
closeVisual(ggi_visual_t visual);
/* Closes a GGI visual.
** Parameters: (ggi_visual_t) visual: The visual to close.
** Returns   : (void)
*/

void
displayPicture(ggi_visual_t visual, picture_t pic, short layer);
/* Displays a picture on a GGI visual.
** Parameters: (ggi_visual_t) visual: The visual to draw to.
**             (picture_t) pic: The picture to draw.
**             (short) layer: The layer of pic to draw.
** Returns   : (void)
*/


void
displayPictureDouble(ggi_visual_t visual, picture_t pic, short layer);
/* deprecated */
/* Displays a double-sized picture on a GGI visual.
** Parameters: (ggi_visual_t) visual: The visual to draw to.
**             (picture_t) pic: The picture to draw.
**             (short) layer: The layer or pic to draw.
** Returns   : (void)
** This should obviously only be used with double-sized GGI visuals.
*/




int
initInputGII(void);
/* Installs an input handler for stdin over libgii
** Parameters: (void)
** Returns   : (int) 0 on success, 1 on failure
** This command won't work if used after a visual has been opened, since
** only one input handler may be used.
** The handler installed by this function does not create mouse events.
*/


#endif /* HAVE_LIBGGI */
#endif /* !_SCI_GRAPHICS_GGI_H */
