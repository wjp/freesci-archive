/* VM and kernel declarations */

#include <script.h>
#include <heap.h>
#include <vocabulary.h>


#ifndef _SCI_VM_H
#define _SCI_VM_H

#define VM_STACK_SIZE 0x1000
/* Number of bytes to be allocated for the stack */

#define SCRIPT_MAX_EXEC_STACK 256
/* Maximum number of calls residing on the stack */
#define SCRIPT_MAX_CLASSTABLE_SIZE 256
/* Maximum number of entries in the class table */
#define SCRIPT_MAX_CLONES 256
/* Maximum number of cloned objects on the heap */


#define SCRIPT_SELECTOR_OFFSET 8 -8
/* Object-relative offset of the selector area inside a script */

#define SCRIPT_LOCALVARPTR_OFFSET 2 -8
/* Object-relative offset of the pointer to the underlying script's local variables */

#define SCRIPT_SELECTORCTR_OFFSET 6 -8
/* Object-relative offset of the selector counter */

#define SCRIPT_FUNCTAREAPTR_OFFSET 4 -8
/* Object-relative offset of the offset of the function area */

#define SCRIPT_FUNCTAREAPTR_MAGIC 8 -8
/* Offset that has to be added to the function area pointer */

#define SCRIPT_NAME_OFFSET 14 -8
/* Offset of the name pointer */

#define SCRIPT_INFO_OFFSET 12 -8
/* Object-relative offset of the -info- selector */

#define SCRIPT_INFO_CLONE 0x0001
/* Flag fo the -info- selector */

#define SCRIPT_INFO_CLASS 0x8000
/* Flag for the -info- selector */


#define SCRIPT_OBJECT_MAGIC_NUMBER 0x1234
/* Magical object identifier */
#define SCRIPT_OBJECT_MAGIC_OFFSET -8
/* Offset of this identifier */

#define SCRIPT_SPECIES_OFFSET 8 -8
/* Script-relative offset of the species ID */

#define SCRIPT_SUPERCLASS_OFFSET 10 -8

#define SCRIPT_LOFS_MAGIC 3
/* Magic adjustment value for lofsa and lofss */


#define SELECTOR_NONE 0
#define SELECTOR_VARIABLE 1
#define SELECTOR_METHOD 2
/* Types of selectors as returned by grep_selector() below */


struct _state; /* engine.h */


typedef void kfunct(struct _state *s, int funct_nr, int argc, heap_ptr argv);
/* Kernel functions take the current state, the function number (for kstub()),
** the number of arguments, and a heap_ptr to the first argument as parameters.
*/

typedef struct
{
  int id;
  object* class;
  byte* heap;
  int offset;
} instance;


typedef struct
{
  int script; /* number of the script the class is in */
  heap_ptr *scriptposp; /* Pointer to the script position entry in the script list */
  int class_offset; /* script-relative position of the class */
} class_t;

typedef struct
{
  heap_ptr heappos; /* Script position on the heap or 0 if not yet loaded */
  heap_ptr localvar_offset; /* Abs. offset of the local variable block or 0 if not present */
  heap_ptr export_table_offset; /* Abs. offset of the export table or 0 if not present */
  int lockers; /* Number of classes and objects that require this script */
} script_t;

typedef struct
{
  int init; /* Init function */
  int play; /* Play function (first function to be called) */
  int x, y, z; /* Coordinates (You probably didn't expect this) */
  int priority;
  int view, loop, cel; /* Description of a specific image */
  int brLeft, brRight, brTop, brBottom; /* Bounding Rectangle */
  int xStep, yStep; /* BR adjustments */
  int nsLeft, nsRight, nsTop, nsBottom; /* View boundaries */
  int text, font; /* Used by controls */
  int type, state; /* Used by contols as well */
  int doit; /* Called (!) by the Animate() system call */
  int delete; /* Called by Animate() to dispose a view object */
  int signal; /* Used by Animate() to control a view's behaviour */
  int underBits; /* Used by the graphics subroutines to store backupped BG pic data */

  /* The following selectors are used by the Bresenham syscalls: */
  int canBeHere; /* Checks for movement validity */
  int client; /* The object that wants to be moved */
  int dx, dy; /* Deltas */
  int b_movCnt, b_i1, b_i2, b_di, b_xAxis, b_incr; /* Various Bresenham vars */
  int completed;
} selector_map_t; /* Contains selector IDs for a few selected selectors */

