/***************************************************************************
 kernel.c Copyright (C) 1999 Christoph Reichenbach, Magnus Reftel


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


#ifndef PI
#define PI 3.14159265358979323846
#endif /* !PI */


#define SCI_KERNEL_DEBUG

#ifdef SCI_KERNEL_DEBUG

#ifdef __GNUC__
#define SCIkdebug(format, param...) \
        fprintf(stderr,"FSCI kernel (%s L%d): " format, __PRETTY_FUNCTION__, __LINE__, ## param);
#else /* !__GNUC__ */
#define SCIkdebug(format, param...) \
        fprintf(stderr,"FSCI kernel (L%d): " format, __LINE__, ## param);
#endif /* !__GNUC__ */

#else /* !SCI_KERNEL_DEBUG */

#define SCIkdebug(format, param...)

#endif /* !SCI_KERNEL_DEBUG */





#ifdef __GNUC__
#define SCIkwarn(format, param...) \
        fprintf(stderr,"FSCI kernel (%s L%d): Warning: " format, __PRETTY_FUNCTION__, \
	__LINE__, ## param);
#else /* !__GNUC__ */
#define SCIkwarn(format, param...) \
        fprintf(stderr,"FSCI kernel (L%d): Warning: " format, __LINE__, ## param);
#endif /* !__GNUC__ */



/******************** Resource Macros ********************/

#define RESOURCE_ID(type, number) (number) | ((type) << 11)
/* Returns the composite resource ID */

#define RESOURCE_NUMBER(resid) ((resid) & 0x7ff)

#define RESOURCE_TYPE(resid) ((resid) >> 11)


/******************** Kernel Oops ********************/

int
kernel_oops(state_t *s, char *reason)
{
  sciprintf("Kernel Oops: %s\n", reason);
  script_debug_flag = script_error_flag = 1;
  return 0;
}


/******************** Heap macros ********************/

#define GET_HEAP(address) ((((guint16)(address)) < 800)? \
kernel_oops(s, "Heap address space violation on read")  \
: getInt16(s->heap + ((guint16)(address))))
/* Reads a heap value if allowed */

#define PUT_HEAP(address, value) if (((guint16)(address)) < 800) \
kernel_oops(s, "Heap address space violation on write");        \
else { s->heap[(guint16) address] = (value) &0xff;               \
 s->heap[((guint16)address) + 1] = ((value) >> 8) & 0xff;}
/* Sets a heap value if allowed */


#define CHECK_THIS_KERNEL_FUNCTION {\
  int i;\
  sciprintf("Kernel CHECK: %s[%x](", s->kernel_names[funct_nr], funct_nr); \
  for (i = 0; i < argc; i++) { \
    sciprintf("%04x", 0xffff & PARAM(i)); \
    if (i+1 < argc) sciprintf(", "); \
  } \
  sciprintf(")\n"); \
} \


/*Here comes implementations of some kernel functions. I haven't found any
 *documentation on these, except for their names in vocab.999.
 */

#if 0

int kIsObject(state* s)
{
  int id=getInt16(s->heap->data+s->acc);

  if(id<=0x4000 && instance_map[id]->offset==s->acc) s->acc=1;
  else s->acc=0;

  s->sp-=4;
  return 0;
}

#endif /* false */


#define PARAM(x) ((gint16) getInt16(s->heap + argp + ((x)*2)))
#define UPARAM(x) ((guint16) getInt16(s->heap + argp + ((x)*2)))

#define PARAM_OR_ALT(x, alt) ((x < argc)? PARAM(x) : (alt))
#define UPARAM_OR_ALT(x, alt) ((x < argc)? UPARAM(x) : (alt))
/* Returns the parameter value or (alt) if not enough parameters were supplied */


/* Allocates a set amount of memory and returns a handle to it. */
int
kalloc(state_t *s, int space)
{
  int seeker = 0;

  while ((seeker < MAX_HUNK_BLOCKS) && (s->hunk[seeker].size))
    seeker++;

  if (seeker == MAX_HUNK_BLOCKS)
    kernel_oops(s, "Out of hunk handles! Try increasing MAX_HUNK_BLOCKS in engine.h");
  else
    s->hunk[seeker].data = malloc(s->hunk[seeker].size = space);

  return seeker | (sci_memory << 11);
}


/* Returns a pointer to the memory indicated by the specified handle */
inline byte *
kmem(state_t *s, int handle)
{
  if ((handle >> 11) != sci_memory) {
    sciprintf("Error: kmem() without a handle\n");
    return 0;
  }

  if ((handle < 0) || (handle >= MAX_HUNK_BLOCKS)) {
    sciprintf("Error: kmem() with invalid handle\n");
    return 0;
  }

  return s->hunk[handle & 0x7ff].data;
}

/* Frees the specified handle. Returns 0 on success, 1 otherwise. */
int
kfree(state_t *s, int handle)
{
  if ((handle >> 11) != sci_memory) {
    sciprintf("Error: Attempt to kfree() non-handle\n");
    return 1;
  }

  if ((handle < 0) || (handle >= MAX_HUNK_BLOCKS)) {
    sciprintf("Error: Attempt to kfree() with invalid handle\n");
    return 1;
  }

  if (s->hunk[handle].size == 0) {
    sciprintf("Error: Attempt to kfree() non-allocated memory\n");
    return 1;
  }

  free(s->hunk[handle].data);
  s->hunk[handle].size = 0;

  return 0;
}


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
      s->acc = kalloc(s, restype);

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
  heap_ptr old_offs = PARAM(0);
  heap_ptr new_offs;
  heap_ptr functareaptr;
  int selectors;
  int object_size;
  int i;

  if (GET_HEAP(old_offs + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    SCIkwarn("Attempt to clone non-object/class at %04x", old_offs);
    return;
  }

  if (GET_HEAP(old_offs + SCRIPT_INFO_OFFSET) != SCRIPT_INFO_CLASS) {
    SCIkwarn("Attempt to clone something other than a class template at %04x\n", old_offs);
    return;
  }

  selectors = GET_HEAP(old_offs + SCRIPT_SELECTORCTR_OFFSET);

  object_size = 8 + 4 + (selectors * 2);
  /* 8: Pre-selector area; 4: Function area (zero overloaded methods) */

  new_offs = heap_allocate(s->_heap, object_size);
  new_offs += 2; /* Step over heap header */

  memcpy(s->heap + new_offs, s->heap + old_offs + SCRIPT_OBJECT_MAGIC_OFFSET, object_size);
  new_offs -= SCRIPT_OBJECT_MAGIC_OFFSET; /* Center new_offs on the first selector value */

  PUT_HEAP(new_offs + SCRIPT_INFO_OFFSET, SCRIPT_INFO_CLONE); /* Set this to be a clone */

  functareaptr = selectors * 2; /* New function area */
  PUT_HEAP(new_offs + functareaptr, 0); /* No functions */
  PUT_HEAP(new_offs + SCRIPT_FUNCTAREAPTR_OFFSET, functareaptr);

  s->acc = new_offs; /* return the object's address */

  i = 0;
  while ((i < SCRIPT_MAX_CLONES) && (s->clone_list[i])) i++;

  if (i < SCRIPT_MAX_CLONES)
    s->clone_list[i] = new_offs; /* Log this clone */
  else SCIkwarn("Could not log clone at %04x\n", new_offs);
}


void
kDisposeClone(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr offset = PARAM(0);
  int i;

  if (GET_HEAP(offset + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    SCIkwarn("Attempt to dispose non-class/object at %04x\n", offset);
    return;
  }

  if (GET_HEAP(offset + SCRIPT_INFO_OFFSET) != SCRIPT_INFO_CLONE) {
    /*  SCIkwarn("Attempt to dispose something other than a clone at %04x\n", offset); */
    /* SCI silently ignores this behaviour; some games actually depend on this */
    return;
  }

  i = 0;
  while ((i < SCRIPT_MAX_CLONES) && (s->clone_list[i] != offset)) i++;
  if (i < SCRIPT_MAX_CLONES)
    s->clone_list[i] = 0; /* un-log clone */
  else SCIkwarn("Could not remove log entry from clone at %04x\n", offset);

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
    index = 1;

  if (s->scripttable[script].heappos == 0)
    script_instantiate(s, script); /* Instantiate script if neccessary */

  disp = s->scripttable[script].export_table_offset;

  if (!disp) {
    SCIkwarn("Script 0x%x does not have a dispatch table\n", script);
    return;
  }

  disp_size = GET_HEAP(disp);

  if (index > disp_size) {
    SCIkwarn("Dispatch index too big: %d > %d\n", index, disp_size);
    return;
  }

  s->acc = GET_HEAP(disp + index*2) + s->scripttable[script].heappos;
}


void
kDisposeScript(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  script_uninstantiate(s, PARAM(0)); /* Does its own sanity checking */
}


void
kIsObject(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr offset = PARAM(0);

  if (offset < 100)
    s->acc = 0;
  else
    s->acc = (GET_HEAP(offset + SCRIPT_OBJECT_MAGIC_OFFSET) == SCRIPT_OBJECT_MAGIC_NUMBER);
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
    kernel_oops(s, "Out of memory while creating a list");
    return;
  }

  listbase += 2; /* Jump over heap header */

  PUT_HEAP(listbase + LIST_FIRST_NODE, 0); /* No first node */
  PUT_HEAP(listbase + LIST_LAST_NODE, 0); /* No last node */

  s->acc = listbase; /* Return list base address */
}


void
kNewNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr nodebase = heap_allocate(s->_heap, 8);
  /*  SCIkdebug("argc=%d; args = (%04x %04x)\n", argc, PARAM(0), PARAM(1)); */

  if (!nodebase) {
    kernel_oops(s, "Out of memory while creating a node");
    return;
  }

  nodebase += 2; /* Jump over heap header */

  PUT_HEAP(nodebase + LIST_PREVIOUS_NODE, 0);
  PUT_HEAP(nodebase + LIST_NEXT_NODE, 0);
  PUT_HEAP(nodebase + LIST_NODE_KEY, PARAM(0));
  PUT_HEAP(nodebase + LIST_NODE_VALUE, PARAM(1));

  s->acc = nodebase; /* Return node base address */
}


void
kAddToEnd(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr listbase = UPARAM(0);
  heap_ptr nodebase = UPARAM(1);
  heap_ptr old_lastnode = GET_HEAP(listbase + LIST_LAST_NODE);

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

  CHECK_THIS_KERNEL_FUNCTION;

  node = GET_HEAP(UPARAM(0) + LIST_FIRST_NODE);

  while (node && (GET_HEAP(node + LIST_NODE_KEY) != key))
    node = GET_HEAP(node + LIST_NEXT_NODE);
  /* Aborts if either the list ends (node == 0) or the key is found */

  s->acc = node;
}


void
kFirstNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = GET_HEAP(UPARAM(0) + LIST_FIRST_NODE);
}


