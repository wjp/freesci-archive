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

#include <sci_memory.h>
#include <gfx_operations.h>
#include <engine.h>
#include <assert.h>
#include <heap.h>

#ifdef _MSC_VER
#include <direct.h>
#endif

#ifdef _WIN32
#pragma warning( disable : 4101 )
#endif

#define HUNK_TYPE_GFX_SNAPSHOT_STRING "g\n"

/* Missing:
** - SFXdriver
** - File input/output state (this is likely not to happen)
*/

static state_t *_global_save_state;
/* Needed for some graphical stuff. */
#define FILE_VERSION _global_save_state->savegame_version


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
		if (sscanf(lastval, "B ( %hd , %hd )", &(foo->content.branches[0]), &(foo->content.branches[1])) < 2)
			return 1; /* Failed to parse everything */
		return 0;
	} else return 1; /* failure to parse anything */
}


void
write_menubar_tp(FILE *fh, menubar_t **foo);
int
read_menubar_tp(FILE *fh, menubar_t **foo, char *lastval, int *line, int *hiteof);

void
write_any_widget(FILE *fh, gfxw_widget_t **widget);
int
read_any_widget(FILE *fh, gfxw_widget_t **widget, char *lastval, int *line, int *hiteof);

void
write_pixmap_color(FILE *fh, gfx_pixmap_color_t *color);
int
read_pixmap_color(FILE *fh, gfx_pixmap_color_t *color, char *lastval, int *line, int *hiteof);

void
write_hunk_block(FILE *fh, hunk_block_t *foo);
int
read_hunk_block(FILE *fh, hunk_block_t *foo, char *lastval, int *line, int *hiteof);


/* Auto-generated CFSML declaration and function block */

#line 796 "savegame.cfsml"
#define CFSML_SUCCESS 0
#define CFSML_FAILURE 1

#line 102 "savegame.cfsml"

#include <stdarg.h> /* We need va_lists */
#include <sci_memory.h>

#ifdef CFSML_DEBUG_MALLOC
/*
#define free(p)        dbg_sci_free(p)
#define malloc(s)      dbg_sci_malloc(s)
#define calloc(n, s)   dbg_sci_calloc(n, s)
#define realloc(p, s)  dbg_sci_realloc(p, s)
*/
#define free        dbg_sci_free
#define malloc      dbg_sci_malloc
#define calloc      dbg_sci_calloc
#define realloc     dbg_sci_realloc
#endif

static void
_cfsml_error(char *fmt, ...)
{
  va_list argp;

  fprintf(stderr, "Error: ");
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);

}


static struct _cfsml_pointer_refstruct {
    struct _cfsml_pointer_refstruct *next;
    void *ptr;
} *_cfsml_pointer_references = NULL;

static struct _cfsml_pointer_refstruct **_cfsml_pointer_references_current = &_cfsml_pointer_references;

static char *_cfsml_last_value_retreived = NULL;
static char *_cfsml_last_identifier_retreived = NULL;

static void
_cfsml_free_pointer_references_recursively(struct _cfsml_pointer_refstruct *refs, int free_pointers)
{
    if (!refs)
	return;
    #ifdef CFSML_DEBUG_MALLOC
    SCI_MEMTEST;
    #endif

    _cfsml_free_pointer_references_recursively(refs->next, free_pointers);
    #ifdef CFSML_DEBUG_MALLOC
    SCI_MEMTEST;

    fprintf(stderr,"Freeing ptrref %p [%p] %s\n", refs->ptr, refs, free_pointers?
	    "ALL": "cleanup only");
    #endif

    if (free_pointers)
	free(refs->ptr);

    #ifdef CFSML_DEBUG_MALLOC
    SCI_MEMTEST;
    #endif
    free(refs);
    #ifdef CFSML_DEBUG_MALLOC
    SCI_MEMTEST;
    #endif
}

static void
_cfsml_free_pointer_references(struct _cfsml_pointer_refstruct **meta_ref, int free_pointers)
{
    _cfsml_free_pointer_references_recursively(*meta_ref, free_pointers);
    *meta_ref = NULL;
    _cfsml_pointer_references_current = meta_ref;
}

static struct _cfsml_pointer_refstruct **
_cfsml_get_current_refpointer()
{
    return _cfsml_pointer_references_current;
}

static void _cfsml_register_pointer(void *ptr)
{
    struct _cfsml_pointer_refstruct *newref = sci_malloc(sizeof (struct _cfsml_pointer_refstruct));
    #ifdef CFSML_DEBUG_MALLOC
    SCI_MEMTEST;
    fprintf(stderr,"Registering ptrref %p [%p]\n", ptr, newref);
    #endif
    newref->next = *_cfsml_pointer_references_current;
    newref->ptr = ptr;
    *_cfsml_pointer_references_current = newref;
}


static char *
_cfsml_mangle_string(char *s)
{
  char *source = s;
  char c;
  char *target = (char *) sci_malloc(1 + strlen(s) * 2); /* We will probably need less than that */
  char *writer = target;

  while ((c = *source++)) {

    if (c < 32) { /* Special character? */
      *writer++ = '\\'; /* Escape... */
      c += ('a' - 1);
    } else if (c == '\\' || c == '"')
      *writer++ = '\\'; /* Escape, but do not change */
    *writer++ = c;

  }
  *writer = 0; /* Terminate string */

  return (char *) sci_realloc(target, strlen(target) + 1);
}


static char *
_cfsml_unmangle_string(char *s)
{
  char *target = (char *) sci_malloc(1 + strlen(s));
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

  return (char *) sci_realloc(target, strlen(target) + 1);
}


