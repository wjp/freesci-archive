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
#include <glib.h>
#include <ctype.h>
#include <graphics_png.h>
#ifdef HAVE_DIRENT_H
#include <sys/types.h> /* Required by e.g. NetBSD */
#include <dirent.h>
#endif

#ifdef _MSC_VER
#include <direct.h>
#endif

/* Missing:
** - SFXdriver
** - File input/output state (this is likely not to happen)
*/

state_t *_global_save_state;
/* Needed for graphical hunk writing. */


void
write_heapptr(FILE *fh, heap_ptr *foo)
{
  fprintf(fh, "0x%04x", *foo);
}

int
read_heapptr(FILE *fh, heap_ptr *foo, char *lastval, int *line, int *hiteof)
{
  *foo = strtol(lastval, NULL, 0);
  return 0;
}

void
write_sci_version(FILE *fh, sci_version_t *foo)
{
  fprintf(fh, "%d.%03d.%03d", SCI_VERSION_MAJOR(*foo), SCI_VERSION_MINOR(*foo),
	  SCI_VERSION_PATCHLEVEL(*foo));
}

int
read_sci_version(FILE *fh, sci_version_t *foo, char *lastval, int *line, int *hiteof)
{
  *foo = version_parse(lastval);
  return 0;
}

void
write_hunk_block(FILE *fh, hunk_block_t *foo)
{
  char filename[14] = "hunk.0";
  int filenr = 0;
  int hunkfile;

  if (!foo->size) {
    fputs("\\null\\", fh);
    return; /* Don't write empty blocks */
  }

  if (foo->type == HUNK_TYPE_GFXBUFFER) {

    int handle = graph_png_save_box(_global_save_state, foo->data);

    if (handle >= 0)
      sprintf(filename, "buffer.%d", handle);
    else
      strcpy(filename, "\\ERROR\\"); /* Treat as empty */

  } else { /* Normal buffer */

    while ((hunkfile = open(filename, O_RDONLY)) > 0) {
      close(hunkfile);
      sprintf(filename + 5, "%d", ++filenr);
    }

    hunkfile = creat(filename, 0600);
    assert (hunkfile > 0);

    write(hunkfile, foo->data, foo->size);

    close(hunkfile);
  }

  fputs(filename, fh);
}

int
read_hunk_block(FILE *fh, hunk_block_t *foo, char *lastval, int *line, int *hiteof)
{
  int hunkfile;

  if (lastval[0] == '\\') { /* Nothing written? */
    foo->size = 0;
    return 0;
  }

  if (lastval[0] == 'b') { /* Graphical buffer */
    int bufnum;

    foo->type = HUNK_TYPE_GFXBUFFER;

    sscanf(lastval, "buffer.%d", &bufnum);
    foo->data = graph_png_load_box(_global_save_state, bufnum, &(foo->size));

    if (!foo->data)
      return 1;

  } else { /* Normal hunk */

    foo->type = HUNK_TYPE_ANY;

    hunkfile = open(lastval, O_RDONLY);

    foo->size = lseek(hunkfile, 0, SEEK_END);
    lseek(hunkfile, 0, SEEK_SET);

    foo->data = (char *) xalloc(foo->size);
    read(hunkfile, foo->data, foo->size);
    close(hunkfile);

    return 0;
  }

  return 0;
}

void
write_PTN(FILE *fh, parse_tree_node_t *foo)
{
  if (foo->type == PARSE_TREE_NODE_LEAF)
    fprintf(fh, "L%d", foo->content.value);
  else
    fprintf(fh, "B(%d,%d)", foo->content.branches[0], foo->content.branches[1]);
}

int
read_PTN(FILE *fh, parse_tree_node_t *foo, char *lastval, int *line, int *hiteof)
{
  if (lastval[0] == 'L') {
    if (sscanf(lastval, "L %d", &(foo->content.value)) < 1)
      return 1; /* Failed to parse everything */
    return 0;
  } else if (lastval[0] == 'B') {
    if (sscanf(lastval, "B ( %d , %d )", &(foo->content.branches[0]), &(foo->content.branches[1])) < 2)
      return 1; /* Failed to parse everything */
    return 0;
  } else return 1; /* failure to parse anything */
}


void
write_menubar_tp(FILE *fh, menubar_t **foo);
int
read_menubar_tp(FILE *fh, menubar_t **foo, char *lastval, int *line, int *hiteof);

void
write_port_tp(FILE *fh, port_t **foo);
int
read_port_tp(FILE *fh, port_t **foo, char *lastval, int *line, int *hiteof);




/* Auto-generated CFSML declaration and function block */

#line 599 "cfsml.pl"
#define CFSML_SUCCESS 0
#define CFSML_FAILURE 1

#line 52 "cfsml.pl"

#include <stdarg.h> /* We need va_lists */

static void
_cfsml_error(char *fmt, ...)
{
  va_list argp;

  fprintf(stderr, "Error: ");
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);

}

static char *
_cfsml_mangle_string(char *s)
{
  char *source = s;
  char c;
  char *target = (char *) g_malloc(1 + strlen(s) * 2); /* We will probably need less than that */
  char *writer = target;

  while (c = *source++) {

    if (c < 32) { /* Special character? */
      *writer++ = '\\'; /* Escape... */
      c += ('a' - 1);
    } else if (c == '\\' || c == '"')
      *writer++ = '\\'; /* Escape, but do not change */
    *writer++ = c;

  }
  *writer = 0; /* Terminate string */

  return (char *) g_realloc(target, strlen(target) + 1);
}


