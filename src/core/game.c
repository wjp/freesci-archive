/***************************************************************************
 game.c Copyright (C) 1999 Christoph Reichenbach


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

#include <script.h>
#include <vm.h>
#include <engine.h>
#include <versions.h>
#include <kernel.h>
#include <kdebug.h>

#ifndef _WIN32
#include <sys/resource.h>
#endif


int
game_init(state_t *s)
{
  heap_ptr stack_handle = heap_allocate(s->_heap, VM_STACK_SIZE);
  heap_ptr parser_handle = heap_allocate(s->_heap, PARSE_HEAP_SIZE);
  heap_ptr script0;
  heap_ptr game_obj; /* Address of the game object */
  heap_ptr game_init; /* Address of the init() method */
  heap_ptr functarea;
  resource_t *resource;
  int i, font_nr;

  s->synonyms = NULL;
  s->synonyms_nr = 0; /* No synonyms */

  s->menubar = menubar_new(); /* Create menu bar */

  /* Initialize script table */
  for (i = 0; i < 1000; i++)
    s->scripttable[i].heappos = 0;
  /* Initialize hunk data */
  for (i = 0; i < MAX_HUNK_BLOCKS; i++)
    s->hunk[i].size = 0;
  /* Initialize ports */
  memset(s->ports, 0, sizeof(port_t *) * MAX_PORTS);
  /* Initialize clone list */
  memset(&(s->clone_list), 0, sizeof(heap_ptr) * SCRIPT_MAX_CLONES);

  if (!stack_handle) {
    sciprintf("game_init(): Insufficient heap space for stack\n");
    return 1;
  }

  if (!parser_handle) {
    sciprintf("script_init(): Insufficient heap space for parser word error block\n");
    return 1;
  }

  if (!(script0 = script_instantiate(s, 0))) {
    sciprintf("script_init(): Could not instantiate script 0\n");
    return 1;
  }

  s->successor = NULL; /* No successor */
  s->status_bar_text = NULL; /* Status bar is blank */

  fprintf(stderr," Script 0 at %04x\n", script0);


  s->restarting_flags = SCI_GAME_IS_NOT_RESTARTING;

  s->stack_base = stack_handle + 2;
  s->parser_base = parser_handle + 2;
  s->global_vars = s->scripttable[0].localvar_offset;
  /* Global variables are script 0's local variables */

  s->file_handles_nr = 5;
  s->file_handles = calloc(sizeof(FILE *), s->file_handles_nr);
  /* Allocate memory for file handles */

  g_get_current_time(&(s->game_start_time)); /* Get start time */
  memcpy(&(s->last_wait_time), &(s->game_start_time), sizeof(GTimeVal));
  /* Use start time as last_wait_time */

  s->mouse_pointer = NULL; /* No mouse pointer */
  s->mouse_pointer_nr = -1; /* No mouse pointer resource */
  s->pointer_x = (320 / 2); /* With centered x coordinate */
  s->pointer_y = 150; /* And an y coordinate somewhere down the screen */
  s->last_pointer_x = 0;
  s->last_pointer_y = 0;
  s->last_pointer_size_x = 0;
  s->last_pointer_size_y = 0; /* No previous pointer */

  s->pic = alloc_empty_picture(SCI_RESOLUTION_320X200, SCI_COLORDEPTH_8BPP);
  s->pic_not_valid = 1; /* Picture is invalid */
  s->pic_is_new = 0;
  s->pic_visible_map = 0; /* Other values only make sense for debugging */
  s->animation_delay = 500; /* Used in kAnimate for pic openings */

  s->pic_views_nr = s->dyn_views_nr = 0;
  s->pic_views = 0; s->dyn_views = 0; /* No PicViews, no DynViews */

  memset(s->ports, sizeof(port_t) * MAX_PORTS, 0); /* Set to no ports */

  s->wm_port.ymin = 10; s->wm_port.ymax = 199;
  s->wm_port.xmin = 0; s->wm_port.xmax = 319;
  s->wm_port.priority = 11;
  s->ports[0] = &(s->wm_port); /* Window Manager port */

  s->titlebar_port.ymin = 0; s->titlebar_port.ymax = 9;
  s->titlebar_port.xmin = 0; s->titlebar_port.xmax = 319;
  s->ports[1] = &(s->titlebar_port);

  s->picture_port.ymin = 10; s->picture_port.ymax = 199;
  s->picture_port.xmin = 0; s->picture_port.xmax = 319;
  s->ports[2] = &(s->picture_port); /* Background picture port */

  s->view_port = 0; /* Currently using the window manager port */

  s->priority_first = 42; /* Priority zone 0 ends here */
  s->priority_last = 200; /* The highest priority zone (15) starts here */

  s->debug_mode = 0x0; /* Disable all debugging */
  s->onscreen_console = 0; /* No onscreen console unless explicitly requested */

  s->bp_list = NULL; /* No breakpoints defined */
  s->have_bp = 0;

  srand(time(NULL)); /* Initialize random number generator */

  font_nr = -1;
  do {
    resource = findResource(sci_font, ++font_nr);
  } while ((!resource) && (font_nr < 999));

  if (!resource) {
    sciprintf("No text font was found.\n");
    return 1;
  }
  for (i = 0; i < 3; i++) {
      s->ports[i]->font = resource->data; /* let all ports default to the 'system' font */
      s->ports[i]->gray_text = 0;
      s->ports[i]->font_nr = font_nr;
      s->ports[i]->color = 0;
      s->ports[i]->bgcolor = -1; /* All ports should be transparent */
  }

  memset(s->hunk, sizeof(s->hunk), 0); /* Sets hunk to be unused */
  memset(s->clone_list, sizeof(s->clone_list), 0); /* No clones */

  s->save_dir = heap_allocate(s->_heap, MAX_HOMEDIR_SIZE + strlen(FREESCI_GAMEDIR)
			      + MAX_GAMEDIR_SIZE + 4); /* +4 for the three slashes and trailing \0 */

  game_obj = script0 + GET_HEAP(s->scripttable[0].export_table_offset + 2);
  /* The first entry in the export table of script 0 points to the game object */

  if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
        game_obj -= 2; /* Adjust for alternative header */

  if (GET_HEAP(game_obj + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    sciprintf("script_init(): Game object is not at 0x%x\n", game_obj);
    return 1;
  }

  s->game_name = s->heap + GET_HEAP(game_obj + SCRIPT_NAME_OFFSET);

  sciprintf(" Game designation is \"%s\"\n", s->game_name);

  if (strlen(s->game_name) >= MAX_GAMEDIR_SIZE) {

    s->game_name[MAX_GAMEDIR_SIZE - 1] = 0; /* Fix length with brute force */
    sciprintf(" Designation too long; was truncated to \"%s\"\n", s->game_name);

  }

  s->game_obj = game_obj;
  s->stack_handle = stack_handle;

  return 0;
}
  
