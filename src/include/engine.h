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

#define MAX_HOMEDIR_SIZE 255

#define FREESCI_GAMEDIR ".freesci"

#define MAX_GAMEDIR_SIZE 32 /* Used for subdirectory inside of "~/.freesci/" */

#define MAX_HUNK_BLOCKS 64 /* Used for SCI "far memory"; only used for sci_memory in FreeSCI */


typedef struct
{
    int size;
    char *data;
} hunk_block_t; /* Used to store dynamically allocated "far" memory */

typedef struct _state
{
  char *game_name; /* Designation of the primary object (which inherits from Game) */

  /* Non-VM information */

  int restarting_flag; /* Flag used for restarting */
  int have_mouse_flag; /* Do we have a hardware pointing device? */

  int pic_not_valid; /* Is 0 if the background picture is "valid" */

  long game_time; /* Counted at 60 ticks per second, reset during start time */

  heap_ptr save_dir; /* Pointer to the allocated space for the save directory */

  int pounter_x, pointer_y; /* Mouse pointer coordinates */
  byte* mouse_pointer; /* Pointer to the current mouse pointer or NULL if none has been selected */

  port_t *view; /* The currently active view */

  port_t titlebar_port; /* Title bar viewport (0,0,9,319) */
  port_t wm_port; /* window manager viewport and designated &heap[0] view (10,0,199,319) */
  port_t picture_port; /* The background picture viewport (10,0,199,319) */

  picture_t bgpic; /* The background picture */
  picture_t pic; /* The foreground picture */

  hunk_block_t hunk[MAX_HUNK_BLOCKS]; /* Hunk memory */

  /* VM Information */

  heap_t *_heap; /* The heap structure */
  byte *heap; /* The actual heap data (equal to _heap->start) */
  gint16 acc; /* Accumulator */
  gint16 prev; /* previous comparison result */

  heap_ptr stack_base; /* The base position of the stack; used for debugging */
  heap_ptr global_vars; /* script 000 selectors */

  int classtable_size; /* Number of classes in the table- for debugging */
  class_t *classtable; /* Table of all classes */
  script_t scripttable[1000]; /* Table of all scripts */

  int selector_names_nr; /* Number of selector names */
  char **selector_names; /* Zero-terminated selector name list */
  int kernel_names_nr; /* Number of kernel function names */
  char **kernel_names; /* List of kernel names */
  kfunct **kfunct_table; /* Table of kernel functions */

  opcode *opcodes;

  selector_map_t selector_map; /* Shortcut list for important selectors */

} state_t;

#endif /* !_SCI_ENGINE_H */
