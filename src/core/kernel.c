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
/*#define SCI_KERNEL_NODES_DEBUG*/

#ifdef SCI_KERNEL_DEBUG

#ifdef __GNUC__
#define SCIkdebug(format, param...) \
        fprintf(stderr,"FSCI kernel (%s L%d): " format, __PRETTY_FUNCTION__, __LINE__, ## param)
#else /* !__GNUC__ */
#define SCIkdebug(format, param...) \
        fprintf(stderr,"FSCI kernel (L%d): " format, __LINE__, ## param)
#endif /* !__GNUC__ */

#else /* !SCI_KERNEL_DEBUG */

#define SCIkdebug(format, param...)

#endif /* !SCI_KERNEL_DEBUG */





#ifdef __GNUC__
#define SCIkwarn(format, param...) \
        fprintf(stderr,"FSCI kernel (%s L%d): Warning: " format, __PRETTY_FUNCTION__, \
	__LINE__, ## param)
#else /* !__GNUC__ */
#define SCIkwarn(format, param...) \
        fprintf(stderr,"FSCI kernel (L%d): Warning: " format, __LINE__, ## param)
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

#define UGET_HEAP(address) ((((guint16)(address)) < 800)? \
kernel_oops(s, "Heap address space violation on read")  \
: getUInt16(s->heap + ((guint16)(address))))
/* Reads a heap value if allowed */

#define PUT_HEAP(address, value) { if (((guint16)(address)) < 800) \
kernel_oops(s, "Heap address space violation on write");        \
else { s->heap[(guint16) address] = (value) &0xff;               \
 s->heap[((guint16)address) + 1] = ((value) >> 8) & 0xff;}}
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

  if (lookup_selector(s, object, selector_id, &address) != SELECTOR_VARIABLE)
    SCIkwarn("Selector '%s' of object at %04x could not be written to\n",
	     s->selector_names[selector_id], object);
  else
    PUT_HEAP(address, value);

  sciprintf("Selector '%s' is at %04x\n", s->selector_names[selector_id], address);
}

#define GET_SELECTOR(_object_, _selector_) read_selector(s, _object_, s->selector_map._selector_)
#define PUT_SELECTOR(_object_, _selector_, _value_)\
 write_selector(s, _object_, s->selector_map._selector_, _value_)

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

  return (seeker | (sci_memory << 11));
}