int
game_run(state_t **_s)
{
  state_t *successor = NULL;
  state_t *s = *_s;
  int game_is_finished = 0;

  sciprintf(" Calling %s::play()\n", s->game_name);
  putInt16(s->heap + s->stack_base, s->selector_map.play); /* Call the play selector... */
  putInt16(s->heap + s->stack_base + 2, 0);                    /* ... with 0 arguments. */

  /* Now: Register the first element on the execution stack- */
  send_selector(s, s->game_obj, s->game_obj, s->stack_base + 2, 4, 0, s->stack_base);
  /* and ENGAGE! */

  do {
    save_ff(s->_heap); /* Save heap state */
    run_vm(s, (successor)? 1 : 0);

    if (s->restarting_flags & SCI_GAME_IS_RESTARTING_NOW) { /* Restart was requested? */

      g_free(s->execution_stack);
      s->execution_stack = NULL;
      s->execution_stack_pos = -1;
      s->execution_stack_pos_changed = 0;
      restore_ff(s->_heap); /* Restore old heap state */

      game_exit(s);
      game_init(s);

      sciprintf(" Restarting game\n");
      /*      putInt16(s->heap + s->stack_base, s->selector_map.replay); /* Call the replay selector */
      putInt16(s->heap + s->stack_base + 2, 0);
      send_selector(s, s->game_obj, s->game_obj, s->stack_base + 2, 4, 0, s->stack_base);

      script_abort_flag = 0;

      s->restarting_flags = SCI_GAME_WAS_RESTARTED;

    } else
      
      if (successor = s->successor) {
	script_abort_flag = 0;
	free(s);
	*_s = s = successor;
      } else

	game_is_finished = 1;

  } while (!game_is_finished);

  sciprintf(" Game::play() finished.\n");
  return 0;
}

int
game_exit(state_t *s)
{
  int i;
  breakpoint_t *bp, *bp_next;

  if (s->synonyms_nr) {
    free(s->synonyms);
    s->synonyms = NULL;
    s->synonyms_nr = 0;
  }

  menubar_free(s->menubar);

  if (s->status_bar_text)
    g_free(s->status_bar_text);

  /* Close all opened file handles */
  for (i = 1; i < s->file_handles_nr; i++)
    if (s->file_handles[i])
      fclose(s->file_handles[i]);

  g_free(s->file_handles);

  sciprintf("Freeing graphics data...\n");
  free_picture(s->pic);

  sciprintf("Freeing miscellaneous data...\n");
  /* Free breakpoint list */
  bp = s->bp_list;
  while (bp)
  {
    bp_next = bp->next;
    if (bp->type == BREAK_SELECTOR) g_free (bp->data.name);
    g_free (bp);
    bp = bp_next;
  }
  s->bp_list = NULL;


  sciprintf("Freeing miscellaneous data...\n");


  heap_free(s->_heap, s->stack_handle);
  heap_free(s->_heap, s->save_dir);
  heap_free(s->_heap, s->parser_base - 2);

  return 0;
}

