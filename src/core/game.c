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

#include <engine.h>
#include <versions.h>
#include <kernel.h>
#include <kdebug.h>

#ifndef _WIN32
#include <sys/resource.h>
#endif


/* Structures and data from vm.c: */
extern calls_struct_t *send_calls;
extern int send_calls_allocated;
extern int bp_flag;



static int
_init_vocabulary(state_t *s) /* initialize vocabulary and related resources */
{
  sciprintf("Initializing vocabulary\n");

  s->parser_valid = 0; /* Invalidate parser */
  s->parser_event = 0; /* Invalidate parser event */
  s->parser_lastmatch_word = SAID_NO_MATCH;

  if (s->parser_words = vocab_get_words(&(s->parser_words_nr))) {
    s->parser_suffices = vocab_get_suffices(&(s->parser_suffices_nr));
    s->parser_branches = vocab_get_branches(&(s->parser_branches_nr));

  } else
    sciprintf("Assuming that this game does not use a parser.\n");


  s->opcodes = vocabulary_get_opcodes();

  if (!(s->selector_names = vocabulary_get_snames(NULL, s->version))) {
    sciprintf("_init_vocabulary(): Could not retreive selector names (vocab.997)!\n");
    return 1;
  }

  for (s->selector_names_nr = 0; s->selector_names[s->selector_names_nr]; s->selector_names_nr++);
  /* Counts the number of selector names */

  script_map_selectors(s, &(s->selector_map));
  /* Maps a few special selectors for later use */

  return 0;
}


static void
_free_vocabulary(state_t *s)
{
  sciprintf("Freeing vocabulary\n");

  if (s->parser_words) {
    vocab_free_words(s->parser_words, s->parser_words_nr);
    vocab_free_suffices(s->parser_suffices, s->parser_suffices_nr);
    vocab_free_branches(s->parser_branches);
  }

  vocabulary_free_snames(s->selector_names);
  vocabulary_free_knames(s->kernel_names);
  g_free(s->opcodes);

  s->selector_names = NULL;
  s->kernel_names = NULL;
  s->opcodes = NULL;
}


static int
_init_graphics_input(state_t *s)
{
  resource_t *resource;
  int i, font_nr;
  sciprintf("Initializing graphics\n");

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
  s->pic_views_nr = s->dyn_views_nr = 0;
  s->pic_views = 0; s->dyn_views = 0; /* No PicViews, no DynViews */

  memset(s->ports, sizeof(s->ports), 0); /* Set to no ports */

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

  return 0;
}

static void
_free_graphics_input(state_t *s)
{
  int i;

  sciprintf("Freeing graphics\n");

  for (i = 3; i < MAX_PORTS; i++) /* Ports 0,1,2 are fixed */
    if (s->ports[i]) {
      g_free(s->ports[i]);
      s->ports[i] = 0;
    }

  if (s->pic_views_nr)
  {
    g_free(s->pic_views);
    s->pic_views = NULL;
  }

  if (s->dyn_views_nr)
  {
    g_free(s->dyn_views);
    s->dyn_views = NULL;
  }

  free_picture(s->pic);
}


/* Architectural stuff: Init/Unintialize engine */

