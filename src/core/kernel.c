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

 History:

   000000 - created (CJR)

***************************************************************************/

#include <script.h>
#include <vm.h>
#include <engine.h>


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




/*Here comes implementations of some kernel functions. I haven't found any
 *documentation on these, except for their names in vocab.999.
 */

#if 0

int kClone(state* s)
{
  instance* old=instance_map[getInt16(s->heap->data+s->acc)];
  instance* new=instantiate(s->heap, old->class);
  int i;
  if(new==0) return 1;

  /*copy the selector values...*/
  for(i=0; i<old->class->selector_count*2; i++)
    new->heap[new->offset+i+2]=old->heap[old->offset+i+2];
  /*...but use the correct id...*/
  putInt16(new->heap+new->offset, new->id);
  /*...mark as clone (ie set bit 0 in -info-)...*/
  putInt16(new->heap+new->offset+6,
	   (getInt16(new->heap+new->offset+6)|1));
  /*...and use old's id as superclass*/
  putInt16(new->heap+new->offset+4, old->id);

  /*set acc to point to the new instance*/
  s->acc=new->offset;
  s->sp-=4;
  return 0;
}

int kDisposeClone(state* s)
{
  int id=getInt16(s->heap->data+s->acc);
  instance* i;
  /*FIXME: check acc for sanity*/

  i=instance_map[id];
  if(h_free(s->heap, i->offset, (i->class->selector_count+1)*2)) return 1;

  /*shuffle the instance_map*/
  instance_map[id]=instance_map[max_instance--];
  putInt16(s->heap->data+instance_map[id]->offset, id);

  s->sp-=4;
  return 0;
}

int kIsObject(state* s)
{
  int id=getInt16(s->heap->data+s->acc);

  if(id<=0x4000 && instance_map[id]->offset==s->acc) s->acc=1;
  else s->acc=0;

  s->sp-=4;
  return 0;
}

#endif /* false */


#define PARAM(x) ((guint16) getInt16(s->heap + argp + ((x)*2)))

#define PARAM_OR_ALT(x, alt) ((x < argc)? PARAM(x) : (alt))
/* Returns the parameter value or (alt) if not enough parameters were supplied */


/* kLoad(restype, resnr):
** Loads an arbitrary resource of type 'restype' with resource numbber 'resnr'
*/
void
kLoad(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
    int restype = PARAM(0);
    int resnr = PARAM(1);

    s->acc = ((restype << 11) | resnr); /* Return the resource identifier as handle */

    if (restype == sci_memory) { /* Request to dynamically allocate hunk memory for later use */
	int seeker;

	for (seeker = 0; seeker < MAX_HUNK_BLOCKS; seeker++)
	    if (s->hunk[seeker].size == resnr) {
		SCIkwarn("Attempt to re-allocate 'memory.%d'\n", resnr);
		return; /* I have a baaad feeling about this... */
	    }

	/* Not allocated yet */

	seeker = 0;
	while ((seeker < MAX_HUNK_BLOCKS) && (s->hunk[seeker].size))
	    seeker++;

	if (seeker == MAX_HUNK_BLOCKS)

	    kernel_oops(s, "Out of hunk handles! Try increasing MAX_HUNK_BLOCKS in engine.h");

	else
	    s->hunk[seeker].data = malloc(s->hunk[seeker].size = resnr);
	/* Requested memory size is equal to resource number for sci_memory */

    }; /* End of "If we are supposed to actually allocate memory" */
}

/* kUnload():
** Unloads an arbitrary resource of type 'restype' with resource numbber 'resnr'
*/
void
kUnLoad(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
    int restype = PARAM(0);
    int resnr = PARAM(1);

    if (restype == sci_memory) {
	int seeker;
	for (seeker = 0; seeker < MAX_HUNK_BLOCKS; seeker++)
	    if (s->hunk[seeker].size == resnr) {
		free(s->hunk[seeker].data);
		s->hunk[seeker].size = 0;
		return;
	    }

	SCIkwarn("Attemt to unallocate nonexisting 'memory.%d'", resnr);
    }
}


/* kGameIsRestarting():
** Returns the restarting_flag in acc
*/
void
kGameIsRestarting(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = s->restarting_flag;
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

  nodebase += 2; /* Jump over heap header */

  PUT_HEAP(nodebase + LIST_PREVIOUS_NODE, 0);
  PUT_HEAP(nodebase + LIST_NEXT_NODE, 0);
  PUT_HEAP(nodebase + LIST_NODE_KEY, PARAM(0));

  s->acc = nodebase; /* Return node base address */
}


void
kAddToEnd(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr listbase = PARAM(0);
  heap_ptr nodebase = PARAM(1);
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
  heap_ptr listbase = PARAM(0);
  heap_ptr nodebase = PARAM(1);
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
  word key = PARAM(1);

  /*  SCIkdebug("argc=%d; args = (%04x %04x %04x)\n", argc, PARAM(0), PARAM(1), PARAM(2)); */

  node = GET_HEAP(PARAM(0) + LIST_FIRST_NODE);

  while (node && (GET_HEAP(node + LIST_NODE_KEY) != key))
    node = GET_HEAP(node + LIST_NEXT_NODE);
  /* Aborts if either the list ends (node == 0) or the key is found */

  s->acc = node;
}


/*************************************************************/

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
  char *targetaddr = PARAM(0) + s->heap;

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
  if (PARAM(1))
    s->mouse_pointer = findResource(sci_cursor, PARAM(0))->data;
  else
    s->mouse_pointer = 0;
}


void
kShow(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
    s->pic_not_valid = 0;
}


void
kPicNotValid(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
    s->acc = s->pic_not_valid;
    if (argc)
	s->pic_not_valid = PARAM(0);
}


void
kstub(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int i;

  sciprintf("Stub: %s[%x](", s->kernel_names[funct_nr], funct_nr);

  for (i = 0; i < argc; i++) {
    sciprintf("%04x", PARAM(i));
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
  {0,0} /* Terminator */
};


void
script_map_kernel(state_t *s)
{
  int functnr;

  s->kfunct_table = malloc(sizeof(kfunct *) * (s->kernel_names_nr + 1));

  for (functnr = 0; functnr < s->kernel_names_nr; functnr++) {
    int seeker, found = -1;

    for (seeker = 0; (found == -1) && kfunct_mappers[seeker].functname; seeker++)
      if (strcmp(kfunct_mappers[seeker].functname, s->kernel_names[functnr]) == 0)
	found = seeker; /* Found a kernel function with the same name! */

    if (found == -1) {

      sciprintf("Warning: Kernel function %s[%x] unmapped\n", s->kernel_names[functnr], functnr);
      s->kfunct_table[functnr] = kstub;

    } else s->kfunct_table[functnr] = kfunct_mappers[found].kernel_function;

  } /* for all functions requesting to be mapped */

}
