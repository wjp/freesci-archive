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

#include <gfx_operations.h>
#include <engine.h>
#include <assert.h>
#include <heap.h>

#ifdef _MSC_VER
#include <direct.h>
#endif

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



/* Auto-generated CFSML declaration and function block */

#line 688 "cfsml.pl"
#define CFSML_SUCCESS 0
#define CFSML_FAILURE 1

#line 53 "cfsml.pl"

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

    _cfsml_free_pointer_references_recursively(refs->next, free_pointers);

    if (free_pointers)
	free(refs->ptr);

    free(refs);
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
    struct _cfsml_pointer_refstruct *newref = malloc(sizeof (struct _cfsml_pointer_refstruct));

    newref->next = *_cfsml_pointer_references_current;
    newref->ptr = ptr;
    *_cfsml_pointer_references_current = newref;
}


static char *
_cfsml_mangle_string(char *s)
{
  char *source = s;
  char c;
  char *target = (char *) malloc(1 + strlen(s) * 2); /* We will probably need less than that */
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

  return (char *) realloc(target, strlen(target) + 1);
}


static char *
_cfsml_unmangle_string(char *s)
{
  char *target = (char *) malloc(1 + strlen(s));
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

  return (char *) realloc(target, strlen(target) + 1);
}


static char *
_cfsml_get_identifier(FILE *fd, int *line, int *hiteof, int *assignment)
{
  char c;
  int mem = 32;
  int pos = 0;
  int done = 0;
  char *retval = (char *) malloc(mem);

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
       retval = (char *) realloc(retval, mem *= 2);

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
     retval = (char *) realloc(retval, mem += 1);

  retval[pos] = 0; /* Terminate string */
#line 241 "cfsml.pl"

  return _cfsml_last_identifier_retreived = retval;
}


