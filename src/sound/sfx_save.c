/***************************************************************************
 sfx_save.cfsml Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* CFSML file providing persistance to the sound system
** Note that this is only useful if the default sound library implementation
** is in use.
*/

#include <stdio.h>
#include <sound.h>
#include <soundserver.h>
#include <sci_memory.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif  /* !O_BINARY */

#define SOUND_SAVEGAME_VERSION 1;

typedef struct {

	int sound_version;

	int songs_nr; /* Number of songs */
	song_t *songs; /* All songs in order */

	int active_song;

	int master_volume; /* duh.. */
	int soundcue; /* Cumulative sound cue */
	long usecs_to_sleep; /* Microseconds until the next tick is due */
	long ticks_to_wait; /* Ticks until the next sound command has to be interpreted */
	long ticks_to_fade; /* Ticks until a fade-out is complete */

} sound_lib_file_t;




/* Auto-generated CFSML declaration and function block */

#define CFSML_SUCCESS 0
#define CFSML_FAILURE 1


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
  return (_cfsml_last_value_retreived = (char *) sci_realloc(retval, strlen(retval) + 1));
  /* Re-allocate; this value might be used for quite some while (if we are
  ** restoring a string)
  */
}
static void
_cfsml_write_song_t(FILE *fh, song_t* save_struc);
static int
_cfsml_read_song_t(FILE *fh, song_t* save_struc, char *lastval, int *line, int *hiteof);

static void
_cfsml_write_string(FILE *fh, char ** save_struc);
static int
_cfsml_read_string(FILE *fh, char ** save_struc, char *lastval, int *line, int *hiteof);

static void
_cfsml_write_sound_lib_file_t(FILE *fh, sound_lib_file_t* save_struc);
static int
_cfsml_read_sound_lib_file_t(FILE *fh, sound_lib_file_t* save_struc, char *lastval, int *line, int *hiteof);

static void
_cfsml_write_long(FILE *fh, long* save_struc);
static int
_cfsml_read_long(FILE *fh, long* save_struc, char *lastval, int *line, int *hiteof);

static void
_cfsml_write_int(FILE *fh, int* save_struc);
static int
_cfsml_read_int(FILE *fh, int* save_struc, char *lastval, int *line, int *hiteof);

static void
_cfsml_write_word(FILE *fh, word* save_struc);
static int
_cfsml_read_word(FILE *fh, word* save_struc, char *lastval, int *line, int *hiteof);