void
kLastNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = GET_HEAP(UPARAM(0) + LIST_LAST_NODE);
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

  if (GET_HEAP(address) != 6) {
    SCIkwarn("Attempt to dispose non-list at %04x\n", address);
  } else heap_free(s->_heap, address);

}

/*************************************************************/

void
kMemoryInfo(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  switch (PARAM(0)) {
  case 0: s->acc = heap_meminfo(s->_heap); break;
  case 1: s->acc = heap_largest(s->_heap); break;
  default: SCIkwarn("Unknown MemoryInfo operation: %04x\n", PARAM(0));
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
    exit(1); /* If we get here, something bad is happening */
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
    if (mkdir(FREESCI_GAMEDIR, 0700)) {

      fprintf(stderr,"Warning: Could not enter ~/"FREESCI_GAMEDIR"; save files"
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
    if (mkdir(s->game_name, 0700)) {

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
    byte *data = findResource(sci_cursor, PARAM(0))->data;
    if (data)
      s->mouse_pointer = calc_mouse_cursor(data);
    else s->mouse_pointer = NULL;

  } else
    s->mouse_pointer = NULL;

  if (argc > 2) {
    s->pointer_x = PARAM(2) + s->ports[s->view_port]->xmin;
    s->pointer_y = PARAM(3) + s->ports[s->view_port]->ymin; /* Port-relative */
  } 

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0,0,0,0); /* Update mouse pointer */
}


void
kShow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->pic_not_valid = 2;
}


void
kPicNotValid(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
    s->acc = s->pic_not_valid;
    if (argc)
	s->pic_not_valid = PARAM(0);
}


void
kRandom(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = PARAM(0) + (int) ((PARAM(1) + 1 - PARAM(0)) * rand() / (RAND_MAX + 1.0));
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
kDoSound(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;
  sciprintf("kDoSound: Stub\n");

  switch (PARAM(0)) {
  case 0x0: s->sound_object = PARAM(1); break;
    /* Initialize and set sound object. Stores a heap_ptr to some special data in the sound
    ** object's "handle" varselector.  */
  case 0x8: s->acc = 0xc; break; /* Determine sound type? */
  case 0xb: s->acc = 1; break; /* Check for sound? */
  }
}

void
kGraph(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;
  sciprintf("kGraph: Stub\n");

  switch(PARAM(0)) {
  case 0x2:
    s->acc = (sci_version < SCI_VERSION_1) ? 0x10 : 0x100; /* number of colors */
  }
}

void
k_Unknown(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  switch (funct_nr) {
  case 0x70: kGraph(s, funct_nr, argc, argp); break;
  default: {
    CHECK_THIS_KERNEL_FUNCTION;
    sciprintf("Unhandled Unknown function %04x\n", funct_nr);
  }
  }
}

void
kGetEvent(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;
  sciprintf("kGetEvent: Stub\n");

  s->acc = 0; /* No event */
}


/* Format(targ_address, textresnr, index_inside_res, ...)
** Formats the text from text.textresnr (offset index_inside_res) according to
** the supplied parameters and writes it to the targ_address.
*/
void
kFormat(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int *arguments;
  char *target = ((char *) s->heap) + PARAM(0);
  int resource_nr = PARAM(1);
  int index = UPARAM(2);
  resource_t *resource = findResource(sci_text, resource_nr);
  char *source;
  int mode = 0;
  int paramindex = 0; /* Next parameter to evaluate */
  char xfer;
  int i;


  if (!resource) {
    SCIkwarn("Could not find text.%03d\n", resource_nr);
    return;
  }

  arguments = malloc(sizeof(int) * argc);
  for (i = 3; i < argc; i++)
    arguments[i-3] = UPARAM(i); /* Parameters are copied to prevent overwriting */ 

  source = resource->data + index;

  while (xfer = *source++) {
    if (xfer == '%') {
      if (mode == 1)
	*target++ = '%'; /* Literal % by using "%%" */
      else mode = 1;
    }
    else { /* xfer != '%' */
      if (mode == 1) {
	switch (xfer) {
	case 's': { /* Copy string */
	  char *tempsource = ((char *) s->heap) + arguments[paramindex++];
	  while (*target++ = *tempsource++);
	}
	break;
	case 'd': { /* Copy decimal */
	  target += sprintf(target, "%d", arguments[paramindex++]);
	  paramindex++;
	}
	break;
	default:
	  *target = '%';
	  *target++;
	  *target = xfer;
	  *target++;
	}
	mode = 0;
      } else { /* mode != 1 */
	*target = xfer;
	*target++;
      }
    }
  }

  free(arguments);

  *target = 0; /* Terminate string */
}

void
kAddMenu(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  menubar_add_menu(s->menubar, s->heap + UPARAM(0), s->heap + UPARAM(1), s->title_font);
}


void
kSetMenu(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int index = UPARAM(0);

  menubar_set_foobar(s->menubar, (index >> 8) - 1, (index & 0xff) - 1, PARAM(1), UPARAM(2));
}



/********************* Graphics ********************/

void
kCelHigh(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *viewres = findResource(sci_view, PARAM(0));
  int result;

  if (!viewres) {
    SCIkwarn("view.%d (0x%x) not found\n", PARAM(0), PARAM(0));
    return;
  }

  s->acc = result = view0_cel_height(PARAM(1), PARAM(2), viewres->data);
  if (result < 0)
    SCIkwarn("Invalid loop (%d) or cel (%d) in view.%d (0x%x)\n", PARAM(1), PARAM(2), PARAM(0));
}

void
kCelWide(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *viewres = findResource(sci_view, PARAM(0));
  int result;

  if (!viewres) {
    SCIkwarn("view.%d (0x%x) not found\n", PARAM(0), PARAM(0));
    return;
  }

  s->acc = result = view0_cel_width(PARAM(1), PARAM(2), viewres->data);
  if (result < 0)
    SCIkwarn("Invalid loop (%d) or cel (%d) in view.%d (0x%x)\n", PARAM(1), PARAM(2), PARAM(0));
}




void
kOnControl(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int map;
  int xstart = PARAM(2);
  int ystart = PARAM(1);
  int xlen = 1, ylen = 1;

  int retval = 0;

  if (argc > 3) {
    xlen = PARAM(4) - xstart + 1;
    ylen = PARAM(3) - ystart + 1;
  }

  for (map = 0; map < 3; map++)
    if (PARAM(0) && (1 << map)) {
      int startindex = (SCI_SCREEN_WIDTH * ystart) + xstart;
      int i;

      while (ylen--) {
	for (i = 0; i < xlen; i++)
	  retval |= (1 << (s->bgpic[map][startindex + i]));

	startindex += SCI_SCREEN_WIDTH;
      }
    }

  s->acc = (word) retval;

}


void
kDrawPic(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *resource = findResource(sci_pic, PARAM(0));

  if (resource) {
    clearPicture(s->bgpic, 15);
    drawPicture0(s->bgpic, PARAM(2), PARAM(3), resource->data);
  } else
    SCIkwarn("Request to draw non-existing pic.%03d", PARAM(0));
}


void
draw_object(state_t *s, heap_ptr object)
{
  int x, y, view, loop, cel, priority;
  resource_t *viewres;
  int i;
  struct {
    int *target;
    int selector;
  } value_getters[] = {
    {&x, s->selector_map.x},
    {&y, s->selector_map.y},
    {&view, s->selector_map.view},
    {&loop, s->selector_map.loop},
    {&cel, s->selector_map.cel},
    {&priority, s->selector_map.priority}
  };

  for (i = 0; i < 6; i++) {
    heap_ptr address;

    if (lookup_selector(s, object, value_getters[i].selector, &address) != SELECTOR_VARIABLE) {
      SCIkwarn("Attempt to get varselector from 0x%04x failed\n", object);
      return;
    }

    *(value_getters[i].target) = GET_HEAP(address);
  }

  viewres = findResource(sci_view, view);

  if (!viewres) {
    SCIkwarn("Resource view.%03d (0x%x) not found for drawing!\n", view, view);
    return;
  }

  priority = 0x22;

  SCIkdebug("Drawing view %d, loop %d, cel %d to %d, %d, priority %d, at port #%d\n", view, loop, cel, x, y, priority, s->view_port);

  drawView0(s->bgpic, s->ports[s->view_port], x, y, priority, loop, cel, viewres->data);
}

void
kAddToPic(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr list = PARAM(0);
  heap_ptr node = GET_HEAP(list + LIST_FIRST_NODE);
  CHECK_THIS_KERNEL_FUNCTION;

  if (GET_HEAP(list - 2) != 0x6) { /* heap size check */
    SCIkwarn("Attempt to draw non-list at %04x\n", list);
    return;
  }

  while (node) {
    draw_object(s, GET_HEAP(node + LIST_NODE_VALUE));
    node = GET_HEAP(node + LIST_NEXT_NODE);
  }
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
    SCIkwarn("Invalid port %04x requested\n", newport);
    return;
  }

  s->view_port = newport;
}


void
kDisposeWindow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  unsigned int goner = PARAM(0);

  if ((goner >= MAX_PORTS) || (goner < 3) ||(s->ports[goner] == NULL)) {
    SCIkwarn("Removal of invalid window %04x requested\n", goner);
    return;
  }

  free(s->ports[goner]);
  s->ports[goner] = NULL; /* Mark as free */
}