static char *
_cfsml_get_value(FILE *fd, int *line, int *hiteof)
{
  char c;
  int mem = 64;
  int pos = 0;
  char *retval = (char *) malloc(mem);

  if (_cfsml_last_value_retreived) {
      free(_cfsml_last_value_retreived);
      _cfsml_last_value_retreived = NULL;
  }

  while (((c = fgetc(fd)) != EOF) && (c != '\n')) {

     if (pos == mem - 1) /* Need more memory? */
       retval = (char *) realloc(retval, mem *= 2);

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
    retval = (char *) realloc(retval, mem += 1);

  retval[pos] = 0; /* Terminate string */
#line 298 "cfsml.pl"
  return (_cfsml_last_value_retreived = (char *) realloc(retval, strlen(retval) + 1));
  /* Re-allocate; this value might be used for quite some while (if we are
  ** restoring a string)
  */
}
#line 350 "cfsml.pl"
static void
_cfsml_write_rect_t(FILE *fh, rect_t* foo);
static int
_cfsml_read_rect_t(FILE *fh, rect_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfxw_visual_t(FILE *fh, gfxw_visual_t* foo);
static int
_cfsml_read_gfxw_visual_t(FILE *fh, gfxw_visual_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfx_line_mode_t(FILE *fh, gfx_line_mode_t* foo);
static int
_cfsml_read_gfx_line_mode_t(FILE *fh, gfx_line_mode_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_view_object_t(FILE *fh, view_object_t* foo);
static int
_cfsml_read_view_object_t(FILE *fh, view_object_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfx_pixmap_color_t(FILE *fh, gfx_pixmap_color_t* foo);
static int
_cfsml_read_gfx_pixmap_color_t(FILE *fh, gfx_pixmap_color_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfxw_primitive_t(FILE *fh, gfxw_primitive_t* foo);
static int
_cfsml_read_gfxw_primitive_t(FILE *fh, gfxw_primitive_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_menu_t(FILE *fh, menu_t* foo);
static int
_cfsml_read_menu_t(FILE *fh, menu_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfxw_view_t(FILE *fh, gfxw_view_t* foo);
static int
_cfsml_read_gfxw_view_t(FILE *fh, gfxw_view_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_drawn_pic_t(FILE *fh, drawn_pic_t* foo);
static int
_cfsml_read_drawn_pic_t(FILE *fh, drawn_pic_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfxw_list_t(FILE *fh, gfxw_list_t* foo);
static int
_cfsml_read_gfxw_list_t(FILE *fh, gfxw_list_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_int(FILE *fh, int* foo);
static int
_cfsml_read_int(FILE *fh, int* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_exec_stack_t(FILE *fh, exec_stack_t* foo);
static int
_cfsml_read_exec_stack_t(FILE *fh, exec_stack_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfx_line_style_t(FILE *fh, gfx_line_style_t* foo);
static int
_cfsml_read_gfx_line_style_t(FILE *fh, gfx_line_style_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_state_t(FILE *fh, state_t* foo);
static int
_cfsml_read_state_t(FILE *fh, state_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfx_color_t(FILE *fh, gfx_color_t* foo);
static int
_cfsml_read_gfx_color_t(FILE *fh, gfx_color_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gint16(FILE *fh, gint16* foo);
static int
_cfsml_read_gint16(FILE *fh, gint16* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfx_box_shade_t(FILE *fh, gfx_box_shade_t* foo);
static int
_cfsml_read_gfx_box_shade_t(FILE *fh, gfx_box_shade_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfxw_widget_types_t(FILE *fh, gfxw_widget_types_t* foo);
static int
_cfsml_read_gfxw_widget_types_t(FILE *fh, gfxw_widget_types_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_script_t(FILE *fh, script_t* foo);
static int
_cfsml_read_script_t(FILE *fh, script_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_class_t(FILE *fh, class_t* foo);
static int
_cfsml_read_class_t(FILE *fh, class_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_synonym_t(FILE *fh, synonym_t* foo);
static int
_cfsml_read_synonym_t(FILE *fh, synonym_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_long(FILE *fh, long* foo);
static int
_cfsml_read_long(FILE *fh, long* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfx_dirty_rect_t(FILE *fh, gfx_dirty_rect_t* foo);
static int
_cfsml_read_gfx_dirty_rect_t(FILE *fh, gfx_dirty_rect_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfxw_container_t(FILE *fh, gfxw_container_t* foo);
static int
_cfsml_read_gfxw_container_t(FILE *fh, gfxw_container_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfxw_widget_t(FILE *fh, gfxw_widget_t* foo);
static int
_cfsml_read_gfxw_widget_t(FILE *fh, gfxw_widget_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfxw_dyn_view_t(FILE *fh, gfxw_dyn_view_t* foo);
static int
_cfsml_read_gfxw_dyn_view_t(FILE *fh, gfxw_dyn_view_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfxw_text_t(FILE *fh, gfxw_text_t* foo);
static int
_cfsml_read_gfxw_text_t(FILE *fh, gfxw_text_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfxw_port_t(FILE *fh, gfxw_port_t* foo);
static int
_cfsml_read_gfxw_port_t(FILE *fh, gfxw_port_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_byte(FILE *fh, byte* foo);
static int
_cfsml_read_byte(FILE *fh, byte* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfx_alignment_t(FILE *fh, gfx_alignment_t* foo);
static int
_cfsml_read_gfx_alignment_t(FILE *fh, gfx_alignment_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_menu_item_t(FILE *fh, menu_item_t* foo);
static int
_cfsml_read_menu_item_t(FILE *fh, menu_item_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_string(FILE *fh, char ** foo);
static int
_cfsml_read_string(FILE *fh, char ** foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_gfxw_box_t(FILE *fh, gfxw_box_t* foo);
static int
_cfsml_read_gfxw_box_t(FILE *fh, gfxw_box_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_menubar_t(FILE *fh, menubar_t* foo);
static int
_cfsml_read_menubar_t(FILE *fh, menubar_t* foo, char *lastval, int *line, int *hiteof);

#line 350 "cfsml.pl"
static void
_cfsml_write_point_t(FILE *fh, point_t* foo);
static int
_cfsml_read_point_t(FILE *fh, point_t* foo, char *lastval, int *line, int *hiteof);

#line 363 "cfsml.pl"
static void
_cfsml_write_rect_t(FILE *fh, rect_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "x = ");
    _cfsml_write_int(fh, &(foo->x));
    fprintf(fh, "\n");
  fprintf(fh, "y = ");
    _cfsml_write_int(fh, &(foo->y));
    fprintf(fh, "\n");
  fprintf(fh, "xl = ");
    _cfsml_write_int(fh, &(foo->xl));
    fprintf(fh, "\n");
  fprintf(fh, "yl = ");
    _cfsml_write_int(fh, &(foo->yl));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_rect_t(FILE *fh, rect_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->x), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "y")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->y), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "xl")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->xl), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "yl")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->yl), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfxw_visual_t(FILE *fh, gfxw_visual_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(foo->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(foo->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(foo->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(foo->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(foo->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(foo->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "zone = ");
    _cfsml_write_rect_t(fh, &(foo->zone));
    fprintf(fh, "\n");
  fprintf(fh, "dirty = ");
    if (!foo->dirty)
      fprintf(fh, "\\null\\");    else 
      _cfsml_write_gfx_dirty_rect_t(fh, foo->dirty);
    fprintf(fh, "\n");
  fprintf(fh, "contents = ");
    write_any_widget(fh, &(foo->contents));
    fprintf(fh, "\n");
  fprintf(fh, "port_refs_nr = ");
    _cfsml_write_int(fh, &(foo->port_refs_nr));
    fprintf(fh, "\n");
  fprintf(fh, "font_nr = ");
    _cfsml_write_int(fh, &(foo->font_nr));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfxw_visual_t(FILE *fh, gfxw_visual_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "magic")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->magic), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bounds")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->bounds), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "next")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->next), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ID")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ID), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "widget_priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->widget_priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "zone")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->zone), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "dirty")) {
#line 634 "cfsml.pl"
        if (strcmp(value, "\\null\\")) { /* null pointer? */
           foo->dirty = malloc(sizeof (gfx_dirty_rect_t));
           _cfsml_register_pointer(foo->dirty);
           if (_cfsml_read_gfx_dirty_rect_t(fh, foo->dirty, value, line, hiteof))
              return CFSML_FAILURE;
        } else foo->dirty = NULL;
      } else
      if (!strcmp(bar, "contents")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->contents), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "port_refs_nr")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->port_refs_nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "font_nr")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->font_nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfx_line_mode_t(FILE *fh, gfx_line_mode_t* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfx_line_mode_t(FILE *fh, gfx_line_mode_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 479 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_view_object_t(FILE *fh, view_object_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "obj = ");
    write_heapptr(fh, &(foo->obj));
    fprintf(fh, "\n");
  fprintf(fh, "signalp = ");
    write_heapptr(fh, &(foo->signalp));
    fprintf(fh, "\n");
  fprintf(fh, "underBitsp = ");
    write_heapptr(fh, &(foo->underBitsp));
    fprintf(fh, "\n");
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
  fprintf(fh, "underBits = ");
    _cfsml_write_int(fh, &(foo->underBits));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_view_object_t(FILE *fh, view_object_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "obj")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->obj), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "signalp")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->signalp), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "underBitsp")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->underBitsp), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "x")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->x), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "y")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->y), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "view_nr")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->view_nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "loop")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->loop), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "cel")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->cel), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "nsTop")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->nsTop), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "nsLeft")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->nsLeft), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "nsRight")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->nsRight), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "nsBottom")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->nsBottom), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "underBits")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->underBits), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfx_pixmap_color_t(FILE *fh, gfx_pixmap_color_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "r = ");
    _cfsml_write_byte(fh, &(foo->r));
    fprintf(fh, "\n");
  fprintf(fh, "g = ");
    _cfsml_write_byte(fh, &(foo->g));
    fprintf(fh, "\n");
  fprintf(fh, "b = ");
    _cfsml_write_byte(fh, &(foo->b));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfx_pixmap_color_t(FILE *fh, gfx_pixmap_color_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "r")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->r), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "g")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->g), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "b")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->b), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfxw_primitive_t(FILE *fh, gfxw_primitive_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(foo->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(foo->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(foo->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(foo->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(foo->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(foo->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "color = ");
    _cfsml_write_gfx_color_t(fh, &(foo->color));
    fprintf(fh, "\n");
  fprintf(fh, "line_mode = ");
    _cfsml_write_gfx_line_mode_t(fh, &(foo->line_mode));
    fprintf(fh, "\n");
  fprintf(fh, "line_style = ");
    _cfsml_write_gfx_line_mode_t(fh, &(foo->line_style));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfxw_primitive_t(FILE *fh, gfxw_primitive_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "magic")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->magic), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bounds")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->bounds), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "next")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->next), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ID")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ID), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "widget_priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->widget_priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "color")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_color_t(fh, &(foo->color), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "line_mode")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_line_mode_t(fh, &(foo->line_mode), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "line_style")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_line_mode_t(fh, &(foo->line_style), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_menu_t(FILE *fh, menu_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
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
#line 407 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_menu_item_t(fh, &(foo->items[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_menu_t(FILE *fh, menu_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
#line 643 "cfsml.pl"
         if (_cfsml_read_string(fh, &(foo->title), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "title_width")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->title_width), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "width")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->width), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "items")) {
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 577 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max) {
           foo->items = (menu_item_t *) malloc(max * sizeof(menu_item_t));
           _cfsml_register_pointer(foo->items);
         }
         else
           foo->items = NULL;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
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
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfxw_view_t(FILE *fh, gfxw_view_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(foo->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(foo->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(foo->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(foo->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(foo->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(foo->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "pos = ");
    _cfsml_write_point_t(fh, &(foo->pos));
    fprintf(fh, "\n");
  fprintf(fh, "color = ");
    _cfsml_write_gfx_color_t(fh, &(foo->color));
    fprintf(fh, "\n");
  fprintf(fh, "view = ");
    _cfsml_write_int(fh, &(foo->view));
    fprintf(fh, "\n");
  fprintf(fh, "loop = ");
    _cfsml_write_int(fh, &(foo->loop));
    fprintf(fh, "\n");
  fprintf(fh, "cel = ");
    _cfsml_write_int(fh, &(foo->cel));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfxw_view_t(FILE *fh, gfxw_view_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "magic")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->magic), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bounds")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->bounds), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "next")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->next), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ID")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ID), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "widget_priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->widget_priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pos")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_point_t(fh, &(foo->pos), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "color")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_color_t(fh, &(foo->color), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "view")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->view), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "loop")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->loop), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "cel")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->cel), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_drawn_pic_t(FILE *fh, drawn_pic_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "nr = ");
    _cfsml_write_int(fh, &(foo->nr));
    fprintf(fh, "\n");
  fprintf(fh, "palette = ");
    _cfsml_write_int(fh, &(foo->palette));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_drawn_pic_t(FILE *fh, drawn_pic_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "nr")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "palette")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->palette), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfxw_list_t(FILE *fh, gfxw_list_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(foo->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(foo->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(foo->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(foo->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(foo->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(foo->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "zone = ");
    _cfsml_write_rect_t(fh, &(foo->zone));
    fprintf(fh, "\n");
  fprintf(fh, "dirty = ");
    if (!foo->dirty)
      fprintf(fh, "\\null\\");    else 
      _cfsml_write_gfx_dirty_rect_t(fh, foo->dirty);
    fprintf(fh, "\n");
  fprintf(fh, "contents = ");
    write_any_widget(fh, &(foo->contents));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfxw_list_t(FILE *fh, gfxw_list_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "magic")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->magic), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bounds")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->bounds), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "next")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->next), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ID")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ID), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "widget_priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->widget_priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "zone")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->zone), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "dirty")) {
#line 634 "cfsml.pl"
        if (strcmp(value, "\\null\\")) { /* null pointer? */
           foo->dirty = malloc(sizeof (gfx_dirty_rect_t));
           _cfsml_register_pointer(foo->dirty);
           if (_cfsml_read_gfx_dirty_rect_t(fh, foo->dirty, value, line, hiteof))
              return CFSML_FAILURE;
        } else foo->dirty = NULL;
      } else
      if (!strcmp(bar, "contents")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->contents), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_int(FILE *fh, int* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 455 "cfsml.pl"
static int
_cfsml_read_int(FILE *fh, int* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 479 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_exec_stack_t(FILE *fh, exec_stack_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
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
#line 407 "cfsml.pl"
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

#line 455 "cfsml.pl"
static int
_cfsml_read_exec_stack_t(FILE *fh, exec_stack_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->objp), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "sendp")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->sendp), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pc")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->pc), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "sp")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->sp), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "argc")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->argc), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "variables")) {
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = 4;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
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
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->selector), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "origin")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->origin), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfx_line_style_t(FILE *fh, gfx_line_style_t* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfx_line_style_t(FILE *fh, gfx_line_style_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 479 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_state_t(FILE *fh, state_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "savegame_version = ");
    _cfsml_write_int(fh, &(foo->savegame_version));
    fprintf(fh, "\n");
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
  fprintf(fh, "game_time = ");
    _cfsml_write_long(fh, &(foo->game_time));
    fprintf(fh, "\n");
  fprintf(fh, "save_dir = ");
    write_heapptr(fh, &(foo->save_dir));
    fprintf(fh, "\n");
  fprintf(fh, "sound_object = ");
    write_heapptr(fh, &(foo->sound_object));
    fprintf(fh, "\n");
  fprintf(fh, "sound_mute = ");
    _cfsml_write_int(fh, &(foo->sound_mute));
    fprintf(fh, "\n");
  fprintf(fh, "sound_volume = ");
    _cfsml_write_int(fh, &(foo->sound_volume));
    fprintf(fh, "\n");
  fprintf(fh, "mouse_pointer_nr = ");
    _cfsml_write_int(fh, &(foo->mouse_pointer_nr));
    fprintf(fh, "\n");
  fprintf(fh, "port_serial = ");
    _cfsml_write_int(fh, &(foo->port_serial));
    fprintf(fh, "\n");
  fprintf(fh, "dyn_views_list_serial = ");
    _cfsml_write_int(fh, &(foo->dyn_views_list_serial));
    fprintf(fh, "\n");
  fprintf(fh, "drop_views_list_serial = ");
    _cfsml_write_int(fh, &(foo->drop_views_list_serial));
    fprintf(fh, "\n");
  fprintf(fh, "visual = ");
    write_any_widget(fh, &(foo->visual));
    fprintf(fh, "\n");
  fprintf(fh, "pic_visible_map = ");
    _cfsml_write_int(fh, &(foo->pic_visible_map));
    fprintf(fh, "\n");
  fprintf(fh, "pic_animate = ");
    _cfsml_write_int(fh, &(foo->pic_animate));
    fprintf(fh, "\n");
  fprintf(fh, "animation_delay = ");
    _cfsml_write_int(fh, &(foo->animation_delay));
    fprintf(fh, "\n");
  fprintf(fh, "hunk = ");
    min = max = MAX_HUNK_BLOCKS;
#line 407 "cfsml.pl"
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
  fprintf(fh, "pics = ");
    min = max = foo->pics_nr;
    if (foo->pics_drawn_nr < min)
       min = foo->pics_drawn_nr;
    if (!foo->pics)
       min = max = 0; /* Don't write if it points to NULL */
#line 407 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_drawn_pic_t(fh, &(foo->pics[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
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
#line 407 "cfsml.pl"
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
  fprintf(fh, "parser_event = ");
    write_heapptr(fh, &(foo->parser_event));
    fprintf(fh, "\n");
  fprintf(fh, "global_vars = ");
    write_heapptr(fh, &(foo->global_vars));
    fprintf(fh, "\n");
  fprintf(fh, "parser_lastmatch_word = ");
    _cfsml_write_int(fh, &(foo->parser_lastmatch_word));
    fprintf(fh, "\n");
  fprintf(fh, "parser_nodes = ");
    min = max = VOCAB_TREE_NODES;
#line 407 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      write_PTN(fh, &(foo->parser_nodes[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "parser_valid = ");
    _cfsml_write_int(fh, &(foo->parser_valid));
    fprintf(fh, "\n");
  fprintf(fh, "synonyms = ");
    min = max = foo->synonyms_nr;
    if (!foo->synonyms)
       min = max = 0; /* Don't write if it points to NULL */
#line 407 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_synonym_t(fh, &(foo->synonyms[i]));
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
#line 407 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_class_t(fh, &(foo->classtable[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "scripttable = ");
    min = max = 1000;
#line 407 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_script_t(fh, &(foo->scripttable[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "clone_list = ");
    min = max = SCRIPT_MAX_CLONES;
#line 407 "cfsml.pl"
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
  fprintf(fh, "port_ID = ");
    _cfsml_write_int(fh, &(foo->port_ID));
    fprintf(fh, "\n");
  fprintf(fh, "ega_colors = ");
    min = max = 16;
#line 407 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_gfx_color_t(fh, &(foo->ega_colors[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_state_t(FILE *fh, state_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
  int reladdresses[2];
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "savegame_version")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->savegame_version), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "restarting_flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->restarting_flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "have_mouse_flag")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->have_mouse_flag), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pic_not_valid")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->pic_not_valid), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pic_is_new")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->pic_is_new), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "onscreen_console")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->onscreen_console), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "game_time")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_long(fh, &(foo->game_time), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "save_dir")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->save_dir), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "sound_object")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->sound_object), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "sound_mute")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->sound_mute), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "sound_volume")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->sound_volume), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "mouse_pointer_nr")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->mouse_pointer_nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "port_serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->port_serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "dyn_views_list_serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->dyn_views_list_serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "drop_views_list_serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->drop_views_list_serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "visual")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->visual), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pic_visible_map")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->pic_visible_map), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pic_animate")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->pic_animate), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "animation_delay")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->animation_delay), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "hunk")) {
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MAX_HUNK_BLOCKS;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
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
#line 643 "cfsml.pl"
         if (read_menubar_tp(fh, &(foo->menubar), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "status_bar_text")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_string(fh, &(foo->status_bar_text), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "priority_first")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->priority_first), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "priority_last")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->priority_last), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pics")) {
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 577 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max) {
           foo->pics = (drawn_pic_t *) malloc(max * sizeof(drawn_pic_t));
           _cfsml_register_pointer(foo->pics);
         }
         else
           foo->pics = NULL;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_drawn_pic_t(fh, &(foo->pics[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
         foo->pics_nr = max ; /* Set array size accordingly */
         foo->pics_drawn_nr = i ; /* Set number of elements */
      } else
      if (!strcmp(bar, "version_lock_flag")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->version_lock_flag), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "version")) {
#line 643 "cfsml.pl"
         if (read_sci_version(fh, &(foo->version), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "max_version")) {
#line 643 "cfsml.pl"
         if (read_sci_version(fh, &(foo->max_version), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "min_version")) {
#line 643 "cfsml.pl"
         if (read_sci_version(fh, &(foo->min_version), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "execution_stack")) {
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 577 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max) {
           foo->execution_stack = (exec_stack_t *) malloc(max * sizeof(exec_stack_t));
           _cfsml_register_pointer(foo->execution_stack);
         }
         else
           foo->execution_stack = NULL;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
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
#line 643 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->acc), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "amp_rest")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->amp_rest), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "prev")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gint16(fh, &(foo->prev), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "stack_base")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->stack_base), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "stack_handle")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->stack_handle), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "parser_base")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->parser_base), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "parser_event")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->parser_event), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "global_vars")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->global_vars), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "parser_lastmatch_word")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->parser_lastmatch_word), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "parser_nodes")) {
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = VOCAB_TREE_NODES;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
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
      if (!strcmp(bar, "parser_valid")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->parser_valid), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "synonyms")) {
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 577 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max) {
           foo->synonyms = (synonym_t *) malloc(max * sizeof(synonym_t));
           _cfsml_register_pointer(foo->synonyms);
         }
         else
           foo->synonyms = NULL;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_synonym_t(fh, &(foo->synonyms[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
         foo->synonyms_nr = max ; /* Set array size accordingly */
      } else
      if (!strcmp(bar, "game_obj")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->game_obj), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "classtable")) {
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 577 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max) {
           foo->classtable = (class_t *) malloc(max * sizeof(class_t));
           _cfsml_register_pointer(foo->classtable);
         }
         else
           foo->classtable = NULL;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
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
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = 1000;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
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
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = SCRIPT_MAX_CLONES;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
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
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->_heap->first_free), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "_heap->old_ff")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->_heap->old_ff), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "_heap->base")) {
#line 556 "cfsml.pl"
         if (_cfsml_read_int(fh, &(reladdresses[0]), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "game_name")) {
#line 556 "cfsml.pl"
         if (_cfsml_read_int(fh, &(reladdresses[1]), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "port_ID")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->port_ID), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ega_colors")) {
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = 16;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_gfx_color_t(fh, &(foo->ega_colors[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  foo->_heap->base = foo->_heap->start + reladdresses[0];
  foo->game_name = foo->_heap->start + reladdresses[1];
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfx_color_t(FILE *fh, gfx_color_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "visual = ");
    write_pixmap_color(fh, &(foo->visual));
    fprintf(fh, "\n");
  fprintf(fh, "alpha = ");
    _cfsml_write_byte(fh, &(foo->alpha));
    fprintf(fh, "\n");
  fprintf(fh, "priority = ");
    _cfsml_write_byte(fh, &(foo->priority));
    fprintf(fh, "\n");
  fprintf(fh, "control = ");
    _cfsml_write_byte(fh, &(foo->control));
    fprintf(fh, "\n");
  fprintf(fh, "mask = ");
    _cfsml_write_byte(fh, &(foo->mask));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfx_color_t(FILE *fh, gfx_color_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "visual")) {
#line 643 "cfsml.pl"
         if (read_pixmap_color(fh, &(foo->visual), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "alpha")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->alpha), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "control")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->control), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "mask")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->mask), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gint16(FILE *fh, gint16* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gint16(FILE *fh, gint16* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 479 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfx_box_shade_t(FILE *fh, gfx_box_shade_t* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfx_box_shade_t(FILE *fh, gfx_box_shade_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 479 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfxw_widget_types_t(FILE *fh, gfxw_widget_types_t* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfxw_widget_types_t(FILE *fh, gfxw_widget_types_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 479 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_script_t(FILE *fh, script_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
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

#line 455 "cfsml.pl"
static int
_cfsml_read_script_t(FILE *fh, script_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->heappos), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "localvar_offset")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->localvar_offset), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "export_table_offset")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->export_table_offset), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "lockers")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->lockers), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_class_t(FILE *fh, class_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "script = ");
    _cfsml_write_int(fh, &(foo->script));
    fprintf(fh, "\n");
  fprintf(fh, "class_offset = ");
    _cfsml_write_int(fh, &(foo->class_offset));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_class_t(FILE *fh, class_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->script), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "class_offset")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->class_offset), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_synonym_t(FILE *fh, synonym_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "replaceant = ");
    _cfsml_write_int(fh, &(foo->replaceant));
    fprintf(fh, "\n");
  fprintf(fh, "replacement = ");
    _cfsml_write_int(fh, &(foo->replacement));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_synonym_t(FILE *fh, synonym_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "replaceant")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->replaceant), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "replacement")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->replacement), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_long(FILE *fh, long* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 455 "cfsml.pl"
static int
_cfsml_read_long(FILE *fh, long* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 479 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfx_dirty_rect_t(FILE *fh, gfx_dirty_rect_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "rect = ");
    _cfsml_write_rect_t(fh, &(foo->rect));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    if (!foo->next)
      fprintf(fh, "\\null\\");    else 
      _cfsml_write_gfx_dirty_rect_t(fh, foo->next);
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfx_dirty_rect_t(FILE *fh, gfx_dirty_rect_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "rect")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->rect), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "next")) {
#line 634 "cfsml.pl"
        if (strcmp(value, "\\null\\")) { /* null pointer? */
           foo->next = malloc(sizeof (gfx_dirty_rect_t));
           _cfsml_register_pointer(foo->next);
           if (_cfsml_read_gfx_dirty_rect_t(fh, foo->next, value, line, hiteof))
              return CFSML_FAILURE;
        } else foo->next = NULL;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfxw_container_t(FILE *fh, gfxw_container_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(foo->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(foo->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(foo->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(foo->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(foo->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(foo->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "zone = ");
    _cfsml_write_rect_t(fh, &(foo->zone));
    fprintf(fh, "\n");
  fprintf(fh, "dirty = ");
    if (!foo->dirty)
      fprintf(fh, "\\null\\");    else 
      _cfsml_write_gfx_dirty_rect_t(fh, foo->dirty);
    fprintf(fh, "\n");
  fprintf(fh, "contents = ");
    write_any_widget(fh, &(foo->contents));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfxw_container_t(FILE *fh, gfxw_container_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "magic")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->magic), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bounds")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->bounds), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "next")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->next), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ID")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ID), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "widget_priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->widget_priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "zone")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->zone), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "dirty")) {
#line 634 "cfsml.pl"
        if (strcmp(value, "\\null\\")) { /* null pointer? */
           foo->dirty = malloc(sizeof (gfx_dirty_rect_t));
           _cfsml_register_pointer(foo->dirty);
           if (_cfsml_read_gfx_dirty_rect_t(fh, foo->dirty, value, line, hiteof))
              return CFSML_FAILURE;
        } else foo->dirty = NULL;
      } else
      if (!strcmp(bar, "contents")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->contents), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfxw_widget_t(FILE *fh, gfxw_widget_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(foo->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(foo->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(foo->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(foo->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(foo->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(foo->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfxw_widget_t(FILE *fh, gfxw_widget_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "magic")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->magic), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bounds")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->bounds), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "next")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->next), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ID")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ID), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "widget_priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->widget_priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfxw_dyn_view_t(FILE *fh, gfxw_dyn_view_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(foo->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(foo->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(foo->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(foo->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(foo->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(foo->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "pos = ");
    _cfsml_write_point_t(fh, &(foo->pos));
    fprintf(fh, "\n");
  fprintf(fh, "color = ");
    _cfsml_write_gfx_color_t(fh, &(foo->color));
    fprintf(fh, "\n");
  fprintf(fh, "view = ");
    _cfsml_write_int(fh, &(foo->view));
    fprintf(fh, "\n");
  fprintf(fh, "loop = ");
    _cfsml_write_int(fh, &(foo->loop));
    fprintf(fh, "\n");
  fprintf(fh, "cel = ");
    _cfsml_write_int(fh, &(foo->cel));
    fprintf(fh, "\n");
  fprintf(fh, "draw_bounds = ");
    _cfsml_write_rect_t(fh, &(foo->draw_bounds));
    fprintf(fh, "\n");
  fprintf(fh, "under_bitsp = ");
    _cfsml_write_int(fh, &(foo->under_bitsp));
    fprintf(fh, "\n");
  fprintf(fh, "signalp = ");
    _cfsml_write_int(fh, &(foo->signalp));
    fprintf(fh, "\n");
  fprintf(fh, "under_bits = ");
    _cfsml_write_int(fh, &(foo->under_bits));
    fprintf(fh, "\n");
  fprintf(fh, "signal = ");
    _cfsml_write_int(fh, &(foo->signal));
    fprintf(fh, "\n");
  fprintf(fh, "z = ");
    _cfsml_write_int(fh, &(foo->z));
    fprintf(fh, "\n");
  fprintf(fh, "force_precedence = ");
    _cfsml_write_int(fh, &(foo->force_precedence));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfxw_dyn_view_t(FILE *fh, gfxw_dyn_view_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "magic")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->magic), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bounds")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->bounds), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "next")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->next), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ID")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ID), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "widget_priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->widget_priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pos")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_point_t(fh, &(foo->pos), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "color")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_color_t(fh, &(foo->color), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "view")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->view), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "loop")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->loop), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "cel")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->cel), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "draw_bounds")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->draw_bounds), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "under_bitsp")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->under_bitsp), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "signalp")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->signalp), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "under_bits")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->under_bits), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "signal")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->signal), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "z")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->z), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "force_precedence")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->force_precedence), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfxw_text_t(FILE *fh, gfxw_text_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(foo->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(foo->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(foo->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(foo->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(foo->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(foo->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "font_nr = ");
    _cfsml_write_int(fh, &(foo->font_nr));
    fprintf(fh, "\n");
  fprintf(fh, "text = ");
    _cfsml_write_string(fh, &(foo->text));
    fprintf(fh, "\n");
  fprintf(fh, "halign = ");
    _cfsml_write_gfx_alignment_t(fh, &(foo->halign));
    fprintf(fh, "\n");
  fprintf(fh, "valign = ");
    _cfsml_write_gfx_alignment_t(fh, &(foo->valign));
    fprintf(fh, "\n");
  fprintf(fh, "color1 = ");
    _cfsml_write_gfx_color_t(fh, &(foo->color1));
    fprintf(fh, "\n");
  fprintf(fh, "color2 = ");
    _cfsml_write_gfx_color_t(fh, &(foo->color2));
    fprintf(fh, "\n");
  fprintf(fh, "bgcolor = ");
    _cfsml_write_gfx_color_t(fh, &(foo->bgcolor));
    fprintf(fh, "\n");
  fprintf(fh, "text_flags = ");
    _cfsml_write_int(fh, &(foo->text_flags));
    fprintf(fh, "\n");
  fprintf(fh, "width = ");
    _cfsml_write_int(fh, &(foo->width));
    fprintf(fh, "\n");
  fprintf(fh, "height = ");
    _cfsml_write_int(fh, &(foo->height));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfxw_text_t(FILE *fh, gfxw_text_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "magic")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->magic), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bounds")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->bounds), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "next")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->next), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ID")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ID), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "widget_priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->widget_priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "font_nr")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->font_nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "text")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_string(fh, &(foo->text), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "halign")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_alignment_t(fh, &(foo->halign), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "valign")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_alignment_t(fh, &(foo->valign), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "color1")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_color_t(fh, &(foo->color1), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "color2")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_color_t(fh, &(foo->color2), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bgcolor")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_color_t(fh, &(foo->bgcolor), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "text_flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->text_flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "width")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->width), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "height")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->height), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfxw_port_t(FILE *fh, gfxw_port_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(foo->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(foo->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(foo->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(foo->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(foo->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(foo->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "zone = ");
    _cfsml_write_rect_t(fh, &(foo->zone));
    fprintf(fh, "\n");
  fprintf(fh, "dirty = ");
    if (!foo->dirty)
      fprintf(fh, "\\null\\");    else 
      _cfsml_write_gfx_dirty_rect_t(fh, foo->dirty);
    fprintf(fh, "\n");
  fprintf(fh, "contents = ");
    write_any_widget(fh, &(foo->contents));
    fprintf(fh, "\n");
  fprintf(fh, "decorations = ");
    write_any_widget(fh, &(foo->decorations));
    fprintf(fh, "\n");
  fprintf(fh, "port_bg = ");
    write_any_widget(fh, &(foo->port_bg));
    fprintf(fh, "\n");
  fprintf(fh, "color = ");
    _cfsml_write_gfx_color_t(fh, &(foo->color));
    fprintf(fh, "\n");
  fprintf(fh, "bgcolor = ");
    _cfsml_write_gfx_color_t(fh, &(foo->bgcolor));
    fprintf(fh, "\n");
  fprintf(fh, "font_nr = ");
    _cfsml_write_int(fh, &(foo->font_nr));
    fprintf(fh, "\n");
  fprintf(fh, "draw_pos = ");
    _cfsml_write_point_t(fh, &(foo->draw_pos));
    fprintf(fh, "\n");
  fprintf(fh, "port_flags = ");
    _cfsml_write_int(fh, &(foo->port_flags));
    fprintf(fh, "\n");
  fprintf(fh, "title_text = ");
    _cfsml_write_string(fh, &(foo->title_text));
    fprintf(fh, "\n");
  fprintf(fh, "gray_text = ");
    _cfsml_write_byte(fh, &(foo->gray_text));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfxw_port_t(FILE *fh, gfxw_port_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "magic")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->magic), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bounds")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->bounds), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "next")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->next), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ID")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ID), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "widget_priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->widget_priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "zone")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->zone), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "dirty")) {
