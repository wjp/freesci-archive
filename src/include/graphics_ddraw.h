/***************************************************************************
 graphics_sdl.h Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

   990819 - created [DJ]

***************************************************************************/
/* Graphics for DirectDraw */

#ifndef _SCI_GRAPHICS_DDRAW_H_
#define _SCI_GRAPHICS_DDRAW_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_DDRAW

#include "graphics.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddraw.h>
#include <mmsystem.h>

extern gfx_driver_t gfx_driver_ddraw;

int
ddraw_init(struct _state *s, struct _picture *pic);

void
ddraw_shutdown(struct _state *s);

void
ddraw_redraw (struct _state *s, int command, int x, int y, int xl, int yl);

void
ddraw_configure (char *key, char *value);


#endif /* HAVE_DDRAW */
#endif /* !_SCI_GRAPHICS_DDRAW_H */