static char *
_cfsml_unmangle_string(char *s)
{
  char *target = (char *) g_malloc(1 + strlen(s));
  char *writer = target;
  char *source = s;
  char c;

  while ((c = *source++) && (c > 31)) {
    if (c == '\\') { /* Escaped character? */
      c = *source++;
      if ((c != '\\') && (c != '"')) /* Un-escape 0-31 only */
	c -= ('a' - 1);
    }
    *writer++ = c;
  }
  *writer = 0; /* Terminate string */

  return (char *) g_realloc(target, strlen(target) + 1);
}


static char *
_cfsml_get_identifier(FILE *fd, int *line, int *hiteof, int *assignment)
{
  char c;
  int mem = 32;
  int pos = 0;
  int done = 0;
  char *retval = (char *) g_malloc(mem);

  while (isspace(c = fgetc(fd)) && (c != EOF));
  if (c == EOF) {
    _cfsml_error("Unexpected end of file at line %d\n", *line);
    g_free(retval);
    *hiteof = 1;
    return NULL;
  }

  ungetc(c, fd);

  while (((c = fgetc(fd)) != EOF) && ((pos == 0) || (c != '\n')) && (c != '=')) {

     if (pos == mem - 1) /* Need more memory? */
       retval = (char *) g_realloc(retval, mem *= 2);

     if (!isspace(c)) {
        if (done) {
           _cfsml_error("Single word identifier expected at line %d\n", *line);
           g_free(retval);
           return NULL;
        }
        retval[pos++] = c;
     } else
        if (pos != 0)
           done = 1; /* Finished the variable name */
        else if (c == '\n')
           ++(*line);
  }

  if (c == EOF) {
    _cfsml_error("Unexpected end of file at line %d\n", *line);
    g_free(retval);
    *hiteof = 1;
    return NULL;
  }

  if (c == '\n') {
    ++(*line);
    if (assignment)
      *assignment = 0;
  } else
    if (assignment)
      *assignment = 1;

  if (pos == 0) {
    _cfsml_error("Missing identifier in assignment at line %d\n", *line);
    g_free(retval);
    return NULL;
  }

  if (pos == mem - 1) /* Need more memory? */
     retval = (char *) g_realloc(retval, mem += 1);

  retval[pos] = 0; /* Terminate string */

  return retval;
}


