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

/* Attempt to guess version of Platform SDK which may not work */
#ifdef _WIN32
#	pragma message("IMPORTANT: You must be using a recent Platform and DirectX SDK")
#	pragma message("for this build to be successful. Download MS SDKs from:")
#	pragma message("www.microsoft.com/msdownload/platformsdk/sdkupdate")
#	include <ntverp.h>
#	if VER_PRODUCTBUILD < 2601
#		error Please download and install more recent Platform and DirectX SDKs from http://www.microsoft.com/msdownload/platformsdk/sdkupdate
#	endif
#endif

#include <sciresource.h>
#include <engine.h>
#include <versions.h>
#include <kernel.h>
#include <kdebug.h>

#if !defined (_WIN32) && !defined (__BEOS__)
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

	s->parser_lastmatch_word = SAID_NO_MATCH;

	if ((s->parser_words = vocab_get_words(s->resmgr, &(s->parser_words_nr)))) {
		s->parser_suffices = vocab_get_suffices(s->resmgr, &(s->parser_suffices_nr));
		s->parser_branches = vocab_get_branches(s->resmgr, &(s->parser_branches_nr));

		/* Now build a GNF grammar out of this */
		s->parser_rules = vocab_build_gnf(s->parser_branches, s->parser_branches_nr);

	} else {
		sciprintf("Assuming that this game does not use a parser.\n");
		s->parser_rules = NULL;
	}


	s->opcodes = vocabulary_get_opcodes(s->resmgr);

	if (!(s->selector_names = vocabulary_get_snames(s->resmgr, NULL, s->version))) {
		sciprintf("_init_vocabulary(): Could not retreive selector names (vocab.997)!\n");
		return 1;
	}

	for (s->selector_names_nr = 0; s->selector_names[s->selector_names_nr]; s->selector_names_nr++);
	/* Counts the number of selector names */

	script_map_selectors(s, &(s->selector_map));
	/* Maps a few special selectors for later use */

	return 0;
}

extern int _allocd_rules;
static void
_free_vocabulary(state_t *s)
{
	sciprintf("Freeing vocabulary\n");

	if (s->parser_words) {
		vocab_free_words(s->parser_words, s->parser_words_nr);
		vocab_free_suffices(s->resmgr, s->parser_suffices, s->parser_suffices_nr);
		vocab_free_branches(s->parser_branches);
		vocab_free_rule_list(s->parser_rules);
	}

	vocabulary_free_snames(s->selector_names);
	vocabulary_free_knames(s->kernel_names);
	vocabulary_free_opcodes(s->opcodes);
	s->opcodes = NULL;

	s->selector_names = NULL;
	s->kernel_names = NULL;
	s->opcodes = NULL;
}


static int
_init_graphics_input(state_t *s)
{
	s->pic_priority_table = NULL;
	s->pics = NULL;
	s->pics_nr = 0;
	return 0;
}


