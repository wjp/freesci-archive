/***************************************************************************
 kernel.c Copyright (C) 1999 Christoph Reichenbach


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
#include <math.h>
#include <ctype.h>
#include <kdebug.h>
#include <uinput.h>
#include <event.h>

#ifdef _MSC_VER
#include <direct.h>
#include <ctype.h>
#endif

#ifndef PI
#define PI 3.14159265358979323846
#endif /* !PI */

extern int _kdebug_cheap_event_hack;
extern int _kdebug_cheap_soundcue_hack;

extern int _debug_seeking;
extern int _debug_step_running;

/*** Portability kludges ***/

#ifdef _WIN32

int sci_ffs(int _mask)
{
  int retval = 0;

  if (!_mask) return 0;
  retval++;
  while (! (_mask & 1))
  {
    retval++;
    _mask >>= 1;
  }
  
  return retval;
}

#else

#define sci_ffs ffs

#endif

/******************** Debug functions ********************/

void
_SCIkvprintf(FILE *file, char *format, va_list args)
{
  vfprintf(file, format, args);
  if (con_file) vfprintf(con_file, format, args);
}

void
_SCIkprintf(FILE *file, char *format, ...)
{
  va_list args;

  va_start(args, format);
  _SCIkvprintf(file, format, args);
  va_end (args);
}


void
_SCIkwarn(state_t *s, char *file, int line, int area, char *format, ...)
{
  va_list args;

  if (area == SCIkERROR_NR)
    _SCIkprintf(stderr, "ERROR: ");
  else
    _SCIkprintf(stderr, "Warning: ");

  va_start(args, format);
  _SCIkvprintf(stderr, format, args);
  va_end(args);
  fflush(NULL);
}

void
_SCIkdebug(state_t *s, char *file, int line, int area, char *format, ...)
{
  va_list args;

  if (s->debug_mode & (1 << area)) {
    _SCIkprintf(stdout, " kernel: (%s L%d): ", file, line);
    va_start(args, format);
    _SCIkvprintf(stdout, format, args);
    va_end(args);
    fflush(NULL);
  }
}

void
_SCIGNUkdebug(char *funcname, state_t *s, char *file, int line, int area, char *format, ...)
{
  va_list xargs;
  int error = ((area == SCIkWARNING_NR) || (area == SCIkERROR_NR));

  if (error || (s->debug_mode & (1 << area))) { /* Is debugging enabled for this area? */

    _SCIkprintf(stderr, "FSCI: ");

    if (area == SCIkERROR_NR)
      _SCIkprintf(stderr, "ERROR in %s ", funcname);
    else if (area == SCIkWARNING_NR)
      _SCIkprintf(stderr, "%s: Warning ", funcname);
    else _SCIkprintf(stderr, funcname);

    _SCIkprintf(stderr, "(%s L%d): ", file, line);

    va_start(xargs, format);
    _SCIkvprintf(stderr, format, xargs);
    va_end(xargs);

  }
}


/******************** Resource Macros ********************/

#define RESOURCE_ID(type, number) (number) | ((type) << 11)
/* Returns the composite resource ID */

#define RESOURCE_NUMBER(resid) ((resid) & 0x7ff)

#define RESOURCE_TYPE(resid) ((resid) >> 11)

/******************** Kernel Oops ********************/

int
kernel_oops(state_t *s, int line, char *reason)
{
  sciprintf("Kernel Oops in line %d: %s\n", line, reason);
  fprintf(stderr,"Kernel Oops in line %d: %s\n", line, reason);
  script_debug_flag = script_error_flag = 1;
  return 0;
}

#define KERNEL_OOPS(reason) kernel_oops(s, __LINE__, reason)

/******************** Heap macros ********************/

#define GET_HEAP(address) ((((guint16)(address)) < 800)? \
KERNEL_OOPS("Heap address space violation on read")  \
: getInt16(s->heap + ((guint16)(address))))
/* Reads a heap value if allowed */

#define UGET_HEAP(address) ((((guint16)(address)) < 800)? \
KERNEL_OOPS("Heap address space violation on read")  \
: getUInt16(s->heap + ((guint16)(address))))
/* Reads a heap value if allowed */

#define PUT_HEAP(address, value) { if (((guint16)(address)) < 800) \
KERNEL_OOPS("Heap address space violation on write");        \
else { s->heap[((guint16)(address))] = (value) &0xff;               \
 s->heap[((guint16)(address)) + 1] = ((value) >> 8) & 0xff;}}
/* Sets a heap value if allowed */

#ifdef SCI_KERNEL_DEBUG

#define CHECK_THIS_KERNEL_FUNCTION if (s->debug_mode & (1 << SCIkFUNCCHK_NR)) {\
  int i;\
  sciprintf("Kernel CHECK: %s[%x](", s->kernel_names[funct_nr], funct_nr); \
  for (i = 0; i < argc; i++) { \
    sciprintf("%04x", 0xffff & PARAM(i)); \
    if (i+1 < argc) sciprintf(", "); \
  } \
  sciprintf(")\n"); \
} \

#else /* !SCI_KERNEL_DEBUG */

#define CHECK_THIS_KERNEL_FUNCTION

#endif /* !SCI_KERNEL_DEBUG */

#define PARAM(x) ((gint16) getInt16(s->heap + argp + ((x)*2)))
#define UPARAM(x) ((guint16) getInt16(s->heap + argp + ((x)*2)))

#define PARAM_OR_ALT(x, alt) ((x < argc)? PARAM(x) : (alt))
#define UPARAM_OR_ALT(x, alt) ((x < argc)? UPARAM(x) : (alt))
/* Returns the parameter value or (alt) if not enough parameters were supplied */



int
read_selector(state_t *s, heap_ptr object, int selector_id)
{
  heap_ptr address;

  if (lookup_selector(s, object, selector_id, &address) != SELECTOR_VARIABLE)
    return 0;
  else
    return GET_HEAP(address);
}


void
write_selector(state_t *s, heap_ptr object, int selector_id, int value)
{
  heap_ptr address;

  if ((selector_id < 0) || (selector_id > s->selector_names_nr)) {
    SCIkwarn(SCIkWARNING, "Attempt to write to invalid selector of object at %04x.\n", object);
    return;
  }

  if (lookup_selector(s, object, selector_id, &address) != SELECTOR_VARIABLE)
    SCIkwarn(SCIkWARNING, "Selector '%s' of object at %04x could not be written to\n",
	     s->selector_names[selector_id], object);
  else
    PUT_HEAP(address, value);

  /*  sciprintf("Selector '%s' is at %04x\n", s->selector_names[selector_id], address); */
}

int
invoke_selector(state_t *s, heap_ptr object, int selector_id, int noinvalid, int kfunct,
		heap_ptr k_argp, int k_argc, /* Kernel function argp/argc */
		int argc, ...)
{
  va_list argp;
  int i;
  int framesize = 4 + 2 * argc;
  heap_ptr address;
  heap_ptr stackframe = k_argp + k_argc * 2;

  exec_stack_t *xstack; /* Execution stack */

  PUT_HEAP(stackframe, selector_id); /* The selector we want to call */
  PUT_HEAP(stackframe + 2, argc); /* The number of arguments */

  if (lookup_selector(s, object, selector_id, &address) != SELECTOR_METHOD) {
    SCIkwarn(SCIkERROR, "Selector '%s' of object at %04x could not be invoked\n",
	     s->selector_names[selector_id], object);
    if (noinvalid == 0)
      KERNEL_OOPS("Not recoverable: VM was halted\n");
    return 1;
  }

  va_start(argp, argc);
  for (i = 0; i < argc; i++) {
    int j = va_arg(argp, heap_ptr);
    PUT_HEAP(stackframe + 4 + (2 * i), j); /* Write each argument */
  }
  va_end(argp);

  /* Write "kernel" call to the stack, for debugging: */
  xstack = 
    add_exec_stack_entry(s, 0, 0, 0, k_argc, k_argp - 2, 0, 0, s->execution_stack_pos);
  xstack->selector = -42 - kfunct; /* Evil debugging hack to identify kernel function */
  xstack->type = EXEC_STACK_TYPE_KERNEL;

  /* Now commit the actual function: */
  xstack = 
    send_selector(s, object, object, stackframe + framesize, framesize, 0, stackframe);

  run_vm(s, 0); /* Start a new vm */

  --(s->execution_stack_pos); /* Get rid of the extra stack entry */

  return 0;
}


int
is_object(state_t *s, heap_ptr offset)
{
  if (offset < 1000)
    return 0;
  else
    return (GET_HEAP(offset + SCRIPT_OBJECT_MAGIC_OFFSET) == SCRIPT_OBJECT_MAGIC_NUMBER);
}


#define GET_SELECTOR(_object_, _selector_) read_selector(s, _object_, s->selector_map._selector_)
#define UGET_SELECTOR(_object_, _selector_) \
 ((guint16) read_selector(s, _object_, s->selector_map._selector_))
#define PUT_SELECTOR(_object_, _selector_, _value_)\
 write_selector(s, _object_, s->selector_map._selector_, _value_)

/*
#define INVOKE_SELECTOR(_object_, _selector_, _noinvalid_, _args_...) \
 invoke_selector(s, _object_, s->selector_map._selector_, _noinvalid_, argp + (argc * 2), ## _args_)
*/

#define INV_SEL(_object_, _selector_, _noinvalid_) \
  s, ##_object_,  s->selector_map.##_selector_, ##_noinvalid_, funct_nr, argp, argc


/* Allocates a set amount of memory for a specified use and returns a handle to it. */
int
kalloc(state_t *s, int type, int space)
{
  int seeker = 0;

  while ((seeker < MAX_HUNK_BLOCKS) && (s->hunk[seeker].size))
    seeker++;

  if (seeker == MAX_HUNK_BLOCKS)
    KERNEL_OOPS("Out of hunk handles! Try increasing MAX_HUNK_BLOCKS in engine.h");
  else {
    s->hunk[seeker].data = malloc(s->hunk[seeker].size = space);
    s->hunk[seeker].type = type;
  }

  return (seeker | (sci_memory << 11));
}


/* Returns a pointer to the memory indicated by the specified handle */
byte *
kmem(state_t *s, int handle)
{
  if ((handle >> 11) != sci_memory) {
    SCIkwarn(SCIkERROR, "Error: kmem() without a handle\n");
    return 0;
  }

  handle &= 0x7ff;

  if ((handle < 0) || (handle >= MAX_HUNK_BLOCKS)) {
    SCIkwarn(SCIkERROR, "Error: kmem() with invalid handle\n");
    return 0;
  }

  return s->hunk[handle & 0x7ff].data;
}

/* Frees the specified handle. Returns 0 on success, 1 otherwise. */
int
kfree(state_t *s, int handle)
{
  if ((handle >> 11) != sci_memory) {
    SCIkwarn(SCIkERROR, "Error: Attempt to kfree() non-handle\n");
    return 1;
  }

  handle &= 0x7ff;

  if ((handle < 0) || (handle >= MAX_HUNK_BLOCKS)) {
    SCIkwarn(SCIkERROR, "Error: Attempt to kfree() with invalid handle\n");
    return 1;
  }

  if (s->hunk[handle].size == 0) {
    SCIkwarn(SCIkERROR, "Error: Attempt to kfree() non-allocated memory\n");
    return 1;
  }

  free(s->hunk[handle].data);
  s->hunk[handle].size = 0;

  return 0;
}


char *
_kernel_lookup_text(state_t *s, int address, int index)
     /* Returns the string the script intended to address */
{
  char *seeker;
  resource_t *textres;

  if (address < 1000) {
    int textlen;
    int _index = index;
    textres = findResource(sci_text, address);

    if (!textres) {
      SCIkwarn(SCIkERROR, "text.%03d not found\n", address);
      return NULL; /* Will probably segfault */
    }

    textlen = textres->length;
    seeker = textres->data;

    while (index--)
      while ((textlen--) && (*seeker++));

    if (textlen)
      return seeker;
    else {
      SCIkwarn(SCIkERROR, "Index %d out of bounds in text.%03d\n", _index, address);
      return 0;
    }

  } else return s->heap + address;
}



/* Flags for the signal selector */
#define _K_VIEW_SIG_FLAG_UPDATE_ENDED   0x0001
#define _K_VIEW_SIG_FLAG_UPDATING       0x0002
#define _K_VIEW_SIG_FLAG_NO_UPDATE      0x0004
#define _K_VIEW_SIG_FLAG_HIDDEN         0x0008
#define _K_VIEW_SIG_FLAG_FIX_PRI_ON     0x0010
#define _K_VIEW_SIG_FLAG_UNKNOWN_5      0x0020
#define _K_VIEW_SIG_FLAG_UNKNOWN_6      0x0040
#define _K_VIEW_SIG_FLAG_DONT_RESTORE   0x0080
#define _K_VIEW_SIG_FLAG_FROZEN         0x0100
#define _K_VIEW_SIG_FLAG_IS_EXTRA       0x0200
#define _K_VIEW_SIG_FLAG_HIT_OBSTACLE   0x0400
#define _K_VIEW_SIG_FLAG_DOESNT_TURN    0x0800
#define _K_VIEW_SIG_FLAG_NO_CYCLER      0x1000
#define _K_VIEW_SIG_FLAG_IGNORE_HORIZON 0x2000
#define _K_VIEW_SIG_FLAG_IGNORE_ACTOR   0x4000
#define _K_VIEW_SIG_FLAG_DISPOSE_ME     0x8000

void
_k_dyn_view_list_prepare_change(state_t *s);
     /* Removes all views in anticipation of a new window or text */
void
_k_dyn_view_list_accept_change(state_t *s);
     /* Removes all views in anticipation of a new window or text */

#define VIEW_PRIORITY(y) (((y) < s->priority_first)? 0 : (((y) > s->priority_last)? 15 : 1\
	+ ((((y) - s->priority_first) * 15) / (s->priority_last - s->priority_first))))

#define PRIORITY_BAND_FIRST(nr) ((((nr) == 0)? 0 :  \
        ((s->priority_first) + ((nr) * (s->priority_last - s->priority_first)) / 15))-10)



#define _K_SOUND_STATUS_STOPPED 0
#define _K_SOUND_STATUS_INITIALIZED 1
#define _K_SOUND_STATUS_PLAYING 2
#define _K_SOUND_STATUS_PAUSED 3

void
process_sound_events(state_t *s) /* Get all sound events, apply their changes to the heap */
{
  sound_event_t *event;

  if (s->sfx_driver == NULL)
    return;

  while ((event = s->sfx_driver->get_event(s))) {
    heap_ptr obj = event->handle;

    if (is_object(s, obj))
      switch (event->signal) {
      case SOUND_SIGNAL_CUMULATIVE_CUE: {
	int signal = GET_SELECTOR(obj, signal);
	SCIkdebug(SCIkSOUND,"Received cumulative cue for %04x\n", obj);

	PUT_SELECTOR(obj, signal, signal + 1);
      }
      break;

      case SOUND_SIGNAL_LOOP:

	SCIkdebug(SCIkSOUND,"Received loop signal for %04x\n", obj);
	PUT_SELECTOR(obj, loop, event->value);
	PUT_SELECTOR(obj, signal, -1);
	break;

      case SOUND_SIGNAL_FINISHED:

	SCIkdebug(SCIkSOUND,"Received finished signal for %04x\n", obj);
	PUT_SELECTOR(obj, state, _K_SOUND_STATUS_STOPPED);
	PUT_SELECTOR(obj, loop, -1);
	break;

      case SOUND_SIGNAL_PLAYING:

	SCIkdebug(SCIkSOUND,"Received playing signal for %04x\n", obj);
	PUT_SELECTOR(obj, state, _K_SOUND_STATUS_PLAYING);
	break;

      case SOUND_SIGNAL_PAUSED:

	SCIkdebug(SCIkSOUND,"Received pause signal for %04x\n", obj);
	PUT_SELECTOR(obj, state, _K_SOUND_STATUS_PAUSED);
	break;

      case SOUND_SIGNAL_RESUMED:

	SCIkdebug(SCIkSOUND,"Received resume signal for %04x\n", obj);
	PUT_SELECTOR(obj, state, _K_SOUND_STATUS_PAUSED);
	break;

      case SOUND_SIGNAL_INITIALIZED:

	PUT_SELECTOR(obj, state, _K_SOUND_STATUS_INITIALIZED);
	SCIkdebug(SCIkSOUND,"Received init signal for %04x\n", obj);
	break;

      case SOUND_SIGNAL_ABSOLUTE_CUE:

	SCIkdebug(SCIkSOUND,"Received absolute cue %d for %04x\n", event->value, obj);
	PUT_SELECTOR(obj, signal, event->value);
	break;

      default:
	SCIkwarn(SCIkERROR, "Unknown sound signal: %d\n", event->signal);
      }

    free(event);
  }

}