static char *
_cfsml_get_identifier(FILE *fd, int *line, int *hiteof, int *assignment)
{
  int c;
  int mem = 32;
  int pos = 0;
  int done = 0;
  char *retval = (char *) sci_malloc(mem);

  if (_cfsml_last_identifier_retreived) {
      free(_cfsml_last_identifier_retreived);
      _cfsml_last_identifier_retreived = NULL;
  }

  while (isspace(c = fgetc(fd)) && (c != EOF));
  if (c == EOF) {
    _cfsml_error("Unexpected end of file at line %d\n", *line);
    free(retval);
    *hiteof = 1;
    return NULL;
  }

  ungetc(c, fd);

  while (((c = fgetc(fd)) != EOF) && ((pos == 0) || (c != '\n')) && (c != '=')) {

     if (pos == mem - 1) /* Need more memory? */
       retval = (char *) sci_realloc(retval, mem *= 2);

     if (!isspace(c)) {
        if (done) {
           _cfsml_error("Single word identifier expected at line %d\n", *line);
           free(retval);
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
    free(retval);
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
    free(retval);
    return NULL;
  }

  if (pos == mem - 1) /* Need more memory? */
     retval = (char *) sci_realloc(retval, mem += 1);

  retval[pos] = 0; /* Terminate string */
#line 322 "savegame.cfsml"

  return _cfsml_last_identifier_retreived = retval;
}


static char *
_cfsml_get_value(FILE *fd, int *line, int *hiteof)
{
  int c;
  int mem = 64;
  int pos = 0;
  char *retval = (char *) sci_malloc(mem);

  if (_cfsml_last_value_retreived) {
      free(_cfsml_last_value_retreived);
      _cfsml_last_value_retreived = NULL;
  }

  while (((c = fgetc(fd)) != EOF) && (c != '\n')) {

     if (pos == mem - 1) /* Need more memory? */
       retval = (char *) sci_realloc(retval, mem *= 2);

     if (pos || (!isspace(c)))
        retval[pos++] = c;

  }

  while ((pos > 0) && (isspace(retval[pos - 1])))
     --pos; /* Strip trailing whitespace */

  if (c == EOF)
    *hiteof = 1;

  if (pos == 0) {
    _cfsml_error("Missing value in assignment at line %d\n", *line);
    free(retval);
    return NULL;
  }

  if (c == '\n')
     ++(*line);

  if (pos == mem - 1) /* Need more memory? */
    retval = (char *) sci_realloc(retval, mem += 1);

  retval[pos] = 0; /* Terminate string */
#line 379 "savegame.cfsml"
  return (_cfsml_last_value_retreived = (char *) sci_realloc(retval, strlen(retval) + 1));
  /* Re-allocate; this value might be used for quite some while (if we are
  ** restoring a string)
  */
}
#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_container_t(FILE *fh, gfxw_container_t* save_struc);
static int
_cfsml_read_gfxw_container_t(FILE *fh, gfxw_container_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_menu_t(FILE *fh, menu_t* save_struc);
static int
_cfsml_read_menu_t(FILE *fh, menu_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_widget_t(FILE *fh, gfxw_widget_t* save_struc);
static int
_cfsml_read_gfxw_widget_t(FILE *fh, gfxw_widget_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_primitive_t(FILE *fh, gfxw_primitive_t* save_struc);
static int
_cfsml_read_gfxw_primitive_t(FILE *fh, gfxw_primitive_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_widget_types_t(FILE *fh, gfxw_widget_types_t* save_struc);
static int
_cfsml_read_gfxw_widget_types_t(FILE *fh, gfxw_widget_types_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_drawn_pic_t(FILE *fh, drawn_pic_t* save_struc);
static int
_cfsml_read_drawn_pic_t(FILE *fh, drawn_pic_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_menubar_t(FILE *fh, menubar_t* save_struc);
static int
_cfsml_read_menubar_t(FILE *fh, menubar_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_string(FILE *fh, char ** save_struc);
static int
_cfsml_read_string(FILE *fh, char ** save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_view_t(FILE *fh, gfxw_view_t* save_struc);
static int
_cfsml_read_gfxw_view_t(FILE *fh, gfxw_view_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_point_t(FILE *fh, point_t* save_struc);
static int
_cfsml_read_point_t(FILE *fh, point_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_text_t(FILE *fh, gfxw_text_t* save_struc);
static int
_cfsml_read_gfxw_text_t(FILE *fh, gfxw_text_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfx_pixmap_color_t(FILE *fh, gfx_pixmap_color_t* save_struc);
static int
_cfsml_read_gfx_pixmap_color_t(FILE *fh, gfx_pixmap_color_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_port_t(FILE *fh, gfxw_port_t* save_struc);
static int
_cfsml_read_gfxw_port_t(FILE *fh, gfxw_port_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_visual_t(FILE *fh, gfxw_visual_t* save_struc);
static int
_cfsml_read_gfxw_visual_t(FILE *fh, gfxw_visual_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_dyn_view_t(FILE *fh, gfxw_dyn_view_t* save_struc);
static int
_cfsml_read_gfxw_dyn_view_t(FILE *fh, gfxw_dyn_view_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfx_line_mode_t(FILE *fh, gfx_line_mode_t* save_struc);
static int
_cfsml_read_gfx_line_mode_t(FILE *fh, gfx_line_mode_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfx_box_shade_t(FILE *fh, gfx_box_shade_t* save_struc);
static int
_cfsml_read_gfx_box_shade_t(FILE *fh, gfx_box_shade_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_list_t(FILE *fh, gfxw_list_t* save_struc);
static int
_cfsml_read_gfxw_list_t(FILE *fh, gfxw_list_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_snapshot_t(FILE *fh, gfxw_snapshot_t* save_struc);
static int
_cfsml_read_gfxw_snapshot_t(FILE *fh, gfxw_snapshot_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_state_t(FILE *fh, state_t* save_struc);
static int
_cfsml_read_state_t(FILE *fh, state_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_menu_item_t(FILE *fh, menu_item_t* save_struc);
static int
_cfsml_read_menu_item_t(FILE *fh, menu_item_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfx_color_t(FILE *fh, gfx_color_t* save_struc);
static int
_cfsml_read_gfx_color_t(FILE *fh, gfx_color_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_long(FILE *fh, long* save_struc);
static int
_cfsml_read_long(FILE *fh, long* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_byte(FILE *fh, byte* save_struc);
static int
_cfsml_read_byte(FILE *fh, byte* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_script_t(FILE *fh, script_t* save_struc);
static int
_cfsml_read_script_t(FILE *fh, script_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_class_t(FILE *fh, class_t* save_struc);
static int
_cfsml_read_class_t(FILE *fh, class_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_synonym_t(FILE *fh, synonym_t* save_struc);
static int
_cfsml_read_synonym_t(FILE *fh, synonym_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_exec_stack_t(FILE *fh, exec_stack_t* save_struc);
static int
_cfsml_read_exec_stack_t(FILE *fh, exec_stack_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfx_line_style_t(FILE *fh, gfx_line_style_t* save_struc);
static int
_cfsml_read_gfx_line_style_t(FILE *fh, gfx_line_style_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gint16(FILE *fh, gint16* save_struc);
static int
_cfsml_read_gint16(FILE *fh, gint16* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_rect_t(FILE *fh, rect_t* save_struc);
static int
_cfsml_read_rect_t(FILE *fh, rect_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfx_alignment_t(FILE *fh, gfx_alignment_t* save_struc);
static int
_cfsml_read_gfx_alignment_t(FILE *fh, gfx_alignment_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_int(FILE *fh, int* save_struc);
static int
_cfsml_read_int(FILE *fh, int* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfxw_box_t(FILE *fh, gfxw_box_t* save_struc);
static int
_cfsml_read_gfxw_box_t(FILE *fh, gfxw_box_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_gfx_dirty_rect_t(FILE *fh, gfx_dirty_rect_t* save_struc);
static int
_cfsml_read_gfx_dirty_rect_t(FILE *fh, gfx_dirty_rect_t* save_struc, char *lastval, int *line, int *hiteof);

#line 431 "savegame.cfsml"
static void
_cfsml_write_view_object_t(FILE *fh, view_object_t* save_struc);
static int
_cfsml_read_view_object_t(FILE *fh, view_object_t* save_struc, char *lastval, int *line, int *hiteof);

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_container_t(FILE *fh, gfxw_container_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(save_struc->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(save_struc->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(save_struc->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(save_struc->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(save_struc->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(save_struc->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(save_struc->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "zone = ");
    _cfsml_write_rect_t(fh, &(save_struc->zone));
    fprintf(fh, "\n");
  fprintf(fh, "dirty = ");
    if (!save_struc->dirty)
      fprintf(fh, "\\null\\");
    else
      _cfsml_write_gfx_dirty_rect_t(fh, save_struc->dirty);
    fprintf(fh, "\n");
  fprintf(fh, "contents = ");
    write_any_widget(fh, &(save_struc->contents));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_container_t(FILE *fh, gfxw_container_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "magic")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->magic), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for magic at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfxw_widget_types_t() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bounds")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->bounds), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for bounds at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "next")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->next), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for next at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "ID")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->ID), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for ID at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "widget_priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->widget_priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for widget_priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "zone")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->zone), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for zone at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "dirty")) {
#line 738 "savegame.cfsml"
        if (strcmp(value, "\\null\\")) { /* null pointer? */
           save_struc->dirty = sci_malloc(sizeof (gfx_dirty_rect_t));
           _cfsml_register_pointer(save_struc->dirty);
           if (_cfsml_read_gfx_dirty_rect_t(fh, save_struc->dirty, value, line, hiteof)) {
              _cfsml_error("Token expected by _cfsml_read_gfx_dirty_rect_t() for dirty at line %d\n", *line);
              return CFSML_FAILURE;
           }
        } else save_struc->dirty = NULL;
      } else
      if (!strcmp(token, "contents")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->contents), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for contents at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_menu_t(FILE *fh, menu_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "title = ");
    _cfsml_write_string(fh, &(save_struc->title));
    fprintf(fh, "\n");
  fprintf(fh, "title_width = ");
    _cfsml_write_int(fh, &(save_struc->title_width));
    fprintf(fh, "\n");
  fprintf(fh, "width = ");
    _cfsml_write_int(fh, &(save_struc->width));
    fprintf(fh, "\n");
  fprintf(fh, "items = ");
    min = max = save_struc->items_nr;
    if (!save_struc->items)
       min = max = 0; /* Don't write if it points to NULL */
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_menu_item_t(fh, &(save_struc->items[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_menu_t(FILE *fh, menu_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "title")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_string(fh, &(save_struc->title), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_string() for title at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "title_width")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->title_width), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for title_width at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "width")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->width), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for width at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "items")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
#line 673 "savegame.cfsml"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", token, *line);
            return CFSML_FAILURE;
         }

         if (max) {
           save_struc->items = (menu_item_t *) sci_malloc(max * sizeof(menu_item_t));
#ifdef SATISFY_PURIFY
           memset(save_struc->items, 0, max * sizeof(menu_item_t));
#endif
           _cfsml_register_pointer(save_struc->items);
         }
         else
           save_struc->items = NULL;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_menu_item_t(fh, &(save_struc->items[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by _cfsml_read_menu_item_t() for items[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
         save_struc->items_nr = max ; /* Set array size accordingly */
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_widget_t(FILE *fh, gfxw_widget_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(save_struc->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(save_struc->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(save_struc->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(save_struc->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(save_struc->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(save_struc->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(save_struc->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_widget_t(FILE *fh, gfxw_widget_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "magic")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->magic), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for magic at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfxw_widget_types_t() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bounds")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->bounds), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for bounds at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "next")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->next), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for next at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "ID")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->ID), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for ID at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "widget_priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->widget_priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for widget_priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_primitive_t(FILE *fh, gfxw_primitive_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(save_struc->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(save_struc->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(save_struc->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(save_struc->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(save_struc->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(save_struc->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(save_struc->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "color = ");
    _cfsml_write_gfx_color_t(fh, &(save_struc->color));
    fprintf(fh, "\n");
  fprintf(fh, "line_mode = ");
    _cfsml_write_gfx_line_mode_t(fh, &(save_struc->line_mode));
    fprintf(fh, "\n");
  fprintf(fh, "line_style = ");
    _cfsml_write_gfx_line_mode_t(fh, &(save_struc->line_style));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_primitive_t(FILE *fh, gfxw_primitive_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "magic")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->magic), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for magic at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfxw_widget_types_t() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bounds")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->bounds), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for bounds at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "next")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->next), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for next at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "ID")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->ID), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for ID at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "widget_priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->widget_priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for widget_priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "color")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_color_t(fh, &(save_struc->color), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_color_t() for color at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "line_mode")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_line_mode_t(fh, &(save_struc->line_mode), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_line_mode_t() for line_mode at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "line_style")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_line_mode_t(fh, &(save_struc->line_style), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_line_mode_t() for line_style at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_widget_types_t(FILE *fh, gfxw_widget_types_t* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_widget_types_t(FILE *fh, gfxw_widget_types_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
#line 564 "savegame.cfsml"

  *save_struc = strtol(lastval, &token, 0);
  if ( (*save_struc == 0) && (token == lastval) ) {
     _cfsml_error("strtol failed at line %d\n", *line);
     return CFSML_FAILURE;
  }
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_drawn_pic_t(FILE *fh, drawn_pic_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "nr = ");
    _cfsml_write_int(fh, &(save_struc->nr));
    fprintf(fh, "\n");
  fprintf(fh, "palette = ");
    _cfsml_write_int(fh, &(save_struc->palette));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_drawn_pic_t(FILE *fh, drawn_pic_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "nr")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->nr), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for nr at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "palette")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->palette), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for palette at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_menubar_t(FILE *fh, menubar_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "menus = ");
    min = max = save_struc->menus_nr;
    if (!save_struc->menus)
       min = max = 0; /* Don't write if it points to NULL */
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_menu_t(fh, &(save_struc->menus[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_menubar_t(FILE *fh, menubar_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "menus")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
#line 673 "savegame.cfsml"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", token, *line);
            return CFSML_FAILURE;
         }

         if (max) {
           save_struc->menus = (menu_t *) sci_malloc(max * sizeof(menu_t));
#ifdef SATISFY_PURIFY
           memset(save_struc->menus, 0, max * sizeof(menu_t));
#endif
           _cfsml_register_pointer(save_struc->menus);
         }
         else
           save_struc->menus = NULL;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_menu_t(fh, &(save_struc->menus[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by _cfsml_read_menu_t() for menus[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
         save_struc->menus_nr = max ; /* Set array size accordingly */
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_string(FILE *fh, char ** save_struc)
{
#line 454 "savegame.cfsml"
  if (!(*save_struc))
    fprintf(fh, "\\null\\");
  else {
    char *token = _cfsml_mangle_string((char *) *save_struc);
    fprintf(fh, "\"%s\"", token);
    free(token);
  }
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_string(FILE *fh, char ** save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
#line 577 "savegame.cfsml"

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
    *save_struc = _cfsml_unmangle_string(lastval);
    _cfsml_register_pointer(*save_struc);
    return CFSML_SUCCESS;
  } else {
    *save_struc = NULL;
    return CFSML_SUCCESS;
  }
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_view_t(FILE *fh, gfxw_view_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(save_struc->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(save_struc->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(save_struc->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(save_struc->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(save_struc->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(save_struc->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(save_struc->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "pos = ");
    _cfsml_write_point_t(fh, &(save_struc->pos));
    fprintf(fh, "\n");
  fprintf(fh, "color = ");
    _cfsml_write_gfx_color_t(fh, &(save_struc->color));
    fprintf(fh, "\n");
  fprintf(fh, "view = ");
    _cfsml_write_int(fh, &(save_struc->view));
    fprintf(fh, "\n");
  fprintf(fh, "loop = ");
    _cfsml_write_int(fh, &(save_struc->loop));
    fprintf(fh, "\n");
  fprintf(fh, "cel = ");
    _cfsml_write_int(fh, &(save_struc->cel));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_view_t(FILE *fh, gfxw_view_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "magic")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->magic), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for magic at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfxw_widget_types_t() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bounds")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->bounds), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for bounds at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "next")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->next), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for next at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "ID")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->ID), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for ID at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "widget_priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->widget_priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for widget_priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "pos")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_point_t(fh, &(save_struc->pos), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_point_t() for pos at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "color")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_color_t(fh, &(save_struc->color), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_color_t() for color at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "view")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->view), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for view at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "loop")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->loop), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for loop at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "cel")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->cel), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for cel at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_point_t(FILE *fh, point_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "x = ");
    _cfsml_write_int(fh, &(save_struc->x));
    fprintf(fh, "\n");
  fprintf(fh, "y = ");
    _cfsml_write_int(fh, &(save_struc->y));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_point_t(FILE *fh, point_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "x")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->x), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for x at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "y")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->y), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for y at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_text_t(FILE *fh, gfxw_text_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(save_struc->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(save_struc->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(save_struc->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(save_struc->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(save_struc->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(save_struc->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(save_struc->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "font_nr = ");
    _cfsml_write_int(fh, &(save_struc->font_nr));
    fprintf(fh, "\n");
  fprintf(fh, "text = ");
    _cfsml_write_string(fh, &(save_struc->text));
    fprintf(fh, "\n");
  fprintf(fh, "halign = ");
    _cfsml_write_gfx_alignment_t(fh, &(save_struc->halign));
    fprintf(fh, "\n");
  fprintf(fh, "valign = ");
    _cfsml_write_gfx_alignment_t(fh, &(save_struc->valign));
    fprintf(fh, "\n");
  fprintf(fh, "color1 = ");
    _cfsml_write_gfx_color_t(fh, &(save_struc->color1));
    fprintf(fh, "\n");
  fprintf(fh, "color2 = ");
    _cfsml_write_gfx_color_t(fh, &(save_struc->color2));
    fprintf(fh, "\n");
  fprintf(fh, "bgcolor = ");
    _cfsml_write_gfx_color_t(fh, &(save_struc->bgcolor));
    fprintf(fh, "\n");
  fprintf(fh, "text_flags = ");
    _cfsml_write_int(fh, &(save_struc->text_flags));
    fprintf(fh, "\n");
  fprintf(fh, "width = ");
    _cfsml_write_int(fh, &(save_struc->width));
    fprintf(fh, "\n");
  fprintf(fh, "height = ");
    _cfsml_write_int(fh, &(save_struc->height));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_text_t(FILE *fh, gfxw_text_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "magic")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->magic), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for magic at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfxw_widget_types_t() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bounds")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->bounds), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for bounds at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "next")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->next), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for next at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "ID")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->ID), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for ID at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "widget_priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->widget_priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for widget_priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "font_nr")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->font_nr), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for font_nr at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "text")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_string(fh, &(save_struc->text), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_string() for text at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "halign")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_alignment_t(fh, &(save_struc->halign), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_alignment_t() for halign at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "valign")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_alignment_t(fh, &(save_struc->valign), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_alignment_t() for valign at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "color1")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_color_t(fh, &(save_struc->color1), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_color_t() for color1 at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "color2")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_color_t(fh, &(save_struc->color2), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_color_t() for color2 at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bgcolor")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_color_t(fh, &(save_struc->bgcolor), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_color_t() for bgcolor at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "text_flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->text_flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for text_flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "width")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->width), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for width at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "height")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->height), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for height at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfx_pixmap_color_t(FILE *fh, gfx_pixmap_color_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "r = ");
    _cfsml_write_byte(fh, &(save_struc->r));
    fprintf(fh, "\n");
  fprintf(fh, "g = ");
    _cfsml_write_byte(fh, &(save_struc->g));
    fprintf(fh, "\n");
  fprintf(fh, "b = ");
    _cfsml_write_byte(fh, &(save_struc->b));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfx_pixmap_color_t(FILE *fh, gfx_pixmap_color_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "r")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->r), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for r at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "g")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->g), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for g at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "b")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->b), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for b at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_port_t(FILE *fh, gfxw_port_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(save_struc->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(save_struc->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(save_struc->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(save_struc->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(save_struc->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(save_struc->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(save_struc->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "zone = ");
    _cfsml_write_rect_t(fh, &(save_struc->zone));
    fprintf(fh, "\n");
  fprintf(fh, "dirty = ");
    if (!save_struc->dirty)
      fprintf(fh, "\\null\\");
    else
      _cfsml_write_gfx_dirty_rect_t(fh, save_struc->dirty);
    fprintf(fh, "\n");
  fprintf(fh, "contents = ");
    write_any_widget(fh, &(save_struc->contents));
    fprintf(fh, "\n");
  fprintf(fh, "decorations = ");
    write_any_widget(fh, &(save_struc->decorations));
    fprintf(fh, "\n");
  fprintf(fh, "port_bg = ");
    write_any_widget(fh, &(save_struc->port_bg));
    fprintf(fh, "\n");
  fprintf(fh, "color = ");
    _cfsml_write_gfx_color_t(fh, &(save_struc->color));
    fprintf(fh, "\n");
  fprintf(fh, "bgcolor = ");
    _cfsml_write_gfx_color_t(fh, &(save_struc->bgcolor));
    fprintf(fh, "\n");
  fprintf(fh, "font_nr = ");
    _cfsml_write_int(fh, &(save_struc->font_nr));
    fprintf(fh, "\n");
  fprintf(fh, "draw_pos = ");
    _cfsml_write_point_t(fh, &(save_struc->draw_pos));
    fprintf(fh, "\n");
  fprintf(fh, "port_flags = ");
    _cfsml_write_int(fh, &(save_struc->port_flags));
    fprintf(fh, "\n");
  fprintf(fh, "title_text = ");
    _cfsml_write_string(fh, &(save_struc->title_text));
    fprintf(fh, "\n");
  fprintf(fh, "gray_text = ");
    _cfsml_write_byte(fh, &(save_struc->gray_text));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_port_t(FILE *fh, gfxw_port_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "magic")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->magic), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for magic at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfxw_widget_types_t() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bounds")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->bounds), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for bounds at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "next")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->next), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for next at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "ID")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->ID), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for ID at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "widget_priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->widget_priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for widget_priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "zone")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->zone), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for zone at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "dirty")) {
#line 738 "savegame.cfsml"
        if (strcmp(value, "\\null\\")) { /* null pointer? */
           save_struc->dirty = sci_malloc(sizeof (gfx_dirty_rect_t));
           _cfsml_register_pointer(save_struc->dirty);
           if (_cfsml_read_gfx_dirty_rect_t(fh, save_struc->dirty, value, line, hiteof)) {
              _cfsml_error("Token expected by _cfsml_read_gfx_dirty_rect_t() for dirty at line %d\n", *line);
              return CFSML_FAILURE;
           }
        } else save_struc->dirty = NULL;
      } else
      if (!strcmp(token, "contents")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->contents), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for contents at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "decorations")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->decorations), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for decorations at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "port_bg")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->port_bg), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for port_bg at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "color")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_color_t(fh, &(save_struc->color), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_color_t() for color at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bgcolor")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_color_t(fh, &(save_struc->bgcolor), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_color_t() for bgcolor at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "font_nr")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->font_nr), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for font_nr at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "draw_pos")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_point_t(fh, &(save_struc->draw_pos), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_point_t() for draw_pos at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "port_flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->port_flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for port_flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "title_text")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_string(fh, &(save_struc->title_text), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_string() for title_text at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "gray_text")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->gray_text), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for gray_text at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_visual_t(FILE *fh, gfxw_visual_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(save_struc->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(save_struc->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(save_struc->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(save_struc->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(save_struc->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(save_struc->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(save_struc->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "zone = ");
    _cfsml_write_rect_t(fh, &(save_struc->zone));
    fprintf(fh, "\n");
  fprintf(fh, "dirty = ");
    if (!save_struc->dirty)
      fprintf(fh, "\\null\\");
    else
      _cfsml_write_gfx_dirty_rect_t(fh, save_struc->dirty);
    fprintf(fh, "\n");
  fprintf(fh, "contents = ");
    write_any_widget(fh, &(save_struc->contents));
    fprintf(fh, "\n");
  fprintf(fh, "port_refs_nr = ");
    _cfsml_write_int(fh, &(save_struc->port_refs_nr));
    fprintf(fh, "\n");
  fprintf(fh, "font_nr = ");
    _cfsml_write_int(fh, &(save_struc->font_nr));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_visual_t(FILE *fh, gfxw_visual_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "magic")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->magic), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for magic at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfxw_widget_types_t() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bounds")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->bounds), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for bounds at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "next")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->next), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for next at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "ID")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->ID), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for ID at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "widget_priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->widget_priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for widget_priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "zone")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->zone), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for zone at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "dirty")) {
#line 738 "savegame.cfsml"
        if (strcmp(value, "\\null\\")) { /* null pointer? */
           save_struc->dirty = sci_malloc(sizeof (gfx_dirty_rect_t));
           _cfsml_register_pointer(save_struc->dirty);
           if (_cfsml_read_gfx_dirty_rect_t(fh, save_struc->dirty, value, line, hiteof)) {
              _cfsml_error("Token expected by _cfsml_read_gfx_dirty_rect_t() for dirty at line %d\n", *line);
              return CFSML_FAILURE;
           }
        } else save_struc->dirty = NULL;
      } else
      if (!strcmp(token, "contents")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->contents), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for contents at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "port_refs_nr")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->port_refs_nr), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for port_refs_nr at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "font_nr")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->font_nr), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for font_nr at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_dyn_view_t(FILE *fh, gfxw_dyn_view_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(save_struc->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(save_struc->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(save_struc->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(save_struc->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(save_struc->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(save_struc->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(save_struc->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "pos = ");
    _cfsml_write_point_t(fh, &(save_struc->pos));
    fprintf(fh, "\n");
  fprintf(fh, "color = ");
    _cfsml_write_gfx_color_t(fh, &(save_struc->color));
    fprintf(fh, "\n");
  fprintf(fh, "view = ");
    _cfsml_write_int(fh, &(save_struc->view));
    fprintf(fh, "\n");
  fprintf(fh, "loop = ");
    _cfsml_write_int(fh, &(save_struc->loop));
    fprintf(fh, "\n");
  fprintf(fh, "cel = ");
    _cfsml_write_int(fh, &(save_struc->cel));
    fprintf(fh, "\n");
  fprintf(fh, "draw_bounds = ");
    _cfsml_write_rect_t(fh, &(save_struc->draw_bounds));
    fprintf(fh, "\n");
  fprintf(fh, "under_bitsp = ");
    _cfsml_write_int(fh, &(save_struc->under_bitsp));
    fprintf(fh, "\n");
  fprintf(fh, "signalp = ");
    _cfsml_write_int(fh, &(save_struc->signalp));
    fprintf(fh, "\n");
  fprintf(fh, "under_bits = ");
    _cfsml_write_int(fh, &(save_struc->under_bits));
    fprintf(fh, "\n");
  fprintf(fh, "signal = ");
    _cfsml_write_int(fh, &(save_struc->signal));
    fprintf(fh, "\n");
  fprintf(fh, "z = ");
    _cfsml_write_int(fh, &(save_struc->z));
    fprintf(fh, "\n");
  fprintf(fh, "sequence = ");
    _cfsml_write_int(fh, &(save_struc->sequence));
    fprintf(fh, "\n");
  fprintf(fh, "force_precedence = ");
    _cfsml_write_int(fh, &(save_struc->force_precedence));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_dyn_view_t(FILE *fh, gfxw_dyn_view_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "magic")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->magic), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for magic at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfxw_widget_types_t() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bounds")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->bounds), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for bounds at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "next")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->next), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for next at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "ID")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->ID), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for ID at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "widget_priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->widget_priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for widget_priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "pos")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_point_t(fh, &(save_struc->pos), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_point_t() for pos at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "color")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_color_t(fh, &(save_struc->color), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_color_t() for color at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "view")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->view), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for view at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "loop")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->loop), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for loop at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "cel")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->cel), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for cel at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "draw_bounds")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->draw_bounds), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for draw_bounds at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "under_bitsp")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->under_bitsp), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for under_bitsp at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "signalp")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->signalp), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for signalp at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "under_bits")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->under_bits), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for under_bits at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "signal")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->signal), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for signal at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "z")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->z), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for z at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "sequence")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->sequence), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for sequence at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "force_precedence")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->force_precedence), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for force_precedence at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfx_line_mode_t(FILE *fh, gfx_line_mode_t* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfx_line_mode_t(FILE *fh, gfx_line_mode_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
#line 564 "savegame.cfsml"

  *save_struc = strtol(lastval, &token, 0);
  if ( (*save_struc == 0) && (token == lastval) ) {
     _cfsml_error("strtol failed at line %d\n", *line);
     return CFSML_FAILURE;
  }
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfx_box_shade_t(FILE *fh, gfx_box_shade_t* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfx_box_shade_t(FILE *fh, gfx_box_shade_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
#line 564 "savegame.cfsml"

  *save_struc = strtol(lastval, &token, 0);
  if ( (*save_struc == 0) && (token == lastval) ) {
     _cfsml_error("strtol failed at line %d\n", *line);
     return CFSML_FAILURE;
  }
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_list_t(FILE *fh, gfxw_list_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(save_struc->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(save_struc->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(save_struc->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(save_struc->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(save_struc->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(save_struc->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(save_struc->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "zone = ");
    _cfsml_write_rect_t(fh, &(save_struc->zone));
    fprintf(fh, "\n");
  fprintf(fh, "dirty = ");
    if (!save_struc->dirty)
      fprintf(fh, "\\null\\");
    else
      _cfsml_write_gfx_dirty_rect_t(fh, save_struc->dirty);
    fprintf(fh, "\n");
  fprintf(fh, "contents = ");
    write_any_widget(fh, &(save_struc->contents));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_list_t(FILE *fh, gfxw_list_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "magic")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->magic), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for magic at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfxw_widget_types_t() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bounds")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->bounds), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for bounds at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "next")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->next), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for next at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "ID")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->ID), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for ID at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "widget_priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->widget_priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for widget_priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "zone")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->zone), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for zone at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "dirty")) {
#line 738 "savegame.cfsml"
        if (strcmp(value, "\\null\\")) { /* null pointer? */
           save_struc->dirty = sci_malloc(sizeof (gfx_dirty_rect_t));
           _cfsml_register_pointer(save_struc->dirty);
           if (_cfsml_read_gfx_dirty_rect_t(fh, save_struc->dirty, value, line, hiteof)) {
              _cfsml_error("Token expected by _cfsml_read_gfx_dirty_rect_t() for dirty at line %d\n", *line);
              return CFSML_FAILURE;
           }
        } else save_struc->dirty = NULL;
      } else
      if (!strcmp(token, "contents")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->contents), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for contents at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_snapshot_t(FILE *fh, gfxw_snapshot_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(save_struc->serial));
    fprintf(fh, "\n");
  fprintf(fh, "area = ");
    _cfsml_write_rect_t(fh, &(save_struc->area));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_snapshot_t(FILE *fh, gfxw_snapshot_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "area")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->area), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for area at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_state_t(FILE *fh, state_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "savegame_version = ");
    _cfsml_write_int(fh, &(save_struc->savegame_version));
    fprintf(fh, "\n");
  fprintf(fh, "restarting_flags = ");
    _cfsml_write_byte(fh, &(save_struc->restarting_flags));
    fprintf(fh, "\n");
  fprintf(fh, "have_mouse_flag = ");
    _cfsml_write_byte(fh, &(save_struc->have_mouse_flag));
    fprintf(fh, "\n");
  fprintf(fh, "pic_not_valid = ");
    _cfsml_write_byte(fh, &(save_struc->pic_not_valid));
    fprintf(fh, "\n");
  fprintf(fh, "pic_is_new = ");
    _cfsml_write_byte(fh, &(save_struc->pic_is_new));
    fprintf(fh, "\n");
  fprintf(fh, "onscreen_console = ");
    _cfsml_write_byte(fh, &(save_struc->onscreen_console));
    fprintf(fh, "\n");
  fprintf(fh, "game_time = ");
    _cfsml_write_long(fh, &(save_struc->game_time));
    fprintf(fh, "\n");
  fprintf(fh, "save_dir = ");
    write_heapptr(fh, &(save_struc->save_dir));
    fprintf(fh, "\n");
  fprintf(fh, "save_dir_copy = ");
    write_heapptr(fh, &(save_struc->save_dir_copy));
    fprintf(fh, "\n");
  fprintf(fh, "save_dir_copy_buf = ");
    _cfsml_write_string(fh, &(save_struc->save_dir_copy_buf));
    fprintf(fh, "\n");
  fprintf(fh, "save_dir_edit_offset = ");
    _cfsml_write_int(fh, &(save_struc->save_dir_edit_offset));
    fprintf(fh, "\n");
  fprintf(fh, "sound_object = ");
    write_heapptr(fh, &(save_struc->sound_object));
    fprintf(fh, "\n");
  fprintf(fh, "sound_mute = ");
    _cfsml_write_int(fh, &(save_struc->sound_mute));
    fprintf(fh, "\n");
  fprintf(fh, "sound_volume = ");
    _cfsml_write_int(fh, &(save_struc->sound_volume));
    fprintf(fh, "\n");
  fprintf(fh, "mouse_pointer_nr = ");
    _cfsml_write_int(fh, &(save_struc->mouse_pointer_nr));
    fprintf(fh, "\n");
  fprintf(fh, "port_serial = ");
    _cfsml_write_int(fh, &(save_struc->port_serial));
    fprintf(fh, "\n");
  fprintf(fh, "dyn_views_list_serial = ");
    _cfsml_write_int(fh, &(save_struc->dyn_views_list_serial));
    fprintf(fh, "\n");
  fprintf(fh, "drop_views_list_serial = ");
    _cfsml_write_int(fh, &(save_struc->drop_views_list_serial));
    fprintf(fh, "\n");
  fprintf(fh, "visual = ");
    write_any_widget(fh, &(save_struc->visual));
    fprintf(fh, "\n");
  fprintf(fh, "pic_visible_map = ");
    _cfsml_write_int(fh, &(save_struc->pic_visible_map));
    fprintf(fh, "\n");
  fprintf(fh, "pic_animate = ");
    _cfsml_write_int(fh, &(save_struc->pic_animate));
    fprintf(fh, "\n");
  fprintf(fh, "animation_delay = ");
    _cfsml_write_long(fh, &(save_struc->animation_delay));
    fprintf(fh, "\n");
  fprintf(fh, "hunk = ");
    min = max = MAX_HUNK_BLOCKS;
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      write_hunk_block(fh, &(save_struc->hunk[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "menubar = ");
    write_menubar_tp(fh, &(save_struc->menubar));
    fprintf(fh, "\n");
  fprintf(fh, "status_bar_text = ");
    _cfsml_write_string(fh, &(save_struc->status_bar_text));
    fprintf(fh, "\n");
  fprintf(fh, "priority_first = ");
    _cfsml_write_int(fh, &(save_struc->priority_first));
    fprintf(fh, "\n");
  fprintf(fh, "priority_last = ");
    _cfsml_write_int(fh, &(save_struc->priority_last));
    fprintf(fh, "\n");
  fprintf(fh, "pics = ");
    min = max = save_struc->pics_nr;
    if (save_struc->pics_drawn_nr < min)
       min = save_struc->pics_drawn_nr;
    if (!save_struc->pics)
       min = max = 0; /* Don't write if it points to NULL */
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_drawn_pic_t(fh, &(save_struc->pics[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "version_lock_flag = ");
    _cfsml_write_byte(fh, &(save_struc->version_lock_flag));
    fprintf(fh, "\n");
  fprintf(fh, "version = ");
    write_sci_version(fh, &(save_struc->version));
    fprintf(fh, "\n");
  fprintf(fh, "max_version = ");
    write_sci_version(fh, &(save_struc->max_version));
    fprintf(fh, "\n");
  fprintf(fh, "min_version = ");
    write_sci_version(fh, &(save_struc->min_version));
    fprintf(fh, "\n");
  fprintf(fh, "execution_stack = ");
    min = max = save_struc->execution_stack_size;
    if (save_struc->execution_stack_pos+1 < min)
       min = save_struc->execution_stack_pos+1;
    if (!save_struc->execution_stack)
       min = max = 0; /* Don't write if it points to NULL */
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_exec_stack_t(fh, &(save_struc->execution_stack[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "acc = ");
    _cfsml_write_gint16(fh, &(save_struc->acc));
    fprintf(fh, "\n");
  fprintf(fh, "amp_rest = ");
    _cfsml_write_gint16(fh, &(save_struc->amp_rest));
    fprintf(fh, "\n");
  fprintf(fh, "prev = ");
    _cfsml_write_gint16(fh, &(save_struc->prev));
    fprintf(fh, "\n");
  fprintf(fh, "stack_base = ");
    write_heapptr(fh, &(save_struc->stack_base));
    fprintf(fh, "\n");
  fprintf(fh, "stack_handle = ");
    write_heapptr(fh, &(save_struc->stack_handle));
    fprintf(fh, "\n");
  fprintf(fh, "parser_base = ");
    write_heapptr(fh, &(save_struc->parser_base));
    fprintf(fh, "\n");
  fprintf(fh, "parser_event = ");
    write_heapptr(fh, &(save_struc->parser_event));
    fprintf(fh, "\n");
  fprintf(fh, "global_vars = ");
    write_heapptr(fh, &(save_struc->global_vars));
    fprintf(fh, "\n");
  fprintf(fh, "parser_lastmatch_word = ");
    _cfsml_write_int(fh, &(save_struc->parser_lastmatch_word));
    fprintf(fh, "\n");
  fprintf(fh, "parser_nodes = ");
    min = max = VOCAB_TREE_NODES;
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      write_PTN(fh, &(save_struc->parser_nodes[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "parser_valid = ");
    _cfsml_write_int(fh, &(save_struc->parser_valid));
    fprintf(fh, "\n");
  fprintf(fh, "synonyms = ");
    min = max = save_struc->synonyms_nr;
    if (!save_struc->synonyms)
       min = max = 0; /* Don't write if it points to NULL */
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_synonym_t(fh, &(save_struc->synonyms[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "game_obj = ");
    write_heapptr(fh, &(save_struc->game_obj));
    fprintf(fh, "\n");
  fprintf(fh, "classtable = ");
    min = max = save_struc->classtable_size;
    if (!save_struc->classtable)
       min = max = 0; /* Don't write if it points to NULL */
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_class_t(fh, &(save_struc->classtable[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "scripttable = ");
    min = max = 1000;
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_script_t(fh, &(save_struc->scripttable[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "clone_list = ");
    min = max = SCRIPT_MAX_CLONES;
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      write_heapptr(fh, &(save_struc->clone_list[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "_heap->first_free = ");
    _cfsml_write_int(fh, &(save_struc->_heap->first_free));
    fprintf(fh, "\n");
  fprintf(fh, "_heap->old_ff = ");
    _cfsml_write_int(fh, &(save_struc->_heap->old_ff));
    fprintf(fh, "\n");
  fprintf(fh, "_heap->base = ");
    fprintf(fh, "%d", save_struc->_heap->base - save_struc->_heap->start); /* Relative pointer */
    fprintf(fh, "\n");
  fprintf(fh, "game_name = ");
    fprintf(fh, "%d", save_struc->game_name - save_struc->_heap->start); /* Relative pointer */
    fprintf(fh, "\n");
  fprintf(fh, "port_ID = ");
    _cfsml_write_int(fh, &(save_struc->port_ID));
    fprintf(fh, "\n");
  fprintf(fh, "ega_colors = ");
    min = max = 16;
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_gfx_color_t(fh, &(save_struc->ega_colors[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_state_t(FILE *fh, state_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
  int reladdresses[2] = {0};
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "savegame_version")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->savegame_version), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for savegame_version at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "restarting_flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->restarting_flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for restarting_flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "have_mouse_flag")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->have_mouse_flag), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for have_mouse_flag at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "pic_not_valid")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->pic_not_valid), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for pic_not_valid at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "pic_is_new")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->pic_is_new), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for pic_is_new at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "onscreen_console")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->onscreen_console), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for onscreen_console at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "game_time")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_long(fh, &(save_struc->game_time), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_long() for game_time at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "save_dir")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->save_dir), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for save_dir at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "save_dir_copy")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->save_dir_copy), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for save_dir_copy at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "save_dir_copy_buf")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_string(fh, &(save_struc->save_dir_copy_buf), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_string() for save_dir_copy_buf at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "save_dir_edit_offset")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->save_dir_edit_offset), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for save_dir_edit_offset at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "sound_object")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->sound_object), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for sound_object at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "sound_mute")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->sound_mute), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for sound_mute at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "sound_volume")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->sound_volume), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for sound_volume at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "mouse_pointer_nr")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->mouse_pointer_nr), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for mouse_pointer_nr at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "port_serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->port_serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for port_serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "dyn_views_list_serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->dyn_views_list_serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for dyn_views_list_serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "drop_views_list_serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->drop_views_list_serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for drop_views_list_serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "visual")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->visual), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for visual at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "pic_visible_map")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->pic_visible_map), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for pic_visible_map at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "pic_animate")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->pic_animate), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for pic_animate at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "animation_delay")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_long(fh, &(save_struc->animation_delay), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_long() for animation_delay at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "hunk")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
         /* Prepare to restore static array */
         max = MAX_HUNK_BLOCKS;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (read_hunk_block(fh, &(save_struc->hunk[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by read_hunk_block() for hunk[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "menubar")) {
#line 749 "savegame.cfsml"
         if (read_menubar_tp(fh, &(save_struc->menubar), value, line, hiteof)) {
            _cfsml_error("Token expected by read_menubar_tp() for menubar at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "status_bar_text")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_string(fh, &(save_struc->status_bar_text), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_string() for status_bar_text at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "priority_first")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->priority_first), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for priority_first at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "priority_last")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->priority_last), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for priority_last at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "pics")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
#line 673 "savegame.cfsml"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", token, *line);
            return CFSML_FAILURE;
         }

         if (max) {
           save_struc->pics = (drawn_pic_t *) sci_malloc(max * sizeof(drawn_pic_t));
#ifdef SATISFY_PURIFY
           memset(save_struc->pics, 0, max * sizeof(drawn_pic_t));
#endif
           _cfsml_register_pointer(save_struc->pics);
         }
         else
           save_struc->pics = NULL;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_drawn_pic_t(fh, &(save_struc->pics[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by _cfsml_read_drawn_pic_t() for pics[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
         save_struc->pics_nr = max ; /* Set array size accordingly */
         save_struc->pics_drawn_nr = i ; /* Set number of elements */
      } else
      if (!strcmp(token, "version_lock_flag")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->version_lock_flag), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for version_lock_flag at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "version")) {
#line 749 "savegame.cfsml"
         if (read_sci_version(fh, &(save_struc->version), value, line, hiteof)) {
            _cfsml_error("Token expected by read_sci_version() for version at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "max_version")) {
#line 749 "savegame.cfsml"
         if (read_sci_version(fh, &(save_struc->max_version), value, line, hiteof)) {
            _cfsml_error("Token expected by read_sci_version() for max_version at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "min_version")) {
#line 749 "savegame.cfsml"
         if (read_sci_version(fh, &(save_struc->min_version), value, line, hiteof)) {
            _cfsml_error("Token expected by read_sci_version() for min_version at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "execution_stack")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
#line 673 "savegame.cfsml"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", token, *line);
            return CFSML_FAILURE;
         }

         if (max) {
           save_struc->execution_stack = (exec_stack_t *) sci_malloc(max * sizeof(exec_stack_t));
#ifdef SATISFY_PURIFY
           memset(save_struc->execution_stack, 0, max * sizeof(exec_stack_t));
#endif
           _cfsml_register_pointer(save_struc->execution_stack);
         }
         else
           save_struc->execution_stack = NULL;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_exec_stack_t(fh, &(save_struc->execution_stack[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by _cfsml_read_exec_stack_t() for execution_stack[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
         save_struc->execution_stack_size = max ; /* Set array size accordingly */
         save_struc->execution_stack_pos = i -1; /* Set number of elements */
      } else
      if (!strcmp(token, "acc")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gint16(fh, &(save_struc->acc), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gint16() for acc at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "amp_rest")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gint16(fh, &(save_struc->amp_rest), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gint16() for amp_rest at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "prev")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gint16(fh, &(save_struc->prev), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gint16() for prev at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "stack_base")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->stack_base), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for stack_base at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "stack_handle")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->stack_handle), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for stack_handle at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "parser_base")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->parser_base), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for parser_base at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "parser_event")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->parser_event), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for parser_event at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "global_vars")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->global_vars), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for global_vars at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "parser_lastmatch_word")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->parser_lastmatch_word), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for parser_lastmatch_word at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "parser_nodes")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
         /* Prepare to restore static array */
         max = VOCAB_TREE_NODES;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (read_PTN(fh, &(save_struc->parser_nodes[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by read_PTN() for parser_nodes[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "parser_valid")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->parser_valid), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for parser_valid at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "synonyms")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
#line 673 "savegame.cfsml"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", token, *line);
            return CFSML_FAILURE;
         }

         if (max) {
           save_struc->synonyms = (synonym_t *) sci_malloc(max * sizeof(synonym_t));
#ifdef SATISFY_PURIFY
           memset(save_struc->synonyms, 0, max * sizeof(synonym_t));
#endif
           _cfsml_register_pointer(save_struc->synonyms);
         }
         else
           save_struc->synonyms = NULL;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_synonym_t(fh, &(save_struc->synonyms[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by _cfsml_read_synonym_t() for synonyms[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
         save_struc->synonyms_nr = max ; /* Set array size accordingly */
      } else
      if (!strcmp(token, "game_obj")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->game_obj), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for game_obj at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "classtable")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
#line 673 "savegame.cfsml"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", token, *line);
            return CFSML_FAILURE;
         }

         if (max) {
           save_struc->classtable = (class_t *) sci_malloc(max * sizeof(class_t));
#ifdef SATISFY_PURIFY
           memset(save_struc->classtable, 0, max * sizeof(class_t));
#endif
           _cfsml_register_pointer(save_struc->classtable);
         }
         else
           save_struc->classtable = NULL;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_class_t(fh, &(save_struc->classtable[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by _cfsml_read_class_t() for classtable[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
         save_struc->classtable_size = max ; /* Set array size accordingly */
      } else
      if (!strcmp(token, "scripttable")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
         /* Prepare to restore static array */
         max = 1000;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_script_t(fh, &(save_struc->scripttable[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by _cfsml_read_script_t() for scripttable[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "clone_list")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
         /* Prepare to restore static array */
         max = SCRIPT_MAX_CLONES;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (read_heapptr(fh, &(save_struc->clone_list[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by read_heapptr() for clone_list[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "_heap->first_free")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->_heap->first_free), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for _heap->first_free at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "_heap->old_ff")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->_heap->old_ff), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for _heap->old_ff at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "_heap->base")) {
#line 650 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(reladdresses[0]), value, line, hiteof)) {
            _cfsml_error("Expected token at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "game_name")) {
#line 650 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(reladdresses[1]), value, line, hiteof)) {
            _cfsml_error("Expected token at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "port_ID")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->port_ID), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for port_ID at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "ega_colors")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
         /* Prepare to restore static array */
         max = 16;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_gfx_color_t(fh, &(save_struc->ega_colors[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by _cfsml_read_gfx_color_t() for ega_colors[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  save_struc->_heap->base = save_struc->_heap->start + reladdresses[0];
  save_struc->game_name = save_struc->_heap->start + reladdresses[1];
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_menu_item_t(FILE *fh, menu_item_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "type = ");
    _cfsml_write_int(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "keytext = ");
    _cfsml_write_string(fh, &(save_struc->keytext));
    fprintf(fh, "\n");
  fprintf(fh, "keytext_size = ");
    _cfsml_write_int(fh, &(save_struc->keytext_size));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(save_struc->flags));
    fprintf(fh, "\n");
  fprintf(fh, "said = ");
    min = max = MENU_SAID_SPEC_SIZE;
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_byte(fh, &(save_struc->said[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "said_pos = ");
    write_heapptr(fh, &(save_struc->said_pos));
    fprintf(fh, "\n");
  fprintf(fh, "text = ");
    _cfsml_write_string(fh, &(save_struc->text));
    fprintf(fh, "\n");
  fprintf(fh, "text_pos = ");
    write_heapptr(fh, &(save_struc->text_pos));
    fprintf(fh, "\n");
  fprintf(fh, "modifiers = ");
    _cfsml_write_int(fh, &(save_struc->modifiers));
    fprintf(fh, "\n");
  fprintf(fh, "key = ");
    _cfsml_write_int(fh, &(save_struc->key));
    fprintf(fh, "\n");
  fprintf(fh, "enabled = ");
    _cfsml_write_int(fh, &(save_struc->enabled));
    fprintf(fh, "\n");
  fprintf(fh, "tag = ");
    _cfsml_write_int(fh, &(save_struc->tag));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_menu_item_t(FILE *fh, menu_item_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "keytext")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_string(fh, &(save_struc->keytext), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_string() for keytext at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "keytext_size")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->keytext_size), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for keytext_size at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "said")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
         /* Prepare to restore static array */
         max = MENU_SAID_SPEC_SIZE;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_byte(fh, &(save_struc->said[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by _cfsml_read_byte() for said[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "said_pos")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->said_pos), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for said_pos at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "text")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_string(fh, &(save_struc->text), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_string() for text at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "text_pos")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->text_pos), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for text_pos at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "modifiers")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->modifiers), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for modifiers at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "key")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->key), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for key at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "enabled")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->enabled), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for enabled at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "tag")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->tag), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for tag at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfx_color_t(FILE *fh, gfx_color_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "visual = ");
    write_pixmap_color(fh, &(save_struc->visual));
    fprintf(fh, "\n");
  fprintf(fh, "alpha = ");
    _cfsml_write_byte(fh, &(save_struc->alpha));
    fprintf(fh, "\n");
  fprintf(fh, "priority = ");
    _cfsml_write_byte(fh, &(save_struc->priority));
    fprintf(fh, "\n");
  fprintf(fh, "control = ");
    _cfsml_write_byte(fh, &(save_struc->control));
    fprintf(fh, "\n");
  fprintf(fh, "mask = ");
    _cfsml_write_byte(fh, &(save_struc->mask));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfx_color_t(FILE *fh, gfx_color_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "visual")) {
#line 749 "savegame.cfsml"
         if (read_pixmap_color(fh, &(save_struc->visual), value, line, hiteof)) {
            _cfsml_error("Token expected by read_pixmap_color() for visual at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "alpha")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->alpha), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for alpha at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "control")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->control), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for control at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "mask")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_byte(fh, &(save_struc->mask), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_byte() for mask at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_long(FILE *fh, long* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_long(FILE *fh, long* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
#line 564 "savegame.cfsml"

  *save_struc = strtol(lastval, &token, 0);
  if ( (*save_struc == 0) && (token == lastval) ) {
     _cfsml_error("strtol failed at line %d\n", *line);
     return CFSML_FAILURE;
  }
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_byte(FILE *fh, byte* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_byte(FILE *fh, byte* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
#line 564 "savegame.cfsml"

  *save_struc = strtol(lastval, &token, 0);
  if ( (*save_struc == 0) && (token == lastval) ) {
     _cfsml_error("strtol failed at line %d\n", *line);
     return CFSML_FAILURE;
  }
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_script_t(FILE *fh, script_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "heappos = ");
    write_heapptr(fh, &(save_struc->heappos));
    fprintf(fh, "\n");
  fprintf(fh, "localvar_offset = ");
    write_heapptr(fh, &(save_struc->localvar_offset));
    fprintf(fh, "\n");
  fprintf(fh, "export_table_offset = ");
    write_heapptr(fh, &(save_struc->export_table_offset));
    fprintf(fh, "\n");
  fprintf(fh, "synonyms_offset = ");
    write_heapptr(fh, &(save_struc->synonyms_offset));
    fprintf(fh, "\n");
  fprintf(fh, "lockers = ");
    _cfsml_write_int(fh, &(save_struc->lockers));
    fprintf(fh, "\n");
  fprintf(fh, "synonyms_nr = ");
    _cfsml_write_int(fh, &(save_struc->synonyms_nr));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_script_t(FILE *fh, script_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "heappos")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->heappos), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for heappos at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "localvar_offset")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->localvar_offset), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for localvar_offset at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "export_table_offset")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->export_table_offset), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for export_table_offset at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "synonyms_offset")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->synonyms_offset), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for synonyms_offset at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "lockers")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->lockers), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for lockers at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "synonyms_nr")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->synonyms_nr), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for synonyms_nr at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_class_t(FILE *fh, class_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "script = ");
    _cfsml_write_int(fh, &(save_struc->script));
    fprintf(fh, "\n");
  fprintf(fh, "class_offset = ");
    _cfsml_write_int(fh, &(save_struc->class_offset));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_class_t(FILE *fh, class_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "script")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->script), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for script at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "class_offset")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->class_offset), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for class_offset at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_synonym_t(FILE *fh, synonym_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "replaceant = ");
    _cfsml_write_int(fh, &(save_struc->replaceant));
    fprintf(fh, "\n");
  fprintf(fh, "replacement = ");
    _cfsml_write_int(fh, &(save_struc->replacement));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_synonym_t(FILE *fh, synonym_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "replaceant")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->replaceant), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for replaceant at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "replacement")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->replacement), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for replacement at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_exec_stack_t(FILE *fh, exec_stack_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "objp = ");
    write_heapptr(fh, &(save_struc->objp));
    fprintf(fh, "\n");
  fprintf(fh, "sendp = ");
    write_heapptr(fh, &(save_struc->sendp));
    fprintf(fh, "\n");
  fprintf(fh, "pc = ");
    write_heapptr(fh, &(save_struc->pc));
    fprintf(fh, "\n");
  fprintf(fh, "sp = ");
    write_heapptr(fh, &(save_struc->sp));
    fprintf(fh, "\n");
  fprintf(fh, "argc = ");
    _cfsml_write_int(fh, &(save_struc->argc));
    fprintf(fh, "\n");
  fprintf(fh, "variables = ");
    min = max = 4;
#line 490 "savegame.cfsml"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      write_heapptr(fh, &(save_struc->variables[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "selector = ");
    _cfsml_write_int(fh, &(save_struc->selector));
    fprintf(fh, "\n");
  fprintf(fh, "origin = ");
    _cfsml_write_int(fh, &(save_struc->origin));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    write_heapptr(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_exec_stack_t(FILE *fh, exec_stack_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "objp")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->objp), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for objp at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "sendp")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->sendp), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for sendp at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "pc")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->pc), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for pc at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "sp")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->sp), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for sp at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "argc")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->argc), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for argc at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "variables")) {
#line 663 "savegame.cfsml"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
         }
         /* Prepare to restore static array */
         max = 4;
#line 699 "savegame.cfsml"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL))) {
#line 707 "savegame.cfsml"
              _cfsml_error("Token expected at line %d\n", *line);
              return 1;
           }
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (read_heapptr(fh, &(save_struc->variables[i++]), value, line, hiteof)) {
                _cfsml_error("Token expected by read_heapptr() for variables[i++] at line %d\n", *line);
                return CFSML_FAILURE;
             }
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "selector")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->selector), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for selector at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "origin")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->origin), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for origin at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfx_line_style_t(FILE *fh, gfx_line_style_t* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfx_line_style_t(FILE *fh, gfx_line_style_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
#line 564 "savegame.cfsml"

  *save_struc = strtol(lastval, &token, 0);
  if ( (*save_struc == 0) && (token == lastval) ) {
     _cfsml_error("strtol failed at line %d\n", *line);
     return CFSML_FAILURE;
  }
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gint16(FILE *fh, gint16* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gint16(FILE *fh, gint16* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
#line 564 "savegame.cfsml"

  *save_struc = strtol(lastval, &token, 0);
  if ( (*save_struc == 0) && (token == lastval) ) {
     _cfsml_error("strtol failed at line %d\n", *line);
     return CFSML_FAILURE;
  }
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_rect_t(FILE *fh, rect_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "x = ");
    _cfsml_write_int(fh, &(save_struc->x));
    fprintf(fh, "\n");
  fprintf(fh, "y = ");
    _cfsml_write_int(fh, &(save_struc->y));
    fprintf(fh, "\n");
  fprintf(fh, "xl = ");
    _cfsml_write_int(fh, &(save_struc->xl));
    fprintf(fh, "\n");
  fprintf(fh, "yl = ");
    _cfsml_write_int(fh, &(save_struc->yl));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_rect_t(FILE *fh, rect_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "x")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->x), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for x at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "y")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->y), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for y at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "xl")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->xl), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for xl at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "yl")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->yl), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for yl at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfx_alignment_t(FILE *fh, gfx_alignment_t* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfx_alignment_t(FILE *fh, gfx_alignment_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
#line 564 "savegame.cfsml"

  *save_struc = strtol(lastval, &token, 0);
  if ( (*save_struc == 0) && (token == lastval) ) {
     _cfsml_error("strtol failed at line %d\n", *line);
     return CFSML_FAILURE;
  }
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_int(FILE *fh, int* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_int(FILE *fh, int* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
#line 564 "savegame.cfsml"

  *save_struc = strtol(lastval, &token, 0);
  if ( (*save_struc == 0) && (token == lastval) ) {
     _cfsml_error("strtol failed at line %d\n", *line);
     return CFSML_FAILURE;
  }
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfxw_box_t(FILE *fh, gfxw_box_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(save_struc->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(save_struc->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(save_struc->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(save_struc->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(save_struc->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(save_struc->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(save_struc->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(save_struc->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "color1 = ");
    _cfsml_write_gfx_color_t(fh, &(save_struc->color1));
    fprintf(fh, "\n");
  fprintf(fh, "color2 = ");
    _cfsml_write_gfx_color_t(fh, &(save_struc->color2));
    fprintf(fh, "\n");
  fprintf(fh, "shade_type = ");
    _cfsml_write_gfx_box_shade_t(fh, &(save_struc->shade_type));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfxw_box_t(FILE *fh, gfxw_box_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "magic")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->magic), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for magic at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "serial")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->serial), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for serial at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "flags")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->flags), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for flags at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(save_struc->type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfxw_widget_types_t() for type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "bounds")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->bounds), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for bounds at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "next")) {
#line 749 "savegame.cfsml"
         if (read_any_widget(fh, &(save_struc->next), value, line, hiteof)) {
            _cfsml_error("Token expected by read_any_widget() for next at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "ID")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->ID), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for ID at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "widget_priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->widget_priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for widget_priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "color1")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_color_t(fh, &(save_struc->color1), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_color_t() for color1 at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "color2")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_color_t(fh, &(save_struc->color2), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_color_t() for color2 at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "shade_type")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_gfx_box_shade_t(fh, &(save_struc->shade_type), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_gfx_box_shade_t() for shade_type at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_gfx_dirty_rect_t(FILE *fh, gfx_dirty_rect_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "rect = ");
    _cfsml_write_rect_t(fh, &(save_struc->rect));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    if (!save_struc->next)
      fprintf(fh, "\\null\\");
    else
      _cfsml_write_gfx_dirty_rect_t(fh, save_struc->next);
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_gfx_dirty_rect_t(FILE *fh, gfx_dirty_rect_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "rect")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_rect_t(fh, &(save_struc->rect), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_rect_t() for rect at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "next")) {
#line 738 "savegame.cfsml"
        if (strcmp(value, "\\null\\")) { /* null pointer? */
           save_struc->next = sci_malloc(sizeof (gfx_dirty_rect_t));
           _cfsml_register_pointer(save_struc->next);
           if (_cfsml_read_gfx_dirty_rect_t(fh, save_struc->next, value, line, hiteof)) {
              _cfsml_error("Token expected by _cfsml_read_gfx_dirty_rect_t() for next at line %d\n", *line);
              return CFSML_FAILURE;
           }
        } else save_struc->next = NULL;
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 444 "savegame.cfsml"
static void
_cfsml_write_view_object_t(FILE *fh, view_object_t* save_struc)
{
  int min, max, i;

#line 464 "savegame.cfsml"
  fprintf(fh, "{\n");
  fprintf(fh, "obj = ");
    write_heapptr(fh, &(save_struc->obj));
    fprintf(fh, "\n");
  fprintf(fh, "signalp = ");
    write_heapptr(fh, &(save_struc->signalp));
    fprintf(fh, "\n");
  fprintf(fh, "underBitsp = ");
    write_heapptr(fh, &(save_struc->underBitsp));
    fprintf(fh, "\n");
  fprintf(fh, "x = ");
    _cfsml_write_int(fh, &(save_struc->x));
    fprintf(fh, "\n");
  fprintf(fh, "y = ");
    _cfsml_write_int(fh, &(save_struc->y));
    fprintf(fh, "\n");
  fprintf(fh, "priority = ");
    _cfsml_write_int(fh, &(save_struc->priority));
    fprintf(fh, "\n");
  fprintf(fh, "view_nr = ");
    _cfsml_write_int(fh, &(save_struc->view_nr));
    fprintf(fh, "\n");
  fprintf(fh, "loop = ");
    _cfsml_write_int(fh, &(save_struc->loop));
    fprintf(fh, "\n");
  fprintf(fh, "cel = ");
    _cfsml_write_int(fh, &(save_struc->cel));
    fprintf(fh, "\n");
  fprintf(fh, "nsTop = ");
    _cfsml_write_int(fh, &(save_struc->nsTop));
    fprintf(fh, "\n");
  fprintf(fh, "nsLeft = ");
    _cfsml_write_int(fh, &(save_struc->nsLeft));
    fprintf(fh, "\n");
  fprintf(fh, "nsRight = ");
    _cfsml_write_int(fh, &(save_struc->nsRight));
    fprintf(fh, "\n");
  fprintf(fh, "nsBottom = ");
    _cfsml_write_int(fh, &(save_struc->nsBottom));
    fprintf(fh, "\n");
  fprintf(fh, "underBits = ");
    _cfsml_write_int(fh, &(save_struc->underBits));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 538 "savegame.cfsml"
static int
_cfsml_read_view_object_t(FILE *fh, view_object_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
#line 599 "savegame.cfsml"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token) {
       _cfsml_error("Expected token at line %d\n", *line);
       return CFSML_FAILURE;
    }
    if (!assignment) {
      if (!strcmp(token, "}"))
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value) {
        _cfsml_error("Expected token at line %d\n", *line);
        return CFSML_FAILURE;
      }
      if (!strcmp(token, "obj")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->obj), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for obj at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "signalp")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->signalp), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for signalp at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "underBitsp")) {
#line 749 "savegame.cfsml"
         if (read_heapptr(fh, &(save_struc->underBitsp), value, line, hiteof)) {
            _cfsml_error("Token expected by read_heapptr() for underBitsp at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "x")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->x), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for x at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "y")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->y), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for y at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "priority")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->priority), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for priority at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "view_nr")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->view_nr), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for view_nr at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "loop")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->loop), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for loop at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "cel")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->cel), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for cel at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "nsTop")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->nsTop), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for nsTop at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "nsLeft")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->nsLeft), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for nsLeft at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "nsRight")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->nsRight), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for nsRight at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "nsBottom")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->nsBottom), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for nsBottom at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
      if (!strcmp(token, "underBits")) {
#line 749 "savegame.cfsml"
         if (_cfsml_read_int(fh, &(save_struc->underBits), value, line, hiteof)) {
            _cfsml_error("Token expected by _cfsml_read_int() for underBits at line %d\n", *line);
            return CFSML_FAILURE;
         }
      } else
#line 758 "savegame.cfsml"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;
       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}


/* Auto-generated CFSML declaration and function block ends here */
/* Auto-generation performed by cfsml.pl 0.8.2 */
#line 448 "savegame.cfsml"

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
		fputs(HUNK_TYPE_GFX_SNAPSHOT_STRING, fh);
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_snapshot_t(fh, (*(((gfxw_snapshot_t**)foo->data))));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 464 "savegame.cfsml"
		return;

	} else { /* Normal buffer */

		while ((hunkfile = open(filename, O_RDONLY | O_BINARY)) > 0) {
			close(hunkfile);
			sprintf(filename + 5, "%d", ++filenr);
		}


		/* Visual C++ doesn't allow to specify O_BINARY with creat() */
#ifdef _MSC_VER
		hunkfile = open(filename, _O_CREAT | _O_BINARY | _O_RDWR);
#else
		hunkfile = creat(filename, 0600);
#endif

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

	if (lastval[0] == HUNK_TYPE_GFX_SNAPSHOT_STRING[0]) { /* Graphical buffer */
		foo->type = HUNK_TYPE_GFXBUFFER;
		foo->size = sizeof(gfxw_snapshot_t);
		foo->data = sci_malloc(sizeof (gfxw_snapshot_t *));
		*((gfxw_snapshot_t **)foo->data) = sci_malloc(sizeof (gfxw_snapshot_t));

/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfxw_snapshot_t(fh, (*(gfxw_snapshot_t**)(foo->data)), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 508 "savegame.cfsml"

		return 0;

	} else { /* Normal hunk */

		foo->type = HUNK_TYPE_ANY;

		hunkfile = open(lastval, O_RDONLY | O_BINARY);

		foo->size = lseek(hunkfile, 0, SEEK_END);
		lseek(hunkfile, 0, SEEK_SET);

		foo->data = (char *) sci_malloc(foo->size);
		read(hunkfile, foo->data, foo->size);
		close(hunkfile);

		return 0;
	}

	return 0;
}

struct {
	gfxw_widget_types_t type;
	char *name;
} widget_string_names[] = {
	{GFXW_BOX, "BOX"},
	{GFXW_RECT, "PRIMITIVE"},
	{GFXW_LINE, "PRIMITIVE"},
	{GFXW_INVERSE_LINE, "PRIMITIVE"},
	{GFXW_STATIC_VIEW, "VIEW"},
	{GFXW_VIEW, "VIEW"},
	{GFXW_DYN_VIEW, "DYNVIEW"},
	{GFXW_PIC_VIEW, "PICVIEW"},
	{GFXW_TEXT, "TEXT"},
	{GFXW_SORTED_LIST, "LIST"},
	{GFXW_LIST, "LIST"},
	{GFXW_VISUAL, "VISUAL"},
	{GFXW_PORT, "PORT"},
	{GFXW_CONTAINER, "_CONTAINER"},
	{GFXW_, "_WIDGET"},
	{-1, NULL}
};

static char *
stringify_widget_type(int type)
{
	int i = 0;
	while (widget_string_names[i].type != type
	       && widget_string_names[i].name)
		i++;

	return widget_string_names[i].name;
}

static gfxw_widget_types_t
parse_widget_type(char *string) /* Rather slow, but WTF */
{
	int i = 0;
	while (widget_string_names[i].name
	       && strcmp(widget_string_names[i].name, string))
		i++;

	return widget_string_names[i].type;
}

void
write_any_widget(FILE *fh, gfxw_widget_t **widget)
{
	char *type_name;

	if (*widget == NULL) {
		fputs("\\null\\", fh);
		return;
	}

	type_name = stringify_widget_type((*widget)->type);
	if (type_name)
		fputs(type_name, fh);
	else {
		sciprintf("While writing widget: Encountered invalid widget type %d\n",
			  (*widget)->type);
		fputs("\\null\\", fh);
		return;
	}

	fputs("\n<\n", fh);

	switch ((*widget)->type) {

	case GFXW_BOX:
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_box_t(fh, ((gfxw_box_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 600 "savegame.cfsml"
		break;

	case GFXW_RECT:
	case GFXW_LINE:
	case GFXW_INVERSE_LINE:
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_primitive_t(fh, ((gfxw_primitive_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 606 "savegame.cfsml"
		break;

	case GFXW_VIEW:
	case GFXW_STATIC_VIEW:
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_view_t(fh, ((gfxw_view_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 611 "savegame.cfsml"
		break;

	case GFXW_DYN_VIEW:
	case GFXW_PIC_VIEW:
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_dyn_view_t(fh, ((gfxw_dyn_view_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 616 "savegame.cfsml"
		break;

	case GFXW_TEXT:
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_text_t(fh, ((gfxw_text_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 620 "savegame.cfsml"
		break;


	case GFXW_SORTED_LIST:
	case GFXW_LIST:
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_list_t(fh, ((gfxw_list_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 626 "savegame.cfsml"
		break;

	case GFXW_VISUAL:
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_visual_t(fh, ((gfxw_visual_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 630 "savegame.cfsml"
		break;

	case GFXW_PORT:
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_port_t(fh, ((gfxw_port_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 634 "savegame.cfsml"
		break;

	case GFXW_:
	case GFXW_CONTAINER:
	default:
		sciprintf("While writing widget: Invalid widget type while writing widget:\n");
		(*widget)->print((*widget), 0);

		if ((*widget)->type == GFXW_CONTAINER) {
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_container_t(fh, ((gfxw_container_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 644 "savegame.cfsml"
		} else {
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_widget_t(fh, (*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 646 "savegame.cfsml"
		}
		break;
	}
	fputs(">\n", fh);
}

void
_gfxw_set_ops_BOX(gfxw_widget_t *prim);
void
_gfxw_set_ops_RECT(gfxw_widget_t *prim);
void
_gfxw_set_ops_LINE(gfxw_widget_t *prim);
void
_gfxw_set_ops_VIEW(gfxw_widget_t *view, char stat);
void
_gfxw_set_ops_DYNVIEW(gfxw_widget_t *widget);
void
_gfxw_set_ops_PICVIEW(gfxw_widget_t *widget);
void
_gfxw_set_ops_TEXT(gfxw_widget_t *widget);
void
_gfxw_set_ops_LIST(gfxw_container_t *widget, char sorted);
void
_gfxw_set_ops_VISUAL(gfxw_container_t *visual);
void
_gfxw_set_ops_PORT(gfxw_container_t *widget);


void
full_widget_tree_traversal(gfxw_widget_t *widget, gfxw_container_t *parent, gfxw_visual_t *visual)
{
	if (!widget)
		return;

	widget->parent = parent;

	if (GFXW_IS_VISUAL(widget)) {
		visual = (gfxw_visual_t *) widget;
		visual->gfx_state = _global_save_state->gfx_state;

		if (visual->port_refs_nr < 1)
			sciprintf("visual->port_refs_nr is too small: %d!\n", visual->port_refs_nr);

		visual->port_refs = sci_calloc(sizeof(gfxw_port_t *), visual->port_refs_nr);
	}

	if (widget->next)
		full_widget_tree_traversal(widget->next, parent, visual);

	widget->visual = visual;

	if (GFXW_IS_CONTAINER(widget)) {
		gfxw_container_t *container = GFXWC(widget);

		full_widget_tree_traversal(container->contents, container, visual);

		if (!container->contents)
			container->nextpp = &(container->next);
		else {
			gfxw_widget_t *next_seeker = container->contents;

			while (next_seeker->next)
				next_seeker = next_seeker->next;

			container->nextpp = &(next_seeker->next);
		}

		if (GFXW_IS_PORT(container)) {
			gfxw_port_t *port = (gfxw_port_t *) container;

			full_widget_tree_traversal(GFXW(port->decorations), container, visual);
			full_widget_tree_traversal(port->port_bg, container, visual);

			if (visual->port_refs_nr <= port->ID
			    || 0 > port->ID)
				sciprintf("Restored port with invalid ID #%d\n", port->ID);
			else
				visual->port_refs[port->ID] = port; /* List port globally */

		}
	}

	switch (widget->type) {

	case GFXW_BOX:
		_gfxw_set_ops_BOX(widget);
		break;

	case GFXW_RECT:
		_gfxw_set_ops_RECT(widget);
		break;

	case GFXW_LINE:
	case GFXW_INVERSE_LINE:
		_gfxw_set_ops_LINE(widget);
		break;

	case GFXW_VIEW:
		_gfxw_set_ops_VIEW(widget, 0);
		break;

	case GFXW_STATIC_VIEW:
		_gfxw_set_ops_VIEW(widget, 1);
		break;

	case GFXW_DYN_VIEW:
		_gfxw_set_ops_DYNVIEW(widget);
		break;

	case GFXW_PIC_VIEW:
		_gfxw_set_ops_PICVIEW(widget);
		break;

	case GFXW_TEXT:
		((gfxw_text_t *) widget)->text_handle = NULL;
		_gfxw_set_ops_TEXT(widget);
		break;

	case GFXW_LIST:
		if (widget->serial == _global_save_state->drop_views_list_serial)
			_global_save_state->drop_views = (gfxw_list_t *) widget;
		_gfxw_set_ops_LIST(GFXWC(widget), 0);
		break;

	case GFXW_SORTED_LIST:
		_gfxw_set_ops_LIST(GFXWC(widget), 1);
		if (widget->serial == _global_save_state->dyn_views_list_serial)
			_global_save_state->dyn_views = (gfxw_list_t *) widget;
		break;

	case GFXW_VISUAL:
		_gfxw_set_ops_VISUAL(GFXWC(widget));
		break;

	case GFXW_PORT:
		if (widget->serial == _global_save_state->port_serial)
			_global_save_state->port = (gfxw_port_t *) widget;
		_gfxw_set_ops_PORT(GFXWC(widget));
		break;
	}
}

gfxw_widget_t *
_gfxw_new_widget(int size, int type);

int
read_any_widget(FILE *fh, gfxw_widget_t **widget, char *lastval, int *line, int *hiteof)
{
	gfxw_widget_types_t expected_type;
	int nextchar;

	if (!strcmp(lastval, "\\null\\")) {
		*widget = NULL;
		return 0;
		/* That was easy */
	}

	expected_type = parse_widget_type(lastval);

	if (expected_type == -1) {
		sciprintf("Invalid widget type \"%s\" specified in line %d\n",
			  lastval, *line);
		return 1;
	}

	while ((nextchar = fgetc(fh)) != '<') {
		if (nextchar == EOF)
			return ((*hiteof = 1));

		if (nextchar == '\n')
			(*line)++;
		else if (!isspace(nextchar)) {
			sciprintf("Invalid character encountered: '%c' (%02x) in line %d\n",
				  nextchar, nextchar, *line);
			return 1;
		}
	}

	switch (expected_type) {

	case GFXW_BOX:
		*widget = _gfxw_new_widget(sizeof(gfxw_box_t), expected_type);
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfxw_box_t(fh, ((gfxw_box_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 829 "savegame.cfsml"
		break;

	case GFXW_RECT:
	case GFXW_LINE:
	case GFXW_INVERSE_LINE:
		*widget = _gfxw_new_widget(sizeof(gfxw_primitive_t), expected_type);
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfxw_primitive_t(fh, ((gfxw_primitive_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 836 "savegame.cfsml"
		break;

	case GFXW_VIEW:
	case GFXW_STATIC_VIEW:
		*widget = _gfxw_new_widget(sizeof(gfxw_view_t), expected_type);
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfxw_view_t(fh, ((gfxw_view_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 842 "savegame.cfsml"
		break;

	case GFXW_DYN_VIEW:
	case GFXW_PIC_VIEW:
		*widget = _gfxw_new_widget(sizeof(gfxw_dyn_view_t), expected_type);
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfxw_dyn_view_t(fh, ((gfxw_dyn_view_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 848 "savegame.cfsml"
		(*widget)->type = expected_type;
		if (FILE_VERSION == 1)
			((gfxw_dyn_view_t *) widget)->force_precedence = 0;
		break;

	case GFXW_TEXT:
		*widget = _gfxw_new_widget(sizeof(gfxw_text_t), expected_type);
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfxw_text_t(fh, ((gfxw_text_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 856 "savegame.cfsml"
		(*widget)->type = expected_type;
		break;


	case GFXW_LIST:
	case GFXW_SORTED_LIST:
		*widget = _gfxw_new_widget(sizeof(gfxw_list_t), expected_type);
		GFXWC(*widget)->contents = NULL;
		GFXWC(*widget)->dirty = NULL;
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfxw_list_t(fh, ((gfxw_list_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 866 "savegame.cfsml"
		(*widget)->type = expected_type;
		break;

	case GFXW_VISUAL:
		*widget = _gfxw_new_widget(sizeof(gfxw_visual_t), expected_type);
		GFXWC(*widget)->contents = NULL;
		GFXWC(*widget)->dirty = NULL;
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfxw_visual_t(fh, ((gfxw_visual_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 874 "savegame.cfsml"
		(*widget)->type = expected_type;

		full_widget_tree_traversal(*widget, NULL, NULL);
		break;

	case GFXW_PORT:
		*widget = _gfxw_new_widget(sizeof(gfxw_port_t), expected_type);
		GFXWC(*widget)->contents = NULL;
		GFXWC(*widget)->dirty = NULL;
		((gfxw_port_t *) (*widget))->title_text = NULL;
		((gfxw_port_t *) (*widget))->decorations = NULL;
		((gfxw_port_t *) (*widget))->port_bg = NULL;
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfxw_port_t(fh, ((gfxw_port_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 887 "savegame.cfsml"
		(*widget)->type = expected_type;
		break;

	case GFXW_CONTAINER:
		*widget = _gfxw_new_widget(sizeof(gfxw_container_t), expected_type);
		GFXWC(*widget)->contents = NULL;
		GFXWC(*widget)->dirty = NULL;
		sciprintf("Warning: Restoring untyped widget container\n");
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfxw_container_t(fh, ((gfxw_container_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 896 "savegame.cfsml"
		(*widget)->type = expected_type;
		break;

	case GFXW_:
		*widget = _gfxw_new_widget(sizeof(gfxw_widget_t), expected_type);
		sciprintf("Warning: Restoring untyped widget\n");
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfxw_widget_t(fh, (*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 903 "savegame.cfsml"
		(*widget)->type = expected_type;
		break;

	}

	(*widget)->flags |= GFXW_FLAG_DIRTY;

	while ((nextchar = fgetc(fh)) != '>') {
		if (nextchar == EOF)
			return ((*hiteof = 1));

		if (nextchar == '\n')
			(*line)++;
		else if (!isspace(nextchar)) {
			sciprintf("Invalid character encountered: '%c' (%02x) in line %d\n",
				  nextchar, nextchar, *line);
			return 1;
		}
	}

	return (*hiteof);
}
/* Identify, handle stuff listed as "handled externally" */

void
write_pixmap_color(FILE *fh, gfx_pixmap_color_t *color)
{
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfx_pixmap_color_t(fh, (color));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 931 "savegame.cfsml"
}

int
read_pixmap_color(FILE *fh, gfx_pixmap_color_t *color, char *lastval, int *line, int *hiteof)
{
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 839 "savegame.cfsml"
    char *_cfsml_inp = lastval;
#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_gfx_pixmap_color_t(fh, (color), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 937 "savegame.cfsml"

        color->global_index = GFX_COLOR_INDEX_UNMAPPED;

	if (_global_save_state->gfx_state->driver->mode->palette)
		if (gfx_alloc_color(_global_save_state->gfx_state->driver->mode->palette, color) < 0)
			sciprintf("While restoring color index: Allocating color entry for (%02x,%02x,%02x) failed!\n",
				  color->r, color->g, color->b);

	return *hiteof;
}



void
write_menubar_tp(FILE *fh, menubar_t **foo)
{
	if (*foo) {

#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_menubar_t(fh, (*foo));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 956 "savegame.cfsml"

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

		*foo = (menubar_t *) sci_malloc(sizeof(menubar_t));
/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 839 "savegame.cfsml"
    char *_cfsml_inp = lastval;
#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_menubar_t(fh, (*foo), _cfsml_inp, &(*line), &_cfsml_eof);
#line 852 "savegame.cfsml"
    *hiteof = _cfsml_error;
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 973 "savegame.cfsml"

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
	sci_dir_t dir;
	char *filename;
	int fd;

	_global_save_state = s;
	s->savegame_version = FREESCI_SAVEGAME_VERSION;
	s->dyn_views_list_serial = (s->dyn_views)? s->dyn_views->serial : -2;
	s->drop_views_list_serial = (s->drop_views)? s->drop_views->serial : -2;
	s->port_serial = (s->port)? s->port->serial : -2;

	if (s->execution_stack_base) {
		sciprintf("Cannot save from below kernel function\n");
		return 1;
	}

	scimkdir (dirname, 0700);

	if (chdir (dirname)) {
		sciprintf("Could not enter directory '%s'\n", dirname);
		return 1;
	}

	sci_init_dir(&dir);
	filename = sci_find_first(&dir, "*");
	while (filename) {
		if (strcmp(filename, "..") && strcmp(filename, "."))
			unlink(filename); /* Delete all files in directory */
		filename = sci_find_next(&dir);
	}
	sci_finish_find(&dir);

	if (s->sound_server) {
		if ((s->sound_server->save)(s, dirname)) {
			sciprintf("Saving failed for the sound subsystem\n");
			chdir ("..");
			return 1;
		}
	}

	fh = fopen("state", "w" FO_TEXT);

	/* Calculate the time spent with this game */
	s->game_time = time(NULL) - s->game_start_time.tv_sec;

SCI_MEMTEST;
#line 877 "savegame.cfsml"
/* Auto-generated CFSML data writer code */
  _cfsml_write_state_t(fh, s);
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 1040 "savegame.cfsml"
SCI_MEMTEST;

	fclose(fh);

	_gamestate_unfrob(s);


	/* Visual C++ doesn't allow to specify O_BINARY with creat() */
#ifdef _MSC_VER
	fd = open("heap", _O_CREAT | _O_BINARY | _O_RDWR);
#else
	fd = creat("heap", 0600);
#endif

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

	if (s->sound_server) {
		if ((s->sound_server->restore)(s, dirname)) {
			sciprintf("Restoring failed for the sound subsystem\n");
			return NULL;
		}
	}

	retval = (state_t *) sci_malloc(sizeof(state_t));
	retval->_heap = heap_new();
	retval->savegame_version = -1;
	_global_save_state = retval;
	retval->gfx_state = s->gfx_state;

	fh = fopen("state", "r" FO_TEXT);
	if (!fh) {
		heap_del(retval->_heap);
		free(retval);
		return NULL;
	}

	 /* Backwards compatibility settings */
	retval->amp_rest = 0;
	retval->dyn_views = NULL;
	retval->drop_views = NULL;
	retval->port = NULL;
	retval->save_dir_copy_buf = NULL;

	retval->sound_mute = s->sound_mute;
	retval->sound_volume = s->sound_volume;

/* Auto-generated CFSML data reader code */
#line 823 "savegame.cfsml"
  {
#line 826 "savegame.cfsml"
    int _cfsml_line_ctr = 0;
#line 831 "savegame.cfsml"
    struct _cfsml_pointer_refstruct **_cfsml_myptrrefptr = _cfsml_get_current_refpointer();
#line 834 "savegame.cfsml"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 842 "savegame.cfsml"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(_cfsml_line_ctr), &_cfsml_eof, &dummy);

#line 847 "savegame.cfsml"
    _cfsml_error = _cfsml_read_state_t(fh, retval, _cfsml_inp, &(_cfsml_line_ctr), &_cfsml_eof);
#line 852 "savegame.cfsml"
    read_eof = _cfsml_error;
#line 856 "savegame.cfsml"
     _cfsml_free_pointer_references(_cfsml_myptrrefptr, _cfsml_error);
#line 859 "savegame.cfsml"
     if (_cfsml_last_value_retreived) {
       free(_cfsml_last_value_retreived);
       _cfsml_last_value_retreived = NULL;
     }
     if (_cfsml_last_identifier_retreived) {
       free(_cfsml_last_identifier_retreived);
       _cfsml_last_identifier_retreived = NULL;
     }
  }
/* End of auto-generated CFSML data reader code */
#line 1107 "savegame.cfsml"

	fclose(fh);

	if ((retval->savegame_version < 1) || (retval->savegame_version > FREESCI_SAVEGAME_VERSION)) {

		if (retval->savegame_version < 3)
			sciprintf("Old savegame version detected- can't load\n");
		else
			sciprintf("Savegame version is %d- maximum supported is %0d\n", retval->savegame_version, FREESCI_SAVEGAME_VERSION);

		chdir("..");
		heap_del(retval->_heap);
		free(retval);
		return NULL;
	}

	if (read_eof || ((fd = open("heap", O_RDONLY | O_BINARY)) < 0)) {
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

	_gamestate_unfrob(retval);

	retval->wm_port = gfxw_find_port(retval->visual, 0);
	retval->titlebar_port = gfxw_find_port(retval->visual, 1);
	retval->picture_port = gfxw_find_port(retval->visual, 2);

	/* Set exec stack base to zero */
	retval->execution_stack_base = 0;

	/* Set the class table pointers to the script positions */
	for (i = 0; i < retval->classtable_size; i++)
		retval->classtable[i].scriptposp = &(retval->scripttable[retval->classtable[i].script].heappos);

	/* Now copy all current state information */
	/* Graphics and input state: */
	retval->animation_delay = s->animation_delay;
	retval->animation_granularity = s->animation_granularity;
	retval->gfx_state = s->gfx_state;
	gfxop_set_pointer_cursor(s->gfx_state, retval->mouse_pointer_nr);

	memcpy(retval->ega_colors, s->ega_colors, 16 * sizeof(gfx_color_t));

	if (FILE_VERSION > 1 && retval->pics_drawn_nr) {
		gfxop_new_pic(s->gfx_state, retval->pics[0].nr, 1, retval->pics[0].palette);
		for (i = 1; i < retval->pics_drawn_nr; i++)
			gfxop_add_to_pic(s->gfx_state, retval->pics[i].nr, 1, retval->pics[i].palette);
	} else {
		if (FILE_VERSION == 1) {
			retval->pics = sci_malloc(sizeof(drawn_pic_t) * (retval->pics_nr = 8));
			retval->pics_drawn_nr = 0;
		}
		gfxop_set_clip_zone(s->gfx_state, gfx_rect_fullscreen);
		gfxop_fill_box(s->gfx_state, gfx_rect_fullscreen, s->ega_colors[0]);
		gfxop_update(s->gfx_state);
	}
	gfxop_update_box(s->gfx_state, gfx_rect(0, 0, 320, 200));
	gfxop_clear_box(s->gfx_state, gfx_rect(0, 0, 320, 200));

	if (retval->visual) {
		gfxop_set_clip_zone(retval->gfx_state, gfx_rect_fullscreen);
		retval->visual->gfx_state = retval->gfx_state;
		retval->visual->add_dirty_abs(GFXWC(retval->visual), gfx_rect_fullscreen, 1);
		retval->visual->draw(GFXW(retval->visual), gfxw_point_zero);
	}
	gfxop_update_box(s->gfx_state, gfx_rect(0, 0, 320, 200));

	if (!retval->port && FILE_VERSION > 1) {
		fprintf(stderr,"Found no valid port for port serial number %08x!\n", retval->port_serial);
		retval->visual->print(GFXW(retval->visual), 0);
		return NULL;
	}

	/* Sound state: */
	retval->sound_server = s->sound_server;
/*
	memcpy(&(retval->sound_pipe_in[0]), &(s->sound_pipe_in[0]), sizeof(int[2]));
	memcpy(&(retval->sound_pipe_out[0]), &(s->sound_pipe_out[0]), sizeof(int[2]));
	memcpy(&(retval->sound_pipe_events[0]), &(s->sound_pipe_events[0]), sizeof(int[2]));
	memcpy(&(retval->sound_pipe_debug[0]), &(s->sound_pipe_debug[0]), sizeof(int[2]));
*/
	/* Time state: */
	sci_get_current_time(&(retval->last_wait_time));
	retval->game_start_time.tv_sec = time(NULL) - retval->game_time;
	retval->game_start_time.tv_usec = 0;

	/* File IO state: */
	retval->file_handles_nr = 2;
	retval->file_handles = sci_calloc(2, sizeof(FILE *));

	if (!retval->save_dir_copy_buf) { /* FIXME: Maybe other versions as well! */
		retval->save_dir_copy_buf = sci_malloc(MAX_SAVE_DIR_SIZE);
		retval->save_dir = heap_allocate(retval->_heap, MAX_SAVE_DIR_SIZE);
		if (!retval->save_dir) {
			fprintf(stderr, "ERROR: While restoring old savegame: Not enough space to allocate space for"
				" save_dir buffer!\n");
			return NULL;
			/* This should only happen with corrupt save states, since all games HAVE enough
			** spare memory for this.  */
		}
	}


	strcpy(retval->save_dir_copy_buf, s->save_dir_copy_buf);
	if (retval->save_dir)
		strcpy(retval->heap + retval->save_dir + 2, s->heap + s->save_dir + 2);

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

	/* Copy version information */
	retval->version = s->version;
	retval->max_version = s->max_version;
	retval->min_version = s->min_version;

	/* Copy breakpoint information from current game instance */
	retval->have_bp = s->have_bp;
	retval->bp_list = s->bp_list;

	retval->debug_mode = s->debug_mode;

	retval->resource_dir = s->resource_dir;
	retval->work_dir = s->work_dir;
	retval->kernel_opt_flags = 0;

	retval->resmgr = s->resmgr;

	if (retval->savegame_version < 3) {
		char *cwd = sci_getcwd();

		retval->save_dir = heap_allocate(retval->_heap, MAX_SAVE_DIR_SIZE);
		/* Compensate for save_dir location change */
		if (strlen(cwd) > MAX_SAVE_DIR_SIZE)
			sciprintf("Warning: cwd '%s' is longer than the"
				  " MAX_SAVE_DIR_SIZE %d\n",
				  cwd, MAX_SAVE_DIR_SIZE);
		else
			strcpy(retval->heap + retval->save_dir + 2, cwd);

		sci_free(cwd);

		retval->save_dir_copy = 0xffff;
		retval->save_dir_edit_offset = 0;
	}

	if (retval->savegame_version < 4) {
		char *cwd = sci_getcwd();
		retval->save_dir_copy_buf = sci_malloc(MAX_SAVE_DIR_SIZE);
		strcpy(retval->save_dir_copy_buf, cwd);
		sci_free(cwd);
	}

	retval->successor = NULL;
	retval->pic_priority_table = gfxop_get_pic_metainfo(retval->gfx_state);

	chdir ("..");

	return retval;
}