int
script_init_engine(state_t *s, sci_version_t version)
{
  resource_t *vocab996 = findResource(sci_vocab, 996);
  int i;
  int scriptnr;
  int seeker;
  int classnr;
  int size;
  resource_t *script;
  int magic_offset; /* For strange scripts in older SCI versions */

  s->max_version = SCI_VERSION(9,999,999); /* :-) */
  s->min_version = 0; /* Set no real limits */
  s->version = SCI_VERSION_DEFAULT_SCI0;

  script_detect_early_versions(s);

  s->_heap = heap_new();
  s->heap = s->_heap->start;
  s->acc = s->amp_rest = s->prev = 0;

  s->execution_stack = NULL;    /* Start without any execution stack */
  s->execution_stack_base = -1; /* No vm is running yet */
  s->execution_stack_pos = -1;   /* Start at execution stack position 0 */

  s->global_vars = 0; /* Set during launch time */

  if (!version) {
    s->version_lock_flag = 0;
  } else {
    s->version = version;
    s->version_lock_flag = 1; /* Lock version */
  }

  if (!vocab996)
    s->classtable_size = 20;
  else
    s->classtable_size = vocab996->length >> 2;

  s->classtable = g_new0(class_t, s->classtable_size);

  for (scriptnr = 0; scriptnr < 1000; scriptnr++) {
    int objtype;
    resource_t *script = findResource(sci_script, scriptnr);

    if (script) {

      size = getInt16(script->data);
      if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
        magic_offset = seeker = 2;
      else
	magic_offset = seeker = 0;

      do {

	while ((objtype = getInt16(script->data + seeker)) && (objtype != sci_obj_class)
	       && (seeker < script->length))
	  seeker += getInt16(script->data + seeker + 2);

	if (objtype) { /* implies sci_obj_class */

	  seeker -= SCRIPT_OBJECT_MAGIC_OFFSET; /* Adjust position; script home is base +8 bytes */

	  classnr = getInt16(script->data + seeker + 4 + SCRIPT_SPECIES_OFFSET);
	  if (classnr >= s->classtable_size) {

	    if (classnr >= SCRIPT_MAX_CLASSTABLE_SIZE) {
	      fprintf(stderr,"Invalid class number 0x%x in script.%d(0x%x), offset %04x\n",
		      classnr, scriptnr, scriptnr, seeker);
	      return 1;
	    }

	    s->classtable = g_realloc(s->classtable, sizeof(class_t) * (classnr + 1));
	    memset(&(s->classtable[s->classtable_size]), 0,
		   sizeof(class_t) * (1 + classnr - s->classtable_size)); /* Clear after resize */

	    s->classtable_size = classnr + 1; /* Adjust maximum number of entries */
	  }

	  s->classtable[classnr].class_offset = seeker + 4 - magic_offset;
	  s->classtable[classnr].script = scriptnr;
	  s->classtable[classnr].scriptposp = &(s->scripttable[scriptnr].heappos);

	  seeker += SCRIPT_OBJECT_MAGIC_OFFSET; /* Re-adjust position */

	  seeker += getInt16(script->data + seeker + 2); /* Move to next */
	}

      } while (objtype != sci_obj_terminator);

    }
  }

  s->kernel_names = vocabulary_get_knames(&s->kernel_names_nr);
  script_map_kernel(s);
  /* Maps the kernel functions */

  if (_init_vocabulary(s)) return 1;

  s->restarting_flags = SCI_GAME_IS_NOT_RESTARTING;

  for (i = 0; i < 1000; i++)
    s->scripttable[i].heappos = 0; /* Mark all scripts as 'not installed' */
  if (!script_instantiate(s, 0)) {
    sciprintf("script_init_engine(): Could not instantiate script 0\n");
    return 1;
  }

  save_ff(s->_heap); /* Save heap state */

  s->bp_list = NULL; /* No breakpoints defined */
  s->have_bp = 0;

  s->file_handles_nr = 5;
  s->file_handles = g_new0(FILE *, s->file_handles_nr);
  /* Allocate memory for file handles */

  return 0;
}



void
script_free_engine(state_t *s)
{
  int i;
  breakpoint_t *bp, *bp_next;

  /* FIXME: file handles will NOT be closed under DOS. DJGPP generates an
     exception fault whenever you try to close a never-opened file */

  sciprintf("Freeing state-dependant data\n");
  for (i = 0; i < MAX_HUNK_BLOCKS; i++)
      if (s->hunk[i].size) {
	  g_free(s->hunk[i].data);
	  s->hunk[i].size = 0;
      }

  /* Close all opened file handles */
  for (i = 1; i < s->file_handles_nr; i++)
    if (s->file_handles[i])
#ifndef _DOS
      fclose(s->file_handles[i]);
#endif

  g_free(s->file_handles);

  heap_del(s->_heap);

  g_free(s->kfunct_table);
  s->kfunct_table = NULL;

  g_free(s->classtable);

  _free_vocabulary(s);

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

}