/*****************************************/
/************* Kernel functions **********/
/*****************************************/

/* kLoad(restype, resnr):
** Loads an arbitrary resource of type 'restype' with resource number 'resnr'
*/
void
kLoad(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
    int restype = UPARAM(0);
    int resnr = UPARAM(1);

    s->acc = ((restype << 11) | resnr); /* Return the resource identifier as handle */

    if (restype == sci_memory)/* Request to dynamically allocate hunk memory for later use */
      s->acc = kalloc(s, HUNK_TYPE_ANY, restype);

}

/* kUnload():
** Unloads an arbitrary resource of type 'restype' with resource numbber 'resnr'
*/
void
kUnLoad(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
    int restype = UPARAM(0);
    int resnr = UPARAM(1);

    if (restype == sci_memory)
      kfree(s, resnr);

}

void
kClone(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr old_offs = UPARAM(0);
  heap_ptr new_offs;
  heap_ptr functareaptr;
  word species;
  int selectors;
  int object_size;
  int i;

  if (GET_HEAP(old_offs + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    SCIkwarn(SCIkERROR, "Attempt to clone non-object/class at %04x failed", old_offs);
    return;
  }

  SCIkdebug(SCIkMEM, "Attempting to clone from %04x\n", old_offs);

  selectors = UGET_HEAP(old_offs + SCRIPT_SELECTORCTR_OFFSET);

  object_size = 8 + 2 + (selectors * 2);
  /* 8: Pre-selector area; 2: Function area (zero overloaded methods) */

  new_offs = heap_allocate(s->_heap, object_size);

  if (!new_offs) {
    KERNEL_OOPS("Out of heap memory while cloning!\n");
    return;
  }

  new_offs += 2; /* Step over heap header */

  SCIkdebug(SCIkMEM, "New clone at %04x, size %04x\n", new_offs, object_size);

  memcpy(s->heap + new_offs, s->heap + old_offs + SCRIPT_OBJECT_MAGIC_OFFSET, object_size);
  new_offs -= SCRIPT_OBJECT_MAGIC_OFFSET; /* Center new_offs on the first selector value */

  PUT_HEAP(new_offs + SCRIPT_INFO_OFFSET, SCRIPT_INFO_CLONE); /* Set this to be a clone */

  functareaptr = selectors * 2 + 2; /* New function area */
  PUT_HEAP(new_offs + functareaptr - 2, 0); /* No functions */
  PUT_HEAP(new_offs + SCRIPT_FUNCTAREAPTR_OFFSET, functareaptr);

  species = GET_HEAP(new_offs + SCRIPT_SPECIES_OFFSET);
  PUT_HEAP(new_offs + SCRIPT_SUPERCLASS_OFFSET, species);

  s->acc = new_offs; /* return the object's address */

  i = 0;
  while ((i < SCRIPT_MAX_CLONES) && (s->clone_list[i])) i++;

  if (i < SCRIPT_MAX_CLONES)
    s->clone_list[i] = new_offs; /* Log this clone */
  else SCIkwarn(SCIkWARNING, "Could not log clone at %04x\n", new_offs);

}


void
kDisposeClone(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr offset = PARAM(0);
  int i;
  word underBits;

  if (GET_HEAP(offset + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    SCIkwarn(SCIkERROR, "Attempt to dispose non-class/object at %04x\n", offset);
    return;
  }

  if (GET_HEAP(offset + SCRIPT_INFO_OFFSET) != SCRIPT_INFO_CLONE) {
    /*  SCIkwarn("Attempt to dispose something other than a clone at %04x\n", offset); */
    /* SCI silently ignores this behaviour; some games actually depend on this */
    return;
  }

  underBits = GET_SELECTOR(offset, underBits);
  if (underBits)
    kfree(s, underBits); /* Views may dispose without cleaning up */

  i = 0;
  while ((i < SCRIPT_MAX_CLONES) && (s->clone_list[i] != offset)) i++;
  if (i < SCRIPT_MAX_CLONES)
    s->clone_list[i] = 0; /* un-log clone */
  else SCIkwarn(SCIkWARNING, "Could not remove log entry from clone at %04x\n", offset);

  for (i = 0; i < s->dyn_views_nr; i++)
    if (s->dyn_views[i].obj == offset) /* Is it in the dyn_view list? */
      s->dyn_views[i].obj = 0; /* Remove it from there */

  offset += SCRIPT_OBJECT_MAGIC_OFFSET; /* Step back to beginning of object */

  heap_free(s->_heap, offset -2); /* -2 to step back on the heap block size indicator */
}


/* kScriptID(script, index):
** Returns script dispatch address index in the supplied script
*/
void
kScriptID(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int script = PARAM(0);
  int index = PARAM(1);
  int disp_size;
  heap_ptr disp;

  if (argc == 1)
    index = 0;

  if (s->scripttable[script].heappos == 0)
    script_instantiate(s, script); /* Instantiate script if neccessary */

  disp = s->scripttable[script].export_table_offset;

  if (!disp) {
    SCIkdebug(SCIkERROR, "Script 0x%x does not have a dispatch table\n", script);
    s->acc = 0;
    return;
  }

  disp_size = UGET_HEAP(disp);

  if (index > disp_size-1) {
    SCIkwarn(SCIkERROR, "Dispatch index too big: %d > %d\n", index, disp_size-1);
    return;
  }

  s->acc = UGET_HEAP(disp + 2 + index*2) + s->scripttable[script].heappos;
}


void
kDisposeScript(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int script = UPARAM(0);

  if (script < 1000)
    script_uninstantiate(s, PARAM(0));
  else
    SCIkwarn(SCIkWARNING, "Attemt to dispose invalid script %04x\n", script);

}


void
kIsObject(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = is_object(s, UPARAM(0));
}


/* kGameIsRestarting():
** Returns the restarting_flag in acc
*/
void
kGameIsRestarting(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = s->restarting_flag;
}

void
kHaveMouse(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = s->have_mouse_flag;
}



/******************** Doubly linked lists ********************/


#define LIST_FIRST_NODE 0
#define LIST_PREVIOUS_NODE 0
#define LIST_LAST_NODE 2
#define LIST_NEXT_NODE 2
#define LIST_NODE_KEY 4
#define LIST_NODE_VALUE 6
/* 'previous' and 'next' nodes for lists mean 'first' and 'last' node, respectively */

void
kNewList(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr listbase = heap_allocate(s->_heap, 4);

  if (!listbase) {
    KERNEL_OOPS("Out of memory while creating a list");
    return;
  }

  listbase += 2; /* Jump over heap header */

  PUT_HEAP(listbase + LIST_FIRST_NODE, 0); /* No first node */
  PUT_HEAP(listbase + LIST_LAST_NODE, 0); /* No last node */

  SCIkdebug(SCIkNODES, "New listbase at %04x\n", listbase);

  s->acc = listbase; /* Return list base address */
}


void
kNewNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr nodebase = heap_allocate(s->_heap, 8);

  if (!nodebase) {
    KERNEL_OOPS("Out of memory while creating a node");
    return;
  }

  nodebase += 2; /* Jump over heap header */

  PUT_HEAP(nodebase + LIST_PREVIOUS_NODE, 0);
  PUT_HEAP(nodebase + LIST_NEXT_NODE, 0);
  PUT_HEAP(nodebase + LIST_NODE_KEY, PARAM(0));
  PUT_HEAP(nodebase + LIST_NODE_VALUE, PARAM(1));

  SCIkdebug(SCIkNODES, "New nodebase at %04x\n", nodebase);

  s->acc = nodebase; /* Return node base address */
}


void
kAddToEnd(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr listbase = UPARAM(0);
  heap_ptr nodebase = UPARAM(1);
  heap_ptr old_lastnode = GET_HEAP(listbase + LIST_LAST_NODE);
  SCIkdebug(SCIkNODES, "Adding node %04x to end of list %04x\n", nodebase, listbase);

  if (old_lastnode)
    PUT_HEAP(old_lastnode + LIST_NEXT_NODE, nodebase);

  PUT_HEAP(nodebase + LIST_PREVIOUS_NODE, old_lastnode);

  PUT_HEAP(listbase + LIST_LAST_NODE, nodebase);

  if (GET_HEAP(listbase + LIST_FIRST_NODE) == 0)
    PUT_HEAP(listbase + LIST_FIRST_NODE, nodebase);
  /* Set node to be the first and last node if it's the only node of the list */
}


void
kAddToFront(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr listbase = UPARAM(0);
  heap_ptr nodebase = UPARAM(1);
  heap_ptr old_firstnode = GET_HEAP(listbase + LIST_FIRST_NODE);
  SCIkdebug(SCIkNODES, "Adding node %04x to start of list %04x\n", nodebase, listbase);

  if (old_firstnode)
    PUT_HEAP(old_firstnode + LIST_PREVIOUS_NODE, nodebase);

  PUT_HEAP(nodebase + LIST_NEXT_NODE, old_firstnode);

  PUT_HEAP(listbase + LIST_FIRST_NODE, nodebase);

  if (GET_HEAP(listbase + LIST_LAST_NODE) == 0)
    PUT_HEAP(listbase + LIST_LAST_NODE, nodebase);
  /* Set node to be the first and last node if it's the only node of the list */
}


void
kFindKey(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr node;
  word key = UPARAM(1);
  SCIkdebug(SCIkNODES, "Looking for key %04x in list %04x\n", key, UPARAM(0));

  node = UGET_HEAP(UPARAM(0) + LIST_FIRST_NODE);

  while (node && (UGET_HEAP(node + LIST_NODE_KEY) != key))
    node = UGET_HEAP(node + LIST_NEXT_NODE);
  /* Aborts if either the list ends (node == 0) or the key is found */

  SCIkdebug(SCIkNODES, "Looking for key: Result is %04x\n", node);
  s->acc = node;
}


void
kDeleteKey(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr list = UPARAM(0);
  heap_ptr node;
  word key = UPARAM(1);
  SCIkdebug(SCIkNODES, "Removing key %04x from list %04x\n", key, list);

  node = UGET_HEAP(list + LIST_FIRST_NODE);

  while (node && ((guint16) UGET_HEAP(node + LIST_NODE_KEY) != key))
    node = GET_HEAP(node + LIST_NEXT_NODE);
  /* Aborts if either the list ends (node == 0) or the key is found */


  if (node) {
    heap_ptr prev_node = UGET_HEAP(node + LIST_PREVIOUS_NODE);
    heap_ptr next_node = UGET_HEAP(node + LIST_NEXT_NODE);

    SCIkdebug(SCIkNODES,"Removing key from list: Succeeded at %04x\n", node);

    if (UGET_HEAP(list + LIST_FIRST_NODE) == node)
      PUT_HEAP(list + LIST_FIRST_NODE, next_node);
    if (UGET_HEAP(list + LIST_LAST_NODE) == node)
      PUT_HEAP(list + LIST_LAST_NODE, prev_node);

    if (next_node)
      PUT_HEAP(next_node + LIST_PREVIOUS_NODE, prev_node);
    if (prev_node)
      PUT_HEAP(prev_node + LIST_NEXT_NODE, next_node);

    heap_free(s->_heap, node - 2);

  } else SCIkdebug(SCIkNODES,"Removing key from list: FAILED\n");
}


void
kFirstNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr list = UPARAM(0);

  if (list)
    s->acc = GET_HEAP(UPARAM(0) + LIST_FIRST_NODE);
  else
    s->acc = 0;
}


void
kLastNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr list = UPARAM(0);

  if (list)
    s->acc = GET_HEAP(UPARAM(0) + LIST_LAST_NODE);
  else
    s->acc = 0;
}


void
kPrevNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = GET_HEAP(UPARAM(0) + LIST_PREVIOUS_NODE);
}


void
kNextNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = GET_HEAP(UPARAM(0) + LIST_NEXT_NODE);
}


void
kNodeValue(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = GET_HEAP(UPARAM(0) + LIST_NODE_VALUE);
}


void
kDisposeList(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr address = PARAM(0) - 2; /* -2 to get the heap header */
  heap_ptr node = GET_HEAP(address + 2 + LIST_FIRST_NODE);

  while (node) { /* Free all nodes */
    heap_ptr node_heapbase = node - 2;

    node = GET_HEAP(node + LIST_NEXT_NODE); /* Next node */
    heap_free(s->_heap, node_heapbase); /* Clear heap space of old node */
  }

  if (GET_HEAP(address) != 6) {
    SCIkwarn(SCIkERROR, "Attempt to dispose non-list at %04x\n", address);
  } else heap_free(s->_heap, address);

}

/*************************************************************/
/************* File control *************/

/* This assumes modern stream implementations. It may break on DOS. */

#define _K_FILE_MODE_OPEN_OR_FAIL 0
#define _K_FILE_MODE_OPEN_OR_CREATE 1
#define _K_FILE_MODE_CREATE 2

void
kFOpen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *filename = s->heap + UPARAM(0);
  int mode = PARAM(1);
  int retval = 1; /* Ignore file_handles[0] */
  FILE *file = NULL;

  if ((mode == _K_FILE_MODE_OPEN_OR_FAIL) || (mode == _K_FILE_MODE_OPEN_OR_CREATE))
    file = fopen(filename, "r+"); /* Attempt to open existing file */

  if ((!file) && ((mode == _K_FILE_MODE_OPEN_OR_CREATE) || (mode == _K_FILE_MODE_CREATE)))
    file = fopen(filename, "w+"); /* Attempt to create file */

  if (!file) { /* Failed */
    s->acc = 0;
    return;
  }

  while (s->file_handles[retval] && (retval < s->file_handles_nr))
    retval++;

  if (retval == s->file_handles_nr) /* Hit size limit => Allocate more space */
    s->file_handles = realloc(s->file_handles, sizeof(FILE *) * ++(s->file_handles_nr));

  s->file_handles[retval] = file;

  s->acc = retval;

}

void
kFClose(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int handle = UPARAM(0);

  if (handle == 0) {
    SCIkwarn(SCIkERROR, "Attempt to close file handle 0\n");
    return;
  }

  if ((handle >= s->file_handles_nr) || (s->file_handles[handle] == NULL)) {
    SCIkwarn(SCIkERROR, "Attempt to close invalid/unused file handle %d\n", handle);
    return;
  }

  fclose(s->file_handles[handle]);

  s->file_handles[handle] = NULL;
}


void kFPuts(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int handle = UPARAM(0);
  char *data = UPARAM(1) + s->heap;

  if (handle == 0) {
    SCIkwarn(SCIkERROR, "Attempt to write to file handle 0\n");
    return;
  }

  if ((handle >= s->file_handles_nr) || (s->file_handles[handle] == NULL)) {
    SCIkwarn(SCIkERROR, "Attempt to write to invalid/unused file handle %d\n", handle);
    return;
  }

  fputs(data, s->file_handles[handle]);

}

void
kFGets(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *dest = UPARAM(0) + s->heap;
  int maxsize = UPARAM(1);
  int handle = UPARAM(2);

  if (handle == 0) {
    SCIkwarn(SCIkERROR, "Attempt to read from file handle 0\n");
    return;
  }

  if ((handle >= s->file_handles_nr) || (s->file_handles[handle] == NULL)) {
    SCIkwarn(SCIkERROR, "Attempt to read from invalid/unused file handle %d\n", handle);
    return;
  }

  fgets(dest, maxsize, s->file_handles[handle]);

}


