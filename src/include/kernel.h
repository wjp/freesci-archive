/***************************************************************************
 kernel.h Copyright (C) 1999..2002 Christoph Reichenbach

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

#ifndef _SCI_KERNEL_H_
#define _SCI_KERNEL_H_

#include <math.h>
#include <ctype.h>
#include <kdebug.h>
#include <uinput.h>
#include <event.h>
#include <heap.h>
#include <console.h> /* sciprintf() */

#ifdef HAVE_FNMATCH_H
#include <fnmatch.h>
#endif /* HAVE_FNMATCH_H */

#ifdef _MSC_VER
#include <direct.h>
#include <ctype.h>
#endif

#ifndef PI
#  define PI 3.14159265358979323846
#endif /* !PI */

extern int _kdebug_cheap_event_hack;
extern int _kdebug_cheap_soundcue_hack;
extern int stop_on_event;

extern DLLEXTERN int _debug_seeking;
extern DLLEXTERN int _debug_step_running;


typedef struct {
	int x, y, xend, yend;
} abs_rect_t;

/* Formerly, the heap macros were here; they have been deprecated, however. */

/******************** Selector functionality ********************/

#define GET_SELECTOR(_object_, _selector_) read_selector16(s, _object_, s->selector_map._selector_, __FILE__, \
							 __LINE__)
#define GET_SEL32(_o_, _slc_) read_selector(s, _o_, s->selector_map._slc_, __FILE__, __LINE)
/* Retrieves a selector from an object
** Parameters: (heap_ptr) object: The address of the object which the selector should be read from
**             (selector_name) selector: The selector to read
** Returns   : (gint16) The selector value
** This macro halts on error. 'selector' must be a selector name registered in vm.h's
** selector_map_t and mapped in script.c.
*/

#define UGET_SELECTOR(_object_, _selector_) \
 ((guint16) read_selector16(s, _object_, s->selector_map._selector_, __FILE__, __LINE__))
/* Retrieves an unsigned selector value from an object
** Parameters: (heap_ptr) object: The address of the object which the selector should be read from
**             (selector_name) selector: The selector to read
** Returns   : (guint16) The selector value
** This macro halts on error. 'selector' must be a selector name registered in vm.h's
** selector_map_t and mapped in script.c.
*/


#define PUT_SELECTOR(_object_, _selector_, _value_)\
 write_selector16(s, _object_, s->selector_map._selector_, _value_, __FILE__, __LINE__)
#define PUT_SEL32(_o_, _slc_, _val_) write_selector(s, _o_, s->selector_map._slc_, _val_, __FILE__, __LINE)
/* Writes a selector value to an object
** Parameters: (heap_ptr) object: The address of the object which the selector should be written to
**             (selector_name) selector: The selector to read
**             (gint16) value: The value to write
** Returns   : (void)
** This macro halts on error. 'selector' must be a selector name registered in vm.h's
** selector_map_t and mapped in script.c.
*/


#define INV_SEL(_object_, _selector_, _noinvalid_) \
  s, _object_,  s->selector_map._selector_, _noinvalid_, funct_nr, argp, argc, __FILE__, __LINE__
/* Kludge for use with invoke_selector(). Used for compatibility with compilers that can't
** handle vararg macros.
*/


/* functions used internally by macros */
int
read_selector16(struct _state *s,  heap_ptr object, selector_t selector_id, char *fname, int line);
void
write_selector16(struct _state *s, heap_ptr object, selector_t selector_id, int value, char *fname, int line);
int
invoke_selector16(struct _state *s, heap_ptr object, int selector_id, int noinvalid, int kfunct,
		stack_ptr_t k_argp, int k_argc, char *fname, int line, int argc, ...);


reg_t
read_selector(struct _state *s,  reg_t object, selector_t selector_id, char *fname, int line);
void
write_selector(struct _state *s, reg_t object, selector_t selector_id, reg_t value,
	       char *fname, int line);
int
invoke_selector(struct _state *s, reg_t object, int selector_id, int noinvalid, int kfunct,
		stack_ptr_t k_argp, int k_argc, char *fname, int line, int argc, ...);




/******************** Text functionality ********************/
char *
kernel_lookup_text(struct _state *s, int address, int index);
/* Looks up text referenced by scripts
** Parameters: (state_t *s): The current state
**             (int) address: The address to look up
**             (int) index: The relative index
** Returns   : (char *): The referenced text, or NULL on error.
** SCI uses two values to reference to text: An address, and an index. The address
** determines whether the text should be read from a resource file, or from the heap,
** while the index either refers to the number of the string in the specified source,
** or to a relative position inside the text.
*/



/******************** Debug functionality ********************/
#define KERNEL_OOPS(reason) kernel_oops(s, __FILE__, __LINE__, reason)

/* Non-fatal assertion */
#define SCIkASSERT(a) if (!(a)) { \
  SCIkwarn(SCIkERROR, "Assertion " #a " failed in " __FILE__ " line %d\n", __LINE__); \
  return; \
}

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