static char *
_cfsml_get_value(FILE *fd, int *line, int *hiteof)
{
  char c;
  int mem = 64;
  int pos = 0;
  char *retval = (char *) g_malloc(mem);

  while (((c = fgetc(fd)) != EOF) && (c != '\n')) {

     if (pos == mem - 1) /* Need more memory? */
       retval = (char *) g_realloc(retval, mem *= 2);

     if (pos || (!isspace(c)))
        retval[pos++] = c;

  }

  while ((pos > 0) && (isspace(retval[pos - 1])))
     --pos; /* Strip trailing whitespace */

  if (c == EOF)
    *hiteof = 1;

  if (pos == 0) {
    _cfsml_error("Missing value in assignment at line %d\n", *line);
    g_free(retval);
    return NULL;
  }

  if (c == '\n')
     ++(*line);

  if (pos == mem - 1) /* Need more memory? */
    retval = (char *) g_realloc(retval, mem += 1);

  retval[pos] = 0; /* Terminate string */
  return (char *) g_realloc(retval, strlen(retval) + 1);
  /* Re-allocate; this value might be used for quite some while (if we are
  ** restoring a string)
  */
}
#line 284 "cfsml.pl"
static void
_cfsml_write_long(FILE *fh, long* foo);
static int
_cfsml_read_long(FILE *fh, long* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_state_t(FILE *fh, state_t* foo);
static int
_cfsml_read_state_t(FILE *fh, state_t* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_view_object_t(FILE *fh, view_object_t* foo);
static int
_cfsml_read_view_object_t(FILE *fh, view_object_t* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_byte(FILE *fh, byte* foo);
static int
_cfsml_read_byte(FILE *fh, byte* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_menu_item_t(FILE *fh, menu_item_t* foo);
static int
_cfsml_read_menu_item_t(FILE *fh, menu_item_t* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_string(FILE *fh, char ** foo);
static int
_cfsml_read_string(FILE *fh, char ** foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_port_t(FILE *fh, port_t* foo);
static int
_cfsml_read_port_t(FILE *fh, port_t* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_script_t(FILE *fh, script_t* foo);
static int
_cfsml_read_script_t(FILE *fh, script_t* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_menu_t(FILE *fh, menu_t* foo);
static int
_cfsml_read_menu_t(FILE *fh, menu_t* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_class_t(FILE *fh, class_t* foo);
static int
_cfsml_read_class_t(FILE *fh, class_t* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_gint16(FILE *fh, gint16* foo);
static int
_cfsml_read_gint16(FILE *fh, gint16* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_int(FILE *fh, int* foo);
static int
_cfsml_read_int(FILE *fh, int* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_exec_stack_t(FILE *fh, exec_stack_t* foo);
static int
_cfsml_read_exec_stack_t(FILE *fh, exec_stack_t* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_menubar_t(FILE *fh, menubar_t* foo);
static int
_cfsml_read_menubar_t(FILE *fh, menubar_t* foo, char *lastval, int *line, int *hiteof);

#line 297 "cfsml.pl"
static void
_cfsml_write_long(FILE *fh, long* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 381 "cfsml.pl"
static int
_cfsml_read_long(FILE *fh, long* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 405 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_state_t(FILE *fh, state_t* foo)
{
  char *bar;
  int min, max, i;

#line 315 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "restarting_flags = ");
    _cfsml_write_byte(fh, &(foo->restarting_flags));
    fprintf(fh, "\n");
  fprintf(fh, "have_mouse_flag = ");
    _cfsml_write_byte(fh, &(foo->have_mouse_flag));
    fprintf(fh, "\n");
  fprintf(fh, "pic_not_valid = ");
    _cfsml_write_byte(fh, &(foo->pic_not_valid));
    fprintf(fh, "\n");
  fprintf(fh, "pic_is_new = ");
    _cfsml_write_byte(fh, &(foo->pic_is_new));
    fprintf(fh, "\n");
  fprintf(fh, "onscreen_console = ");
    _cfsml_write_byte(fh, &(foo->onscreen_console));
    fprintf(fh, "\n");
  fprintf(fh, "mouse_pointer_nr = ");
    _cfsml_write_int(fh, &(foo->mouse_pointer_nr));
    fprintf(fh, "\n");
  fprintf(fh, "debug_mode = ");
    _cfsml_write_int(fh, &(foo->debug_mode));
    fprintf(fh, "\n");
  fprintf(fh, "game_time = ");
    _cfsml_write_long(fh, &(foo->game_time));
    fprintf(fh, "\n");
  fprintf(fh, "save_dir = ");
    write_heapptr(fh, &(foo->save_dir));
    fprintf(fh, "\n");
  fprintf(fh, "sound_object = ");
    write_heapptr(fh, &(foo->sound_object));
    fprintf(fh, "\n");
  fprintf(fh, "last_pointer_x = ");
    _cfsml_write_int(fh, &(foo->last_pointer_x));
    fprintf(fh, "\n");
  fprintf(fh, "last_pointer_y = ");
    _cfsml_write_int(fh, &(foo->last_pointer_y));
    fprintf(fh, "\n");
  fprintf(fh, "last_pointer_size_x = ");
    _cfsml_write_int(fh, &(foo->last_pointer_size_x));
    fprintf(fh, "\n");
  fprintf(fh, "last_pointer_size_y = ");
    _cfsml_write_int(fh, &(foo->last_pointer_size_y));
    fprintf(fh, "\n");
  fprintf(fh, "view_port = ");
    _cfsml_write_int(fh, &(foo->view_port));
    fprintf(fh, "\n");
  fprintf(fh, "ports = ");
    min = max = MAX_PORTS;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      write_port_tp(fh, &(foo->ports[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "pic_visible_map = ");
    _cfsml_write_int(fh, &(foo->pic_visible_map));
    fprintf(fh, "\n");
  fprintf(fh, "pic_animate = ");
    _cfsml_write_int(fh, &(foo->pic_animate));
    fprintf(fh, "\n");
  fprintf(fh, "dyn_view_port = ");
    _cfsml_write_int(fh, &(foo->dyn_view_port));
    fprintf(fh, "\n");
  fprintf(fh, "pic_views = ");
    min = max = foo->pic_views_nr;
    if (!foo->pic_views)
       min = max = 0; /* Don't write if it points to NULL */
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_view_object_t(fh, &(foo->pic_views[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "dyn_views = ");
    min = max = foo->dyn_views_nr;
    if (!foo->dyn_views)
       min = max = 0; /* Don't write if it points to NULL */
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_view_object_t(fh, &(foo->dyn_views[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "animation_delay = ");
    _cfsml_write_int(fh, &(foo->animation_delay));
    fprintf(fh, "\n");
  fprintf(fh, "hunk = ");
    min = max = MAX_HUNK_BLOCKS;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      write_hunk_block(fh, &(foo->hunk[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "menubar = ");
    write_menubar_tp(fh, &(foo->menubar));
    fprintf(fh, "\n");
  fprintf(fh, "status_bar_text = ");
    _cfsml_write_string(fh, &(foo->status_bar_text));
    fprintf(fh, "\n");
  fprintf(fh, "priority_first = ");
    _cfsml_write_int(fh, &(foo->priority_first));
    fprintf(fh, "\n");
  fprintf(fh, "priority_last = ");
    _cfsml_write_int(fh, &(foo->priority_last));
    fprintf(fh, "\n");
  fprintf(fh, "version_lock_flag = ");
    _cfsml_write_byte(fh, &(foo->version_lock_flag));
    fprintf(fh, "\n");
  fprintf(fh, "version = ");
    write_sci_version(fh, &(foo->version));
    fprintf(fh, "\n");
  fprintf(fh, "max_version = ");
    write_sci_version(fh, &(foo->max_version));
    fprintf(fh, "\n");
  fprintf(fh, "min_version = ");
    write_sci_version(fh, &(foo->min_version));
    fprintf(fh, "\n");
  fprintf(fh, "execution_stack = ");
    min = max = foo->execution_stack_size;
    if (foo->execution_stack_pos+1 < min)
       min = foo->execution_stack_pos+1;
    if (!foo->execution_stack)
       min = max = 0; /* Don't write if it points to NULL */
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_exec_stack_t(fh, &(foo->execution_stack[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "acc = ");
    _cfsml_write_gint16(fh, &(foo->acc));
    fprintf(fh, "\n");
  fprintf(fh, "amp_rest = ");
    _cfsml_write_gint16(fh, &(foo->amp_rest));
    fprintf(fh, "\n");
  fprintf(fh, "prev = ");
    _cfsml_write_gint16(fh, &(foo->prev));
    fprintf(fh, "\n");
  fprintf(fh, "stack_base = ");
    write_heapptr(fh, &(foo->stack_base));
    fprintf(fh, "\n");
  fprintf(fh, "stack_handle = ");
    write_heapptr(fh, &(foo->stack_handle));
    fprintf(fh, "\n");
  fprintf(fh, "parser_base = ");
    write_heapptr(fh, &(foo->parser_base));
    fprintf(fh, "\n");
  fprintf(fh, "global_vars = ");
    write_heapptr(fh, &(foo->global_vars));
    fprintf(fh, "\n");
  fprintf(fh, "parser_nodes = ");
    min = max = VOCAB_TREE_NODES;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      write_PTN(fh, &(foo->parser_nodes[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "game_obj = ");
    write_heapptr(fh, &(foo->game_obj));
    fprintf(fh, "\n");
  fprintf(fh, "classtable = ");
    min = max = foo->classtable_size;
    if (!foo->classtable)
       min = max = 0; /* Don't write if it points to NULL */
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_class_t(fh, &(foo->classtable[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "scripttable = ");
    min = max = 1000;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_script_t(fh, &(foo->scripttable[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "clone_list = ");
    min = max = SCRIPT_MAX_CLONES;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      write_heapptr(fh, &(foo->clone_list[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "_heap->first_free = ");
    _cfsml_write_int(fh, &(foo->_heap->first_free));
    fprintf(fh, "\n");
  fprintf(fh, "_heap->old_ff = ");
    _cfsml_write_int(fh, &(foo->_heap->old_ff));
    fprintf(fh, "\n");
  fprintf(fh, "_heap->base = ");
    fprintf(fh, "%d", foo->_heap->base - foo->_heap->start); /* Relative pointer */
    fprintf(fh, "\n");
  fprintf(fh, "game_name = ");
    fprintf(fh, "%d", foo->game_name - foo->_heap->start); /* Relative pointer */
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 381 "cfsml.pl"
static int
_cfsml_read_state_t(FILE *fh, state_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
  int reladdresses[2];
#line 435 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d\n",*line);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    bar = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!bar)
       return CFSML_FAILURE;
    if (!assignment) {
      if (!strcmp(bar, "}")) 
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(bar, "restarting_flags")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->restarting_flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "have_mouse_flag")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->have_mouse_flag), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pic_not_valid")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->pic_not_valid), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pic_is_new")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->pic_is_new), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "onscreen_console")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->onscreen_console), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "mouse_pointer_nr")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->mouse_pointer_nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "debug_mode")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->debug_mode), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "game_time")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_long(fh, &(foo->game_time), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "save_dir")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->save_dir), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "sound_object")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->sound_object), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "last_pointer_x")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->last_pointer_x), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "last_pointer_y")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->last_pointer_y), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "last_pointer_size_x")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->last_pointer_size_x), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "last_pointer_size_y")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->last_pointer_size_y), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "view_port")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->view_port), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ports")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MAX_PORTS;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (read_port_tp(fh, &(foo->ports[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "pic_visible_map")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->pic_visible_map), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pic_animate")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->pic_animate), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "dyn_view_port")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->dyn_view_port), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pic_views")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 497 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max)
           foo->pic_views = (view_object_t *) g_malloc(max * sizeof(view_object_t));
         else
           foo->pic_views = NULL;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_view_object_t(fh, &(foo->pic_views[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
         foo->pic_views_nr = max ; /* Set array size accordingly */
      } else
      if (!strcmp(bar, "dyn_views")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 497 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max)
           foo->dyn_views = (view_object_t *) g_malloc(max * sizeof(view_object_t));
         else
           foo->dyn_views = NULL;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_view_object_t(fh, &(foo->dyn_views[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
         foo->dyn_views_nr = max ; /* Set array size accordingly */
      } else
      if (!strcmp(bar, "animation_delay")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->animation_delay), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "hunk")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MAX_HUNK_BLOCKS;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (read_hunk_block(fh, &(foo->hunk[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "menubar")) {
#line 553 "cfsml.pl"
         if (read_menubar_tp(fh, &(foo->menubar), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "status_bar_text")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_string(fh, &(foo->status_bar_text), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "priority_first")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->priority_first), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "priority_last")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->priority_last), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "version_lock_flag")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->version_lock_flag), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "version")) {
#line 553 "cfsml.pl"
         if (read_sci_version(fh, &(foo->version), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "max_version")) {
#line 553 "cfsml.pl"
         if (read_sci_version(fh, &(foo->max_version), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "min_version")) {
#line 553 "cfsml.pl"
         if (read_sci_version(fh, &(foo->min_version), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "execution_stack")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 497 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max)
           foo->execution_stack = (exec_stack_t *) g_malloc(max * sizeof(exec_stack_t));
         else
           foo->execution_stack = NULL;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_exec_stack_t(fh, &(foo->execution_stack[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
         foo->execution_stack_size = max ; /* Set array size accordingly */
         foo->execution_stack_pos = i -1; /* Set number of elements */
      } else
      if (!strcmp(bar, "acc")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->acc), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "amp_rest")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->amp_rest), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "prev")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->prev), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "stack_base")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->stack_base), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "stack_handle")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->stack_handle), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "parser_base")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->parser_base), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "global_vars")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->global_vars), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "parser_nodes")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = VOCAB_TREE_NODES;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (read_PTN(fh, &(foo->parser_nodes[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "game_obj")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->game_obj), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "classtable")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 497 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max)
           foo->classtable = (class_t *) g_malloc(max * sizeof(class_t));
         else
           foo->classtable = NULL;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_class_t(fh, &(foo->classtable[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
         foo->classtable_size = max ; /* Set array size accordingly */
      } else
      if (!strcmp(bar, "scripttable")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = 1000;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_script_t(fh, &(foo->scripttable[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "clone_list")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = SCRIPT_MAX_CLONES;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (read_heapptr(fh, &(foo->clone_list[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "_heap->first_free")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->_heap->first_free), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "_heap->old_ff")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->_heap->old_ff), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "_heap->base")) {
#line 476 "cfsml.pl"
         if (_cfsml_read_int(fh, &(reladdresses[0]), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "game_name")) {
#line 476 "cfsml.pl"
         if (_cfsml_read_int(fh, &(reladdresses[1]), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 560 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }

    g_free (bar);
  } while (!closed); /* Until closing braces are hit */
  foo->_heap->base = foo->_heap->start + reladdresses[0];
  foo->game_name = foo->_heap->start + reladdresses[1];
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_view_object_t(FILE *fh, view_object_t* foo)
{
  char *bar;
  int min, max, i;

#line 315 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "x = ");
    _cfsml_write_int(fh, &(foo->x));
    fprintf(fh, "\n");
  fprintf(fh, "y = ");
    _cfsml_write_int(fh, &(foo->y));
    fprintf(fh, "\n");
  fprintf(fh, "priority = ");
    _cfsml_write_int(fh, &(foo->priority));
    fprintf(fh, "\n");
  fprintf(fh, "view_nr = ");
    _cfsml_write_int(fh, &(foo->view_nr));
    fprintf(fh, "\n");
  fprintf(fh, "loop = ");
    _cfsml_write_int(fh, &(foo->loop));
    fprintf(fh, "\n");
  fprintf(fh, "cel = ");
    _cfsml_write_int(fh, &(foo->cel));
    fprintf(fh, "\n");
  fprintf(fh, "nsTop = ");
    _cfsml_write_int(fh, &(foo->nsTop));
    fprintf(fh, "\n");
  fprintf(fh, "nsLeft = ");
    _cfsml_write_int(fh, &(foo->nsLeft));
    fprintf(fh, "\n");
  fprintf(fh, "nsRight = ");
    _cfsml_write_int(fh, &(foo->nsRight));
    fprintf(fh, "\n");
  fprintf(fh, "nsBottom = ");
    _cfsml_write_int(fh, &(foo->nsBottom));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 381 "cfsml.pl"
static int
_cfsml_read_view_object_t(FILE *fh, view_object_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 435 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d\n",*line);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    bar = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!bar)
       return CFSML_FAILURE;
    if (!assignment) {
      if (!strcmp(bar, "}")) 
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(bar, "x")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->x), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "y")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->y), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "priority")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "view_nr")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->view_nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "loop")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->loop), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "cel")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->cel), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "nsTop")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->nsTop), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "nsLeft")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->nsLeft), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "nsRight")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->nsRight), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "nsBottom")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->nsBottom), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 560 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }

    g_free (bar);
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_byte(FILE *fh, byte* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 381 "cfsml.pl"
static int
_cfsml_read_byte(FILE *fh, byte* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 405 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_menu_item_t(FILE *fh, menu_item_t* foo)
{
  char *bar;
  int min, max, i;

#line 315 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "type = ");
    _cfsml_write_int(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "keytext = ");
    _cfsml_write_string(fh, &(foo->keytext));
    fprintf(fh, "\n");
  fprintf(fh, "keytext_size = ");
    _cfsml_write_int(fh, &(foo->keytext_size));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "said = ");
    min = max = 8;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_byte(fh, &(foo->said[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "said_pos = ");
    write_heapptr(fh, &(foo->said_pos));
    fprintf(fh, "\n");
  fprintf(fh, "text = ");
    _cfsml_write_string(fh, &(foo->text));
    fprintf(fh, "\n");
  fprintf(fh, "text_pos = ");
    write_heapptr(fh, &(foo->text_pos));
    fprintf(fh, "\n");
  fprintf(fh, "modifiers = ");
    _cfsml_write_int(fh, &(foo->modifiers));
    fprintf(fh, "\n");
  fprintf(fh, "key = ");
    _cfsml_write_int(fh, &(foo->key));
    fprintf(fh, "\n");
  fprintf(fh, "enabled = ");
    _cfsml_write_int(fh, &(foo->enabled));
    fprintf(fh, "\n");
  fprintf(fh, "tag = ");
    _cfsml_write_int(fh, &(foo->tag));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 381 "cfsml.pl"
static int
_cfsml_read_menu_item_t(FILE *fh, menu_item_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 435 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d\n",*line);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    bar = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!bar)
       return CFSML_FAILURE;
    if (!assignment) {
      if (!strcmp(bar, "}")) 
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(bar, "type")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "keytext")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_string(fh, &(foo->keytext), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "keytext_size")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->keytext_size), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "said")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = 8;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_byte(fh, &(foo->said[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "said_pos")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->said_pos), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "text")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_string(fh, &(foo->text), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "text_pos")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->text_pos), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "modifiers")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->modifiers), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "key")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->key), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "enabled")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->enabled), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "tag")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->tag), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 560 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }

    g_free (bar);
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_string(FILE *fh, char ** foo)
{
  char *bar;
  int min, max, i;

#line 305 "cfsml.pl"
  if (!(*foo))
    fprintf(fh, "\\null\\");  else {
    bar = _cfsml_mangle_string((char *) *foo);
    fprintf(fh, "\"%s\"", bar);
    g_free(bar);
  }
}