/*************************************************************/
/* Parser */
/**********/

void
kSaid(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr said_block = UPARAM(0);

  CHECK_THIS_KERNEL_FUNCTION;

  if (s->debug_mode & (1 << SCIkPARSER_NR)) {
    SCIkdebug(SCIkPARSER, "Said block:", 0);
    vocab_decypher_said_block(s, said_block);
  }

  SCIkdebug(SCIkSTUB,"stub\n");
  
  s->acc = 0; /* Never true */
}


void
kParse(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int stringpos = UPARAM(0);
  char *string = s->heap + stringpos;
  int words_nr;
  char *error;
  result_word_t *words;
  heap_ptr event = UPARAM(1);

  CHECK_THIS_KERNEL_FUNCTION;

  words = vocab_tokenize_string(string, &words_nr,
				s->parser_words, s->parser_words_nr,
				s->parser_suffices, s->parser_suffices_nr,
				&error);

  if (words) {

    int syntax_fail = 0;

    s->acc = 1;

    if (s->debug_mode & (1 << SCIkPARSER_NR)) {
      int i;

      SCIkdebug(SCIkPARSER, "Parsed to the following blocks:\n", 0);

      for (i = 0; i < words_nr; i++)
	SCIkdebug(SCIkPARSER, "   Type[%04x] Group[%04x]\n", words[i].class, words[i].group);
    }

    if (vocab_build_parse_tree(&(s->parser_nodes[0]), words, words_nr, s->parser_branches,
			       s->parser_branches_nr))
      syntax_fail = 1; /* Building a tree failed */

    free(words);

    if (syntax_fail) {

      s->acc = 1;
      PUT_SELECTOR(event, claimed, 1);
      invoke_selector(INV_SEL(s->game_obj, syntaxFail, 0), 2, s->parser_base, stringpos);
      /* Issue warning */

      SCIkdebug(SCIkPARSER, "Tree building failed\n");

    } else {
      if (s->debug_mode & (1 << SCIkPARSER_NR))
	vocab_dump_parse_tree(s->parser_nodes);
    }

  } else {

    s->acc = 0;
    PUT_SELECTOR(event, claimed, 1);
    if (error) {

      strcpy(s->heap + s->parser_base, error);
      SCIkdebug(SCIkPARSER,"Word unknown: %s\n", error);
      /* Issue warning: */

      invoke_selector(INV_SEL(s->game_obj, wordFail, 0), 2, s->parser_base, stringpos);
      free(error);
      s->acc = 1; /* Tell them that it dind't work */
    }
  }
}


/*************************************************************/

void
kMemoryInfo(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  switch (PARAM(0)) {
  case 0: s->acc = heap_meminfo(s->_heap); break;
  case 1: s->acc = heap_largest(s->_heap); break;
  default: SCIkwarn(SCIkWARNING, "Unknown MemoryInfo operation: %04x\n", PARAM(0));
  }
}


/* kGetCWD(address):
** Writes the cwd to the supplied address and returns the address in acc.
** This implementation tries to use a game-specific directory in the user's
** home directory first.
*/
void
kGetCWD(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *homedir = getenv("HOME");
  char _cwd[256];
  char *cwd = &(_cwd[0]);
  char *savedir;
  char *targetaddr = UPARAM(0) + s->heap;

  s->acc = PARAM(0);

  if (!homedir) { /* We're probably not under UNIX if this happens */

    if (!getcwd(cwd, 255))
      cwd = "."; /* This might suffice */

    memcpy(targetaddr, cwd, strlen(cwd) + 1);
    return;
  }

  /* So we've got a home directory */

  if (chdir(homedir)) {
    fprintf(stderr,"Error: Could not enter home directory %s.\n", homedir);
    perror("Reason");
    exit(1); /* If we get here, something really bad is happening */
  }

  if (strlen(homedir) > MAX_HOMEDIR_SIZE) {
    fprintf(stderr, "Your home directory path is too long. Re-compile FreeSCI with "
	    "MAX_HOMEDIR_SIZE set to at least %i and try again.\n", strlen(homedir));
    exit(1);
  }

  memcpy(targetaddr, homedir, strlen(homedir));
  targetaddr += strlen(homedir); /* Target end of string for concatenation */
  *targetaddr++ = '/';
  *(targetaddr + 1) = 0;

  if (chdir(FREESCI_GAMEDIR))
    if (scimkdir(FREESCI_GAMEDIR, 0700)) {

      SCIkwarn(SCIkWARNING, "Warning: Could not enter ~/"FREESCI_GAMEDIR"; save files"
	      " will be written to ~/\n");

      return;

    }
    else /* mkdir() succeeded */
      chdir(FREESCI_GAMEDIR);

  memcpy(targetaddr, FREESCI_GAMEDIR, strlen(FREESCI_GAMEDIR));
  targetaddr += strlen(FREESCI_GAMEDIR);
  *targetaddr++ = '/';
  *targetaddr = 0;

  if (chdir(s->game_name))
    if (scimkdir(s->game_name, 0700)) {

      fprintf(stderr,"Warning: Could not enter ~/"FREESCI_GAMEDIR"/%s; "
	      "save files will be written to ~/"FREESCI_GAMEDIR"\n", s->game_name);

      return;
    }
    else /* mkdir() succeeded */
      chdir(s->game_name);

  memcpy(targetaddr, s->game_name, strlen(s->game_name));
  targetaddr += strlen(s->game_name);
  *targetaddr++ = '/';
  *targetaddr++ = 0;

}


void
kGetSaveDir(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = s->save_dir + 2; /* +2 to step over heap block size */
}


void
kSetCursor(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  free_mouse_cursor(s->mouse_pointer);

  if (PARAM(1)) {
    resource_t *resource = findResource(sci_cursor, PARAM(0));
    byte *data = (resource)? resource->data : NULL;
    if (data) {
      s->mouse_pointer = calc_mouse_cursor(data);
      s->mouse_pointer_nr = PARAM(0);
    } else {
      s->mouse_pointer = NULL;
      s->mouse_pointer_nr = -1;
    }

  } else
    s->mouse_pointer = NULL;

  if (argc > 2) {
    s->pointer_x = PARAM(2) + s->ports[s->view_port]->xmin;
    s->pointer_y = PARAM(3) + s->ports[s->view_port]->ymin; /* Port-relative */
  }

  (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0,0,0,0); /* Update mouse pointer */
}


void
kShow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->pic_visible_map = sci_ffs(UPARAM_OR_ALT(0, 1)) - 1;

  CHECK_THIS_KERNEL_FUNCTION;
  s->pic_not_valid = 2;
}


void
kPicNotValid(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;
  s->acc = s->pic_not_valid;
  if (argc)
    s->pic_not_valid = PARAM(0);
}


void
kRandom(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = PARAM(0) + (int) ((PARAM(1) + 1.0 - PARAM(0)) * (rand() / (RAND_MAX + 1.0)));
}


void
kAbs(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = abs(PARAM(0));
}


void
kSqrt(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = (gint16) sqrt((float) abs(PARAM(0)));
}


void
kGetAngle(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int xrel = PARAM(3) - PARAM(1);
  int yrel = PARAM(2) - PARAM(0);

  if ((xrel == 0) && (yrel == 0))
    s->acc = 0;
  else
    s->acc = (int) -(180.0/PI * atan2(yrel, xrel)) + 180;
}


void
kGetDistance(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int xrel = PARAM(1) - PARAM_OR_ALT(3, 0);
  int yrel = PARAM(0) - PARAM_OR_ALT(2, 0);

  s->acc = sqrt((float) xrel*xrel + yrel*yrel);
}

void
kTimesSin(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int angle = PARAM(0);
  int factor = PARAM(1);

  s->acc = (int) (factor * 1.0 * sin(angle * PI / 180.0));
}


void
kTimesCos(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int angle = PARAM(0);
  int factor = PARAM(1);

  s->acc = (int) (factor * 1.0 * cos(angle * PI / 180.0));
}

void
kCosDiv(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int angle = PARAM(1);
  int value = PARAM(0);
  double cosval = cos(angle * PI / 180.0);

  if ((cosval < 0.0001) && (cosval > 0.0001)) {
    SCIkwarn(SCIkWARNING,"Attepted division by zero\n");
    s->acc = 0x8000;
  } else
    s->acc = (gint16) (value/cosval);
}

void
kSinDiv(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int angle = PARAM(1);
  int value = PARAM(0);
  double sinval = sin(angle * PI / 180.0);

  if ((sinval < 0.0001) && (sinval > 0.0001)) {
    SCIkwarn(SCIkWARNING,"Attepted division by zero\n");
    s->acc = 0x8000;
  } else
    s->acc = (gint16) (value/sinval);
}

#define _K_SOUND_INIT 0
#define _K_SOUND_PLAY 1
#define _K_SOUND_NOP 2
#define _K_SOUND_DISPOSE 3
#define _K_SOUND_SETON 4
#define _K_SOUND_STOP 5
#define _K_SOUND_SUSPEND 6
#define _K_SOUND_RESUME 7
#define _K_SOUND_VOLUME 8
#define _K_SOUND_UPDATE 9
#define _K_SOUND_FADE 10
#define _K_SOUND_CHECK_DRIVER 11
#define _K_SOUND_STOP_ALL 12

void
kDoSound(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  word command = UPARAM(0);
  heap_ptr obj = UPARAM_OR_ALT(1, 0);

  CHECK_THIS_KERNEL_FUNCTION;

  if (s->debug_mode & (1 << SCIkSOUNDCHK_NR)) {
    int i;

    SCIkdebug(SCIkSOUND, "Command 0x%x", command);
    switch (command) {
    case 0: sciprintf("[InitObj]"); break;
    case 1: sciprintf("[Play]"); break;
    case 2: sciprintf("[NOP]"); break;
    case 3: sciprintf("[DisposeHandle]"); break;
    case 4: sciprintf("[SetSoundOn(?)]"); break;
    case 5: sciprintf("[Stop]"); break;
    case 6: sciprintf("[Suspend]"); break;
    case 7: sciprintf("[Resume]"); break;
    case 8: sciprintf("[Get(Set?)Volume]"); break;
    case 9: sciprintf("[Signal: Obj changed]"); break;
    case 10: sciprintf("[Fade(?)]"); break;
    case 11: sciprintf("[ChkDriver]"); break;
    case 12: sciprintf("[StopAll]"); break;
    }

    sciprintf("(");
    for (i = 1; i < argc; i++) {
      sciprintf("%04x", UPARAM(i));
      if (i + 1 < argc)
	sciprintf(", ");
    }
    sciprintf(")\n");
  }


  if (s->sfx_driver)
    switch (command) {
    case _K_SOUND_INIT:

      s->sfx_driver->command(s, SOUND_COMMAND_INIT_SONG, obj, GET_SELECTOR(obj, number));
      break;

    case _K_SOUND_PLAY:

      s->sfx_driver->command(s, SOUND_COMMAND_SET_LOOPS, obj, GET_SELECTOR(obj, loop));
      s->sfx_driver->command(s, SOUND_COMMAND_PLAY_HANDLE, obj, 0);
      break;

    case _K_SOUND_NOP:

      break;

    case _K_SOUND_DISPOSE:

      s->sfx_driver->command(s, SOUND_COMMAND_DISPOSE_HANDLE, obj, 0);
      break;

    case _K_SOUND_SETON:

      s->sfx_driver->command(s, SOUND_COMMAND_SET_MUTE, obj, 0);
      break;

    case _K_SOUND_STOP:

      s->sfx_driver->command(s, SOUND_COMMAND_STOP_HANDLE, obj, 0);
      break;

    case _K_SOUND_SUSPEND:

      s->sfx_driver->command(s, SOUND_COMMAND_SUSPEND_HANDLE, obj, 0);
      break;

    case _K_SOUND_RESUME:

      s->sfx_driver->command(s, SOUND_COMMAND_RESUME_HANDLE, obj, 0);
      break;

    case _K_SOUND_VOLUME:

      s->acc = 0xc; /* FIXME */
      break;

    case _K_SOUND_UPDATE:

      s->sfx_driver->command(s, SOUND_COMMAND_RENICE_HANDLE, obj, GET_SELECTOR(obj, priority));
      s->sfx_driver->command(s, SOUND_COMMAND_SET_LOOPS, obj, GET_SELECTOR(obj, loop));
      break;

    case _K_SOUND_FADE:

      s->sfx_driver->command(s, SOUND_COMMAND_FADE_HANDLE, obj, 120); /* Fade out in 2 secs */
      break;

    case _K_SOUND_CHECK_DRIVER:

      s->acc = s->sfx_driver->command(s, SOUND_COMMAND_TEST, 0, 0);
      break;

    case _K_SOUND_STOP_ALL:

      s->acc = s->sfx_driver->command(s, SOUND_COMMAND_STOP_ALL, 0, 0);
      break;

    default:
      SCIkwarn(SCIkWARNING, "Unhandled DoSound command: %x\n", command);

  }
  process_sound_events(s); /* Take care of incoming events */

}

#define K_GRAPH_GET_COLORS_NR 2
#define K_GRAPH_DRAW_LINE 4
#define K_GRAPH_SAVE_BOX 7
#define K_GRAPH_RESTORE_BOX 8
#define K_GRAPH_FILL_BOX_BACKGROUND 9
#define K_GRAPH_FILL_BOX_FOREGROUND 10
#define K_GRAPH_FILL_BOX_ANY 11
#define K_GRAPH_UPDATE_BOX 12
#define K_GRAPH_REDRAW_BOX 13
#define K_GRAPH_ADJUST_PRIORITY 14

void
kGraph(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int color, priority, special;
  port_t *port = s->ports[s->view_port];

  _k_dyn_view_list_prepare_change(s);

  switch(PARAM(0)) {

  case K_GRAPH_GET_COLORS_NR:

    s->acc = (sci_version < SCI_VERSION_1) ? 0x10 : 0x100; /* number of colors */
    break;

  case K_GRAPH_DRAW_LINE:

    color = PARAM(5);
    priority = PARAM(6);
    special = PARAM(7);
    dither_line(s->pic, PARAM(2), PARAM(1) , PARAM(4), PARAM(3),
		color, color, priority, special,
		!(color & 0x80) | (!(priority & 0x80) << 1) | (!(special & 0x80) << 2));
    break;

  case K_GRAPH_SAVE_BOX:

    s->acc = graph_save_box(s, PARAM(2), PARAM(1), PARAM(4), PARAM(3), PARAM(5));
    break;

  case K_GRAPH_RESTORE_BOX:

    graph_restore_box(s, PARAM(1));
    break;

  case K_GRAPH_FILL_BOX_BACKGROUND:

    graph_clear_box(s, port->xmin, port->ymin,
		    port->xmax - port->xmin + 1, port->ymax - port->ymin + 1,
		    port->bgcolor);
    CHECK_THIS_KERNEL_FUNCTION;
    break;

  case K_GRAPH_FILL_BOX_FOREGROUND:

    graph_clear_box(s, port->xmin, port->ymin,
		    port->xmax - port->xmin + 1, port->ymax - port->ymin + 1,
		    port->color);
    CHECK_THIS_KERNEL_FUNCTION;
    break;

  case K_GRAPH_FILL_BOX_ANY: {

    int x = PARAM(2);
    int y = PARAM(1);

    SCIkdebug(SCIkGRAPHICS, "fill_box_any(%d, %d, %d, %d) map=%d (%d %d)\n",
	      PARAM(1), PARAM(2), PARAM(3), PARAM(4), PARAM(5), PARAM(6), PARAM_OR_ALT(7, -1));

    graph_fill_box_custom(s, x + s->ports[s->view_port]->xmin, y + s->ports[s->view_port]->ymin,
			  PARAM(4)-x, PARAM(3)-y, PARAM(6), PARAM_OR_ALT(7, -1),
			  PARAM_OR_ALT(8, -1), UPARAM(5));
    CHECK_THIS_KERNEL_FUNCTION;

  }
  break;

  case K_GRAPH_UPDATE_BOX: {

    int x = PARAM(2);
    int y = PARAM(1);

    SCIkdebug(SCIkGRAPHICS, "update_box(%d, %d, %d, %d)\n",
	      PARAM(1), PARAM(2), PARAM(3), PARAM(4));

    (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_BOX, x, y + 10, PARAM(4)-x+1, PARAM(3)-y+1);
    CHECK_THIS_KERNEL_FUNCTION;

  }
  break;

  case K_GRAPH_REDRAW_BOX:
    CHECK_THIS_KERNEL_FUNCTION;

    SCIkdebug(SCIkGRAPHICS, "redraw_box(%d, %d, %d, %d)\n",
	      PARAM(1), PARAM(2), PARAM(3), PARAM(4));

    SCIkwarn(SCIkWARNING, "KERNEL_GRAPH_REDRAW_BOX: stub\n");
    break;

  case K_GRAPH_ADJUST_PRIORITY:

    s->priority_first = PARAM(1) - 10;
    s->priority_last = PARAM(2) - 10;
    break;

  default:
    
    CHECK_THIS_KERNEL_FUNCTION;
    SCIkdebug(SCIkSTUB, "Unhandled Graph() operation %04x\n", PARAM(0));
    
  }

  _k_dyn_view_list_accept_change(s);
}