int
listp(struct _state *s, reg_t address);
/* Determines whether the object at <address> is a list
** Parameters: (state_t *) s: The state to use
**             (reg_t) address: The address to check
** Returns   : (int) 0 if not, non-zero if it is a list.
*/

int
is_object(struct _state *s, heap_ptr offset);
/* Checks whether a heap address contains an object
** Parameters: (state_t *) s: The current state
**             (heap_ptr) offset: The heap address
** Returns   : (int) 1 if it is an object, 0 otherwise
*/


/* Functions for internal macro use */
void
_SCIkvprintf(FILE *file, char *format, va_list args);
void
_SCIkprintf(FILE *file, char *format, ...);





/******************** Kernel function parameter macros ********************/

#define PARAM(x) ((gint16) getInt16(s->heap + argp + ((x)*2)))
#define UPARAM(x) ((guint16) getInt16(s->heap + argp + ((x)*2)))

#define PARAM_OR_ALT(x, alt) ((x < argc)? PARAM(x) : (alt))
#define UPARAM_OR_ALT(x, alt) ((x < argc)? UPARAM(x) : (alt))
/* Returns the parameter value or (alt) if not enough parameters were supplied */


#define KP_ALT(x, alt) ((x < argc)? argv[x] : (alt))
#define KP_UINT(x) ((guint16) x.offset)
#define KP_SINT(x) ((gint16) x.offset)

reg_t *
kernel_dereference_pointer(struct _state *s, reg_t pointer, int entries);
/* Dereferences a heap pointer
** Parameters: (state_t *) s: The state to operate on
**             (reg_t ) pointer: The pointer to dereference
**             (int) entries: The number of values expected (for checking)
**                            (use 0 for strings)
** Returns   : (reg_t *): A physical reference to the address pointed
**                        to, or NULL on error or if not enugh entries
**                        were available
*/

/******************** Resource Macros ********************/

/* Returns the composite resource ID: */
#define RESOURCE_ID(type, number) (number) | ((type) << 11)
#define RESOURCE_NUMBER(resid) ((resid) & 0x7ff)
#define RESOURCE_TYPE(resid) ((resid) >> 11)





int
kernel_oops(struct _state *s, char *file, int line, char *reason);
/* Halts script execution and informs the user about an internal kernel error or failed assertion
** Paramters: (state_t *) s: The state to use
**            (char *) file: The file the oops occured in
**            (int) line: The line the oops occured in
**            (char *) reason: Reason for the kernel oops
*/




/******************** Priority macros/functions ********************/

struct _state;

extern int sci01_priority_table_flags; /* 1: delete, 2: print */

int
_find_priority_band(struct _state *s, int band);
/* Finds the position of the priority band specified
** Parameters: (state_t *) s: State to search in
**             (int) band: Band to look for
** Returns   : (int) Offset at which the band starts
*/

int
_find_view_priority(struct _state *s, int y);
/* Does the opposite of _find_priority_band
** Parameters: (state_t *) s: State
**             (int) y: Coordinate to check
** Returns   : (int) The priority band y belongs to
*/

#define SCI0_VIEW_PRIORITY_14_ZONES(y) (((y) < s->priority_first)? 0 : (((y) >= s->priority_last)? 14 : 1\
	+ ((((y) - s->priority_first) * 14) / (s->priority_last - s->priority_first))))

#define SCI0_PRIORITY_BAND_FIRST_14_ZONES(nr) ((((nr) == 0)? 0 :  \
        ((s->priority_first) + (((nr)-1) * (s->priority_last - s->priority_first)) / 14)))

#define SCI0_VIEW_PRIORITY(y) (((y) < s->priority_first)? 0 : (((y) >= s->priority_last)? 14 : 1\
	+ ((((y) - s->priority_first) * 15) / (s->priority_last - s->priority_first))))

#define SCI0_PRIORITY_BAND_FIRST(nr) ((((nr) == 0)? 0 :  \
        ((s->priority_first) + (((nr)-1) * (s->priority_last - s->priority_first)) / 15)))

#define VIEW_PRIORITY(y) _find_view_priority(s, y)
#define PRIORITY_BAND_FIRST(nr) _find_priority_band(s, nr)





/******************** Dynamic view list functions ********************/

abs_rect_t
set_base(struct _state *s, heap_ptr object);
/* Determines the base rectangle of the specified view object
** Parameters: (state_t *) s: The state to use
**             (heap_ptr) object: The object to check
** Returns   : (abs_rect) The absolute base rectangle
*/

extern abs_rect_t
get_nsrect(struct _state *s, heap_ptr object, byte clip);
/* Determines the now-seen rectangle of a view object
** Parameters: (state_t *) s: The state to use
**             (heap_ptr) object: The object to check
**             (byte) clip: Flag to determine wheter priority band
**                          clipping should be performed
** Returns   : (abs_rect) The absolute rectangle describing the
** now-seen area.
*/

void
_k_dyn_view_list_prepare_change(struct _state *s);
     /* Removes all views in anticipation of a new window or text */