static int
_reset_graphics_input(state_t *s)
{
	resource_t *resource;
	int font_nr;
	gfx_color_t transparent	= {0};
	sciprintf("Initializing graphics\n");

	if (s->resmgr->sci_version <= SCI_VERSION_01) {
		int i;

		for (i = 0; i < 16; i++) {
			if (gfxop_set_color(s->gfx_state, &(s->ega_colors[i]),
					    gfx_sci0_image_colors[sci0_palette][i].r,
					    gfx_sci0_image_colors[sci0_palette][i].g,
					    gfx_sci0_image_colors[sci0_palette][i].b,
					    0, -1, -1))
				return 1;
			gfxop_set_system_color(s->gfx_state, &(s->ega_colors[i]));
		}
	} else
		if (gfxop_set_color(s->gfx_state, &(s->ega_colors[0]), 0, 0, 0, 0, -1, -1)) return 1; /* We usually need black */

	transparent.mask = 0;

	gfxop_fill_box(s->gfx_state, gfx_rect(0, 0, 320, 200), s->ega_colors[0]); /* Fill screen black */
	gfxop_update(s->gfx_state);

	s->mouse_pointer_nr = -1; /* No mouse pointer resource */
	gfxop_set_pointer_position(s->gfx_state, gfx_point(160, 150));

	s->mouse_pointer_nr = -1;

	s->pic_not_valid = 1; /* Picture is invalid */
	s->pic_is_new = 0;
	s->pic_visible_map = 0; /* Other values only make sense for debugging */
	s->dyn_views = NULL; /* no DynViews */
	s->drop_views = NULL; /* And, consequently, no list for dropped views */

	s->priority_first = 42; /* Priority zone 0 ends here */
	s->priority_last = 200; /* The highest priority zone (15) starts here */

	font_nr = -1;
	do {
		resource = scir_test_resource(s->resmgr, sci_font, ++font_nr);
	} while ((!resource) && (font_nr < 999));

	if (!resource) {
		sciprintf("No text font was found.\n");
		return 1;
	}

	s->visual = gfxw_new_visual(s->gfx_state, font_nr);

	s->wm_port = gfxw_new_port(s->visual, NULL, gfx_rect(0, 10, 320, 190), s->ega_colors[0], transparent);
	s->titlebar_port = gfxw_new_port(s->visual, NULL, gfx_rect(0, 0, 320, 10), s->ega_colors[0], s->ega_colors[15]);
	s->picture_port = gfxw_new_port(s->visual, NULL, gfx_rect(0, 10, 320, 190), s->ega_colors[0], transparent);
	s->titlebar_port->color.mask |= GFX_MASK_PRIORITY;
	s->titlebar_port->color.priority = 11;
	s->titlebar_port->bgcolor.mask |= GFX_MASK_PRIORITY;
	s->titlebar_port->bgcolor.priority = 11;

	s->pics_drawn_nr = 0;

	s->visual->add(GFXWC(s->visual), GFXW(s->picture_port));
	s->visual->add(GFXWC(s->visual), GFXW(s->wm_port));
	s->visual->add(GFXWC(s->visual), GFXW(s->titlebar_port));
	/* Add ports to visual */

	s->port = s->wm_port; /* Currently using the window manager port */

#if 0
	s->titlebar_port->bgcolor.mask |= GFX_MASK_PRIORITY;
	s->titlebar_port->bgcolor.priority = 11; /* Standard priority for the titlebar port */
#endif

	return 0;
}

int
game_init_graphics(state_t *s)
{
	return _reset_graphics_input(s);
}


static void
_free_graphics_input(state_t *s)
{
	sciprintf("Freeing graphics\n");

	s->visual->widfree(GFXW(s->visual));

	s->wm_port = s->titlebar_port = s->picture_port = NULL;
	s->visual = NULL;
	s->dyn_views = NULL;
	s->port = NULL;

	if (s->pics)
		sci_free(s->pics);
	s->pics = NULL;
}


/* Returns the script number suggested by vocab.996, or -1 if there's none */
static int
suggested_script(resource_t *res, unsigned int class)
{
	int offset;
	if (class >= res->size >> 2)
		return -1;

	offset = 2 + (class << 2);

	return getInt16(res->data + offset);
}