void
k_Unknown(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  switch (funct_nr) {
  case 0x70: kGraph(s, funct_nr, argc, argp); break;
  default: {
    CHECK_THIS_KERNEL_FUNCTION;
    SCIkdebug(SCIkSTUB, "Unhandled Unknown function %04x\n", funct_nr);
  }
  }
}

void
kGetEvent(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int mask = UPARAM(0);
  heap_ptr obj = UPARAM(1);
  sci_event_t e;
  int oldx, oldy;
  
  CHECK_THIS_KERNEL_FUNCTION;
  
  /*If there's a simkey pending, and the game wants a keyboard event, use the
   *simkey instead of a normal event*/
  if (_kdebug_cheap_event_hack && (mask&SCI_EVT_KEYBOARD)) {
    PUT_SELECTOR(obj, type, SCI_EVT_KEYBOARD); /*Keyboard event*/
    s->acc=1;
    PUT_SELECTOR(obj, message, _kdebug_cheap_event_hack);
    PUT_SELECTOR(obj, modifiers, SCI_EVM_NUMLOCK); /*Numlock on*/
    PUT_SELECTOR(obj, x, s->pointer_x);
    PUT_SELECTOR(obj, y, s->pointer_y);
    _kdebug_cheap_event_hack = 0;
    return;
  }
  
  oldx=s->pointer_x;
  oldy=s->pointer_y;
  e=getEvent(s);

  PUT_SELECTOR(obj, x, s->pointer_x);
  PUT_SELECTOR(obj, y, s->pointer_y);
  if((oldx!=s->pointer_x || oldy!=s->pointer_y) && s->have_mouse_flag)
    s->gfx_driver->Redraw(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0, 0, 0, 0);
  
  switch(e.type)
    {
    case SCI_EVT_KEYBOARD:
      {
	if ((e.buckybits & SCI_EVM_LSHIFT) && (e.buckybits & SCI_EVM_RSHIFT)
	    && (e.data == '-')) {

	  sciprintf("Debug mode activated\n");

	  script_debug_flag = 1; /* Enter debug mode */
	  _debug_seeking = _debug_step_running = 0;
	  s->onscreen_console = 0;

	} else

	  if ((e.buckybits & SCI_EVM_CTRL) && (e.data == '`')) {

	    script_debug_flag = 1; /* Enter debug mode */
	    _debug_seeking = _debug_step_running = 0;
	    s->onscreen_console = 1;

	} else {

	  PUT_SELECTOR(obj, type, SCI_EVT_KEYBOARD); /*Keyboard event*/
	  s->acc=1;
	  PUT_SELECTOR(obj, message, e.data);
	  PUT_SELECTOR(obj, modifiers, e.buckybits);
	}
      } break;
    case SCI_EVT_CLOCK:
      {
	s->acc = 0; /* Null event */
	return;
      } break;
    case SCI_EVT_REDRAW:
      {
	s->acc = 0; /* Not supported yet */
      } break;
    case SCI_EVT_MOUSE_RELEASE:
    case SCI_EVT_MOUSE_PRESS:
      {
	int extra_bits=0;
	if(mask&e.type)
          {
            switch(e.data)
	      {
	      case 2: extra_bits=SCI_EVM_LSHIFT|SCI_EVM_RSHIFT; break;
	      case 3: extra_bits=SCI_EVM_CTRL;
	      }
	    PUT_SELECTOR(obj, type, e.type);
	    PUT_SELECTOR(obj, message, 1);
	    PUT_SELECTOR(obj, modifiers, e.buckybits|extra_bits);
	    s->acc=1;
	  }
	return;
      } break;
    default:
      {
	s->acc = 0; /* Unknown event */
      }
    }
}

void
kMapKeyToDir(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = UPARAM(0);

  if (GET_SELECTOR(obj, type) == SCI_EVT_KEYBOARD) { /* Keyboard */
    int mover = -1;
    switch (GET_SELECTOR(obj, message)) {
    case SCI_K_HOME: mover = 8; break;
    case SCI_K_UP: mover = 1; break;
    case SCI_K_PGUP: mover = 2; break;
    case SCI_K_LEFT: mover = 7; break;
    case SCI_K_CENTER:
    case 76: mover = 0; break;
    case SCI_K_RIGHT: mover = 3; break;
    case SCI_K_END: mover = 6; break;
    case SCI_K_DOWN: mover = 5; break;
    case SCI_K_PGDOWN: mover = 4; break;
    }

    if (mover >= 0) {
      PUT_SELECTOR(obj, type, SCI_EVT_JOYSTICK);
      PUT_SELECTOR(obj, message, mover);
      s->acc = 1;
    } else s->acc = 0;
  }
}


void
kGlobalToLocal(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = UPARAM_OR_ALT(0, 0);

  if (obj) {
    int x = GET_SELECTOR(obj, x);
    int y = GET_SELECTOR(obj, y);

    PUT_SELECTOR(obj, x, x - s->ports[s->view_port]->xmin);
    PUT_SELECTOR(obj, y, y - s->ports[s->view_port]->ymin);
  }
}


void
kLocalToGlobal(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = UPARAM_OR_ALT(0, 0);

  if (obj) {
    int x = GET_SELECTOR(obj, x);
    int y = GET_SELECTOR(obj, y);

    PUT_SELECTOR(obj, x, x + s->ports[s->view_port]->xmin);
    PUT_SELECTOR(obj, y, y + s->ports[s->view_port]->ymin);
  }
}


void
kStrEnd(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr address = UPARAM(0);
  char *seeker = s->heap + address;

  while (*seeker++)
    ++address;

  s->acc = address + 1; /* End of string */
}

void
kStrCat(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;

  strcat(s->heap + UPARAM(0), s->heap + UPARAM(1));
}

void
kStrCmp(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int length;

  if (argc > 2)
    s->acc = strncmp(s->heap + UPARAM(0), s->heap + UPARAM(1), UPARAM(2));
  else
    s->acc = strcmp(s->heap + UPARAM(0), s->heap + UPARAM(1));
}


void
kStrCpy(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int length;

  CHECK_THIS_KERNEL_FUNCTION;

  if (argc > 2)
    strncpy(s->heap + UPARAM(0), s->heap + UPARAM(1), UPARAM(2));
  else
    strcpy(s->heap + UPARAM(0), s->heap + UPARAM(1));

  s->acc = PARAM(0);

}


void
kStrAt(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr address = UPARAM(0) + UPARAM(1);

  s->acc = s->heap[address];

  if (argc > 2)
    s->heap[address]=UPARAM(2); /* Request to modify this char */
}


void
kReadNumber(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *source = s->heap + UPARAM(0);

  while (isspace(*source))
    source++; /* Skip whitespace */

  if (*source == '$') /* SCI uses this for hex numbers */
    s->acc = strtol(source + 1, NULL, 16); /* Hex */
  else
    s->acc = strtol(source, NULL, 10); /* Force decimal */
}


/*  Format(targ_address, textresnr, index_inside_res, ...)
** or
**  Format(targ_address, heap_text_addr, ...)
** Formats the text from text.textresnr (offset index_inside_res) or heap_text_addr according to
** the supplied parameters and writes it to the targ_address.
*/
void
kFormat(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int *arguments;
  int dest = UPARAM(0);
  char *target = ((char *) s->heap) + dest;
  int position = UPARAM(1);
  int index = UPARAM(2);
  resource_t *resource;
  char *source;
  int mode = 0;
  int paramindex = 0; /* Next parameter to evaluate */
  char xfer;
  int i;
  int startarg;
  int str_leng; /* Used for stuff like "%13s" */
  char *abstarget = target;

  CHECK_THIS_KERNEL_FUNCTION;

  source = _kernel_lookup_text(s, position, index);

  SCIkdebug(SCIkSTRINGS, "Formatting \"%s\"\n", source);

  if (position < 1000) {
    startarg = 3; /* First parameter to use for formatting */
  } else  { /* position >= 1000 */
    startarg = 2;
  }

  arguments = malloc(sizeof(int) * argc);
  for (i = startarg; i < argc; i++)
    arguments[i-startarg] = UPARAM(i); /* Parameters are copied to prevent overwriting */

  while (xfer = *source++) {
    if (xfer == '%') {
      if (mode == 1)
	*target++ = '%'; /* Literal % by using "%%" */
      else {
	mode = 1;
	str_leng = 0;
      }
    }
    else { /* xfer != '%' */
      if (mode == 1) {
	switch (xfer) {
	case 's': { /* Copy string */
	  char *tempsource = _kernel_lookup_text(s, arguments[paramindex], arguments[paramindex + 1]);
	  int extralen = str_leng - strlen(tempsource);

	  if (arguments[paramindex] > 1000) /* Heap address? */
	    paramindex++;
	  else
	    paramindex += 2; /* No, text resource address */

	  while (extralen-- > 0)
	    *target++ = ' '; /* Format into the text */

	  while (*target++ = *tempsource++);
	  target--; /* Step back on terminator */
	  mode = 0;
	}
	break;

	case 'c': { /* insert character */
	  while (str_leng-- > 1)
	    *target++ = ' '; /* Format into the text */

	  *target++ = arguments[paramindex++];
	}
	break;

	case 'd': { /* Copy decimal */
	  int templen = sprintf(target, "%d", arguments[paramindex++]);

	  if (templen < str_leng) {
	    int diff = str_leng - templen;
	    char *dest = target + str_leng - 1;

	    while (templen--) {
	      *dest = *(dest + diff);
	      dest--;
	    } /* Copy the number */

	    while (diff--)
	      *dest-- = ' '; /* And fill up with blanks */

	    templen = str_leng;
	  }

	  target += templen;
	  mode = 0;
	}
	break;
	default:
	  if (isdigit(xfer))
	    str_leng = (str_leng * 10) + (xfer - '0');
	  else {
	    *target = '%';
	    *target++;
	    *target = xfer;
	    *target++;
	    mode = 0;
	  } /* if (!isdigit(xfer)) */
	}
      } else { /* mode != 1 */
	*target = xfer;
	*target++;
      }
    }
  }

  free(arguments);

  *target = 0; /* Terminate string */
  s->acc = dest; /* Return target addr */
}


void
kTextSize(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int width, height;
  heap_ptr heap_text = PARAM(1);
  char *text = s->heap + heap_text;
  heap_ptr dest = PARAM(0);
  int maxwidth = UPARAM_OR_ALT(3, 0);
  resource_t *fontres = findResource(sci_font, UPARAM(2));

  if (!maxwidth)
    maxwidth = MAX_TEXT_WIDTH_MAGIC_VALUE;

  if (!fontres) {
    SCIkwarn(SCIkERROR, "font.%03d not found!\n", UPARAM(2));
    return;
  }

  get_text_size(text, fontres->data, maxwidth, &width, &height);

  PUT_HEAP(dest + 0, 0);
  PUT_HEAP(dest + 2, 0);
  PUT_HEAP(dest + 4, height);
  PUT_HEAP(dest + 6, width); /* The reason for this is unknown to me */
}



void
kAddMenu(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  menubar_add_menu(s->menubar, s->heap + UPARAM(0), s->heap + UPARAM(1), s->titlebar_port.font);
}


void
kSetMenu(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int index = UPARAM(0);

  menubar_set_foobar(s->menubar, (index >> 8) - 1, (index & 0xff) - 1, PARAM(1), UPARAM(2));
}

void
kGetTime(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  struct tm* loc_time;
  GTimeVal time_prec;
  time_t the_time;

  if (argc) { /* Get seconds since last am/pm switch */
    the_time = time(NULL);
    loc_time = localtime(&the_time);
    s->acc = loc_time->tm_sec + loc_time->tm_min * 60 + (loc_time->tm_hour % 12) * 3600;
  } else { /* Get time since game started */
    g_get_current_time (&time_prec);
    s-> acc = ((time_prec.tv_usec - s->game_start_time.tv_usec) * 60 / 1000000) +
      (time_prec.tv_sec - s->game_start_time.tv_sec) * 60;
  }
}

void
kStrLen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = strlen(s->heap + UPARAM(0));
}


void
kGetFarText(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *textres = findResource(sci_text, UPARAM(0));
  char *seeker;
  int counter = PARAM(1);

  CHECK_THIS_KERNEL_FUNCTION;

  if (!textres) {
    SCIkwarn(SCIkERROR, "text.%d does not exist\n", PARAM(0));
    return;
  }

  seeker = textres->data;

  while (counter--)
    while (*seeker++);
  /* The second parameter (counter) determines the number of the string inside the text
  ** resource.
  */

  s->acc = UPARAM(2);
  strcpy(s->heap + UPARAM(2), seeker); /* Copy the string and get return value */
}

void
kWait(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  GTimeVal time;
  int SleepTime = PARAM(0);

  g_get_current_time (&time);

  s-> acc = ((time.tv_usec - s->last_wait_time.tv_usec) * 60 / 1000000) +
    (time.tv_sec - s->last_wait_time.tv_sec) * 60;

  memcpy(&(s->last_wait_time), &time, sizeof(GTimeVal));

  (*s->gfx_driver->Wait)(s, SleepTime * 1000000 / 60);
}


void
kCoordPri(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int y = PARAM(0);

  s->acc = VIEW_PRIORITY(y);
}


void
kPriCoord(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int priority = PARAM(0);

  s->acc = PRIORITY_BAND_FIRST(priority);
}


void
kValidPath(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *pathname = s->heap + UPARAM(0);

  s->acc = !chdir(pathname); /* Try to go there. If it works, return 1, 0 otherwise. */
}


void
kRespondsTo(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int obj = PARAM(0);
  int selector = PARAM(1);

  s->acc = (lookup_selector(s, obj, selector, NULL) != SELECTOR_NONE);
}


void
kDirLoop(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = PARAM(0);
  word angle = UPARAM(1);
  int view = GET_SELECTOR(obj, view);
  int signal = UGET_SELECTOR(obj, signal);
  resource_t *viewres = findResource(sci_view, view);
  int loop;
  int maxloops;

  if (signal & _K_VIEW_SIG_FLAG_DOESNT_TURN)
    return;

  if (!viewres) {
    SCIkwarn(SCIkERROR, "Invalid view.%03d\n", view);
    PUT_SELECTOR(obj, loop, 0xffff); /* Invalid */
    return;
  }

  if (angle > 360) {
    SCIkwarn(SCIkERROR, "Invalid angle %d\n", angle);
    PUT_SELECTOR(obj, loop, 0xffff);
    return;
  }

  if (angle < 45)
    loop = 3;
  else
    if (angle < 135)
      loop = 0;
  else
    if (angle < 225)
      loop = 2;
  else
    if (angle < 314)
      loop = 1;
  else
    loop = 3;

  maxloops = view0_loop_count(viewres->data);

  if (loop >= maxloops) {
    SCIkwarn(SCIkWARNING, "With view.%03d: loop %d > maxloop %d\n", view, loop, maxloops);
    loop = 0;
  }

  PUT_SELECTOR(obj, loop, loop);
}