/* Returns a pointer to the memory indicated by the specified handle */
inline byte *
kmem(state_t *s, int handle)
{
  if ((handle >> 11) != sci_memory) {
    sciprintf("Error: kmem() without a handle\n");
    return 0;
  }

  handle &= 0x7ff;

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
  word species;
  int selectors;
  int object_size;
  int i;

  if (GET_HEAP(old_offs + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    SCIkwarn("Attempt to clone non-object/class at %04x failed", old_offs);
    return;
  }

  if (GET_HEAP(old_offs + SCRIPT_INFO_OFFSET) != SCRIPT_INFO_CLASS) {
    SCIkwarn("Attempt to clone something other than a class template at %04x\n", old_offs);
    if (sci_version < SCI_VERSION_01)
      return;
    SCIkwarn("Allowing clone process\n",0);
  }

  selectors = GET_HEAP(old_offs + SCRIPT_SELECTORCTR_OFFSET);

  object_size = 8 + 2 + (selectors * 2);
  /* 8: Pre-selector area; 2: Function area (zero overloaded methods) */

  new_offs = heap_allocate(s->_heap, object_size);
  new_offs += 2; /* Step over heap header */

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
#ifdef SCI_KERNEL_NODES_DEBUG
  CHECK_THIS_KERNEL_FUNCTION;
#endif /* SCI_KERNEL_NODES_DEBUG */

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
#ifdef SCI_KERNEL_NODES_DEBUG
  CHECK_THIS_KERNEL_FUNCTION;
#endif /* SCI_KERNEL_NODES_DEBUG */

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
#ifdef SCI_KERNEL_NODES_DEBUG
  CHECK_THIS_KERNEL_FUNCTION;
#endif /* SCI_KERNEL_NODES_DEBUG */

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
#ifdef SCI_KERNEL_NODES_DEBUG
  CHECK_THIS_KERNEL_FUNCTION;
#endif /* SCI_KERNEL_NODES_DEBUG */

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
#ifdef SCI_KERNEL_NODES_DEBUG
  CHECK_THIS_KERNEL_FUNCTION;
#endif /* SCI_KERNEL_NODES_DEBUG */

  node = GET_HEAP(UPARAM(0) + LIST_FIRST_NODE);

  while (node && (GET_HEAP(node + LIST_NODE_KEY) != key))
    node = GET_HEAP(node + LIST_NEXT_NODE);
  /* Aborts if either the list ends (node == 0) or the key is found */

  s->acc = node;
}


void
kDeleteKey(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr list = UPARAM(0);
  heap_ptr node;
  word key = UPARAM(1);
#ifdef SCI_KERNEL_NODES_DEBUG
  CHECK_THIS_KERNEL_FUNCTION;
#endif /* SCI_KERNEL_NODES_DEBUG */

  node = UGET_HEAP(list + LIST_FIRST_NODE);

  while (node && ((guint16) UGET_HEAP(node + LIST_NODE_KEY) != key))
    node = GET_HEAP(node + LIST_NEXT_NODE);
  /* Aborts if either the list ends (node == 0) or the key is found */

  if (node) {
    heap_ptr prev_node = UGET_HEAP(node + LIST_PREVIOUS_NODE);
    heap_ptr next_node = UGET_HEAP(node + LIST_NEXT_NODE);

    if (UGET_HEAP(list + LIST_FIRST_NODE) == node)
      PUT_HEAP(list + LIST_FIRST_NODE, next_node);
    if (UGET_HEAP(list + LIST_LAST_NODE) == node)
      PUT_HEAP(list + LIST_LAST_NODE, prev_node);

    if (next_node)
      PUT_HEAP(next_node + LIST_PREVIOUS_NODE, prev_node);
    if (prev_node)
      PUT_HEAP(prev_node + LIST_NEXT_NODE, next_node);

    heap_free(s->_heap, node - 2);

  } else
    SCIkwarn("DeleteKey %04x failed on %04x\n", key, list);
    
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
#ifdef SCI_KERNEL_NODES_DEBUG
  CHECK_THIS_KERNEL_FUNCTION;
#endif /* SCI_KERNEL_NODES_DEBUG */

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

  switch(PARAM(0)) {

  case K_GRAPH_GET_COLORS_NR:

    s->acc = (sci_version < SCI_VERSION_1) ? 0x10 : 0x100; /* number of colors */
    break;

  case K_GRAPH_DRAW_LINE:

    color = PARAM(5);
    priority = PARAM(6);
    special = PARAM(7);
    dither_line(s->bgpic, PARAM(2), PARAM(1), PARAM(4), PARAM(3),
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
    break;

  case K_GRAPH_FILL_BOX_FOREGROUND:

    graph_clear_box(s, port->xmin, port->ymin,
		    port->xmax - port->xmin + 1, port->ymax - port->ymin + 1,
		    port->color);
    break;

  case K_GRAPH_FILL_BOX_ANY:
    CHECK_THIS_KERNEL_FUNCTION;
    SCIkwarn("KERNEL_GRAPH_FILL_BOX_ANY: stub\n",0);
    break;

  case K_GRAPH_UPDATE_BOX:
    CHECK_THIS_KERNEL_FUNCTION;
    SCIkwarn("KERNEL_GRAPH_UPDATE_BOX: stub\n",0);
    break;

  case K_GRAPH_REDRAW_BOX:
    CHECK_THIS_KERNEL_FUNCTION;
    SCIkwarn("KERNEL_GRAPH_REDRAW_BOX: stub\n",0);
    break;

  case K_GRAPH_ADJUST_PRIORITY:

    s->priority_first = PARAM(1);
    s->priority_last = PARAM(2);
    break;

  default:

    CHECK_THIS_KERNEL_FUNCTION;
    sciprintf("Unhandled Graph() operation %04x\n", PARAM(0));

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

void
kStrEnd(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr address = PARAM(0);
  char *seeker = s->heap + address;

  while (*seeker++)
    ++address;

  s->acc = address + 1; /* End of string */
}

void
kStrCat(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;

  strcat(s->heap + PARAM(0), s->heap + PARAM(1));
}

void
kStrCmp(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int length;

  if (argc > 2)
    s->acc = strncmp(s->heap + PARAM(0), s->heap + PARAM(1), PARAM(2));
  else
    s->acc = strcmp(s->heap + PARAM(0), s->heap + PARAM(1));
}


void
kStrCpy(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int length;

  if (argc > 2)
    strncpy(s->heap + (s->acc = PARAM(0)), s->heap + PARAM(1), PARAM(2));
  else
    strcpy(s->heap + (s->acc = PARAM(0)), s->heap + PARAM(1));

}


void
kStrAt(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr address = PARAM(0) + PARAM(1);

  s->acc = UGET_HEAP(address);

  if (argc > 2)
    PUT_HEAP(address, PARAM(2)); /* Request to modify this char */
}


void
kReadNumber(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *source = s->heap + PARAM(0);

  while (isspace(*source))
    source++; /* Skip whitespace */

  if (*source = '$') /* SCI uses this for hex numbers */
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
  int dest = PARAM(0);
  char *target = ((char *) s->heap) + dest;
  int position = PARAM(1);
  int index = UPARAM(2);
  resource_t *resource;
  char *source;
  int mode = 0;
  int paramindex = 0; /* Next parameter to evaluate */
  char xfer;
  int i;
  int startarg;


  if (position < 1000) { /* reading from a text resource */
    resource = findResource(sci_text, position);

    if (!resource) {
      SCIkwarn("Could not find text.%03d\n", position);
      return;
    }

    source = resource->data + index;
    startarg = 3; /* First parameter to use for formatting */

  } else  { /* position >= 1000 */
    source = s->heap;
    startarg = 2;
  }

  arguments = malloc(sizeof(int) * argc);
  for (i = startarg; i < argc; i++)
    arguments[i-3] = UPARAM(i); /* Parameters are copied to prevent overwriting */ 


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

  s->acc = dest; /* Return target addr */
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
  struct timeval time_prec;
  time_t the_time;

  if (argc) { /* Get seconds since last am/pm switch */
    the_time = time(NULL);
    loc_time = localtime(&the_time);
    s->acc = loc_time->tm_sec + loc_time->tm_min * 60 + (loc_time->tm_hour % 12) * 3600;
  } else { /* Get time since game started */
    gettimeofday(&time_prec, NULL);
    s-> acc = ((time_prec.tv_usec - s->game_start_time.tv_usec) * 60 / 1000000) +
      (time_prec.tv_sec - s->game_start_time.tv_sec) * 60;
  }
}

void
kStrLen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = strlen(s->heap + PARAM(0));
}


void
kGetFarText(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *textres = findResource(sci_text, PARAM(0));
  char *seeker;
  int counter = PARAM(1);

  if (!textres) {
    SCIkwarn("text.%d does not exist\n", PARAM(0));
    return;
  }

  seeker = textres->data;

  while (counter--)
    while (*seeker++)
  /* The second parameter (counter) determines the number of the string inside the text
  ** resource.
  */

  strcpy(s->heap + (s->acc = UPARAM(2)), seeker); /* Copy the string and get return value */
}

void
kWait(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  struct timeval time;

  gettimeofday(&time, NULL);

  s-> acc = ((time.tv_usec - s->last_wait_time.tv_usec) * 60 / 1000000) +
    (time.tv_sec - s->last_wait_time.tv_sec) * 60;

  memcpy(&(s->last_wait_time), &time, sizeof(struct timeval));
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
  CHECK_THIS_KERNEL_FUNCTION;

  if (resource) {
    if (PARAM_OR_ALT(2, 1)) clearPicture(s->bgpic, 15);
    drawPicture0(s->bgpic, 1, PARAM_OR_ALT(3, 0), resource->data);

    if (argc > 1)
      s->pic_animate = PARAM(1); /* The animation used during kAnimate() later on */

    s->pic_not_valid = 1;

  } else
    SCIkwarn("Request to draw non-existing pic.%03d\n", PARAM(0));
}


void
kBaseSetter(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int x, y, xstep, ystep, xsize, ysize;
  int xbase, ybase, xend, yend;
  int view, loop, cell;
  resource_t *viewres;
  heap_ptr object = PARAM(0);

  CHECK_THIS_KERNEL_FUNCTION;

  x = GET_SELECTOR(object, x);
  y = GET_SELECTOR(object, y);
  xstep = GET_SELECTOR(object, xStep);
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
  }

  if ((xsize < 0) || (ysize < 0))
    xsize = ysize = 0; /* Invalid view/loop */


  xbase = x - xsize / 2;
  ybase = y - ysize / 2;

  /*  if (xstep) {
    xend = xbase;
    xbase += xstep;
    } else*/ xend = xbase + xsize;

  /*  if (ystep) {
    yend = ybase;
    ybase += ystep;
    } else*/ yend = ybase + ysize;

  PUT_SELECTOR(object, brLeft, xbase);
  PUT_SELECTOR(object, brRight, xend);
  PUT_SELECTOR(object, brTop, ybase);
  PUT_SELECTOR(object, brBottom, ybase);

  sciprintf("BaseSetter done.\n");

} /* kBaseSetter */

#define _K_DRAW_MODE_MIN 0    /* size does not affect placement */
#define _K_DRAW_MODE_CENTER 1 /* center the specified axis */
#define _K_DRAW_MODE_MAX 2    /* use right/bottom end for placement */

void
draw_object(state_t *s, heap_ptr object, int xmode, int ymode)
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

  SCIkdebug("Drawing from %04x to view %d, loop %d, cel %d to %d, %d, priority %d, at port #%d\n", object, view, loop, cel, x, y, priority, s->view_port);
  SCIkdebug("    width=%d, height=%d\n", view0_cel_width(loop, cel, viewres->data),
	    view0_cel_height(loop, cel, viewres->data));


  switch (xmode) {
  case _K_DRAW_MODE_MIN:
      break;
  case _K_DRAW_MODE_CENTER:
      x -= view0_cel_width(loop, cel, viewres->data) / 2;
      break;
  case _K_DRAW_MODE_MAX:
      x -= view0_cel_width(loop, cel, viewres->data);
      break;
  }

  switch (ymode) {
  case _K_DRAW_MODE_MIN:
      break;
  case _K_DRAW_MODE_CENTER:
      y -= view0_cel_height(loop, cel, viewres->data) / 2;
      break;
  case _K_DRAW_MODE_MAX:
      y -= view0_cel_height(loop, cel, viewres->data);
      break;
  }

  drawView0(s->bgpic, s->ports[s->view_port], x, y,
	    priority, loop, cel, viewres->data);
}


#define K_CONTROL_BUTTON 1
#define K_CONTROL_TEXT 2
#define K_CONTROL_EDIT 3
#define K_CONTROL_ICON 4
#define K_CONTROL_CONTROL 6




void
kDrawControl(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = PARAM(0);
  int x = GET_SELECTOR(obj, nsLeft);
  int y = GET_SELECTOR(obj, nsTop);
  int xl = GET_SELECTOR(obj, nsRight) - x + 1;
  int yl = GET_SELECTOR(obj, nsBottom) - y + 1;

  int font_nr = GET_SELECTOR(obj, font);
  char *text = s->heap + GET_SELECTOR(obj, text);
  int view = GET_SELECTOR(obj, view);
  int cel = GET_SELECTOR(obj, cel);
  int loop = GET_SELECTOR(obj, loop);

  int type = GET_SELECTOR(obj, type);
  int state = GET_SELECTOR(obj, state);

  resource_t *font_res;
  resource_t *view_res;

  CHECK_THIS_KERNEL_FUNCTION;

  switch (type) {

  case K_CONTROL_BUTTON:

    font_res  = findResource(sci_font, font_nr);
    if (!font_res) {
      SCIkwarn("Font.%03d not found!\n", font_nr);
      return;
    }
    graph_draw_selector_button(s, s->ports[s->view_port], state, x, y, xl, yl, text, font_res->data);
    break;

  case K_CONTROL_TEXT:

    font_res  = findResource(sci_font, font_nr);
    if (!font_res) {
      SCIkwarn("Font.%03d not found!\n", font_nr);
      return;
    }
    graph_draw_selector_text(s, s->ports[s->view_port], state, x, y, xl, yl, text, font_res->data);
    break;

  case K_CONTROL_EDIT:

    font_res  = findResource(sci_font, font_nr);
    if (!font_res) {
      SCIkwarn("Font.%03d not found!\n", font_nr);
      return;
    }
    graph_draw_selector_edit(s, s->ports[s->view_port], state, x, y, xl, yl, text, font_res->data);
    break;

  case K_CONTROL_ICON:
    
    view_res = findResource(sci_view, view);
    if (!view_res) {
      SCIkwarn("View.%03d not found!\n", font_nr);
      return;
    }
    graph_draw_selector_icon(s, s->ports[s->view_port], state, x, y, xl, yl,
			     view_res->data, loop, cel);
    break;

  case K_CONTROL_CONTROL:

    graph_draw_selector_control(s, s->ports[s->view_port], state, x, y, xl, yl);
    break;

  default:
    SCIkwarn("Unknown control type: %d at %04x\n", type, obj);
  }
}



void
_k_draw_node_list(state_t *s, heap_ptr list, int xmode, int ymode)
     /* Draw a node list, with the specified axis modifiers (_K_DRAW_MODE_*) */
{
  heap_ptr node = GET_HEAP(list + LIST_FIRST_NODE);

  if (GET_HEAP(list - 2) != 0x6) { /* heap size check */
    SCIkwarn("Attempt to draw non-list at %04x\n", list);
    return;
  }

  while (node) {
    draw_object(s, GET_HEAP(node + LIST_NODE_VALUE), xmode, ymode);
    node = GET_HEAP(node + LIST_NEXT_NODE);
  }
}


void
kAddToPic(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr list = PARAM(0);
  CHECK_THIS_KERNEL_FUNCTION;

  _k_draw_node_list(s, list, _K_DRAW_MODE_CENTER /* X */, _K_DRAW_MODE_MAX /* Y */);
  /* Draw relative to the bottom center */
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
  port_t *wnd;

  CHECK_THIS_KERNEL_FUNCTION;

  while ((window < MAX_PORTS) && (s->ports[window]))
    ++window;

  if (window == MAX_PORTS) {
    kernel_oops(s, "Out of window/port handles in kNewWindow! Increase MAX_PORTS in engine.h\n");
    return;
  }

  wnd = calloc(sizeof(port_t), 1);

  wnd->ymin = PARAM(0);
  wnd->xmin = PARAM(1);
  wnd->ymax = PARAM(2);
  wnd->xmax = PARAM(3);
  wnd->title = PARAM(4);
  wnd->flags = PARAM(5);
  wnd->priority = PARAM(6);
  wnd->color = PARAM(7);
  wnd->bgcolor = PARAM(8);
  wnd->font = s->titlebar_port.font; /* Default to 'system' font */

  s->ports[window] = wnd;

  drawWindow(s->bgpic, s->ports[window], wnd->bgcolor, wnd->priority,
	     s->heap + wnd->title, s->titlebar_port.font , wnd->flags); /* Draw window */

  drawWindow(s->pic, s->ports[window], wnd->bgcolor, wnd->priority,
	     s->heap + wnd->title, s->titlebar_port.font , wnd->flags); /* Draw window */

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_BOX,
		       wnd->xmin - 1, wnd->ymin - (wnd->flags & WINDOW_FLAG_TITLE)? 11 : 1,
		       wnd->xmax + 2, wnd->ymin + 2); /* Update screen */
  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0,0,0,0); /* Update mouse pointer */

  s->view_port = window; /* Set active port */

  s->acc = window;
}