void
kNewWindow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  unsigned int window = 3;
  window_t *wnd;

  while ((window < MAX_PORTS) && (s->ports[window]))
    ++window;

  if (window == MAX_PORTS) {
    kernel_oops(s, "Out of window/port handles in kNewWindow! Increase MAX_PORTS in engine.h\n");
    return;
  }

  wnd = malloc(sizeof(window_t));

  wnd->ymin = PARAM(0);
  wnd->xmin = PARAM(1);
  wnd->ymax = PARAM(2);
  wnd->xmax = PARAM(3);
  wnd->title = PARAM(4);
  wnd->flags = PARAM(5);
  wnd->priority = PARAM(6);
  wnd->color = PARAM(7);
  wnd->bgcolor = PARAM(8);

  s->ports[window] = (port_t *) wnd; /* Typecast to a port; it only carries some extra information */

  drawWindow(s->pic, s->ports[window], wnd->bgcolor, wnd->priority,
	     s->heap + wnd->title, s->title_font , wnd->flags); /* Draw window */

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_BOX,
		       wnd->xmin - 1, wnd->ymin - (wnd->flags & WINDOW_FLAG_TITLE)? 11 : 1,
		       wnd->xmax + 2, wnd->ymin + 2); /* Update screen */

  s->view_port = window; /* Set active port */

  s->acc = window;
}