#define _K_BRESEN_AXIS_X 0
#define _K_BRESEN_AXIS_Y 1

void
kInitBresen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr mover = PARAM(0);
  heap_ptr client = GET_SELECTOR(mover, client);
  int step_factor = PARAM_OR_ALT(1, 1);
  int deltax = GET_SELECTOR(mover, x) - GET_SELECTOR(client, x);
  int deltay = GET_SELECTOR(mover, y) - GET_SELECTOR(client, y);
  int stepx = GET_SELECTOR(client, xStep) * step_factor;
  int stepy = GET_SELECTOR(client, yStep) * step_factor;
  int numsteps_x = (abs(deltax) + stepx-1) / stepx;
  int numsteps_y = (abs(deltay) + stepy-1) / stepy;
  int bdi, i1;
  int numsteps;
  int deltax_step;
  int deltay_step;

  if (numsteps_x > numsteps_y) {
    numsteps = numsteps_x;
    deltax_step = (deltax < 0)? -stepx : stepx;
    deltay_step = numsteps? deltay / numsteps : deltay;
  } else { /* numsteps_x <= numsteps_y */
    numsteps = numsteps_y;
    deltay_step = (deltay < 0)? -stepy : stepy;
    deltax_step = numsteps? deltax / numsteps : deltax;
  }

  /*  if (abs(deltax) > abs(deltay)) {*/ /* Bresenham on y */
  if (numsteps_y < numsteps_x) {

    PUT_SELECTOR(mover, b_xAxis, _K_BRESEN_AXIS_Y);
    PUT_SELECTOR(mover, b_incr, (deltay < 0)? -1 : 1);
    i1 = 2 * (abs(deltay) - abs(deltay_step * numsteps)) * abs(deltax_step);
    bdi = -abs(deltax);

  } else { /* Bresenham on x */

    PUT_SELECTOR(mover, b_xAxis, _K_BRESEN_AXIS_X);
    PUT_SELECTOR(mover, b_incr, (deltax < 0)? -1 : 1);
    i1= 2 * (abs(deltax) - abs(deltax_step * numsteps)) * abs(deltay_step);
    bdi = -abs(deltay);

  }

  PUT_SELECTOR(mover, b_movCnt, 0);

  PUT_SELECTOR(mover, dx, deltax_step);
  PUT_SELECTOR(mover, dy, deltay_step);

  SCIkdebug(SCIkBRESEN, "Init bresen for mover %04x: d=(%d,%d)\n", mover, deltax, deltay);
  SCIkdebug(SCIkBRESEN, "    steps=%d, mv=(%d, %d), i1= %d, i2=%d\n",
	    numsteps, deltax_step, deltay_step, i1, bdi*2);

  PUT_SELECTOR(mover, b_di, bdi);
  PUT_SELECTOR(mover, b_i1, i1);
  PUT_SELECTOR(mover, b_i2, bdi * 2);

}


void
kDoBresen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr mover = PARAM(0);
  heap_ptr client = GET_SELECTOR(mover, client);
  heap_ptr cycler = GET_SELECTOR(client, cycler);

  int x = GET_SELECTOR(client, x);
  int y = GET_SELECTOR(client, y);
  int oldx = x, oldy = y;
  int destx = GET_SELECTOR(mover, x);
  int desty = GET_SELECTOR(mover, y);
  int dx = GET_SELECTOR(mover, dx);
  int dy = GET_SELECTOR(mover, dy);
  int bdi = GET_SELECTOR(mover, b_di);
  int bi1 = GET_SELECTOR(mover, b_i1);
  int bi2 = GET_SELECTOR(mover, b_i2);
  int movcnt = GET_SELECTOR(mover, b_movCnt);
  int bdelta = GET_SELECTOR(mover, b_incr);
  int axis = GET_SELECTOR(mover, b_xAxis);

  PUT_SELECTOR(mover, b_movCnt, movcnt + 1);

  if ((bdi += bi1) > 0) {
    bdi += bi2;

    if (axis == _K_BRESEN_AXIS_X)
      dx += bdelta;
    else
      dy += bdelta;
  }

  PUT_SELECTOR(mover, b_di, bdi);

  x += dx;
  y += dy;
  if ((((x <= destx) && (oldx >= destx)) || ((x >= destx) && (oldx <= destx)))
      && (((y <= desty) && (oldy >= desty)) || ((y >= desty) && (oldy <= desty))))
    /* Whew... in short: If we have reached or passed our target position */
    {
      x = destx;
      y = desty;

      PUT_SELECTOR(mover, completed, 1); /* Finish! */

      SCIkdebug(SCIkBRESEN, "Finished mover %04x\n", mover);
    }

  PUT_SELECTOR(client, x, x);
  PUT_SELECTOR(client, y, y);

  invoke_selector(INV_SEL(client, canBeHere, 0), 0);

  if (s->acc) { /* Contains the return value */
    s->acc = GET_SELECTOR(mover, completed);
    return;
  } else {
    word signal = GET_SELECTOR(client, signal);

    PUT_SELECTOR(client, x, oldx);
    PUT_SELECTOR(client, y, oldy);
    PUT_SELECTOR(mover, completed, 1);
 
    PUT_SELECTOR(mover, x, oldx);
    PUT_SELECTOR(mover, y, oldy);    

    PUT_SELECTOR(client, signal, (signal | _K_VIEW_SIG_FLAG_HIT_OBSTACLE));

    SCIkdebug(SCIkBRESEN, "Finished mover %04x\n", mover);
    s->acc = 1;
  }

}


void
kCanBeHere(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = PARAM(0);
  heap_ptr cliplist = UPARAM_OR_ALT(1, 0);
  word retval;
  word signal;

  int x = GET_SELECTOR(obj, brLeft);
  int y = GET_SELECTOR(obj, brTop);
  int xend = GET_SELECTOR(obj, brRight);
  int yend = GET_SELECTOR(obj, brBottom);
  int xl = xend - x + 1;
  int yl = yend - y + 1;
  word edgehit;

  signal = GET_SELECTOR(obj, signal);
  /*  SCIkdebug(SCIkBRESEN,"Checking collision: (%d,%d) to (%d,%d)\n",
      x, y, xend, yend);*/

  s->acc = !(((word)GET_SELECTOR(obj, illegalBits))
	     & (edgehit = graph_on_control(s, x, y + 10, xl, yl, SCI_MAP_CONTROL)));

  if (s->acc == 0)
    return; /* Can'tBeHere */
  if ((signal & _K_VIEW_SIG_FLAG_DONT_RESTORE) || (signal & _K_VIEW_SIG_FLAG_IGNORE_ACTOR))
    return; /* CanBeHere- it's either being disposed, or it ignores actors anyway */
  if (cliplist) {
    heap_ptr node = GET_HEAP(cliplist + LIST_FIRST_NODE);

    s->acc = 0; /* Assume that we Can'tBeHere... */

    while (node) { /* Check each object in the list against our bounding rectangle */
      heap_ptr other_obj = UGET_HEAP(node + LIST_NODE_VALUE);
      if (other_obj != obj) { /* Clipping against yourself is not recommended */

	int other_x = GET_SELECTOR(other_obj, brLeft);
	int other_y = GET_SELECTOR(other_obj, brTop);
	int other_xend = GET_SELECTOR(other_obj, brRight);
	int other_yend = GET_SELECTOR(other_obj, brBottom);
	/*	SCIkdebug(SCIkBRESEN, "  against (%d,%d) to (%d, %d)\n",
		other_x, other_y, other_xend, other_yend);*/

	if ((((other_x >= x) && (other_x <= xend)) /* Other's left boundary inside of our object? */
	    || ((other_xend >= x) && (other_xend <= xend))) /* ...right boundary... ? */
	    &&
	    (((other_y >= y) && (other_y <= yend)) /* Other's top boundary inside of our object? */
	    || ((other_yend >= y) && (other_yend <= yend)))) /* ...bottom boundary... ? */
	  return;

	if (((other_x >= x) && (other_xend <= xend)
	    && (other_y >= y) && (other_yend <= yend)) /* Other object inside this object? */
	  ||
	    ((other_x <= x) && (other_xend >= xend)
	     && (other_y <= y) && (other_yend >= yend))) /* Other object surrounds this one? */
	  return;

	/*	SCIkdebug(SCIkBRESEN, " (no)\n");*/

      } /* if (other_obj != obj) */
      node = GET_HEAP(node + LIST_NEXT_NODE); /* Move on */
    }
  }

  s->acc = 1;
  /* CanBeHere */
}


/********************* Graphics ********************/

void
kCelHigh(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *viewres = findResource(sci_view, PARAM(0));
  int result;

  if (!viewres) {
    SCIkwarn(SCIkERROR, "view.%d (0x%x) not found\n", PARAM(0), PARAM(0));
    return;
  }

  s->acc = result = view0_cel_height(PARAM(1), PARAM(2), viewres->data);
  if (result < 0)
    SCIkwarn(SCIkERROR, "Invalid loop (%d) or cel (%d) in view.%d (0x%x)\n", PARAM(1), PARAM(2), PARAM(0), PARAM(0));
}

void
kCelWide(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *viewres = findResource(sci_view, PARAM(0));
  int result;

  if (!viewres) {
    SCIkwarn(SCIkERROR, "view.%d (0x%x) not found\n", PARAM(0), PARAM(0));
    return;
  }

  s->acc = result = view0_cel_width(PARAM(1), PARAM(2), viewres->data);
  if (result < 0)
    SCIkwarn(SCIkERROR, "Invalid loop (%d) or cel (%d) in view.%d (0x%x)\n", PARAM(1), PARAM(2), PARAM(0), PARAM(0));
}



void
kNumLoops(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = PARAM(0);
  resource_t *viewres = findResource(sci_view, GET_SELECTOR(obj, view));

  CHECK_THIS_KERNEL_FUNCTION;

  if (!viewres) {
    SCIkwarn(SCIkERROR, "view.%d (0x%x) not found\n", PARAM(0), PARAM(0));
    return;
  }

  s->acc = view0_loop_count(viewres->data);
}


void
kNumCels(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = PARAM(0);
  int loop = GET_SELECTOR(obj, loop);
  int view;
  resource_t *viewres = findResource(sci_view, view = GET_SELECTOR(obj, view));

  CHECK_THIS_KERNEL_FUNCTION;

  if (!viewres) {
    SCIkwarn(SCIkERROR, "view.%d (0x%x) not found\n", PARAM(0), PARAM(0));
    return;
  }

  s->acc = view0_cel_count(loop, viewres->data);
}

void
kOnControl(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int arg = 0;
  int map, xstart, ystart;
  int xlen = 1, ylen = 1;

  CHECK_THIS_KERNEL_FUNCTION;

  if (argc == 2 || argc == 4)
    map = 4;
  else {
    arg = 1;
    map = PARAM(0);
  }

  ystart = PARAM(arg+1);
  xstart = PARAM(arg);

  if (argc > 3) {
    ylen = PARAM(arg+3) - ystart + 1;
    xlen = PARAM(arg+2) - xstart + 1;
  }
  s->acc = graph_on_control(s, xstart, ystart + 10, xlen, ylen, map);

}

void
_k_view_list_free_backgrounds(state_t *s, view_object_t *list, int list_nr);
void
_k_view_list_dispose(state_t *s, view_object_t **list_ptr, int *list_nr_ptr);

void
kDrawPic(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *resource = findResource(sci_pic, PARAM(0));
  CHECK_THIS_KERNEL_FUNCTION;

  if (resource) {

    if (s->version < SCI_VERSION_FTU_NEWER_DRAWPIC_PARAMETERS) {
      if (!PARAM_OR_ALT(2, 0)) {
	clear_picture(s->pic, 15);
	_k_view_list_dispose(s, &(s->dyn_views), &(s->dyn_views_nr));
      }
    } else
      if (PARAM_OR_ALT(2, 1)) {
	clear_picture(s->pic, 15);
	_k_view_list_dispose(s, &(s->dyn_views), &(s->dyn_views_nr));
      }

    draw_pic0(s->pic, 1, PARAM_OR_ALT(3, 0), resource->data);

    if (argc > 1)
      s->pic_animate = PARAM(1); /* The animation used during kAnimate() later on */

    s->pic_not_valid = 1;
    s->pic_is_new = 1;

    if (s->pic_views_nr)
      free(s->pic_views);
    s->pic_views_nr = 0;

  } else
    SCIkwarn(SCIkERROR, "Request to draw non-existing pic.%03d\n", PARAM(0));
}


void
kBaseSetter(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int x, y, original_y, z, ystep, xsize, ysize;
  int xbase, ybase, xend, yend;
  int view, loop, cell;
  resource_t *viewres;
  heap_ptr object = PARAM(0);

  CHECK_THIS_KERNEL_FUNCTION;

  x = GET_SELECTOR(object, x);
  original_y = y = GET_SELECTOR(object, y);
  z = GET_SELECTOR(object, z);

  y -= z; /* Subtract z offset */

  ystep = GET_SELECTOR(object, yStep);
  view = GET_SELECTOR(object, view);
  loop = GET_SELECTOR(object, loop);
  cell = GET_SELECTOR(object, cel);

  viewres = findResource(sci_view, view);

  if (!viewres)
    xsize = ysize = 0;
  else {
    xsize = view0_cel_width(loop, cell, viewres->data);
    ysize = view0_cel_height(loop, cell, viewres->data);
    /*    view0_base_modify(loop,cell,viewres->data, &x, &y); */
  }

  if ((xsize < 0) || (ysize < 0))
    xsize = ysize = 0; /* Invalid view/loop */

  xbase = x - (xsize - 1) / 2;
  xend = xbase + xsize -1;
  yend = y;
  ybase = yend - ystep;

  PUT_SELECTOR(object, brLeft, xbase);
  PUT_SELECTOR(object, brRight, xend);
  PUT_SELECTOR(object, brTop, ybase);
  PUT_SELECTOR(object, brBottom, yend);


  if (s->debug_mode & (1 << SCIkBASESETTER_NR)) {
    graph_clear_box(s, xbase, ybase + 10, xend-xbase+1, yend-ybase+1, VIEW_PRIORITY(original_y));
    (*s->gfx_driver->Wait)(s, 100000);
  }

} /* kBaseSetter */



#define K_CONTROL_BUTTON 1
#define K_CONTROL_TEXT 2
#define K_CONTROL_EDIT 3
#define K_CONTROL_ICON 4
#define K_CONTROL_CONTROL 6
#define K_CONTROL_SELECT_BOX 10

void
_k_draw_control(state_t *s, heap_ptr obj);


void
kDrawControl(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = PARAM(0);

  CHECK_THIS_KERNEL_FUNCTION;

  _k_dyn_view_list_prepare_change(s);
  _k_draw_control(s, obj);
  _k_dyn_view_list_accept_change(s);
}


#define _K_EDIT_DELETE \
 if (cursor < textlen) { \
  memmove(text + cursor, text + cursor + 1, textlen - cursor +1); \
}

#define _K_EDIT_BACKSPACE \
 if (cursor) { \
  --cursor;    \
  memmove(text + cursor, text + cursor + 1, textlen - cursor +1); \
  --textlen; \
}