void
_k_dyn_view_list_accept_change(struct _state *s);
     /* Redraws all views after a new window or text was added */




/******************** Misc functions ********************/

void
process_sound_events(struct _state *s); /* Get all sound events, apply their changes to the heap */

#define LOOKUP_NODE(addr) lookup_node(s, (addr), __FILE__, __LINE__)
#define LOOKUP_LIST(addr) lookup_list(s, addr, __FILE__, __LINE__)

node_t *
lookup_node(struct _state *s, reg_t addr, char *file, int line);
/* Resolves an address into a list node
** Parameters: (state_t *) s: The state to operate on
**             (reg_t) addr: The address to resolve
**             (char *) file: The file the function was called from
**             (int) line: The line number the function was called from
** Returns   : (node_t *) The list node referenced, or NULL on error
*/


list_t *
lookup_list(struct _state *s, reg_t addr, char *file, int line);
/* Resolves a list pointer to a list
** Parameters: (state_t *) s: The state to operate on
**             (reg_t) addr: The address to resolve
**             (char *) file: The file the function was called from
**             (int) line: The line number the function was called from
** Returns   : (list_t *) The list referenced, or NULL on error
*/



/******************** Constants ********************/

/* Maximum length of a savegame name (including terminator character) */
#define SCI_MAX_SAVENAME_LENGTH 0x24

/* Flags for the signal selector */
#define _K_VIEW_SIG_FLAG_STOP_UPDATE    0x0001
#define _K_VIEW_SIG_FLAG_UPDATED        0x0002
#define _K_VIEW_SIG_FLAG_NO_UPDATE      0x0004
#define _K_VIEW_SIG_FLAG_HIDDEN         0x0008
#define _K_VIEW_SIG_FLAG_FIX_PRI_ON     0x0010
#define _K_VIEW_SIG_FLAG_ALWAYS_UPDATE  0x0020
#define _K_VIEW_SIG_FLAG_FORCE_UPDATE   0x0040
#define _K_VIEW_SIG_FLAG_REMOVE         0x0080
#define _K_VIEW_SIG_FLAG_FROZEN         0x0100
#define _K_VIEW_SIG_FLAG_IS_EXTRA       0x0200
#define _K_VIEW_SIG_FLAG_HIT_OBSTACLE   0x0400
#define _K_VIEW_SIG_FLAG_DOESNT_TURN    0x0800
#define _K_VIEW_SIG_FLAG_NO_CYCLER      0x1000
#define _K_VIEW_SIG_FLAG_IGNORE_HORIZON 0x2000
#define _K_VIEW_SIG_FLAG_IGNORE_ACTOR   0x4000
#define _K_VIEW_SIG_FLAG_DISPOSE_ME     0x8000

#define _K_VIEW_SIG_FLAG_FREESCI_PRIVATE 0x10000000
#define _K_VIEW_SIG_FLAG_FREESCI_STOPUPD 0x20000000 /* View has been stop-updated */


/* Sound status */
#define _K_SOUND_STATUS_STOPPED 0
#define _K_SOUND_STATUS_INITIALIZED 1
#define _K_SOUND_STATUS_PAUSED 2
#define _K_SOUND_STATUS_PLAYING 3



/* Kernel optimization flags */
#define KERNEL_OPT_FLAG_GOT_EVENT (1<<0)
#define KERNEL_OPT_FLAG_GOT_2NDEVENT (1<<1)


/******************** Kernel functions ********************/

/* Generic description: */
typedef reg_t kfunct(struct _state *s, int funct_nr, int argc, reg_t *argv);

/* Old compatibility functions: */
typedef void kfunct_old(struct _state *s, int funct_nr, int argc, heap_ptr argv);

/* void
** kFunct_old(state_t *s, int funct_nr, int argc, heap_ptr argp);
** Kernel function 'Funct'
** Parameters: (state_t *) s: The current state
**             (int funct_nr): The function number (used almost exclusively in kstub)
**             (int) argc: The number of script space arguments passed to the function
**             (int) argp: The address of the first argument passed to the function
** Returns   : (void)
** Inside kernel functions, the convenience macros PARAM(), UPARAM(), PARAM_OR_ALT() and
** UPARAM_OR_ALT() may be used to read the arguments. Return values must be stored in
** s->acc, the accumulator.
*/

#define FREESCI_KFUNCT_OLD 0
#define FREESCI_KFUNCT_GLUTTON 1

typedef struct {
	kfunct *fun; /* The actual function */
	char *signature;  /* kfunct signature */
} kfunct_sig_pair_t;

#define KF_OLD 0
#define KF_NEW 1
#define KF_NONE -1 /* No mapping, but name is known */
#define KF_TERMINATOR -42 /* terminates kfunct_mappers */

typedef struct {
	int type; /* KF_* */
	char *name;
	union {
		kfunct_sig_pair_t new;
		kfunct_old *old;
	} data;
} sci_kernel_function_t;

extern sci_kernel_function_t kfunct_mappers[];

#endif /* _SCI_KERNEL_H_ */
