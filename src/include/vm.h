/* VM and kernel declarations */

#include <script.h>
#include <heap.h>


#ifndef _SCI_VM_H
#define _SCI_VM_H



#define SCRIPT_SELECTOR_OFFSET 12
/* Offset of the selector area inside a script */

typedef struct
{
  int id;
  object* class;
  byte* heap;
  int offset;
} instance;



typedef struct
{
  heap_t *_heap; /* The heap structure */
  byte *heap; /* The actual heap data (equal to _heap->start) */
  gint16 acc; /* Accumulator */
  gint16 prev; /* previous comparison result */

  heap_ptr global_vars; /* script 000 selectors */
} state;



extern void
(*VM_debug_function)(state *s, heap_ptr pc, heap_ptr sp, heap_ptr pp, heap_ptr objp);
/* Set this to an actual function to do single step debugging */


typedef int kernel_function(state* s);

extern kernel_function* kfuncs[];
extern int max_instance;

void
executeMethod(state *s, word script, word pubfunct, heap_ptr sp, word argc, heap_ptr argp);
/* Executes function pubfunct of the specified script.
** Parameters: (state *) s: The state which is to be executed with
**             (word) script: The script which is called
**             (word) pubfunct: The exported script function which is to be called
**             (word) argc: Number of arguments supplied
**             (heap_ptr) argp: Pointer to the first supplied argument
*/

void
execute(state *s, heap_ptr pc, heap_ptr sp, heap_ptr objp, word argc, heap_ptr argp);
/* Executes the code on s->heap[pc] until it hits a 'ret' command
** Parameters: (state *) s: The state with which to execute
**             (heap_ptr) pc: The initial program counter
**             (heap_ptr) sp: The initial stack pointer
**             (heap_ptr) objp: Pointer to the beginning of the current object
**             (word) argc: Number of parameters to call with
**             (heap_ptr) argp: Heap pointer to the first parameter
** This function will execute SCI bytecode. It requires s to be set up
** correctly.
*/

int emulate(state* s, object* obj, int method);
instance* instantiate(heap_t* h, object* o);

#endif /* !_SCI_VM_H */
