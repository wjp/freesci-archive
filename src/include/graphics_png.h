/***************************************************************************
 graphics_png.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

***************************************************************************/
/* Provides facilities for writing pictures and seperate views to .png files */


#ifndef _SCI_GRAPHICS_PNG_H_
#define _SCI_GRAPHICS_PNG_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_LIBPNG

#include <resource.h>
#include <graphics.h>
#include <png.h>


int
write_pic_png(char *filename, guint8 *pic);
/* Stores a picture_t in a png file
** Parameters: (filename) The name of the file to create
**             (pic) A pointer to the *picture_t to store
** Returns   : (int) 0 on success, != 0 otherwise
** Please note that pic must not point to a picture_t, it must point to a
** *picture_t.
** E.g. to store the foreground picture, you could use
** write_pic_png("filename.png", picture[0]).
*/

#endif /* HAVE_LIBPNG */
#endif /* !_SCI_GRAPHICS_PNG_H_ */