/* Architectural stuff: Init/Unintialize engine */
int
script_init_engine(state_t *s, sci_version_t version)
{
	resource_t *vocab996 = scir_find_resource(s->resmgr, sci_vocab, 996, 1);
	int i;
	int scriptnr;
	unsigned int seeker;
	int classnr;
	int size;
	int magic_offset; /* For strange scripts in older SCI versions */

	s->max_version = SCI_VERSION(9,999,999); /* :-) */
	s->min_version = 0; /* Set no real limits */
	s->version = SCI_VERSION_DEFAULT_SCI0;
	s->kernel_opt_flags = 0;


	if (!version) {
		s->version_lock_flag = 0;
	} else {
		s->version = version;
		s->version_lock_flag = 1; /* Lock version */
	}

	script_detect_early_versions(s);

	if (!vocab996)
		s->classtable_size = 20;
	else
		s->classtable_size = vocab996->size >> 2;

	s->classtable = sci_calloc(sizeof(class_t), s->classtable_size);

	for (scriptnr = 0; scriptnr < 1000; scriptnr++) {
		int objtype = 0;
		resource_t *script = scir_find_resource(s->resmgr, sci_script,
							scriptnr, 0);

		if (script) {
			size = getInt16(script->data);
			if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
				magic_offset = seeker = 2;
			else
				magic_offset = seeker = 0;

			do {

				while (seeker < script->size)	{
					unsigned int lastseeker = seeker;
					objtype = getInt16(script->data + seeker);
					if (objtype == sci_obj_class || objtype == sci_obj_terminator)
						break;
					seeker += getInt16(script->data + seeker + 2);
					if (seeker <= lastseeker) {
						sciprintf("Warning: Script version is invalid.\n");
						sci_free(s->classtable);
						return  SCI_ERROR_INVALID_SCRIPT_VERSION;
					}
				}

				if (objtype == sci_obj_class) {
					int sugg_script;

					seeker -= SCRIPT_OBJECT_MAGIC_OFFSET; /* Adjust position; script home is base +8 bytes */

					classnr = getInt16(script->data + seeker + 4 + SCRIPT_SPECIES_OFFSET);
					if (classnr >= s->classtable_size) {

						if (classnr >= SCRIPT_MAX_CLASSTABLE_SIZE) {
							fprintf(stderr,"Invalid class number 0x%x in script.%d(0x%x), offset %04x\n",
								classnr, scriptnr, scriptnr, seeker);
							return 1;
						}

						s->classtable = sci_realloc(s->classtable, sizeof(class_t) * (classnr + 1));
						memset(&(s->classtable[s->classtable_size]), 0,
						       sizeof(class_t) * (1 + classnr - s->classtable_size)); /* Clear after resize */

						s->classtable_size = classnr + 1; /* Adjust maximum number of entries */
					}

					sugg_script = suggested_script(vocab996, classnr);

					/* First, test whether the script hasn't been claimed, or if it's been claimed by the wrong script */
					if (sugg_script == -1 || scriptnr == sugg_script || !s->classtable[classnr].scriptposp) {
						/* Now set the home script of the class */
						s->classtable[classnr].class_offset = seeker + 4 - magic_offset;
						s->classtable[classnr].script = scriptnr;
						s->classtable[classnr].scriptposp = &(s->scripttable[scriptnr].heappos);
					}

					seeker += SCRIPT_OBJECT_MAGIC_OFFSET; /* Re-adjust position */

					seeker += getInt16(script->data + seeker + 2); /* Move to next */
				}

			} while (objtype != sci_obj_terminator && seeker <= script->size);

		}
	}
	scir_unlock_resource(s->resmgr, vocab996);
	vocab996 = NULL;

	s->_heap = heap_new();
	s->heap = s->_heap->start;

	/* Allocate static buffer for savegame and CWD directories */
	s->save_dir = heap_allocate(s->_heap, MAX_SAVE_DIR_SIZE);
	s->save_dir_copy = 0xffff;
	s->save_dir_edit_offset = 0;
	s->save_dir_copy_buf = sci_malloc(MAX_SAVE_DIR_SIZE);
	s->save_dir_copy_buf[0] = 0; /* Terminate string */

	save_ff(s->_heap); /* Save heap state */

	s->acc = s->amp_rest = s->prev = 0;

	s->execution_stack = NULL;    /* Start without any execution stack */
	s->execution_stack_base = -1; /* No vm is running yet */
	s->execution_stack_pos = -1;   /* Start at execution stack position 0 */

	s->global_vars = 0; /* Set during launch time */


	s->kernel_names = vocabulary_get_knames(s->resmgr, &s->kernel_names_nr);
	script_map_kernel(s);
	/* Maps the kernel functions */

	if (_init_vocabulary(s)) return 1;

	s->restarting_flags = SCI_GAME_IS_NOT_RESTARTING;


	for (i = 0; i < 1000; i++)
		s->scripttable[i].heappos = 0; /* Mark all scripts as 'not installed' */

	s->bp_list = NULL; /* No breakpoints defined */
	s->have_bp = 0;

	s->file_handles_nr = 5;
	s->file_handles = sci_calloc(sizeof(FILE *), s->file_handles_nr);
	/* Allocate memory for file handles */

	sci_init_dir(&(s->dirseeker));
	s->dirseeker_outbuffer = 0;
	/* Those two are used by FileIO for FIND_FIRST, FIND_NEXT */

	sciprintf("Engine initialized\n");

	if (_init_graphics_input(s))
		return 1;

	return 0;
}


void
script_set_gamestate_save_dir(state_t *s)
{
	char *cwd = sci_getcwd();
	if (strlen(cwd) > MAX_SAVE_DIR_SIZE)
		sciprintf("Warning: cwd '%s' is longer than the"
			  " MAX_SAVE_DIR_SIZE %d\n",
			  cwd, MAX_SAVE_DIR_SIZE);
	else
		strcpy((char *)(s->heap + s->save_dir + 2), cwd);
	sci_free(cwd);
}

