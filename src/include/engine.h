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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <graphics.h>
#include <resource.h>
#include <script.h>
#include <vocabulary.h>
#include <sound.h>
#include <uinput.h>
#include <console.h>
#include <vm.h>
#include <menubar.h>
#include <time.h>
#include <versions.h>
#ifdef HAVE_LIBGGI
#include <ggi/ggi.h>
#endif

#define MAX_HOMEDIR_SIZE 255

#define FREESCI_GAMEDIR ".freesci"
#define FREESCI_CONFFILE "config"
#define FREESCI_CONFFILE_DOS "freesci.cfg"

#define MAX_GAMEDIR_SIZE 32 /* Used for subdirectory inside of "~/.freesci/" */

#define MAX_HUNK_BLOCKS 256 /* Used for SCI "far memory"; only used for sci_memory in FreeSCI */

#define MAX_PORTS 16 /* Maximum number of view ports */


typedef struct
{
    int size;
    char *data;
} hunk_block_t; /* Used to store dynamically allocated "far" memory */

typedef struct _state
{
  char *game_name; /* Designation of the primary object (which inherits from Game) */

  /* Non-VM information */

  gfx_driver_t *gfx_driver; /* Graphics driver */

  union {
#ifdef HAVE_LIBGGI
    ggi_visual_t ggi_visual; /* for libggi */
#endif    
    int _dummy;
  } graphics;

  sfx_driver_t *sfx_driver; /* Sound driver */

  int sound_pipe_in[2];  /* Sound command pipeline: Engine => Sound server */
  int sound_pipe_out[2]; /* Sound return value pipeline: Engine <= Sound server */
  int sound_pipe_events[2]; /* Sound events returned by the server: Engine <= Sound server */
  int sound_pipe_debug[2]; /* Text pipeline for debug data Engine: <= Sound server */

  byte restarting_flag; /* Flag used for restarting */
  byte have_mouse_flag; /* Do we have a hardware pointing device? */

  byte pic_not_valid; /* Is 0 if the background picture is "valid" */
  byte pic_is_new;    /* Set to 1 if a picture has just been loaded */

  int debug_mode; /* Contains flags for the various debug modes */

  long game_time; /* Counted at 60 ticks per second, reset during start time */

  heap_ptr save_dir; /* Pointer to the allocated space for the save directory */

  heap_ptr sound_object; /* Some sort of object for sound management */

  int pointer_x, pointer_y; /* Mouse pointer coordinates */
  int last_pointer_x, last_pointer_y; /* Mouse pointer coordinates as last drawn */
  int last_pointer_size_x, last_pointer_size_y; /* Mouse pointer size as last used */
  mouse_pointer_t *mouse_pointer; /* The current mouse pointer, or NULL if disabled */

  int buckybits; /* Bitmask of all pressed bucky-keys */

  int view_port; /* The currently active view port */
  port_t *ports[MAX_PORTS]; /* A list of all available ports */

  port_t titlebar_port; /* Title bar viewport (0,0,9,319) */
  port_t wm_port; /* window manager viewport and designated &heap[0] view (10,0,199,319) */
  port_t picture_port; /* The background picture viewport (10,0,199,319) */

  picture_t pic; /* The graphics storage thing */
  int pic_visible_map; /* The number of the map to display in update commands */
  int pic_animate; /* The animation used by Animate() to display the picture */

  int pic_views_nr, dyn_views_nr; /* Number of entries in the pic_views and dyn_views lists */
  view_object_t *pic_views, *dyn_views; /* Pointers to pic and dynamic view lists */

  int animation_delay; /* A delay factor for pic opening animations. Defaults to 500. */

  hunk_block_t hunk[MAX_HUNK_BLOCKS]; /* Hunk memory */

  menubar_t *menubar; /* The menu bar */

  int priority_first; /* The line where priority zone 0 ends */
  int priority_last; /* The line where the highest priority zone starts */

  GTimeVal game_start_time; /* The time at which the interpreter was started */
  GTimeVal last_wait_time; /* The last time the game invoked Wait() */

  byte version_lock_flag; /* Set to 1 to disable any autodetection mechanisms */
  sci_version_t version; /* The approximated patchlevel of the version to emulate */

  int file_handles_nr; /* maximum numer of allowed file handles */
  FILE **file_handles; /* Array of file handles. Dynamically increased if required. */

  /* VM Information */

  heap_t *_heap; /* The heap structure */
  byte *heap; /* The actual heap data (equal to _heap->start) */
  gint16 acc; /* Accumulator */
  gint16 prev; /* previous comparison result */

  heap_ptr stack_base;   /* The base position of the stack; used for debugging */
  heap_ptr stack_handle; /* The stack's heap handle */
  heap_ptr global_vars;  /* script 000 selectors */

  heap_ptr game_obj; /* Pointer to the game object */

  int classtable_size; /* Number of classes in the table- for debugging */
  class_t *classtable; /* Table of all classes */
  script_t scripttable[1000]; /* Table of all scripts */

  heap_ptr clone_list[SCRIPT_MAX_CLONES];

  int selector_names_nr; /* Number of selector names */
  char **selector_names; /* Zero-terminated selector name list */
  int kernel_names_nr; /* Number of kernel function names */
  char **kernel_names; /* List of kernel names */
  kfunct **kfunct_table; /* Table of kernel functions */

  opcode *opcodes;

  selector_map_t selector_map; /* Shortcut list for important selectors */

} state_t;

#define STATE_T_DEFINED

#endif /* !_SCI_ENGINE_H */