typedef struct {
  heap_ptr obj;
  heap_ptr signalp;    /* Used only indirectly */
  heap_ptr underBitsp; /* The same goes for the handle storage */

  int x, y;
  int priority;
  byte *view;
  int loop, cel;
  int nsTop, nsLeft, nsRight, nsBottom;
} view_object_t;

typedef struct {
  heap_ptr *objpp;
  heap_ptr *pcp;
  heap_ptr *spp;
  heap_ptr *ppp;
  int *argcp;
  heap_ptr *argpp;
  int selector; /* The selector which was used to call or -1 if not applicable */
} script_exec_stack_t;

extern script_exec_stack_t script_exec_stack[];
/* Pointers to values used for debugging purposes. */

extern int script_exec_stackpos;
/* current position inside the script_exec_stack */


extern int script_debug_flag;
/* Set this to 1 to activate script debugging */

extern int script_error_flag;
/* Set to 1 to move pc back to last position, even though action is executed */

extern int script_checkloads_flag;
/* Displays the numbers of scripts when they are (un)loaded */

extern int script_abort_flag;
/* Set this to 1 to abort script execution immediately. Aborting will leave the
** debug exec stack intact.
*/

extern int script_step_counter;
/* Number of steps executed */


extern char *(*_debug_get_input)(void);
/* The function used to get input for debugging */


typedef int kernel_function(struct _state* s);

extern kernel_function* kfuncs[];
extern int max_instance;

void
execute_method(struct _state *s, word script, word pubfunct, heap_ptr sp, heap_ptr calling_obj,
	       word argc, heap_ptr argp);
/* Executes function pubfunct of the specified script.
** Parameters: (state_t *) s: The state which is to be executed with
**             (word) script: The script which is called
**             (word) pubfunct: The exported script function which is to be called
**             (heap_ptr) calling_obj: The heap address of the object which executed the call
**             (word) argc: Number of arguments supplied
**             (heap_ptr) argp: Pointer to the first supplied argument
*/

void
execute(struct _state *s, heap_ptr pc, heap_ptr sp, heap_ptr objp, int argc, heap_ptr argp,
	int selector);
/* Executes the code on s->heap[pc] until it hits a 'ret' command
** Parameters: (state_t *) s: The state with which to execute
**             (heap_ptr) pc: The initial program counter
**             (heap_ptr) sp: The initial stack pointer
**             (heap_ptr) objp: Pointer to the beginning of the current object
**             (int) argc: Number of parameters to call with
**             (heap_ptr) argp: Heap pointer to the first parameter
**             (int) selector: The selector over which it was called or -1 if n.a. For debugging.
** This function will execute SCI bytecode. It requires s to be set up
** correctly.
*/

void
script_debug(struct _state *s, heap_ptr *pc, heap_ptr *sp, heap_ptr *pp, heap_ptr *objp,
	     int *restadjust);
/* Debugger functionality
** Parameters: (state_t *) s: The state at which debugging should take place
**             (heap_ptr *) pc: Pointer to the program counter
**             (heap_ptr *) sp: Pointer to the stack pointer
**             (heap_ptr *) pp: Pointer to the frame pointer
**             (heap_ptr *) objp: Pointer to the object base pointer
**             (int *) restadjust: Pointer to the &rest adjustment value
** Returns   : (void)
*/

int
script_init_state(struct _state *s);
/* Initializes a state_t block
** Parameters: (state_t *) s: The state to initialize
** Returns   : 0 on success, 1 if vocab.996 (the class table) is missing or corrupted
*/