static void
_cfsml_write_song_t(FILE *fh, song_t* save_struc)
{
  int min, max, i;

  fprintf(fh, "{\n");
  fprintf(fh, "flags = ");
    min = max = MIDI_CHANNELS;
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(save_struc->flags[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "instruments = ");
    min = max = MIDI_CHANNELS;
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(save_struc->instruments[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "velocity = ");
    min = max = MIDI_CHANNELS;
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(save_struc->velocity[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "pressure = ");
    min = max = MIDI_CHANNELS;
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(save_struc->pressure[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "pitch = ");
    min = max = MIDI_CHANNELS;
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(save_struc->pitch[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "channel_map = ");
    min = max = MIDI_CHANNELS;
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(save_struc->channel_map[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "size = ");
    _cfsml_write_int(fh, &(save_struc->size));
    fprintf(fh, "\n");
  fprintf(fh, "pos = ");
    _cfsml_write_int(fh, &(save_struc->pos));
    fprintf(fh, "\n");
  fprintf(fh, "loopmark = ");
    _cfsml_write_int(fh, &(save_struc->loopmark));
    fprintf(fh, "\n");
  fprintf(fh, "fading = ");
    _cfsml_write_long(fh, &(save_struc->fading));
    fprintf(fh, "\n");
  fprintf(fh, "reverb = ");
    _cfsml_write_int(fh, &(save_struc->reverb));
    fprintf(fh, "\n");
  fprintf(fh, "maxfade = ");
    _cfsml_write_long(fh, &(save_struc->maxfade));
    fprintf(fh, "\n");
  fprintf(fh, "resetflag = ");
    _cfsml_write_int(fh, &(save_struc->resetflag));
    fprintf(fh, "\n");
  fprintf(fh, "polyphony = ");
    min = max = MIDI_CHANNELS;
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(save_struc->polyphony[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "file_nr = ");
    _cfsml_write_int(fh, &(save_struc->file_nr));
    fprintf(fh, "\n");
  fprintf(fh, "priority = ");
    _cfsml_write_int(fh, &(save_struc->priority));
    fprintf(fh, "\n");
  fprintf(fh, "loops = ");
    _cfsml_write_int(fh, &(save_struc->loops));
    fprintf(fh, "\n");
  fprintf(fh, "status = ");
    _cfsml_write_int(fh, &(save_struc->status));
    fprintf(fh, "\n");
  fprintf(fh, "handle = ");
    _cfsml_write_word(fh, &(save_struc->handle));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

static int
_cfsml_read_song_t(FILE *fh, song_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token)
       return CFSML_FAILURE;
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
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(token, "flags")) {
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(save_struc->flags[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "instruments")) {
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(save_struc->instruments[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "velocity")) {
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(save_struc->velocity[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "pressure")) {
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(save_struc->pressure[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "pitch")) {
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(save_struc->pitch[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "channel_map")) {
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(save_struc->channel_map[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "size")) {
         if (_cfsml_read_int(fh, &(save_struc->size), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "pos")) {
         if (_cfsml_read_int(fh, &(save_struc->pos), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "loopmark")) {
         if (_cfsml_read_int(fh, &(save_struc->loopmark), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "fading")) {
         if (_cfsml_read_long(fh, &(save_struc->fading), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "reverb")) {
         if (_cfsml_read_int(fh, &(save_struc->reverb), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "maxfade")) {
         if (_cfsml_read_long(fh, &(save_struc->maxfade), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "resetflag")) {
         if (_cfsml_read_int(fh, &(save_struc->resetflag), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "polyphony")) {
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(save_struc->polyphony[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(token, "file_nr")) {
         if (_cfsml_read_int(fh, &(save_struc->file_nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "priority")) {
         if (_cfsml_read_int(fh, &(save_struc->priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "loops")) {
         if (_cfsml_read_int(fh, &(save_struc->loops), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "status")) {
         if (_cfsml_read_int(fh, &(save_struc->status), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "handle")) {
         if (_cfsml_read_word(fh, &(save_struc->handle), value, line, hiteof))
            return CFSML_FAILURE;
      } else
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

static void
_cfsml_write_string(FILE *fh, char ** save_struc)
{
  if (!(*save_struc))
    fprintf(fh, "\\null\\");  else {
    char *token = _cfsml_mangle_string((char *) *save_struc);
    fprintf(fh, "\"%s\"", token);
    free(token);
  }
}

static int
_cfsml_read_string(FILE *fh, char ** save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;

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

static void
_cfsml_write_sound_lib_file_t(FILE *fh, sound_lib_file_t* save_struc)
{
  int min, max, i;

  fprintf(fh, "{\n");
  fprintf(fh, "sound_version = ");
    _cfsml_write_int(fh, &(save_struc->sound_version));
    fprintf(fh, "\n");
  fprintf(fh, "songs = ");
    min = max = save_struc->songs_nr;
    if (!save_struc->songs)
       min = max = 0; /* Don't write if it points to NULL */
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_song_t(fh, &(save_struc->songs[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "active_song = ");
    _cfsml_write_int(fh, &(save_struc->active_song));
    fprintf(fh, "\n");
  fprintf(fh, "soundcue = ");
    _cfsml_write_int(fh, &(save_struc->soundcue));
    fprintf(fh, "\n");
  fprintf(fh, "usecs_to_sleep = ");
    _cfsml_write_long(fh, &(save_struc->usecs_to_sleep));
    fprintf(fh, "\n");
  fprintf(fh, "ticks_to_wait = ");
    _cfsml_write_long(fh, &(save_struc->ticks_to_wait));
    fprintf(fh, "\n");
  fprintf(fh, "ticks_to_fade = ");
    _cfsml_write_long(fh, &(save_struc->ticks_to_fade));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

static int
_cfsml_read_sound_lib_file_t(FILE *fh, sound_lib_file_t* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;
int min, max, i;
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d, got \"%s\"\n",line, lastval);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    token = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!token)
       return CFSML_FAILURE;
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
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(token, "sound_version")) {
         if (_cfsml_read_int(fh, &(save_struc->sound_version), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "songs")) {
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", token, *line);
            return CFSML_FAILURE;
;         }

         if (max) {
           save_struc->songs = (song_t *) sci_malloc(max * sizeof(song_t));
#ifdef SATISFY_PURIFY
           memset(save_struc->songs, 0, max * sizeof(song_t));
#endif
           _cfsml_register_pointer(save_struc->songs);
         }
         else
           save_struc->songs = NULL;
         done = i = 0;
         do {
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, token, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_song_t(fh, &(save_struc->songs[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
         save_struc->songs_nr = max ; /* Set array size accordingly */
      } else
      if (!strcmp(token, "active_song")) {
         if (_cfsml_read_int(fh, &(save_struc->active_song), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "soundcue")) {
         if (_cfsml_read_int(fh, &(save_struc->soundcue), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "usecs_to_sleep")) {
         if (_cfsml_read_long(fh, &(save_struc->usecs_to_sleep), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "ticks_to_wait")) {
         if (_cfsml_read_long(fh, &(save_struc->ticks_to_wait), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(token, "ticks_to_fade")) {
         if (_cfsml_read_long(fh, &(save_struc->ticks_to_fade), value, line, hiteof))
            return CFSML_FAILURE;
      } else
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", token, *line);
          return CFSML_FAILURE;       }
     }
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

static void
_cfsml_write_long(FILE *fh, long* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

static int
_cfsml_read_long(FILE *fh, long* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;

  *save_struc = strtol(lastval, &token, 0);
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

static void
_cfsml_write_int(FILE *fh, int* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

static int
_cfsml_read_int(FILE *fh, int* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;

  *save_struc = strtol(lastval, &token, 0);
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

static void
_cfsml_write_word(FILE *fh, word* save_struc)
{
  fprintf(fh, "%li", (long) *save_struc);
}

static int
_cfsml_read_word(FILE *fh, word* save_struc, char *lastval, int *line, int *hiteof)
{
  char *token;

  *save_struc = strtol(lastval, &token, 0);
  if (*token != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}


/* Auto-generated CFSML declaration and function block ends here */
/* Auto-generation performed by cfsml.pl 0.8.2 */


/* Sound state saving reference implementation */
int
soundsrv_save_state(FILE *debugstream, char *dir, songlib_t songlib, song_t *curr_song,
		    int soundcue, long usecs_to_sleep, long ticks_to_wait, long ticks_to_fade, int master_volume)
{
	sound_lib_file_t write_rec;
	song_t *seeker;
	int songctr = 0;
	int curr_song_nr = -1;
	FILE *fh;

	if (dir && chdir(dir)) {
		char *cwd;

		cwd = getcwd(NULL, 0);
		fprintf(debugstream, "Failed to enter '%s', cwd is '%s'\n", dir, cwd);
		free(cwd);
		return 1;
	}

	write_rec.sound_version = SOUND_SAVEGAME_VERSION;

	write_rec.soundcue = soundcue;
	write_rec.usecs_to_sleep = usecs_to_sleep;
	write_rec.ticks_to_wait = ticks_to_wait;
	write_rec.ticks_to_fade = ticks_to_fade;
	write_rec.master_volume = master_volume;

	/* Determine number of songs */
	seeker = *songlib;
	while (seeker) {

		if (seeker == curr_song)
			curr_song_nr = songctr;

		++songctr;
		seeker = seeker->next;
	}

	write_rec.songs_nr = songctr;
	write_rec.active_song = curr_song_nr;

	write_rec.songs = sci_malloc(sizeof(song_t) * songctr);

	/* Now memcpy those songs into a row and write their song data to a file */
	songctr = -1;
	seeker = *songlib;
	while (seeker) {
		char filename[10];
		int fd;
		++songctr;

		memcpy(&(write_rec.songs[songctr]), seeker, sizeof(song_t));
		write_rec.songs[songctr].file_nr = songctr;

		/* Now write to an external file */
		sprintf(filename, "song.%d", songctr);
		fd = creat(filename, 0600);

		if (fd < 0) {
			fprintf(debugstream, "Error creating file: %s while creating '%s/%s'\n",
			      strerror(errno), dir, filename);
			free(write_rec.songs);
			if (dir)
				chdir ("..");
			return 1;
		}

		if (write(fd, seeker->data, seeker->size) < seeker->size) {
			fprintf(debugstream, "Write error: Failed to write to '%s/%s'\n", dir, filename);
			free(write_rec.songs);
			if (dir)
				chdir ("..");
			return 1;
		}
		close(fd);

		seeker = seeker->next;
	}

	fh = fopen("sound", "w" FO_BINARY);

/* Auto-generated CFSML data writer code */
  _cfsml_write_sound_lib_file_t(fh, &write_rec);
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */

	fclose(fh);
	fprintf(stderr,"Finished all writing.\n");

	free(write_rec.songs);

	if (dir)
		chdir ("..");
	return 0;
}

static void
recover_version0(sound_lib_file_t *rec)
{
	int i;

	for (i = 0; i < rec->songs_nr; i++) {
		int j;
		song_t *song = rec->songs + i;

		song->reverb = 0;
		song->maxfade = 16;
		song->resetflag = 0;

		for (j = 0; j < MIDI_CHANNELS; j++)
			song->polyphony[j] = 1;
	}
}

/* Sound state restore complement for the saving reference implementation */
int
soundsrv_restore_state(FILE *debugstream, char *dir, songlib_t songlib, song_t **curr_song,
		       int *soundcue, long *usecs_to_sleep, long *ticks_to_wait, long *ticks_to_fade, int *master_volume)
{
	FILE *fh;
	sound_lib_file_t read_rec;
	song_t *seeker, *next = NULL;
	int error;
	int i;
	if (dir && chdir(dir)) {
		fprintf(debugstream, "Failed to enter '%s'\n", dir);
		return 1;
	}

	fh = fopen("sound", "r" FO_BINARY);

	if (!fh) {
		fprintf(debugstream, "'%s/sound' not found!\n", dir);
		if (dir)
	    chdir ("..");
		return 1;
	}

	read_rec.songs = NULL;

	/* Backwards capability crap */
	read_rec.sound_version = 0;
	read_rec.master_volume = *master_volume;
/* Auto-generated CFSML data reader code */
  {
    int _cfsml_line_ctr = 0;
    struct _cfsml_pointer_refstruct **_cfsml_myptrrefptr = _cfsml_get_current_refpointer();
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(_cfsml_line_ctr), &_cfsml_eof, &dummy);

    _cfsml_error = _cfsml_read_sound_lib_file_t(fh, &read_rec, _cfsml_inp, &(_cfsml_line_ctr), &_cfsml_eof);
    error = _cfsml_error;
     _cfsml_free_pointer_references(_cfsml_myptrrefptr, _cfsml_error);
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

	if (read_rec.sound_version == 0)
		recover_version0(&read_rec);

	if (error) {
		if(read_rec.songs)
			free(read_rec.songs);

		if (dir)
			chdir ("..");
		fprintf(debugstream, "Failed to parse '%s/sound'\n", dir);
		return 1;
	}

	for (i = 0; i < read_rec.songs_nr; i++) {
		int size = read_rec.songs[i].size;
		char filename[10];
		int fd;

		sprintf(filename, "song.%d", read_rec.songs[i].file_nr);
		fd = open(filename, O_BINARY | O_RDONLY);

		if (fd == -1) {
			if (dir)
				chdir ("..");
			fprintf(debugstream, "Opening %s/%s failed: %s\n", dir, filename, strerror(errno));
			if(read_rec.songs)
				free(read_rec.songs);
			return 1;
		}

		if (read(fd, read_rec.songs[i].data = (byte *) sci_malloc(size), size) < size) {
			int j;
			for (j = 0; i < i; j++)
				free(read_rec.songs[i].data);
			if (dir)
				chdir ("..");
			fprintf(debugstream, "Reading %d bytes from %s/%s failed\n", size, dir, filename);
			return 1;
		}

		close(fd);
	}


	song_lib_free(songlib); /* Everything is well, so free all  songs */

	*curr_song = NULL;

	for (i = 0; i < read_rec.songs_nr; i++) {

		next = sci_malloc(sizeof(song_t));	
		memcpy(next, &(read_rec.songs[i]), sizeof(song_t));
		next->next = NULL;
		if (i > 0)
			seeker->next = next;

		seeker = next;

		if (i == 0)
			*songlib = seeker;

		if (i == read_rec.active_song)
			*curr_song = seeker;
	}

	*soundcue = read_rec.soundcue;
	*usecs_to_sleep = read_rec.usecs_to_sleep;
	*ticks_to_wait = read_rec.ticks_to_wait;
	*ticks_to_fade = read_rec.ticks_to_fade;
	*master_volume = read_rec.master_volume;

	if (read_rec.songs_nr)
		free(read_rec.songs);

	if (dir)
	  chdir ("..");

	return 0;
}