void
kDrawStatus(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  draw_titlebar(s->pic, 0xf);
  drawText0(s->pic, &(s->titlebar_port), 1, 1, ((char *)s->heap) + PARAM(0), s->title_font, 0);

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_BOX,
		       0, 0, 319, 9);
}

void
kDrawMenuBar(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;

  if (PARAM(1))
    menubar_draw(s->pic, &(s->titlebar_port) ,s->menubar, -1, s->title_font);
  else
    draw_titlebar(s->pic, 0);

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_BOX,
		       0, 0, 319, 9);
}


void
kAnimate(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;

  memcpy(s->pic[0] + 320 * 10, s->bgpic[0] + 320 * 10, 320 * 190);

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_ALL,
		       0, 0, 0, 0);
}


void
kstub(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int i;

  sciprintf("Stub: %s[%x](", s->kernel_names[funct_nr], funct_nr);

  for (i = 0; i < argc; i++) {
    sciprintf("%04x", 0xffff & PARAM(i));
    if (i+1 < argc) sciprintf(", ");
  }
  sciprintf(")\n");
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
  {"DrawStatus", kDrawStatus }, /* <= last published */
  {"DrawMenuBar", kDrawMenuBar },
  {"AddMenu", kAddMenu },
  {"SetMenu", kSetMenu },
  {"AddToPic", kAddToPic },
  {"CelWide", kCelWide },
  {"CelHigh", kCelHigh },

  /* Experimental functions */
  {"Animate", kAnimate },
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