void
script_free_state(struct _state *s);
/* Frees all additional memory associated with a state_t block
** Parameters: (state_t *) s: The state_t whose elements should be cleared
** Returns   : (void)
*/

int
lookup_selector(struct _state *s, heap_ptr obj, int selectorid, heap_ptr *address);
/* Looks up a selector and returns its type and value
** Parameters: (state_t *) s: The state_t to use
**             (heap_ptr) obj: Address of the object to look the selector up in
**             (int) selectorid: The selector to look up
**             (heap_ptr *) address: A pointer to a variable which the result will be stored in
** Returns   : (int) SELECTOR_NONE if the selector was not found in the object or its superclasses
**                   SELECTOR_VARIABLE if the selector represents an object-relative variable
**                   SELECTOR_METHOD if the selector represents a method
** The result value stored in 'address' depends on the type of selector; variable selectors will
** return pointers to the selector value (and NOT the actual value), method selectors will return
** the heap address of the method the selector references to.
*/


heap_ptr
script_instantiate(struct _state *s, int script_nr);
/* Makes sure that a script and its superclasses get loaded to the heap
** Parameters: (state_t *) s: The state to operate on
**             (int) script_nr: The script number to load
** Returns   : (heap_ptr) The address of the script on the heap or 0 if out of heap
** If the script already has been loaded, only the number of lockers is increased.
** All scripts containing superclasses of this script get loaded recursively as well.
** The complementary function is "script_uninstantiate()" below.
*/

void
script_uninstantiate(struct _state *s, int script_nr);
/* Decreases the numer of lockers of a script and unloads it if that number reaches zero
** Parameters: (state_t *) s: The state to operate on
**             (int) script_nr: The script number that is requestet to be unloaded
** Returns   : (void)
** This function will recursively unload scripts containing its superclasses, if those
** aren't locked by other scripts as well.
*/


int
game_init(struct _state *s);
/* Initializes an SCI game
** Parameters: (state_t *) s: The state to operate on
** Returns   : (int): 0 on success, 1 if an error occured.
** This function must be run before script_run() is executed.
*/


int
game_run(struct _state *s);
/* Runs an SCI game
** Parameters: (state_t *) s: The state to operate on
** Returns   : (int): 0 on success, 1 if an error occured.
** This is the main function for SCI games. It takes a valid state, loads script 0 to it,
** finds the game object, allocates a stack, and runs the init method of the game object.
** In layman's terms, this runs an SCI game.
*/


int
game_exit(struct _state *s);
/* Uninitializes an initialized SCI game
** Parameters: (state_t *) s: The state to operate on
** Returns   : (int): 0 on success, 1 if an error occured.
** This function should be run after each script_run() call.
*/


void
script_map_selectors(struct _state *s, selector_map_t *map);
/* Maps special selectors
** Parameters: (state_t *) s: The state from which the selector information should be taken
**             (selector_map_t *) map: Pointer to the selector map to map
** Returns   : (void)
** Called by script_run();
*/

void
script_map_kernel(struct _state *s);
/* Maps kernel functions
** Parameters: (state_t *) s: The state which the kernel_names are retreived from
** Returns   : (void)
** This function reads from and writes to s. It is called by script_run().
*/


int
kalloc(struct _state *s, int space);
/* Allocates "kernel" memory and returns a handle suitable to be passed on to SCI scripts
** Parameters: (state_t *) s: Pointer to the state_t to operate on
**             (int) space: The space to allocate
** Returns   : (int) The handle
*/


byte *
kmem(struct _state *s, int handle);
/* Returns a pointer to "kernel" memory based on the handle
** Parameters: (state_t *) s: Pointer to the state_t to operate on
**             (int) handle: The handle to use
** Returns   : (byte *) A pointer to the allocated memory
*/


int
kfree(struct _state *s, int handle);
/* Frees all "kernel" memory associated with a handle
** Parameters: (state_t *) s: Pointer to the state_t to operate on
**             (handle) space: The space to allocate
** Returns   : (int) 0 on success, 1 otherwise
*/

#endif /* !_SCI_VM_H */