void
script_free_vm_memory(state_t *s)
{
	int i;

	sciprintf("Freeing VM memory\n");
	heap_free(s->_heap, s->save_dir);
	sci_free(s->save_dir_copy_buf);
	s->save_dir_copy_buf = NULL;

	for (i = 0; i < MAX_HUNK_BLOCKS; i++)
		if (s->hunk[i].size) {
			if (s->hunk[i].type == HUNK_TYPE_GFXBUFFER) {
				gfxw_snapshot_t *snapshot = *((gfxw_snapshot_t **) s->hunk[i].data);
				sci_free(snapshot);
			}
			sci_free(s->hunk[i].data);
			s->hunk[i].size = 0;
		}


	heap_del(s->_heap);
	s->_heap = NULL;
	sci_free(s->classtable);
	s->classtable = NULL;

	/* Close all opened file handles */
#ifndef _DOS
	for (i = 1; i < s->file_handles_nr; i++)
		if (s->file_handles[i])
			fclose(s->file_handles[i]);
#endif

	sci_free(s->file_handles);
	s->file_handles = NULL;

	/* FIXME: file handles will NOT be closed under DOS. DJGPP generates an
	   exception fault whenever you try to close a never-opened file */
}

void
script_free_engine(state_t *s)
{
	breakpoint_t *bp, *bp_next;

	script_free_vm_memory(s);

	sciprintf("Freeing state-dependant data\n");

	sci_free(s->kfunct_table);
	s->kfunct_table = NULL;

	_free_vocabulary(s);

	/* Free breakpoint list */
	bp = s->bp_list;
	while (bp) {
		bp_next = bp->next;
		if (bp->type == BREAK_SELECTOR) sci_free (bp->data.name);
		free (bp);
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
	heap_ptr stack_handle;
	heap_ptr parser_handle;
	heap_ptr script0;
	heap_ptr game_obj; /* Address of the game object */
	int i;

	if (!script_instantiate(s, 0, 0)) {
		sciprintf("game_init(): Could not instantiate script 0\n");
		return 1;
	}

	s->parser_valid = 0; /* Invalidate parser */
	s->parser_event = 0; /* Invalidate parser event */

	stack_handle = heap_allocate(s->_heap, VM_STACK_SIZE);
	parser_handle = heap_allocate(s->_heap, PARSE_HEAP_SIZE);

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
	/* Initialize clone list */
	memset(&(s->clone_list), 0, sizeof(heap_ptr) * SCRIPT_MAX_CLONES);
	/* Initialize send_calls buffer */

	if (!send_calls_allocated)
		send_calls = sci_calloc(sizeof(calls_struct_t), send_calls_allocated = 16);

	if (!stack_handle) {
		sciprintf("game_init(): Insufficient heap space for stack\n");
		return 1;
	}

	if (!parser_handle) {
		sciprintf("game_init(): Insufficient heap space for parser word error block\n");
		return 1;
	}

	if (s->gfx_state && _reset_graphics_input(s))
		return 1;

	s->successor = NULL; /* No successor */
	s->status_bar_text = NULL; /* Status bar is blank */
	s->status_bar_foreground = 0;
	s->status_bar_background = 15;

	fprintf(stderr," Script 0 at %04x\n", script0);

	s->stack_base = stack_handle + 2;
	s->parser_base = parser_handle + 2;
	s->global_vars = s->scripttable[0].localvar_offset;
	/* Global variables are script 0's local variables */

	sci_get_current_time(&(s->game_start_time)); /* Get start time */
	memcpy(&(s->last_wait_time), &(s->game_start_time), sizeof(GTimeVal));
	/* Use start time as last_wait_time */

	s->debug_mode = 0x0; /* Disable all debugging */
	s->onscreen_console = 0; /* No onscreen console unless explicitly requested */

	srand(time(NULL)); /* Initialize random number generator */

	memset(s->hunk, sizeof(s->hunk), 0); /* Sets hunk to be unused */
	memset(s->clone_list, sizeof(s->clone_list), 0); /* No clones */

	/*	script_dissect(0, s->selector_names, s->selector_names_nr); */
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

	if (strlen((char *) s->game_name) >= MAX_GAMEDIR_SIZE) {

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

	if (s->execution_stack) {
		sci_free(s->execution_stack);
	}

	if (s->synonyms_nr) {
		sci_free(s->synonyms);
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
	heap_free(s->_heap, s->parser_base - 2);
	restore_ff(s->_heap); /* Restore former heap state */

	if (send_calls_allocated) {
		sci_free(send_calls);
		send_calls_allocated = 0;
	}

	for (i = 0; i < 1000; i++)
		/*  if (s->scripttable[i].heappos > s->_heap->old_ff)*/
		s->scripttable[i].heappos = 0; /* Mark all non-high scripts as 'not installed', except for 0 */

	menubar_free(s->menubar);

	_free_graphics_input(s);

	return 0;
}