void
kEditControl(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = UPARAM(0);
  heap_ptr event = UPARAM(1);

  CHECK_THIS_KERNEL_FUNCTION;

  if (obj) {
    word ct_type = GET_SELECTOR(obj, type);
    if (ct_type) {
      if ((ct_type == K_CONTROL_EDIT) && event && (GET_SELECTOR(event, type) == SCI_EVT_KEYBOARD)) {
	  int x = GET_SELECTOR(obj, nsLeft);
	  int y = GET_SELECTOR(obj, nsTop);
	  int xl = GET_SELECTOR(obj, nsRight) - x + 1;
	  int yl = GET_SELECTOR(obj, nsBottom) - y + 1;
	  int max = GET_SELECTOR(obj, max);
	  int cursor = GET_SELECTOR(obj, cursor);
	  int modifiers = GET_SELECTOR(event, modifiers);
	  byte key = GET_SELECTOR(event, message);

	  int font_nr = GET_SELECTOR(obj, font);
	  char *text = s->heap + UGET_SELECTOR(obj, text);
	  int textlen = strlen(text);

	  port_t *port = s->ports[s->view_port];

	  if (cursor > textlen)
	    cursor = textlen;

	  graph_fill_box_custom(s, x + port->xmin, y + port->ymin,
				xl, yl, port->bgcolor, -1, 0, 1); /* Clear input box background */

	  /*	  fprintf(stderr,"EditControl: mod=%04x, key=%04x, maxlen=%04x, cursor=%04x\n",
		  modifiers, key, max, cursor);*/

	  if (modifiers & SCI_EVM_CTRL) {

	    switch (tolower(key)) {
	    case 'a': cursor = 0; break;
	    case 'e': cursor = textlen; break;
	    case 'f': if (cursor < textlen) ++cursor; break;
	    case 'b': if (cursor > 1) --cursor; break;
	    case 'k': text[cursor] = 0; break; /* Terminate string */
	    case 'h': _K_EDIT_BACKSPACE; break;
	    case 'd': _K_EDIT_DELETE; break;
	    }
	    PUT_SELECTOR(event, claimed, 1);

	  } else if (modifiers & SCI_EVM_ALT) { /* Ctrl has precedence over Alt */

	    switch (tolower(key)) {
	    case 'f': while ((cursor < textlen) && (text[cursor++] != ' ')); break;
	    case 'b': while ((cursor > 0) && (text[--cursor - 1] != ' ')); break;
	    }
	    PUT_SELECTOR(event, claimed, 1);

          } 
          else if (key < 31) {

	    PUT_SELECTOR(event, claimed, 1);

	    switch(key) {
	    case SCI_K_BACKSPACE: _K_EDIT_BACKSPACE; break;
	    default:
	      PUT_SELECTOR(event, claimed, 0);
	    }

	  } 

          else if ((key >= SCI_K_HOME) && (key <= SCI_K_DELETE))
          {
            switch(key) {
	    case SCI_K_HOME: cursor = 0; break;
	    case SCI_K_END: cursor = textlen; break;
	    case SCI_K_RIGHT: if (cursor + 1 <= textlen) ++cursor; break;
	    case SCI_K_LEFT: if (cursor > 0) --cursor; break;
	    case SCI_K_DELETE: _K_EDIT_DELETE; break;
	    }
	    PUT_SELECTOR(event, claimed, 1);
          }
          
          else if ((key > 31) && (key < 128)) 
          {
            int inserting = modifiers & SCI_EVM_INSERT;
            
            if (modifiers & (SCI_EVM_RSHIFT | SCI_EVM_LSHIFT))
	      key = toupper(key);
            if (modifiers & SCI_EVM_CAPSLOCK)
              key = toupper(key);
            if (modifiers & ((SCI_EVM_RSHIFT | SCI_EVM_LSHIFT) & SCI_EVM_CAPSLOCK))
              key = tolower(key);
            modifiers &= ~(SCI_EVM_RSHIFT | SCI_EVM_LSHIFT | SCI_EVM_CAPSLOCK);
           
	    if (cursor == textlen) {
	      if (textlen < max) {
		text[cursor++] = key;
		text[cursor] = 0; /* Terminate string */
	      }
	    } else if (inserting) {
	      if (textlen < max) {
		int i;

		for (i = textlen + 2; i >= cursor; i--)
		  text[i] = text[i - 1];
		text[cursor++] = key;

	      }
	    } else { /* Overwriting */
	      text[cursor++] = key;
	    }

	    PUT_SELECTOR(event, claimed, 1);
	  }
          
          PUT_SELECTOR(obj, cursor, cursor); /* Write back cursor position */
      }

      if (event) PUT_SELECTOR(event, claimed, 1);
      _k_draw_control(s, obj);
    }
  }
}


void
_k_draw_control(state_t *s, heap_ptr obj)
{
  int x = GET_SELECTOR(obj, nsLeft);
  int y = GET_SELECTOR(obj, nsTop);
  int xl = GET_SELECTOR(obj, nsRight) - x + 1;
  int yl = GET_SELECTOR(obj, nsBottom) - y + 1;

  int font_nr = GET_SELECTOR(obj, font);
  char *text = s->heap + UGET_SELECTOR(obj, text);
  int view = GET_SELECTOR(obj, view);
  int cel = GET_SELECTOR(obj, cel);
  int loop = GET_SELECTOR(obj, loop);

  int type = GET_SELECTOR(obj, type);
  int state = GET_SELECTOR(obj, state);
  int cursor;

  resource_t *font_res;
  resource_t *view_res;

  switch (type) {

  case K_CONTROL_BUTTON:

    font_res  = findResource(sci_font, font_nr);
    if (!font_res) {
      SCIkwarn(SCIkERROR, "Font.%03d not found!\n", font_nr);
      return;
    }
    graph_draw_selector_button(s, s->ports[s->view_port], state, x, y, xl, yl, text, font_res->data);
    break;

  case K_CONTROL_TEXT:

    font_res  = findResource(sci_font, font_nr);
    if (!font_res) {
      SCIkwarn(SCIkERROR, "Font.%03d not found!\n", font_nr);
      return;
    }
    graph_draw_selector_text(s, s->ports[s->view_port], state, x, y, xl, yl, text, font_res->data,
			     (s->version < SCI_VERSION_FTU_CENTERED_TEXT_AS_DEFAULT)
			     ? ALIGN_TEXT_LEFT : ALIGN_TEXT_CENTER);
    break;

  case K_CONTROL_EDIT:

    font_res  = findResource(sci_font, font_nr);
    if (!font_res) {
      SCIkwarn(SCIkERROR, "Font.%03d not found!\n", font_nr);
      return;
    }

    cursor = GET_SELECTOR(obj, cursor);

    graph_draw_selector_edit(s, s->ports[s->view_port], state, x, y, xl, yl, cursor,
			     text, font_res->data);
    break;

  case K_CONTROL_ICON:

    view_res = findResource(sci_view, view);
    if (!view_res) {
      SCIkwarn(SCIkERROR, "View.%03d not found!\n", font_nr);
      return;
    }
    graph_draw_selector_icon(s, s->ports[s->view_port], state, x, y, xl, yl,
			     view_res->data, loop, cel);
    break;

  case K_CONTROL_CONTROL:

    graph_draw_selector_control(s, s->ports[s->view_port], state, x, y, xl, yl);
    break;

  case K_CONTROL_SELECT_BOX:

    break;

  default:
    SCIkwarn(SCIkWARNING, "Unknown control type: %d at %04x\n", type, obj);
  }

  if (!s->pic_not_valid)
    graph_update_port(s, s->ports[s->view_port]);
}

void
_k_dyn_view_list_prepare_change(state_t *s)
     /* Removes all views in anticipation of a new window or text */
{
  view_object_t *list = s->dyn_views;
  int list_nr = s->dyn_views_nr;
  int i;

  for (i = 0; i < list_nr; i++) 
    if (list[i].obj) {
    word signal = GET_HEAP(list[i].signalp);

    if (!(signal & _K_VIEW_SIG_FLAG_NO_UPDATE)) {

      if (list[i].underBitsp && !(signal & _K_VIEW_SIG_FLAG_DONT_RESTORE)) {
	int under_bits = GET_HEAP(list[i].underBitsp);

	if (under_bits) {

	  SCIkdebug(SCIkGRAPHICS, "Restoring BG for obj %04x with signal %04x\n", list[i].obj, signal);
	  graph_restore_box(s, under_bits);

	  PUT_HEAP(list[i].underBitsp, 0); /* Restore and mark as restored */
	}
      }
    } /* if NOT (signal & _K_VIEW_SIG_FLAG_NO_UPDATE) */
  } /* For each list member */
}

void
_k_restore_view_list_backgrounds(state_t *s, view_object_t *list, int list_nr)
     /* Restores the view backgrounds of list_nr members of list */
{
  int i;

  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
    word signal = GET_HEAP(list[i].signalp);
    SCIkdebug(SCIkGRAPHICS, "Trying to restore with signal = %04x\n", signal);

    if (signal & _K_VIEW_SIG_FLAG_NO_UPDATE) {

      if (signal & _K_VIEW_SIG_FLAG_UPDATE_ENDED)
	PUT_HEAP(list[i].signalp, (signal & ~_K_VIEW_SIG_FLAG_UPDATE_ENDED) |
		 _K_VIEW_SIG_FLAG_NO_UPDATE);

      continue; /* Don't restore this view */
    } else {

      if (list[i].underBitsp && !(signal & _K_VIEW_SIG_FLAG_DONT_RESTORE)) {
	int under_bits = GET_HEAP(list[i].underBitsp);

	if (under_bits) {

	  SCIkdebug(SCIkGRAPHICS, "Restoring BG for obj %04x with signal %04x\n", list[i].obj, signal);
	  graph_restore_box(s, under_bits);

	  PUT_HEAP(list[i].underBitsp, 0); /* Restore and mark as restored */
	}
      }
      signal &= ~_K_VIEW_SIG_FLAG_UNKNOWN_6;

      if (signal & _K_VIEW_SIG_FLAG_UPDATING)
	signal &= ~(_K_VIEW_SIG_FLAG_UPDATING |
		    _K_VIEW_SIG_FLAG_NO_UPDATE); /* Clear those two flags */

    } /* if NOT (signal & _K_VIEW_SIG_FLAG_NO_UPDATE) */
  } /* For each list member */
}

void
_k_view_list_free_backgrounds(state_t *s, view_object_t *list, int list_nr)
     /* Frees all backgrounds from the list without restoring; called by DrawPic */
{
  int i;

  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
    int handle = GET_HEAP(list[i].underBitsp);
    int signal = GET_HEAP(list[i].signalp);

    if (handle)
      kfree(s, handle);

    PUT_HEAP(list[i].underBitsp, 0);
  }
}

void
_k_save_view_list_backgrounds(state_t *s, view_object_t *list, int list_nr)
     /* Stores the view backgrounds of list_nr members of list in their underBits selectors */
{
  int i;

  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
    int handle;

    if (!(list[i].underBitsp))
      continue; /* No underbits? */

    if (GET_HEAP(list[i].underBitsp))
      continue; /* Don't overwrite an existing backup */

    SCIkdebug(SCIkGRAPHICS, "Saving BG for obj %04x with signal %04x\n",
	      list[i].obj, UGET_HEAP(list[i].signalp));
    handle = view0_backup_background(s, list[i].x, list[i].y,
				     list[i].loop, list[i].cel, list[i].view);

    PUT_HEAP(list[i].underBitsp, handle);
  }
}

void
_k_view_list_dispose_loop(state_t *s, view_object_t *list, int list_nr,
			  int funct_nr, int argc, int argp)
     /* disposes all list members flagged for disposal; funct_nr is the invoking kfunction */
{
  int i;

  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
    if (UGET_HEAP(list[i].signalp) & _K_VIEW_SIG_FLAG_DISPOSE_ME)
      if (invoke_selector(INV_SEL(list[i].obj, delete, 1), 0))
	SCIkwarn(SCIkWARNING, "Object at %04x requested deletion, but does not have"
		 " a delete funcselector\n", list[i].obj);
  }
}


void
_k_invoke_view_list(state_t *s, heap_ptr list, int funct_nr, int argc, int argp)
     /* Invokes all elements of a view list. funct_nr is the number of the calling funcion. */
{
  heap_ptr node = GET_HEAP(list + LIST_LAST_NODE);

  while (node) {
    heap_ptr obj = UGET_HEAP(node + LIST_NODE_VALUE); /* The object we're using */

    if (!(GET_SELECTOR(obj, signal) & _K_VIEW_SIG_FLAG_FROZEN))
      invoke_selector(INV_SEL(obj, doit, 1), 0); /* Call obj::doit() if neccessary */

    node = UGET_HEAP(node + LIST_PREVIOUS_NODE);
  }

}

view_object_t *
_k_make_view_list(state_t *s, heap_ptr list, int *list_nr, int cycle, int funct_nr, int argc, int argp)
     /* Creates a view_list from a node list in heap space. Returns the list, stores the
     ** number of list entries in *list_nr. Calls doit for each entry if cycle is set.
     ** argc, argp, funct_nr should be the same as in the calling kernel function.
     */
{
  heap_ptr node;
  view_object_t *retval;
  int i;

  if (cycle)
    _k_invoke_view_list(s, list, funct_nr, argc, argp); /* Invoke all objects bif requested */

  node = GET_HEAP(list + LIST_LAST_NODE);
  *list_nr = 0;

  SCIkdebug(SCIkGRAPHICS, "Making list from %04x\n", list);

  if (GET_HEAP(list - 2) != 0x6) { /* heap size check */
    SCIkwarn(SCIkWARNING, "Attempt to draw non-list at %04x\n", list);
    return NULL; /* Return an empty list */
  }

  while (node) {
    (*list_nr)++;
    node = UGET_HEAP(node + LIST_PREVIOUS_NODE);
  } /* Counting the nodes */

  if (!(*list_nr))
    return NULL; /* Empty */

  retval = malloc(*list_nr * (sizeof(view_object_t))); /* Allocate the list */

  node = UGET_HEAP(list + LIST_LAST_NODE); /* Start over */

  i = 0;
  while (node) {
    heap_ptr obj = GET_HEAP(node + LIST_NODE_VALUE); /* The object we're using */
    int view_nr = GET_SELECTOR(obj, view);
    resource_t *viewres = findResource(sci_view, view_nr);

    SCIkdebug(SCIkGRAPHICS, " - Adding %04x\n", obj);

    if (i == (*list_nr)) {
      (*list_nr)++;
      retval = realloc(retval, *list_nr * sizeof(view_object_t));
      /* This has been reported to happen */
    }

    retval[i].obj = obj;

    retval[i].x = GET_SELECTOR(obj, x);
    retval[i].y = GET_SELECTOR(obj, y) - GET_SELECTOR(obj, z);

    retval[i].nsLeft = GET_SELECTOR(obj, nsLeft);
    retval[i].nsRight = GET_SELECTOR(obj, nsRight);
    retval[i].nsTop = GET_SELECTOR(obj, nsTop);
    retval[i].nsBottom = GET_SELECTOR(obj, nsBottom);

    retval[i].loop = GET_SELECTOR(obj, loop);
    retval[i].cel = GET_SELECTOR(obj, cel);

    if (!viewres) {
      SCIkwarn(SCIkERROR, "Attempt to draw invalid view.%03d!\n", view_nr);
      retval[i].view = NULL;
      retval[i].view_nr = 0;
    } else {
      retval[i].view = viewres->data;
      retval[i].view_nr = view_nr;

      if ((retval[i].loop > view0_loop_count(viewres->data))) {
	  retval[i].loop = 0;
	  PUT_SELECTOR(obj, loop, 0);
      }

      if ((retval[i].cel > view0_cel_count(retval[i].loop, viewres->data))) {
	  retval[i].cel = 0;
	  PUT_SELECTOR(obj, cel, 0);
      }
    }

    if (lookup_selector(s, obj, s->selector_map.underBits, &(retval[i].underBitsp))
	!= SELECTOR_VARIABLE) {
      retval[i].underBitsp = 0;
      SCIkdebug(SCIkGRAPHICS, "Object at %04x has no underBits\n", obj);
    }

    if (lookup_selector(s, obj, s->selector_map.signal, &(retval[i].signalp))
	!= SELECTOR_VARIABLE) {
      retval[i].signalp = 0;
      SCIkdebug(SCIkGRAPHICS, "Object at %04x has no signal selector\n", obj);
    }

    if (!(UGET_HEAP(retval[i].signalp) & _K_VIEW_SIG_FLAG_FIX_PRI_ON)) { /* Calculate priority */
      int _priority, y = retval[i].y;
      _priority = VIEW_PRIORITY(y);

      PUT_SELECTOR(obj, priority, _priority);

      retval[i].priority = _priority;
    } else /* DON'T calculate the priority */
      retval[i].priority = GET_SELECTOR(obj, priority);

    s->pic_not_valid++; /* There ought to be some kind of check here... */

    i++; /* Next object in the list */
    node = UGET_HEAP(node + LIST_PREVIOUS_NODE); /* Next node */
  }

  return retval;
}