void
kDrawStatus(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  draw_titlebar(s->pic, 0xf);
  drawText0(s->pic, &(s->titlebar_port), 1, 1, ((char *)s->heap) + PARAM(0), s->titlebar_port.font, 0);

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_BOX,
		       0, 0, 320, 10);
  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0,0,0,0); /* Update mouse pointer */
}

void
kDrawMenuBar(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;

  if (PARAM(0))
    menubar_draw(s->pic, &(s->titlebar_port) ,s->menubar, -1, s->titlebar_port.font);
  else
    draw_titlebar(s->pic, 0);

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_BOX,
		       0, 0, 320, 10);
  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0,0,0,0); /* Update mouse pointer */
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

  CHECK_THIS_KERNEL_FUNCTION;

  i = 0;
  while (UPARAM(i) > 1000)
      _k_draw_node_list(s, UPARAM(i++), _K_DRAW_MODE_CENTER, _K_DRAW_MODE_MAX);

  if (s->pic_not_valid) {
    SCIkdebug("Animating pic opening type %x\n", s->pic_animate);

    switch(s->pic_animate) {
    case K_ANIMATE_BORDER_CLOSE_H_CENTER_OPEN_H :

      for (i = 0; i < 160; i++) {
	graph_clear_box(s, i, 10, 1, 190, 0);
	graph_clear_box(s, 319-i, 10, 1, 190, 0);
	usleep(3 * s->animation_delay);
      }

    case K_ANIMATE_CENTER_OPEN_H :

      for (i = 159; i >= 0; i--) {
	graph_update_box(s, i, 10, 1, 190);
	graph_update_box(s, 319-i, 10, 1, 190);
	usleep(3 * s->animation_delay);
      }
      break;


    case K_ANIMATE_BORDER_CLOSE_V_CENTER_OPEN_V :

      for (i = 0; i < 95; i++) {
	graph_clear_box(s, 0, i + 10, 320, 1, 0);
	graph_clear_box(s, 0, 199 - i, 320, 1, 0);
	usleep(5 * s->animation_delay);
      }

    case K_ANIMATE_CENTER_OPEN_V :

      for (i = 94; i >= 0; i--) {
	graph_update_box(s, 0, i + 10, 320, 1);
	graph_update_box(s, 0, 199 - i, 320, 1);
	usleep(5 * s->animation_delay);
      }
      break;


    case K_ANIMATE_LEFT_CLOSE_RIGHT_OPEN :

      for(i = 0; i < 320; i++) {
	graph_clear_box(s, i, 10, 1, 190, 0);
	usleep(s->animation_delay);
      }

    case K_ANIMATE_RIGHT_OPEN :
      for(i = 319; i >= 0; i--) {
	graph_update_box(s, i, 10, 1, 190);
	usleep(s->animation_delay);
      }
      break;


    case K_ANIMATE_RIGHT_CLOSE_LEFT_OPEN :

      for(i = 319; i >= 0; i--) {
	graph_clear_box(s, i, 10, 1, 190, 0);
	usleep(s->animation_delay);
      }

    case K_ANIMATE_LEFT_OPEN :

      for(i = 0; i < 320; i++) {
	graph_update_box(s, i, 10, 1, 190);
	usleep(s->animation_delay);
      }
      break;


    case K_ANIMATE_TOP_CLOSE_BOTTOM_OPEN :

      for (i = 10; i < 200; i++) {
	graph_clear_box(s, 0, i, 320, 1, 0);
	usleep(2 * s->animation_delay);
      }

    case K_ANIMATE_BOTTOM_OPEN :

      for (i = 199; i >= 10; i--) {
	graph_update_box(s, 0, i, 320, 1);
	usleep(2 * s->animation_delay);
      }
      break;


    case K_ANIMATE_BOTTOM_CLOSE_TOP_OPEN :

      for (i = 199; i >= 10; i--) {
	graph_clear_box(s, 0, i, 320, 1, 0);
	usleep(2 * s->animation_delay);
      }

    case K_ANIMATE_TOP_OPEN :

      for (i = 10; i < 200; i++) {
	graph_update_box(s, 0, i, 320, 1);
	usleep(2 * s->animation_delay);
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

	usleep(7 * s->animation_delay);
      }

    case K_ANIMATE_BORDER_OPEN_F :

      for (i = 0; i < 32; i++) {
	int height = i * 3;
	int width = i * 5;

	graph_update_box(s, width, 10 + height, 5, 190 - 2*height);
	graph_update_box(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height);

	graph_update_box(s, width, 10 + height, 320 - 2*width, 3);
	graph_update_box(s, width, 200 - 3 - height, 320 - 2*width, 3);

	usleep(7 * s->animation_delay);
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

	usleep(7 * s->animation_delay);
      }

    case K_ANIMATE_CENTER_OPEN_F :

      for (i = 31; i >= 0; i--) {
	int height = i * 3;
	int width = i * 5;

	graph_update_box(s, width, 10 + height, 5, 190 - 2*height);
	graph_update_box(s, 320 - 5 - width, 10 + height, 5, 190 - 2*height);

	graph_update_box(s, width, 10 + height, 320 - 2 * width, 3);
	graph_update_box(s, width, 200 - 3 - height, 320 - 2 * width, 3);

	usleep(7 * s->animation_delay);
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
	usleep(s->animation_delay / 2);

	--remaining_checkers;
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
	usleep(s->animation_delay / 2);

	--remaining_checkers;
      }

      break;


    default:
      memcpy(s->pic[0] + 320 * 10, s->bgpic[0] + 320 * 10, 320 * 190);

      s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_ALL,
			   0, 0, 0, 0);
    }
  }

  s->pic_not_valid = 0;
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
  int argpt = 1;
  int textp = UPARAM(0);
  int width = -1;
  int save_under = 0;
  char *text;
  resource_t *font_resource;
  port_t *port = s->ports[s->view_port];

  CHECK_THIS_KERNEL_FUNCTION;

  if (textp < 1000) {
    resource_t *res;
    res = findResource(sci_text, textp);

    if (!res) {
      SCIkwarn("Invalid text resource: 0x%x\n", textp);
      return;
    }

    text = res->data + UPARAM(argpt++);
  }
  else text = s->heap + textp;

  while (argpt < argc) {

    switch(PARAM(argpt++)) {

    case K_DISPLAY_SET_COORDS:

      port->x = PARAM(argpt++);
      port->y = PARAM(argpt++);
      SCIkdebug("Display: set_coords(%d, %d)\n", port->x, port->y);
      break;

    case K_DISPLAY_SET_ALIGNMENT:

      port->alignment = PARAM(argpt++);
      SCIkdebug("Display: set_align(%d)\n", port->alignment);
      break;

    case K_DISPLAY_SET_COLOR:

      port->color = PARAM(argpt++);
      SCIkdebug("Display: set_color(%d)\n", port->color);
      break;

    case K_DISPLAY_SET_BGCOLOR:

      port->bgcolor = PARAM(argpt++);
      SCIkdebug("Display: set_bg_color(%d)\n", port->bgcolor);
      break;

    case K_DISPLAY_SET_GRAYTEXT:

      port->gray_text = PARAM(argpt++);
      SCIkdebug("Display: set_align(%d)\n", port->alignment);
      break;

    case K_DISPLAY_SET_FONT:

      font_resource = findResource(sci_font, PARAM(argpt++));

      if (font_resource)
	port->font = font_resource->data;
      else port->font = NULL;

      SCIkdebug("Display: set_font(\"font.%03d\")\n", PARAM(argpt - 1));
      break;

    case K_DISPLAY_WIDTH:

      width = PARAM(argpt++);
      SCIkdebug("Display: set_width(%d)\n", width);
      break;

    case K_DISPLAY_SAVE_UNDER:

      save_under = 1;
      SCIkdebug("Display: set_save_under()\n", 0);
      break;

    case K_DISPLAY_RESTORE_UNDER:

      SCIkdebug("Display: restore_under(%04x)\n", PARAM(argpt - 1));
      graph_restore_box(s, PARAM(argpt++));
      return;

    default:
      SCIkdebug("Unknown Display() command %x\n", PARAM(argpt-1));
      SCIkdebug("Display: set_align(%d)\n", port->alignment);
      return;
    }
  }

  if (save_under)     /* Backup */
    s->acc = graph_save_box(s, port->xmin, port->ymin,
			    port->xmax - port->xmin + 1, port->ymax - port->ymin + 1, 1);


  SCIkdebug("Display: Commiting\n", 0);

  if (s->view_port)
    graph_fill_port(s, port, port->bgcolor);  /* Only fill if we aren't writing to the main view */

  text_draw(s->bgpic, port, text, width);

  if (!s->pic_not_valid) /* Refresh if drawn to valid picture */
    graph_update_box(s, port->xmin, port->ymin,
		     port->xmax - port->xmin + 1, port->ymax - port->ymin + 1);
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

  /* Experimental functions */
  {"Wait", kWait },
  {"BaseSetter", kBaseSetter},
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