#line 381 "cfsml.pl"
static int
_cfsml_read_string(FILE *fh, char ** foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 414 "cfsml.pl"

  if (strcmp(lastval, "\\null\\")) { /* null pointer? */
    if (*lastval == '"') { /* Quoted string? */
      int seeker = strlen(lastval);

      while (lastval[seeker] != '"')
        --seeker;

      if (!seeker) { /* No matching double-quotes? */
        _cfsml_error("Unbalanced quotes at line %d\n", *line);
        return CFSML_FAILURE;
      }

      lastval[seeker] = 0; /* Terminate string at closing quotes... */
      lastval++; /* ...and skip the opening quotes locally */
    }
    *foo = _cfsml_unmangle_string(lastval);
    return CFSML_SUCCESS;
  } else {
    *foo = NULL;
    return CFSML_SUCCESS;
  }
}

#line 297 "cfsml.pl"
static void
_cfsml_write_port_t(FILE *fh, port_t* foo)
{
  char *bar;
  int min, max, i;

#line 315 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "ymin = ");
    _cfsml_write_gint16(fh, &(foo->ymin));
    fprintf(fh, "\n");
  fprintf(fh, "xmin = ");
    _cfsml_write_gint16(fh, &(foo->xmin));
    fprintf(fh, "\n");
  fprintf(fh, "ymax = ");
    _cfsml_write_gint16(fh, &(foo->ymax));
    fprintf(fh, "\n");
  fprintf(fh, "xmax = ");
    _cfsml_write_gint16(fh, &(foo->xmax));
    fprintf(fh, "\n");
  fprintf(fh, "title = ");
    write_heapptr(fh, &(foo->title));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_gint16(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "alignment = ");
    _cfsml_write_int(fh, &(foo->alignment));
    fprintf(fh, "\n");
  fprintf(fh, "x = ");
    _cfsml_write_int(fh, &(foo->x));
    fprintf(fh, "\n");
  fprintf(fh, "y = ");
    _cfsml_write_int(fh, &(foo->y));
    fprintf(fh, "\n");
  fprintf(fh, "font_nr = ");
    _cfsml_write_int(fh, &(foo->font_nr));
    fprintf(fh, "\n");
  fprintf(fh, "priority = ");
    _cfsml_write_gint16(fh, &(foo->priority));
    fprintf(fh, "\n");
  fprintf(fh, "bgcolor = ");
    _cfsml_write_gint16(fh, &(foo->bgcolor));
    fprintf(fh, "\n");
  fprintf(fh, "color = ");
    _cfsml_write_gint16(fh, &(foo->color));
    fprintf(fh, "\n");
  fprintf(fh, "gray_text = ");
    _cfsml_write_byte(fh, &(foo->gray_text));
    fprintf(fh, "\n");
  fprintf(fh, "bg_handle = ");
    _cfsml_write_int(fh, &(foo->bg_handle));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 381 "cfsml.pl"
static int
_cfsml_read_port_t(FILE *fh, port_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 435 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d\n",*line);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    bar = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!bar)
       return CFSML_FAILURE;
    if (!assignment) {
      if (!strcmp(bar, "}")) 
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(bar, "ymin")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->ymin), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "xmin")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->xmin), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ymax")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->ymax), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "xmax")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->xmax), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "title")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->title), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "alignment")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->alignment), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "x")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->x), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "y")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->y), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "font_nr")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->font_nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "priority")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bgcolor")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->bgcolor), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "color")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->color), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "gray_text")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->gray_text), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bg_handle")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->bg_handle), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 560 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }

    g_free (bar);
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_script_t(FILE *fh, script_t* foo)
{
  char *bar;
  int min, max, i;

#line 315 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "heappos = ");
    write_heapptr(fh, &(foo->heappos));
    fprintf(fh, "\n");
  fprintf(fh, "localvar_offset = ");
    write_heapptr(fh, &(foo->localvar_offset));
    fprintf(fh, "\n");
  fprintf(fh, "export_table_offset = ");
    write_heapptr(fh, &(foo->export_table_offset));
    fprintf(fh, "\n");
  fprintf(fh, "lockers = ");
    _cfsml_write_int(fh, &(foo->lockers));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 381 "cfsml.pl"
static int
_cfsml_read_script_t(FILE *fh, script_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 435 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d\n",*line);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    bar = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!bar)
       return CFSML_FAILURE;
    if (!assignment) {
      if (!strcmp(bar, "}")) 
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(bar, "heappos")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->heappos), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "localvar_offset")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->localvar_offset), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "export_table_offset")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->export_table_offset), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "lockers")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->lockers), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 560 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }

    g_free (bar);
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_menu_t(FILE *fh, menu_t* foo)
{
  char *bar;
  int min, max, i;

#line 315 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "title = ");
    _cfsml_write_string(fh, &(foo->title));
    fprintf(fh, "\n");
  fprintf(fh, "title_width = ");
    _cfsml_write_int(fh, &(foo->title_width));
    fprintf(fh, "\n");
  fprintf(fh, "width = ");
    _cfsml_write_int(fh, &(foo->width));
    fprintf(fh, "\n");
  fprintf(fh, "items = ");
    min = max = foo->items_nr;
    if (!foo->items)
       min = max = 0; /* Don't write if it points to NULL */
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_menu_item_t(fh, &(foo->items[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 381 "cfsml.pl"
static int
_cfsml_read_menu_t(FILE *fh, menu_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 435 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d\n",*line);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    bar = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!bar)
       return CFSML_FAILURE;
    if (!assignment) {
      if (!strcmp(bar, "}")) 
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(bar, "title")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_string(fh, &(foo->title), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "title_width")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->title_width), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "width")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->width), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "items")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 497 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max)
           foo->items = (menu_item_t *) g_malloc(max * sizeof(menu_item_t));
         else
           foo->items = NULL;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_menu_item_t(fh, &(foo->items[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
         foo->items_nr = max ; /* Set array size accordingly */
      } else
#line 560 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }

    g_free (bar);
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_class_t(FILE *fh, class_t* foo)
{
  char *bar;
  int min, max, i;

#line 315 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "script = ");
    _cfsml_write_int(fh, &(foo->script));
    fprintf(fh, "\n");
  fprintf(fh, "class_offset = ");
    _cfsml_write_int(fh, &(foo->class_offset));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 381 "cfsml.pl"
static int
_cfsml_read_class_t(FILE *fh, class_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 435 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d\n",*line);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    bar = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!bar)
       return CFSML_FAILURE;
    if (!assignment) {
      if (!strcmp(bar, "}")) 
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(bar, "script")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->script), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "class_offset")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->class_offset), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 560 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }

    g_free (bar);
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_gint16(FILE *fh, gint16* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 381 "cfsml.pl"
static int
_cfsml_read_gint16(FILE *fh, gint16* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 405 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_int(FILE *fh, int* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 381 "cfsml.pl"
static int
_cfsml_read_int(FILE *fh, int* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 405 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_exec_stack_t(FILE *fh, exec_stack_t* foo)
{
  char *bar;
  int min, max, i;

#line 315 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "objp = ");
    write_heapptr(fh, &(foo->objp));
    fprintf(fh, "\n");
  fprintf(fh, "sendp = ");
    write_heapptr(fh, &(foo->sendp));
    fprintf(fh, "\n");
  fprintf(fh, "pc = ");
    write_heapptr(fh, &(foo->pc));
    fprintf(fh, "\n");
  fprintf(fh, "sp = ");
    write_heapptr(fh, &(foo->sp));
    fprintf(fh, "\n");
  fprintf(fh, "argc = ");
    _cfsml_write_int(fh, &(foo->argc));
    fprintf(fh, "\n");
  fprintf(fh, "variables = ");
    min = max = 4;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      write_heapptr(fh, &(foo->variables[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "selector = ");
    _cfsml_write_int(fh, &(foo->selector));
    fprintf(fh, "\n");
  fprintf(fh, "origin = ");
    _cfsml_write_int(fh, &(foo->origin));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    write_heapptr(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 381 "cfsml.pl"
static int
_cfsml_read_exec_stack_t(FILE *fh, exec_stack_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 435 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d\n",*line);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    bar = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!bar)
       return CFSML_FAILURE;
    if (!assignment) {
      if (!strcmp(bar, "}")) 
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(bar, "objp")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->objp), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "sendp")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->sendp), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pc")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->pc), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "sp")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->sp), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "argc")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->argc), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "variables")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = 4;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (read_heapptr(fh, &(foo->variables[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "selector")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->selector), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "origin")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->origin), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 553 "cfsml.pl"
         if (read_heapptr(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 560 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }

    g_free (bar);
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_menubar_t(FILE *fh, menubar_t* foo)
{
  char *bar;
  int min, max, i;

#line 315 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "menus = ");
    min = max = foo->menus_nr;
    if (!foo->menus)
       min = max = 0; /* Don't write if it points to NULL */
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_menu_t(fh, &(foo->menus[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 381 "cfsml.pl"
static int
_cfsml_read_menubar_t(FILE *fh, menubar_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 435 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d\n",*line);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    bar = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!bar)
       return CFSML_FAILURE;
    if (!assignment) {
      if (!strcmp(bar, "}")) 
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(bar, "menus")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 497 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max)
           foo->menus = (menu_t *) g_malloc(max * sizeof(menu_t));
         else
           foo->menus = NULL;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           g_free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_menu_t(fh, &(foo->menus[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
         foo->menus_nr = max ; /* Set array size accordingly */
      } else
#line 560 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }

    g_free (bar);
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}


/* Auto-generated CFSML declaration and function block ends here */
/* Auto-generation performed by cfsml.pl 0.6.7 */
#line 378 "CFSML input file"



void
write_menubar_tp(FILE *fh, menubar_t **foo)
{
  if (*foo) {

#line 661 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_menubar_t(fh, (*foo));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 388 "CFSML input file"

  } else { /* Nothing to write */
    fputs("\\null\\", fh);
  }
}


int
read_menubar_tp(FILE *fh, menubar_t **foo, char *lastval, int *line, int *hiteof)
{

  if (lastval[0] == '\\') {
    *foo = NULL; /* No menu bar */
  } else {

    *foo = (menubar_t *) malloc(sizeof(menubar_t));
/* Auto-generated CFSML data reader code */
#line 626 "cfsml.pl"
  {
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
    char *_cfsml_inp = lastval;
    _cfsml_error = _cfsml_read_menubar_t(fh, (*foo), _cfsml_inp, &(*line), &_cfsml_eof);
    *hiteof = _cfsml_error;
  }
/* End of auto-generated CFSML data reader code */
#line 405 "CFSML input file"

  }
  return *hiteof;
}

void
write_port_tp(FILE *fh, port_t **foo)
{
  if (*foo) {

#line 661 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_port_t(fh, (*foo));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 416 "CFSML input file"

  } else { /* Nothing to write */
    fputs("\\null\\", fh);
  }
}


int
read_port_tp(FILE *fh, port_t **foo, char *lastval, int *line, int *hiteof)
{
  resource_t *res;

  if (lastval[0] == '\\') {
    *foo = NULL; /* No port */
  } else {

    *foo = (port_t *) malloc(sizeof(port_t));
/* Auto-generated CFSML data reader code */
#line 626 "cfsml.pl"
  {
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
    char *_cfsml_inp = lastval;
    _cfsml_error = _cfsml_read_port_t(fh, (*foo), _cfsml_inp, &(*line), &_cfsml_eof);
    *hiteof = _cfsml_error;
  }
/* End of auto-generated CFSML data reader code */
#line 434 "CFSML input file"

    res =  findResource(sci_font, (*foo)->font_nr);
    if (res)
      (*foo)->font = res->data;
    else
      (*foo)->font = NULL;

  }
  return *hiteof;
}



/* This function is called to undo some strange stuff done in preparation
** to writing a gamestate to disk
*/
void
_gamestate_unfrob(state_t *s)
{ /* Remnant of ancient times. May be restored to its full glory one day. */
  s->heap = s->_heap->start;
}


int
gamestate_save(state_t *s, char *dirname)
{
  FILE *fh;
  DIR *dir;
  struct dirent* dirent;
  int fd;

  _global_save_state = s;

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

#line 661 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_state_t(fh, s);
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 500 "CFSML input file"

  fclose(fh);

  _gamestate_unfrob(s);

  fd = creat("heap", 0600);
  write(fd, s->_heap->start, SCI_HEAP_SIZE);
  close(fd);

  png_save_pic(s->pic);

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

  _global_save_state = s;

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

  fh = fopen("state", "r");
  if (!fh) {
    heap_del(retval->_heap);
    free(retval);
    return NULL;
  }

  retval->amp_rest = 0; /* Backwards compatibility */

/* Auto-generated CFSML data reader code */
#line 626 "cfsml.pl"
  {
    int _cfsml_line_ctr = 0;
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(_cfsml_line_ctr), &_cfsml_eof, &dummy);

    _cfsml_error = _cfsml_read_state_t(fh, retval, _cfsml_inp, &(_cfsml_line_ctr), &_cfsml_eof);
    g_free(_cfsml_inp);
    read_eof = _cfsml_error;
  }
/* End of auto-generated CFSML data reader code */
#line 552 "CFSML input file"

  fclose(fh);

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

  if (png_load_pic(s->pic)) {
    sciprintf("Picture data does not exist for game state '%s'\n", dirname);
    chdir("..");
    heap_del(retval->_heap);
    free(retval);
    return NULL;
  }

  retval->pic = s->pic; /* Continue using that pic */

  _gamestate_unfrob(retval);

  /* Set exec stack base to zero */
  retval->execution_stack_base = 0;

  /* Set the class table pointers to the script positions */
  for (i = 0; i < retval->classtable_size; i++)
    retval->classtable[i].scriptposp = &(retval->scripttable[retval->classtable[i].script].heappos);

  /* Set views in pic_views */
  for (i = 0; i < retval->pic_views_nr; i++) {
    resource_t *res = findResource(sci_view, retval->pic_views[i].view_nr);
    retval->pic_views[i].view = (res? res->data : NULL);
  }

  /* Set views in dyn_views */
  for (i = 0; i < retval->dyn_views_nr; i++) {
    resource_t *res = findResource(sci_view, retval->dyn_views[i].view_nr);
    retval->dyn_views[i].view = (res? res->data : NULL);
  }

  /* Now set all standard ports */
  memcpy(&(retval->wm_port), retval->ports[0], sizeof(port_t));
  memcpy(&(retval->titlebar_port), retval->ports[1], sizeof(port_t));
  memcpy(&(retval->picture_port), retval->ports[2], sizeof(port_t));

  for (i = 0; i < 3; i++)
    free(retval->ports[i]); /* Free those ports... */

  /* ...and let them point to where they belong */
  retval->ports[0] = &(retval->wm_port);
  retval->ports[1] = &(retval->titlebar_port);
  retval->ports[2] = &(retval->picture_port);

  /* Map port fonts */
  for (i = 0; i < MAX_PORTS; i++)
    if (retval->ports[i]) {
      resource_t *resource = findResource(sci_font, retval->ports[i]->font_nr);
      if (resource)
	retval->ports[i]->font = resource->data;
  }

  /* Now copy all current state information */
  /* Graphics and input state: */
  retval->gfx_driver = s->gfx_driver;
  memcpy(&(retval->graphics), &(s->graphics), sizeof (s->graphics));

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

  /* Mouse pointer */
  retval->pointer_x = s->pointer_x;
  retval->pointer_y = s->pointer_y;
  if (retval->mouse_pointer_nr == -1)
    retval->mouse_pointer = NULL;
  else {
    resource_t *resource = findResource(sci_cursor, retval->mouse_pointer_nr);
    byte *data = (resource)? resource->data : NULL;
    if (data)
      retval->mouse_pointer = calc_mouse_cursor(data);
    else {
      retval->mouse_pointer = NULL;
      retval->mouse_pointer_nr = -1;
    }
  }

  memcpy(&(retval->selector_map), &(s->selector_map), sizeof(selector_map_t));

  /* Copy breakpoint information from current game instance */
  retval->have_bp = s->have_bp;
  retval->bp_list = s->bp_list;

  retval->successor = NULL;

  chdir ("..");

  return retval;
}
