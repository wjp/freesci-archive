/***************************************************************************
 engine.h Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
#ifndef _SCI_ENGINE_H
#define _SCI_ENGINE_H

#include <config.h>
#include <graphics.h>
#include <resource.h>
#include <script.h>
#include <vocabulary.h>
#include <sound.h>
#include <uinput.h>
#include <console.h>
#include <vm.h>




typedef struct {

  int pointer_nr; /* Mouse pointer number */
  int pounter_x, pointer_y; /* Mouse pointer coordinates */

  port_t *view; /* The currently active view */

  port_t titlebar_port; /* Title bar viewport (0,0,9,319) */
  port_t wm_port; /* window manager viewport and designated &heap[0] view (10,0,199,319) */
  port_t picture_port; /* The background picture viewport (10,0,199,319) */

  picture_t bgpic; /* The background picture */
  picture_t pic; /* The foreground picture */

  state vm_state; /* Heap and VM status */
  heap* heap; /* Pointer to the heap stored in vm_state, for ease of use */

} gamestate_t;


#endif /* !_SCI_ENGINE_H */
