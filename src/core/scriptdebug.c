/***************************************************************************
 scriptdebug.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Script debugger functionality */

#include <resource.h>
#include <vm.h>

void
script_debug(state *s, heap_ptr *pc, heap_ptr *sp, heap_ptr *pp, heap_ptr *objp, int allowreturn)
{
  fprintf(stderr,"script_debug(): stub\n");

  if (allowreturn)
    return;
  else 
    exit(-1);
}