void
_k_view_list_dispose(state_t *s, view_object_t **list_ptr, int *list_nr_ptr)
     /* Unallocates all list element data, frees the list */
{
  int i;
  view_object_t *list = *list_ptr;

  if (!*list_nr_ptr)
    return; /* Nothing to do :-( */

  _k_view_list_free_backgrounds(s, list, *list_nr_ptr);

  free (list);
  *list_ptr = NULL;
  *list_nr_ptr = 0;
}

void
_k_draw_view_list(state_t *s, view_object_t *list, int list_nr, int use_signal)
     /* Draws list_nr members of list to s->pic. If use_signal is set, do some magic with the
     ** "signal" selector of the base object
     */
{
  int i;

  if (s->view_port != s->dyn_view_port)
    return; /* Return if the pictures are meant for a different port */

  for (i = 0; i < list_nr; i++) 
    if (list[i].obj) {
    word signal = (use_signal)? GET_HEAP(list[i].signalp) : 0;

    /* Now, if we either don't use signal OR if signal allows us to draw, do so: */
    if ((use_signal == 0) || (!(signal & _K_VIEW_SIG_FLAG_NO_UPDATE) &&
			      !(signal & _K_VIEW_SIG_FLAG_HIDDEN))) {
      SCIkdebug(SCIkGRAPHICS, "Drawing obj %04x with signal %04x\n", list[i].obj, signal);
      draw_view0(s->pic, s->ports[s->view_port],
		 list[i].x, list[i].y, list[i].priority, list[i].loop, list[i].cel,
		 GRAPHICS_VIEW_CENTER_BASE | GRAPHICS_VIEW_USE_ADJUSTMENT, list[i].view);
    }

    if (use_signal) {
      signal &= ~(_K_VIEW_SIG_FLAG_UPDATE_ENDED | _K_VIEW_SIG_FLAG_UPDATING |
		   _K_VIEW_SIG_FLAG_NO_UPDATE | _K_VIEW_SIG_FLAG_UNKNOWN_6);
      /* Clear all of those flags */

      PUT_HEAP(list[i].signalp, signal); /* Write the changes back */
    } else continue;

    if (signal & _K_VIEW_SIG_FLAG_IGNORE_ACTOR)
      continue; /* I assume that this is used for PicViews as well, so no use_signal check */
    else { /* Yep, the continue doesn't really make sense. It's for clarification. */
      int yl, y = list[i].nsTop;
      int priority_band_start = PRIORITY_BAND_FIRST(list[i].priority);
      /* Get start of priority band */

      if (priority_band_start > y)
	y = priority_band_start;

      yl = abs(y - list[i].nsBottom);

      fill_box(s->pic, list[i].nsLeft, y,
	       list[i].nsRight - list[i].nsLeft, yl, 0xf, 2);
      /* Fill the control map with solidity */

    } /* NOT Ignoring the actor */
  } /* for (i = 0; i < list_nr; i++) */

}

void
_k_dyn_view_list_accept_change(state_t *s)
     /* Restores all views after backupping their new bgs */
{
  view_object_t *list = s->dyn_views;
  int list_nr = s->dyn_views_nr;
  int i;

  int oldvp = s->view_port;

  if (s->view_port != s->dyn_view_port)
    return; /* Return if the pictures are meant for a different port */

  s->view_port = 0; /* WM Viewport */
  _k_save_view_list_backgrounds(s, list, list_nr);
  s->view_port = oldvp;

  for (i = 0; i < list_nr; i++)
    if (list[i].obj) {
    word signal = GET_HEAP(list[i].signalp);

    if (!(signal & _K_VIEW_SIG_FLAG_NO_UPDATE) && !(signal & _K_VIEW_SIG_FLAG_HIDDEN)) {
      SCIkdebug(SCIkGRAPHICS, "Drawing obj %04x with signal %04x\n", list[i].obj, signal);
      draw_view0(s->pic, &(s->wm_port),
		 list[i].x, list[i].y, list[i].priority, list[i].loop, list[i].cel,
		 GRAPHICS_VIEW_CENTER_BASE | GRAPHICS_VIEW_USE_ADJUSTMENT, list[i].view);
    }
  }

}


void
kAddToPic(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr list = PARAM(0);
  CHECK_THIS_KERNEL_FUNCTION;

  if (s->pic_views_nr)
    free(s->pic_views);

  SCIkdebug(SCIkGRAPHICS, "Preparing list...\n");
  s->pic_views = _k_make_view_list(s, list, &(s->pic_views_nr), 0, funct_nr, argc, argp);
  /* Store pic views for later re-use */

  SCIkdebug(SCIkGRAPHICS, "Drawing list...\n");
  _k_draw_view_list(s, s->pic_views, s->pic_views_nr, 0);
  /* Draw relative to the bottom center */
  SCIkdebug(SCIkGRAPHICS, "Returning.\n");
}


void
kGetPort(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = s->view_port;
}


void
kSetPort(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  unsigned int newport = PARAM(0);

  if ((newport >= MAX_PORTS) || (s->ports[newport] == NULL)) {
    SCIkwarn(SCIkERROR, "Invalid port %04x requested\n", newport);
    return;
  }

  s->view_port = newport;
}


void
kDrawCel(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *view = findResource(sci_view, PARAM(0));
  int loop = PARAM(1);
  int cel = PARAM(2);
  int x = PARAM(3);
  int y = PARAM(4);
  int priority = PARAM(5);

  if (priority < 0)
    priority = 16;

  CHECK_THIS_KERNEL_FUNCTION;

  if (!view) {
    SCIkwarn(SCIkERROR, "Attempt to draw non-existing view.%03n\n", PARAM(0));
    return;
  }

  draw_view0(s->pic, s->ports[s->view_port], x, y, priority, loop, cel, 0, view->data);
}



void
kDisposeWindow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  unsigned int goner = PARAM(0);

  if ((goner >= MAX_PORTS) || (goner < 3) ||(s->ports[goner] == NULL)) {
    SCIkwarn(SCIkERROR, "Removal of invalid window %04x requested\n", goner);
    return;
  }

  if (goner == s->view_port) /* Are we killing the active port? */
    s->view_port = 0; /* Set wm_port as active port if so */

  _k_dyn_view_list_prepare_change(s);
  graph_restore_box(s, s->ports[goner]->bg_handle);

  _k_dyn_view_list_accept_change(s);

  graph_update_port(s, s->ports[goner]);

  free(s->ports[goner]);
  s->ports[goner] = NULL; /* Mark as free */
}


void
kNewWindow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  unsigned int window = 3;
  port_t *wnd;
  int xlo, ylo;

  CHECK_THIS_KERNEL_FUNCTION;

  while ((window < MAX_PORTS) && (s->ports[window]))
    ++window;

  if (window == MAX_PORTS) {
    KERNEL_OOPS("Out of window/port handles in kNewWindow! Increase MAX_PORTS in engine.h\n");
    return;
  }

  wnd = calloc(sizeof(port_t), 1);

  wnd->ymin = PARAM(0) + 10;
  wnd->xmin = PARAM(1);
  wnd->ymax = PARAM(2) + 10; /*  +10 because of the menu bar- SCI scripts don't count it */
  wnd->xmax = PARAM(3) + 1;
  wnd->title = PARAM(4);
  wnd->flags = PARAM(5);
  wnd->priority = PARAM(6);
  wnd->color = PARAM_OR_ALT(7, 0);
  wnd->bgcolor = PARAM_OR_ALT(8, 15);
  wnd->font = s->titlebar_port.font; /* Default to 'system' font */
  wnd->font_nr = s->titlebar_port.font_nr;

  wnd->alignment = ALIGN_TEXT_LEFT; /* FIXME?? */

  if (wnd->priority == -1)
    wnd->priority = 16; /* Max priority + 1*/

  s->ports[window] = wnd;

  xlo = wnd->xmin;
  ylo = wnd->ymin - ((wnd->flags & WINDOW_FLAG_TITLE)? 10 : 0);
  /* Windows with a title bar get positioned in a way ignoring the title bar. */

  _k_dyn_view_list_prepare_change(s);


  wnd->bg_handle = graph_save_box(s, xlo, ylo, wnd->xmax - xlo + 1, wnd->ymax - ylo + 1, 3);

  draw_window(s->pic, s->ports[window], wnd->bgcolor, wnd->priority,
	     s->heap + wnd->title, s->titlebar_port.font , wnd->flags); /* Draw window */

  _k_dyn_view_list_accept_change(s);

  /* Now sanitize the port values */
  if (wnd->xmin < 0)
    wnd->xmin = 0;
  if (wnd->xmax > 319)
    wnd->xmax = 319;
  if (wnd->ymin < 10)
    wnd->ymin = 10;
  if (wnd->ymax > 199)
    wnd->ymax = 199;

  graph_update_port(s, wnd); /* Update viewscreen */

  s->view_port = window; /* Set active port */

  s->acc = window;
}

void
kDrawStatus(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr text = PARAM(0);

  if (text) {
    draw_titlebar(s->pic, 0xf);
    draw_text0(s->pic, &(s->titlebar_port), 1, 1,
	       ((char *)s->heap) + PARAM(0), s->titlebar_port.font, 0);
  } else
    draw_titlebar(s->pic, 0);

  graph_update_box(s, 0, 0, 320, 10);
}

void
kDrawMenuBar(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;

  if (!s->titlebar_port.font) {
    SCIkwarn(SCIkERROR, "No titlebar font is set: %d\n", s->titlebar_port.font_nr);
  }

  if (PARAM(0))
    menubar_draw(s->pic, &(s->titlebar_port) ,s->menubar, -1, s->titlebar_port.font);
  else
    draw_titlebar(s->pic, 0);

  graph_update_box(s, 0, 0, 320, 10);
}


#define K_ANIMATE_CENTER_OPEN_H  0 /* horizontally open from center */
#define K_ANIMATE_CENTER_OPEN_V  1 /* vertically open from center */
#define K_ANIMATE_RIGHT_OPEN     2 /* open from right */
#define K_ANIMATE_LEFT_OPEN      3 /* open from left */
#define K_ANIMATE_BOTTOM_OPEN    4 /* open from bottom */
#define K_ANIMATE_TOP_OPEN       5 /* open from top */
#define K_ANIMATE_BORDER_OPEN_F  6 /* open from edges to center */
#define K_ANIMATE_CENTER_OPEN_F  7 /* open from center to edges */
#define K_ANIMATE_OPEN_CHECKERS  8 /* open random checkboard */
#define K_ANIMATE_BORDER_CLOSE_H_CENTER_OPEN_H  9 /* horizontally close to center,reopen from center */
#define K_ANIMATE_BORDER_CLOSE_V_CENTER_OPEN_V 10 /* vertically close to center, reopen from center */
#define K_ANIMATE_LEFT_CLOSE_RIGHT_OPEN        11 /* close to right, reopen from right */
#define K_ANIMATE_RIGHT_CLOSE_LEFT_OPEN        12 /* close to left,  reopen from left */
#define K_ANIMATE_TOP_CLOSE_BOTTOM_OPEN        13 /* close to bottom, reopen from bottom */
#define K_ANIMATE_BOTTOM_CLOSE_TOP_OPEN        14 /* close to top, reopen from top */
#define K_ANIMATE_CENTER_CLOSE_F_BORDER_OPEN_F 15 /* close from center to edges,
						  ** reopen from edges to center */
#define K_ANIMATE_BORDER_CLOSE_F_CENTER_OPEN_F 16 /* close from edges to center, reopen from
						  ** center to edges */
#define K_ANIMATE_CLOSE_CHECKERS_OPEN_CHECKERS 17 /* close random checkboard, reopen */



