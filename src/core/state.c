/***************************************************************************
 state.c Copyright (C) 1999 Christoph Reichenbach


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
#include <kdebug.h>

#ifndef _WIN32
#include <sys/resource.h>
#endif


int
script_init_state(state_t *s, sci_version_t version)
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

  s->parser_valid = 0;

  script_detect_early_versions(s);

  s->_heap = heap_new();
  s->heap = s->_heap->start;
  s->acc = s->amp_rest = s->prev = 0;

  /* Init parser */
  if (s->parser_words = vocab_get_words(&(s->parser_words_nr))) {
    s->parser_suffices = vocab_get_suffices(&(s->parser_suffices_nr));
    s->parser_branches = vocab_get_branches(&(s->parser_branches_nr));

    /* Mark parse tree as unused */
    s->parser_nodes[0].type = PARSE_TREE_NODE_LEAF;
    s->parser_nodes[0].content.value = 0;
  } else
    sciprintf("Assuming that this game does not use a parser.\n");

  s->opcodes = vocabulary_get_opcodes();

  if (!(s->selector_names = vocabulary_get_snames(&(s->selector_names_nr)))) {
    sciprintf("script_init(): Could not retreive selector names (vocab.997)!\n");
    return 1;
  }


  script_map_selectors(s, &(s->selector_map));
  /* Maps a few special selectors for later use */

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

  s->kernel_names = vocabulary_get_knames(&s->kernel_names_nr);
  script_map_kernel(s);
  /* Maps the kernel functions */

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

  return 0;
}


void
script_free_state(state_t *s)
{
  int i;

  sciprintf("Freeing state-dependant data\n");
  for (i = 0; i < MAX_HUNK_BLOCKS; i++)
      if (s->hunk[i].size) {
	  g_free(s->hunk[i].data);
	  s->hunk[i].size = 0;
      }

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

  if (s->execution_stack)
    g_free(s->execution_stack);

  heap_del(s->_heap);

  g_free(s->classtable);

  g_free(s->kfunct_table);
  s->kfunct_table = NULL;

  vocabulary_free_knames(s->kernel_names);
  s->kernel_names = NULL;

  sciprintf("Freeing vocabulary...\n");

  if (s->parser_words) {
    vocab_free_words(s->parser_words, s->parser_words_nr);
    vocab_free_suffices(s->parser_suffices, s->parser_suffices_nr);
    vocab_free_branches(s->parser_branches);
  }

  g_free(s->opcodes);
  vocabulary_free_snames(s->selector_names);
  s->selector_names = NULL;
  s->opcodes = NULL;
  /* Make sure to segfault if any of those are dereferenced */

}