#line 634 "cfsml.pl"
        if (strcmp(value, "\\null\\")) { /* null pointer? */
           foo->dirty = malloc(sizeof (gfx_dirty_rect_t));
           _cfsml_register_pointer(foo->dirty);
           if (_cfsml_read_gfx_dirty_rect_t(fh, foo->dirty, value, line, hiteof))
              return CFSML_FAILURE;
        } else foo->dirty = NULL;
      } else
      if (!strcmp(bar, "contents")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->contents), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "decorations")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->decorations), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "port_bg")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->port_bg), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "color")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_color_t(fh, &(foo->color), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bgcolor")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_color_t(fh, &(foo->bgcolor), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "font_nr")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->font_nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "draw_pos")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_point_t(fh, &(foo->draw_pos), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "port_flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->port_flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "title_text")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_string(fh, &(foo->title_text), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "gray_text")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_byte(fh, &(foo->gray_text), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_byte(FILE *fh, byte* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 455 "cfsml.pl"
static int
_cfsml_read_byte(FILE *fh, byte* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 479 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfx_alignment_t(FILE *fh, gfx_alignment_t* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfx_alignment_t(FILE *fh, gfx_alignment_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 479 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_menu_item_t(FILE *fh, menu_item_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
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
    min = max = MENU_SAID_SPEC_SIZE;
#line 407 "cfsml.pl"
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

#line 455 "cfsml.pl"
static int
_cfsml_read_menu_item_t(FILE *fh, menu_item_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "keytext")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_string(fh, &(foo->keytext), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "keytext_size")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->keytext_size), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "said")) {
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MENU_SAID_SPEC_SIZE;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
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
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->said_pos), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "text")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_string(fh, &(foo->text), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "text_pos")) {
#line 643 "cfsml.pl"
         if (read_heapptr(fh, &(foo->text_pos), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "modifiers")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->modifiers), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "key")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->key), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "enabled")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->enabled), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "tag")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->tag), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_string(FILE *fh, char ** foo)
{
  char *bar;
  int min, max, i;

#line 371 "cfsml.pl"
  if (!(*foo))
    fprintf(fh, "\\null\\");  else {
    bar = _cfsml_mangle_string((char *) *foo);
    fprintf(fh, "\"%s\"", bar);
    free(bar);
  }
}