void
kAnimate(state_t *s, int funct_nr, int argc, heap_ptr argp)
     /* Animations are supposed to take a maximum of s->animation_delay milliseconds. */
{
  int i, remaining_checkers;
  char checkers[32 * 19];
  heap_ptr cast_list = UPARAM_OR_ALT(0, 0);
  int cycle = UPARAM_OR_ALT(1, 0);
  int open_animation = 0;

  CHECK_THIS_KERNEL_FUNCTION;

  process_sound_events(s); /* Take care of incoming events (kAnimate is called semi-regularly) */

  if (s->dyn_views_nr) {
    free(s->dyn_views);
    s->dyn_views_nr = 0; /* No more dynamic views */
  }

  if (cast_list) {
    s->dyn_view_port = s->view_port; /* The list is valid for view_port */

    s->dyn_views = _k_make_view_list(s, cast_list, &(s->dyn_views_nr), cycle, funct_nr, argc, argp);
    /* Initialize pictures- Steps 3-9 in Lars' V 0.1 list */

    SCIkdebug(SCIkGRAPHICS, "Handling PicViews:\n");
    if ((s->pic_not_valid) && (s->pic_views_nr))
      _k_draw_view_list(s, s->pic_views, s->pic_views_nr, 0); /* Step 10 */
    SCIkdebug(SCIkGRAPHICS, "Handling Dyn:\n");

    _k_restore_view_list_backgrounds(s, s->dyn_views, s->dyn_views_nr);
    _k_save_view_list_backgrounds(s, s->dyn_views, s->dyn_views_nr);
    _k_draw_view_list(s, s->dyn_views, s->dyn_views_nr, 1); /* Step 11 */
    _k_view_list_dispose_loop(s, s->dyn_views, s->dyn_views_nr, funct_nr, argc, argp); /* Step 15 */
  } /* if (cast_list) */

  open_animation = (s->pic_is_new) && (s->pic_not_valid);

  if (open_animation) {

    SCIkdebug(SCIkGRAPHICS, "Animating pic opening type %x\n", s->pic_animate);

    switch(s->pic_animate) {
    case K_ANIMATE_BORDER_CLOSE_H_CENTER_OPEN_H :

      for (i = 0; i < 160; i++) {
	graph_clear_box(s, i, 10, 1, 190, 0);
	graph_clear_box(s, 319-i, 10, 1, 190, 0);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_CENTER_OPEN_H :

      for (i = 159; i >= 0; i--) {
	graph_update_box(s, i, 10, 1, 190);
	graph_update_box(s, 319-i, 10, 1, 190);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_BORDER_CLOSE_V_CENTER_OPEN_V :

      for (i = 0; i < 95; i++) {
	graph_clear_box(s, 0, i + 10, 320, 1, 0);
	graph_clear_box(s, 0, 199 - i, 320, 1, 0);
	(*s->gfx_driver->Wait)(s, 2 * s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_CENTER_OPEN_V :

      for (i = 94; i >= 0; i--) {
	graph_update_box(s, 0, i + 10, 320, 1);
	graph_update_box(s, 0, 199 - i, 320, 1);
	(*s->gfx_driver->Wait)(s, 2 * s->animation_delay);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_LEFT_CLOSE_RIGHT_OPEN :

      for(i = 0; i < 320; i++) {
	graph_clear_box(s, i, 10, 1, 190, 0);
	(*s->gfx_driver->Wait)(s, s->animation_delay / 2);
	process_sound_events(s);
      }

    case K_ANIMATE_RIGHT_OPEN :
      for(i = 319; i >= 0; i--) {
	graph_update_box(s, i, 10, 1, 190);
	(*s->gfx_driver->Wait)(s, s->animation_delay / 2);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_RIGHT_CLOSE_LEFT_OPEN :

      for(i = 319; i >= 0; i--) {
	graph_clear_box(s, i, 10, 1, 190, 0);
	(*s->gfx_driver->Wait)(s, s->animation_delay / 2);
	process_sound_events(s);
      }

    case K_ANIMATE_LEFT_OPEN :

      for(i = 0; i < 320; i++) {
	graph_update_box(s, i, 10, 1, 190);
	(*s->gfx_driver->Wait)(s, s->animation_delay / 2);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_TOP_CLOSE_BOTTOM_OPEN :

      for (i = 10; i < 200; i++) {
	graph_clear_box(s, 0, i, 320, 1, 0);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_BOTTOM_OPEN :

      for (i = 199; i >= 10; i--) {
	graph_update_box(s, 0, i, 320, 1);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_BOTTOM_CLOSE_TOP_OPEN :

      for (i = 199; i >= 10; i--) {
	graph_clear_box(s, 0, i, 320, 1, 0);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_TOP_OPEN :

      for (i = 10; i < 200; i++) {
	graph_update_box(s, 0, i, 320, 1);
	(*s->gfx_driver->Wait)(s, s->animation_delay);
	process_sound_events(s);
      }
      break;


    case K_ANIMATE_CENTER_CLOSE_F_BORDER_OPEN_F :

      for (i = 31; i >= 0; i--) {
	int height = i * 3;
	int width = i * 5;

	graph_clear_box(s, width, 10 + height, 5, 190 - 2*height, 0);
	graph_clear_box(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height, 0);

	graph_clear_box(s, width, 10 + height, 320 - 2*width, 3, 0);
	graph_clear_box(s, width, 200 - 3 - height, 320 - 2*width, 3, 0);

	(*s->gfx_driver->Wait)(s, 4 * s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_BORDER_OPEN_F :

      for (i = 0; i < 32; i++) {
	int height = i * 3;
	int width = i * 5;

	graph_update_box(s, width, 10 + height, 5, 190 - 2*height);
	graph_update_box(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height);

	graph_update_box(s, width, 10 + height, 320 - 2*width, 3);
	graph_update_box(s, width, 200 - 3 - height, 320 - 2*width, 3);

	(*s->gfx_driver->Wait)(s, 4 * s->animation_delay);
	process_sound_events(s);
      }

      break;


    case K_ANIMATE_BORDER_CLOSE_F_CENTER_OPEN_F :

      for (i = 0; i < 32; i++) {
	int height = i * 3;
	int width = i * 5;

	graph_clear_box(s, width, 10 + height, 5, 190 - 2*height, 0);
	graph_clear_box(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height, 0);

	graph_clear_box(s, width, 10 + height, 320 - 2*width, 3, 0);
	graph_clear_box(s, width, 200 - 3 - height, 320 - 2*width, 3, 0);

	(*s->gfx_driver->Wait)(s, 7 * s->animation_delay);
	process_sound_events(s);
      }

    case K_ANIMATE_CENTER_OPEN_F :

      for (i = 31; i >= 0; i--) {
	int height = i * 3;
	int width = i * 5;

	graph_update_box(s, width, 10 + height, 5, 190 - 2*height);
	graph_update_box(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height);

	graph_update_box(s, width, 10 + height, 320 - 2 * width, 3);
	graph_update_box(s, width, 200 - 3 - height, 320 - 2 * width, 3);

	(*s->gfx_driver->Wait)(s, 7 * s->animation_delay);
	process_sound_events(s);
      }

      break;


    case K_ANIMATE_CLOSE_CHECKERS_OPEN_CHECKERS :

      memset(checkers, 0, sizeof(checkers));
      remaining_checkers = 19 * 32;

      while (remaining_checkers) {
	int x, y, checker = 1 + (int) (1.0 * remaining_checkers*rand()/(RAND_MAX+1.0));
	i = -1;

	while (checker)
	  if (checkers[++i] == 0) --checker;
	checkers[i] = 1; /* Mark checker as used */

	x = i % 32;
	y = i / 32;

	graph_clear_box(s, x * 10, 10 + y * 10, 10, 10, 0);
        if (remaining_checkers & 1)
	  s->gfx_driver->Wait(s, s->animation_delay / 4);

	--remaining_checkers;
	process_sound_events(s);
      }

    case K_ANIMATE_OPEN_CHECKERS :

      memset(checkers, 0, sizeof(checkers));
      remaining_checkers = 19 * 32;

      while (remaining_checkers) {
	int x, y, checker = 1 + (int) (1.0 * remaining_checkers * rand()/(RAND_MAX+1.0));
	i = -1;

	while (checker)
	  if (checkers[++i] == 0) --checker;
	checkers[i] = 1; /* Mark checker as used */

	x = i % 32;
	y = i / 32;

	graph_update_box(s, x * 10, 10 + y * 10, 10, 10);

	if (remaining_checkers & 1)
	  s->gfx_driver->Wait(s, s->animation_delay / 4);

	--remaining_checkers;
	process_sound_events(s);
      }

      break;


    default:
      graph_update_box(s, 0, 10, 320, 190);
    }

    s->pic_is_new = 0;
    s->pic_not_valid = 0;
    return;

  } /* if ((cast_list == 0) && (!s->pic_not_valid)) */

  graph_update_box(s, 0, 10, 320, 190); /* Just update and return */

  s->pic_not_valid = 0;

}

void
kShakeScreen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int shakes = PARAM_OR_ALT(0, 1);
  int i;

  for (i = 0; i < shakes * 3; i++) {
    (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_ALL, 0, -10,0,0);
    (*s->gfx_driver->Wait)(s, 25);
    (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_ALL, 0, 10,0,0);
    (*s->gfx_driver->Wait)(s, 25);
  }
  (*s->gfx_driver->Redraw)(s, GRAPHICS_CALLBACK_REDRAW_ALL, 0,0,0,0);
}

#define K_DISPLAY_SET_COORDS 100
#define K_DISPLAY_SET_ALIGNMENT 101
#define K_DISPLAY_SET_COLOR 102
#define K_DISPLAY_SET_BGCOLOR 103
#define K_DISPLAY_SET_GRAYTEXT 104
#define K_DISPLAY_SET_FONT 105
#define K_DISPLAY_WIDTH 106
#define K_DISPLAY_SAVE_UNDER 107
#define K_DISPLAY_RESTORE_UNDER 108


void
kDisplay(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int argpt;
  int textp = UPARAM(0);
  int index = UPARAM(1);
  int width = -1;
  int temp;
  int save_under = 0;
  char *text;
  resource_t *font_resource;
  port_t *port = s->ports[s->view_port];
  port_t save;

  save=*port;
  port->alignment = ALIGN_TEXT_LEFT;

  CHECK_THIS_KERNEL_FUNCTION;


  text = _kernel_lookup_text(s, textp, index);

  if (textp < 1000)
    argpt = 2;
  else
    argpt = 1;

  while (argpt < argc) {

    switch(PARAM(argpt++)) {

    case K_DISPLAY_SET_COORDS:

      port->x = PARAM(argpt++);
      port->y = PARAM(argpt++) + 1; /* ? */
      SCIkdebug(SCIkGRAPHICS, "Display: set_coords(%d, %d)\n", port->x, port->y);
      break;

    case K_DISPLAY_SET_ALIGNMENT:

      port->alignment = PARAM(argpt++);
      SCIkdebug(SCIkGRAPHICS, "Display: set_align(%d)\n", port->alignment);
      break;

    case K_DISPLAY_SET_COLOR:

      port->color = PARAM(argpt++);
      SCIkdebug(SCIkGRAPHICS, "Display: set_color(%d)\n", port->color);
      break;

    case K_DISPLAY_SET_BGCOLOR:

      port->bgcolor = PARAM(argpt++);
      SCIkdebug(SCIkGRAPHICS, "Display: set_bg_color(%d)\n", port->bgcolor);
      break;

    case K_DISPLAY_SET_GRAYTEXT:

      port->gray_text = PARAM(argpt++);
      SCIkdebug(SCIkGRAPHICS, "Display: set_align(%d)\n", port->alignment);
      break;

    case K_DISPLAY_SET_FONT:

      font_resource = findResource(sci_font, temp = PARAM(argpt++));

      port->font_nr = temp;

      if (font_resource)
	port->font = font_resource->data;
      else
	port->font = NULL;

      SCIkdebug(SCIkGRAPHICS, "Display: set_font(\"font.%03d\")\n", PARAM(argpt - 1));
      break;

    case K_DISPLAY_WIDTH:

      width = PARAM(argpt);
      argpt++;
      SCIkdebug(SCIkGRAPHICS, "Display: set_width(%d)\n", width);
      break;

    case K_DISPLAY_SAVE_UNDER:

      save_under = 1;
      SCIkdebug(SCIkGRAPHICS, "Display: set_save_under()\n");
      break;

    case K_DISPLAY_RESTORE_UNDER:

      SCIkdebug(SCIkGRAPHICS, "Display: restore_under(%04x)\n", UPARAM(argpt));
      graph_restore_box(s, UPARAM(argpt));
      argpt++;
      return;

    default:
      SCIkdebug(SCIkGRAPHICS, "Unknown Display() command %x\n", PARAM(argpt-1));
      SCIkdebug(SCIkGRAPHICS, "Display: set_align(%d)\n", port->alignment);
      return;
    }
  }

  if (save_under) {    /* Backup */
    int _width, _height;

    get_text_size(text, port->font, width, &_width, &_height);
    _width = port->xmax - port->xmin; /* To avoid alignment calculations */

    s->acc = graph_save_box(s, port->x + port->xmin, port->y + port->ymin, _width, _height, 3);

    SCIkdebug(SCIkGRAPHICS, "Saving (%d, %d) size (%d, %d)\n",
	      port->x + port->xmin, port->y + port->ymin, _width, _height);

  }


  SCIkdebug(SCIkGRAPHICS, "Display: Commiting text '%s'\n", text);

  _k_dyn_view_list_prepare_change(s);

  /*  if (s->view_port)
      graph_fill_port(s, port, port->bgcolor); */ /* Only fill if we aren't writing to the main view */

  text_draw(s->pic, port, text, width);

  _k_dyn_view_list_accept_change(s);

  if (!s->pic_not_valid) /* Refresh if drawn to valid picture */
    graph_update_box(s, port->xmin, port->ymin,
		     port->xmax - port->xmin + 1, port->ymax - port->ymin + 1);

  *port=save;

}


void
kstub(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int i;

  SCIkdebug(SCIkSTUB, "Stub: %s[%x](", s->kernel_names[funct_nr], funct_nr);

  if (s->debug_mode & (1 << SCIkSTUB_NR)) {
    for (i = 0; i < argc; i++) {
      sciprintf("%04x", 0xffff & PARAM(i));
      if (i+1 < argc) sciprintf(", ");
    }
    sciprintf(")\n");
  }
}


struct {
  char *functname; /* String representation of the kernel function as in script.999 */
  kfunct *kernel_function; /* The actual function */
} kfunct_mappers[] = {
  {"Load", kLoad},
  {"UnLoad", kUnLoad},
  {"GameIsRestarting", kGameIsRestarting },
  {"NewList", kNewList },
  {"GetSaveDir", kGetSaveDir },
  {"GetCWD", kGetCWD },
  {"SetCursor", kSetCursor },
  {"FindKey", kFindKey },
  {"NewNode", kNewNode },
  {"AddToFront", kAddToFront },
  {"AddToEnd", kAddToEnd },
  {"Show", kShow },
  {"PicNotValid", kPicNotValid },
  {"Random", kRandom },
  {"Abs", kAbs },
  {"Sqrt", kSqrt },
  {"OnControl", kOnControl },
  {"HaveMouse", kHaveMouse },
  {"GetAngle", kGetAngle },
  {"GetDistance", kGetDistance },
  {"LastNode", kLastNode },
  {"FirstNode", kFirstNode },
  {"NextNode", kNextNode },
  {"PrevNode", kPrevNode },
  {"NodeValue", kNodeValue },
  {"Clone", kClone },
  {"DisposeClone", kDisposeClone },
  {"ScriptID", kScriptID },
  {"MemoryInfo", kMemoryInfo },
  {"DrawPic", kDrawPic },
  {"DisposeList", kDisposeList },
  {"DisposeScript", kDisposeScript },
  {"GetPort", kGetPort },
  {"SetPort", kSetPort },
  {"NewWindow", kNewWindow },
  {"DisposeWindow", kDisposeWindow },
  {"IsObject", kIsObject },
  {"Format", kFormat },
  {"DrawStatus", kDrawStatus },
  {"DrawMenuBar", kDrawMenuBar },
  {"AddMenu", kAddMenu },
  {"SetMenu", kSetMenu },
  {"AddToPic", kAddToPic },
  {"CelWide", kCelWide },
  {"CelHigh", kCelHigh },
  {"Display", kDisplay },
  {"Animate", kAnimate },
  {"GetTime", kGetTime },
  {"DeleteKey", kDeleteKey },
  {"StrLen", kStrLen },
  {"GetFarText", kGetFarText },
  {"StrEnd", kStrEnd },
  {"StrCat", kStrCat },
  {"StrCmp", kStrCmp },
  {"StrCpy", kStrCpy },
  {"StrAt", kStrAt },
  {"ReadNumber", kReadNumber },
  {"DrawControl", kDrawControl },
  {"NumCels", kNumCels },
  {"NumLoops", kNumLoops },
  {"TextSize", kTextSize },
  {"InitBresen", kInitBresen },
  {"DoBresen", kDoBresen },
  {"CanBeHere", kCanBeHere },
  {"DrawCel", kDrawCel },
  {"DirLoop", kDirLoop },
  {"CoordPri", kCoordPri },
  {"PriCoord", kPriCoord },
  {"ValidPath", kValidPath },
  {"RespondsTo", kRespondsTo },
  {"FOpen", kFOpen },
  {"FPuts", kFPuts },
  {"FGets", kFGets },
  {"FClose", kFClose },
  {"TimesSin", kTimesSin },
  {"SinMult", kTimesSin },
  {"TimesCos", kTimesCos },
  {"CosMult", kTimesCos },
  {"MapKeyToDir", kMapKeyToDir },
  {"GlobalToLocal", kGlobalToLocal },
  {"LocalToGlobal", kLocalToGlobal },
  {"Wait", kWait },
  {"CosDiv", kCosDiv },
  {"SinDiv", kSinDiv },
  {"BaseSetter", kBaseSetter },
  {"Parse", kParse },
  {"ShakeScreen", kShakeScreen },

  /* Experimental functions */
  {"Said", kSaid },
  {"EditControl", kEditControl },
  {"DoSound", kDoSound },
  {"Graph", kGraph },
  {"GetEvent", kGetEvent },
  {SCRIPT_UNKNOWN_FUNCTION_STRING, k_Unknown },
  {0,0} /* Terminator */
};


void
script_map_kernel(state_t *s)
{
  int functnr;
  int mapped = 0;

  s->kfunct_table = malloc(sizeof(kfunct *) * (s->kernel_names_nr + 1));

  for (functnr = 0; functnr < s->kernel_names_nr; functnr++) {
    int seeker, found = -1;

    for (seeker = 0; (found == -1) && kfunct_mappers[seeker].functname; seeker++)
      if (strcmp(kfunct_mappers[seeker].functname, s->kernel_names[functnr]) == 0) {
	found = seeker; /* Found a kernel function with the same name! */
	mapped++;
      }

    if (found == -1) {

      sciprintf("Warning: Kernel function %s[%x] unmapped\n", s->kernel_names[functnr], functnr);
      s->kfunct_table[functnr] = kstub;

    } else s->kfunct_table[functnr] = kfunct_mappers[found].kernel_function;

  } /* for all functions requesting to be mapped */

  sciprintf("Mapped %d of %d kernel functions.\n", mapped, s->kernel_names_nr);

}