/*************************************************************/
/* Game instance stuff: Init/Unitialize state-dependant data */
/*************************************************************/


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

  script0 = s->scripttable[0].heappos; /* Get script 0 position */

  if (!script0) {
    sciprintf("Game initialization requested, but script.000 not loaded\n");
    return 1;
  }

  s->synonyms = NULL;
  s->synonyms_nr = 0; /* No synonyms */

  /* Initialize hunk data */
  for (i = 0; i < MAX_HUNK_BLOCKS; i++)
    s->hunk[i].size = 0;
  /* Initialize ports */
  memset(s->ports, 0, sizeof(port_t *) * MAX_PORTS);
  /* Initialize clone list */
  memset(&(s->clone_list), 0, sizeof(heap_ptr) * SCRIPT_MAX_CLONES);
  /* Initialize send_calls buffer */

  if (!send_calls_allocated)
    send_calls = g_new(calls_struct_t, send_calls_allocated = 16);

  if (!stack_handle) {
    sciprintf("game_init(): Insufficient heap space for stack\n");
    return 1;
  }

  if (!parser_handle) {
    sciprintf("game_init(): Insufficient heap space for parser word error block\n");
    return 1;
  }

  s->successor = NULL; /* No successor */
  s->status_bar_text = NULL; /* Status bar is blank */

  fprintf(stderr," Script 0 at %04x\n", script0);

  s->stack_base = stack_handle + 2;
  s->parser_base = parser_handle + 2;
  s->global_vars = s->scripttable[0].localvar_offset;
  /* Global variables are script 0's local variables */

  g_get_current_time(&(s->game_start_time)); /* Get start time */
  memcpy(&(s->last_wait_time), &(s->game_start_time), sizeof(GTimeVal));
  /* Use start time as last_wait_time */

  if (_init_graphics_input(s))
    return 1;

  s->animation_delay = 500; /* Used in kAnimate for pic openings */

  s->debug_mode = 0x0; /* Disable all debugging */
  s->onscreen_console = 0; /* No onscreen console unless explicitly requested */

  srand(time(NULL)); /* Initialize random number generator */

  memset(s->hunk, sizeof(s->hunk), 0); /* Sets hunk to be unused */
  memset(s->clone_list, sizeof(s->clone_list), 0); /* No clones */

  s->save_dir = heap_allocate(s->_heap, MAX_HOMEDIR_SIZE + strlen(FREESCI_GAMEDIR)
			      + MAX_GAMEDIR_SIZE + 4); /* +4 for the three slashes and trailing \0 */

  game_obj = script0 + GET_HEAP(s->scripttable[0].export_table_offset + 2);
  /* The first entry in the export table of script 0 points to the game object */

  if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
        game_obj -= 2; /* Adjust for alternative header */

  if (GET_HEAP(game_obj + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    sciprintf("game_init(): Game object is not at 0x%x\n", game_obj);
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

  /* Mark parse tree as unused */
  s->parser_nodes[0].type = PARSE_TREE_NODE_LEAF;
  s->parser_nodes[0].content.value = 0;

  s->menubar = menubar_new(); /* Create menu bar */

  return 0;
}


  
int
game_exit(state_t *s)
{
  int i;

  if (s->execution_stack)
    g_free(s->execution_stack);

  if (s->synonyms_nr) {
    free(s->synonyms);
    s->synonyms = NULL;
    s->synonyms_nr = 0;
  }

  sciprintf("Freeing miscellaneous data...\n");

  /* HACK WARNING: This frees all scripts that were allocated prior to the stack, i.e. those
  ** that won't survive a stack restauration.
  */
  for (i = 1; i < 1000; i++)
    if (s->scripttable[i].heappos > s->stack_handle)
      s->scripttable[i].heappos = 0;

  heap_free(s->_heap, s->stack_handle);
  heap_free(s->_heap, s->save_dir);
  heap_free(s->_heap, s->parser_base - 2);
  restore_ff(s->_heap); /* Restore former heap state */

  if (send_calls_allocated) {
    g_free(send_calls);
    send_calls_allocated = 0;
  }

  for (i = 1; i < 1000; i++)
    if (s->scripttable[i].heappos > s->_heap->first_free)
      s->scripttable[i].heappos = 0; /* Mark all non-high scripts as 'not installed', except for 0 */

  menubar_free(s->menubar);

  return 0;
}