#line 455 "cfsml.pl"
static int
_cfsml_read_string(FILE *fh, char ** foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 488 "cfsml.pl"

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
    _cfsml_register_pointer(foo);
    return CFSML_SUCCESS;
  } else {
    *foo = NULL;
    return CFSML_SUCCESS;
  }
}

#line 363 "cfsml.pl"
static void
_cfsml_write_gfxw_box_t(FILE *fh, gfxw_box_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "magic = ");
    _cfsml_write_int(fh, &(foo->magic));
    fprintf(fh, "\n");
  fprintf(fh, "serial = ");
    _cfsml_write_int(fh, &(foo->serial));
    fprintf(fh, "\n");
  fprintf(fh, "flags = ");
    _cfsml_write_int(fh, &(foo->flags));
    fprintf(fh, "\n");
  fprintf(fh, "type = ");
    _cfsml_write_gfxw_widget_types_t(fh, &(foo->type));
    fprintf(fh, "\n");
  fprintf(fh, "bounds = ");
    _cfsml_write_rect_t(fh, &(foo->bounds));
    fprintf(fh, "\n");
  fprintf(fh, "next = ");
    write_any_widget(fh, &(foo->next));
    fprintf(fh, "\n");
  fprintf(fh, "ID = ");
    _cfsml_write_int(fh, &(foo->ID));
    fprintf(fh, "\n");
  fprintf(fh, "widget_priority = ");
    _cfsml_write_int(fh, &(foo->widget_priority));
    fprintf(fh, "\n");
  fprintf(fh, "color1 = ");
    _cfsml_write_gfx_color_t(fh, &(foo->color1));
    fprintf(fh, "\n");
  fprintf(fh, "color2 = ");
    _cfsml_write_gfx_color_t(fh, &(foo->color2));
    fprintf(fh, "\n");
  fprintf(fh, "shade_type = ");
    _cfsml_write_gfx_box_shade_t(fh, &(foo->shade_type));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_gfxw_box_t(FILE *fh, gfxw_box_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
      if (!strcmp(bar, "magic")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->magic), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "serial")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->serial), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "flags")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->flags), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "type")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfxw_widget_types_t(fh, &(foo->type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "bounds")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_rect_t(fh, &(foo->bounds), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "next")) {
#line 643 "cfsml.pl"
         if (read_any_widget(fh, &(foo->next), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ID")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ID), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "widget_priority")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->widget_priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "color1")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_color_t(fh, &(foo->color1), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "color2")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_color_t(fh, &(foo->color2), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "shade_type")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_gfx_box_shade_t(fh, &(foo->shade_type), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_menubar_t(FILE *fh, menubar_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "menus = ");
    min = max = foo->menus_nr;
    if (!foo->menus)
       min = max = 0; /* Don't write if it points to NULL */
#line 407 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_menu_t(fh, &(foo->menus[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_menubar_t(FILE *fh, menubar_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
#line 567 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 577 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max) {
           foo->menus = (menu_t *) malloc(max * sizeof(menu_t));
           _cfsml_register_pointer(foo->menus);
         }
         else
           foo->menus = NULL;
#line 600 "cfsml.pl"
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 608 "cfsml.pl"
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
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 363 "cfsml.pl"
static void
_cfsml_write_point_t(FILE *fh, point_t* foo)
{
  char *bar;
  int min, max, i;

#line 381 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "x = ");
    _cfsml_write_int(fh, &(foo->x));
    fprintf(fh, "\n");
  fprintf(fh, "y = ");
    _cfsml_write_int(fh, &(foo->y));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 455 "cfsml.pl"
static int
_cfsml_read_point_t(FILE *fh, point_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 510 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
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
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->x), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "y")) {
#line 643 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->y), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 650 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}


/* Auto-generated CFSML declaration and function block ends here */
/* Auto-generation performed by cfsml.pl 0.8.1 */
#line 497 "CFSML input file"
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
#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_box_t(fh, ((gfxw_box_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 568 "CFSML input file"
		break;

	case GFXW_RECT:
	case GFXW_LINE:
	case GFXW_INVERSE_LINE:
#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_primitive_t(fh, ((gfxw_primitive_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 574 "CFSML input file"
		break;

	case GFXW_VIEW:
	case GFXW_STATIC_VIEW:
#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_view_t(fh, ((gfxw_view_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 579 "CFSML input file"
		break;

	case GFXW_DYN_VIEW:
	case GFXW_PIC_VIEW:
#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_dyn_view_t(fh, ((gfxw_dyn_view_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 584 "CFSML input file"
		break;

	case GFXW_TEXT:
#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_text_t(fh, ((gfxw_text_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 588 "CFSML input file"
		break;


	case GFXW_SORTED_LIST:
	case GFXW_LIST:
#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_list_t(fh, ((gfxw_list_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 594 "CFSML input file"
		break;

	case GFXW_VISUAL:
#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_visual_t(fh, ((gfxw_visual_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 598 "CFSML input file"
		break;

	case GFXW_PORT:
#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_port_t(fh, ((gfxw_port_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 602 "CFSML input file"
		break;

	case GFXW_:
	case GFXW_CONTAINER:
	default:
		sciprintf("While writing widget: Invalid widget type while writing widget:\n");
		(*widget)->print((*widget), 0);

		if ((*widget)->type == GFXW_CONTAINER) {
#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_container_t(fh, ((gfxw_container_t*)*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 612 "CFSML input file"
		} else {
#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfxw_widget_t(fh, (*widget));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 614 "CFSML input file"
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

		visual->port_refs = calloc(sizeof(gfxw_port_t *), visual->port_refs_nr);
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
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 734 "cfsml.pl"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_gfxw_box_t(fh, ((gfxw_box_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 797 "CFSML input file"
		break;

	case GFXW_RECT:
	case GFXW_LINE:
	case GFXW_INVERSE_LINE:
		*widget = _gfxw_new_widget(sizeof(gfxw_primitive_t), expected_type);
/* Auto-generated CFSML data reader code */
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 734 "cfsml.pl"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_gfxw_primitive_t(fh, ((gfxw_primitive_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 804 "CFSML input file"
		break;

	case GFXW_VIEW:
	case GFXW_STATIC_VIEW:
		*widget = _gfxw_new_widget(sizeof(gfxw_view_t), expected_type);
/* Auto-generated CFSML data reader code */
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 734 "cfsml.pl"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_gfxw_view_t(fh, ((gfxw_view_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 810 "CFSML input file"
		break;

	case GFXW_DYN_VIEW:
	case GFXW_PIC_VIEW:
		*widget = _gfxw_new_widget(sizeof(gfxw_dyn_view_t), expected_type);
/* Auto-generated CFSML data reader code */
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 734 "cfsml.pl"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_gfxw_dyn_view_t(fh, ((gfxw_dyn_view_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 816 "CFSML input file"
		(*widget)->type = expected_type;
		if (FILE_VERSION == 1)
			((gfxw_dyn_view_t *) widget)->force_precedence = 0;
		break;

	case GFXW_TEXT:
		*widget = _gfxw_new_widget(sizeof(gfxw_text_t), expected_type);
/* Auto-generated CFSML data reader code */
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 734 "cfsml.pl"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_gfxw_text_t(fh, ((gfxw_text_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 824 "CFSML input file"
		(*widget)->type = expected_type;
		break;


	case GFXW_LIST:
	case GFXW_SORTED_LIST:
		*widget = _gfxw_new_widget(sizeof(gfxw_list_t), expected_type);
		GFXWC(*widget)->contents = NULL;
		GFXWC(*widget)->dirty = NULL;
/* Auto-generated CFSML data reader code */
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 734 "cfsml.pl"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_gfxw_list_t(fh, ((gfxw_list_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 834 "CFSML input file"
		(*widget)->type = expected_type;
		break;

	case GFXW_VISUAL:
		*widget = _gfxw_new_widget(sizeof(gfxw_visual_t), expected_type);
		GFXWC(*widget)->contents = NULL;
		GFXWC(*widget)->dirty = NULL;
/* Auto-generated CFSML data reader code */
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 734 "cfsml.pl"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_gfxw_visual_t(fh, ((gfxw_visual_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 842 "CFSML input file"
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
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 734 "cfsml.pl"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_gfxw_port_t(fh, ((gfxw_port_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 855 "CFSML input file"
		(*widget)->type = expected_type;
		break;

	case GFXW_CONTAINER:
		*widget = _gfxw_new_widget(sizeof(gfxw_container_t), expected_type);
		GFXWC(*widget)->contents = NULL;
		GFXWC(*widget)->dirty = NULL;
		sciprintf("Warning: Restoring untyped widget container\n");
/* Auto-generated CFSML data reader code */
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 734 "cfsml.pl"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_gfxw_container_t(fh, ((gfxw_container_t*)*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 864 "CFSML input file"
		(*widget)->type = expected_type;
		break;

	case GFXW_:
		*widget = _gfxw_new_widget(sizeof(gfxw_widget_t), expected_type);
		sciprintf("Warning: Restoring untyped widget\n");
/* Auto-generated CFSML data reader code */
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 734 "cfsml.pl"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(*line), &_cfsml_eof, &dummy);

#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_gfxw_widget_t(fh, (*widget), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 871 "CFSML input file"
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
#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_gfx_pixmap_color_t(fh, (color));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 899 "CFSML input file"
}

int
read_pixmap_color(FILE *fh, gfx_pixmap_color_t *color, char *lastval, int *line, int *hiteof)
{
/* Auto-generated CFSML data reader code */
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 731 "cfsml.pl"
    char *_cfsml_inp = lastval;
#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_gfx_pixmap_color_t(fh, (color), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 905 "CFSML input file"

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

#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_menubar_t(fh, (*foo));
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 924 "CFSML input file"

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
#line 715 "cfsml.pl"
  {
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 731 "cfsml.pl"
    char *_cfsml_inp = lastval;
#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_menubar_t(fh, (*foo), _cfsml_inp, &(*line), &_cfsml_eof);
#line 744 "cfsml.pl"
    *hiteof = _cfsml_error;
#line 751 "cfsml.pl"
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
#line 941 "CFSML input file"

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

#line 769 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_state_t(fh, s);
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 1007 "CFSML input file"

	fclose(fh);

	_gamestate_unfrob(s);

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
	_global_save_state = retval;
	retval->gfx_state = s->gfx_state;

	fh = fopen("state", "r");
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

	retval->sound_mute = s->sound_mute;
	retval->sound_volume = s->sound_volume;

/* Auto-generated CFSML data reader code */
#line 715 "cfsml.pl"
  {
#line 718 "cfsml.pl"
    int _cfsml_line_ctr = 0;
#line 723 "cfsml.pl"
    struct _cfsml_pointer_refstruct **_cfsml_myptrrefptr = _cfsml_get_current_refpointer();
#line 726 "cfsml.pl"
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
#line 734 "cfsml.pl"
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(_cfsml_line_ctr), &_cfsml_eof, &dummy);

#line 739 "cfsml.pl"
    _cfsml_error = _cfsml_read_state_t(fh, retval, _cfsml_inp, &(_cfsml_line_ctr), &_cfsml_eof);
#line 744 "cfsml.pl"
    read_eof = _cfsml_error;
#line 748 "cfsml.pl"
     _cfsml_free_pointer_references(_cfsml_myptrrefptr, _cfsml_error);
#line 751 "cfsml.pl"
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
#line 1065 "CFSML input file"

	fclose(fh);

	if ((retval->savegame_version < 1) || (retval->savegame_version > FREESCI_SAVEGAME_VERSION)) {

		if (retval->savegame_version < 1)
			sciprintf("Old savegame version detected- can't load\n");
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
			retval->pics = malloc(sizeof(drawn_pic_t) * (retval->pics_nr = 8));
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
	retval->sfx_driver = s->sfx_driver;
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

	retval->debug_mode = s->debug_mode;

	retval->resource_dir = s->resource_dir;
	retval->work_dir = s->work_dir;
	retval->kernel_opt_flags = 0;

	retval->successor = NULL;

	chdir ("..");

	return retval;
}
