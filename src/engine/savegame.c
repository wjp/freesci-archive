/***************************************************************************
 savegame.cfsml Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Savegame handling for state_t structs. Makes heavy use of cfsml magic. */
/* DON'T EDIT savegame.c ! Only modify savegame.cfsml, if something needs
** to be changed. Refer to freesci/docs/misc/cfsml.spec if you don't understand
** savegame.cfsml. If this doesn't solve your problem, contact the maintainer.
*/

#include <engine.h>
#include <assert.h>
#include <heap.h>
#ifndef _DOS
# include <glib.h>
#endif
#include <ctype.h>
#ifdef HAVE_DIRENT_H
#include <sys/types.h> /* Required by e.g. NetBSD */
#include <dirent.h>
#endif

#ifdef _MSC_VER
#include <direct.h>
#endif


int
gamestate_save(state_t *s, char *dirname)
{
  FILE *fh;
  DIR *dir;
  struct dirent* dirent;
  int fd;

  s->savegame_version = FREESCI_SAVEGAME_VERSION;

  if (s->execution_stack_base) {
    sciprintf("Cannot save from below kernel function\n");
    return 1;
  }

  scimkdir (dirname, 0700);

  if (chdir (dirname)) {
    sciprintf("Could not enter directory '%s'\n", dirname);
    return 1;
  }

  dir = opendir(".");
  while ((dir) && (dirent = readdir(dir)))
    if (strcmp(dirent->d_name, "..") && strcmp(dirent->d_name, ".")) {
      unlink(dirent->d_name); /* Delete all files in directory */
  }
  closedir(dir);

  if (s->sfx_driver) {
    if ((s->sfx_driver->save)(s, dirname)) {
      sciprintf("Saving failed for the sound subsystem\n");
      chdir ("..");
      return 1;
    }
  }

  fh = fopen("state", "w");

  /* Calculate the time spent with this game */
  s->game_time = time(NULL) - s->game_start_time.tv_sec;

  fclose(fh);

  fd = creat("heap", 0600);
  write(fd, s->_heap->start, SCI_HEAP_SIZE);
  close(fd);

  chdir ("..");
  return 0;
}


state_t *
gamestate_restore(state_t *s, char *dirname)
{
  FILE *fh;
  int fd;
  int i;
  int read_eof = 0;
  state_t *retval;

  if (chdir (dirname)) {
    sciprintf("Game state '%s' does not exist\n", dirname);
    return NULL;
  }

  if (s->sfx_driver) {
    if ((s->sfx_driver->restore)(s, dirname)) {
      sciprintf("Restoring failed for the sound subsystem\n");
      return NULL;
    }
  }

  retval = (state_t *) xalloc(sizeof(state_t));
  retval->_heap = heap_new();
  retval->savegame_version = -1;

  fh = fopen("state", "r");
  if (!fh) {
    heap_del(retval->_heap);
    free(retval);
    return NULL;
  }

  retval->amp_rest = 0; /* Backwards compatibility */

  fclose(fh);

  if ((retval->savegame_version < 0) || (retval->savegame_version > FREESCI_SAVEGAME_VERSION)) {

    if (retval->savegame_version < 0)
      sciprintf("Very old save game encountered- can't load\n");
    else
      sciprintf("Savegame version is %d- maximum supported is %0d\n", retval->savegame_version, FREESCI_SAVEGAME_VERSION);

    chdir("..");
    heap_del(retval->_heap);
    free(retval);
    return NULL;
  }

  if (read_eof || ((fd = open("heap", O_RDONLY)) < 0)) {
    if (read_eof)
      sciprintf("Error while reading gamestate '%s'\n", dirname);
    else
      sciprintf("Heap file does not exist for game state '%s'\n", dirname);
    chdir("..");
    heap_del(retval->_heap);
    free(retval);
    return NULL;
  }

  read(fd, retval->_heap->start, SCI_HEAP_SIZE);
  close(fd);

  /* Set exec stack base to zero */
  retval->execution_stack_base = 0;

  /* Set the class table pointers to the script positions */
  for (i = 0; i < retval->classtable_size; i++)
    retval->classtable[i].scriptposp = &(retval->scripttable[retval->classtable[i].script].heappos);

  /* Now copy all current state information */
  /* Graphics and input state: */

  /* Sound state: */
  retval->sfx_driver = s->sfx_driver;
  memcpy(&(retval->sound_pipe_in[0]), &(s->sound_pipe_in[0]), sizeof(int[2]));
  memcpy(&(retval->sound_pipe_out[0]), &(s->sound_pipe_out[0]), sizeof(int[2]));
  memcpy(&(retval->sound_pipe_events[0]), &(s->sound_pipe_events[0]), sizeof(int[2]));
  memcpy(&(retval->sound_pipe_debug[0]), &(s->sound_pipe_debug[0]), sizeof(int[2]));

  /* Time state: */
  g_get_current_time(&(retval->last_wait_time));
  retval->game_start_time.tv_sec = time(NULL) - retval->game_time;
  retval->game_start_time.tv_usec = 0;

  /* File IO state: */
  retval->file_handles_nr = 2;
  retval->file_handles = calloc(2, sizeof(FILE *));

  /* static parser information: */
  retval->parser_rules = s->parser_rules;
  retval->parser_words_nr = s->parser_words_nr;
  retval->parser_words = s->parser_words;
  retval->parser_suffices_nr = s->parser_suffices_nr;
  retval->parser_suffices = s->parser_suffices;
  retval->parser_branches_nr = s->parser_branches_nr;
  retval->parser_branches = s->parser_branches;

  /* static VM/Kernel information: */
  retval->selector_names_nr = s->selector_names_nr;
  retval->selector_names = s->selector_names;
  retval->kernel_names_nr = s->kernel_names_nr;
  retval->kernel_names = s->kernel_names;
  retval->kfunct_table = s->kfunct_table;
  retval->opcodes = s->opcodes;

  memcpy(&(retval->selector_map), &(s->selector_map), sizeof(selector_map_t));

  /* Copy breakpoint information from current game instance */
  retval->have_bp = s->have_bp;
  retval->bp_list = s->bp_list;

  retval->resource_dir = s->resource_dir;
  retval->work_dir = s->work_dir;

  retval->successor = NULL;

  chdir ("..");

WARNING( FIXME Savegames are completely broken)
  return retval;
}
