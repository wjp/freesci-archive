/***************************************************************************
 kernel.h Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
#define PI 3.14159265358979323846
#endif /* !PI */

extern int _kdebug_cheap_event_hack;
extern int _kdebug_cheap_soundcue_hack;
extern int stop_on_event;

extern DLLEXTERN int _debug_seeking;
extern DLLEXTERN int _debug_step_running;


typedef struct {
	int x, y, xend, yend;
} abs_rect_t;


/******************** Heap macros ********************/

/* Minimal heap position */
#define HEAP_MIN 800

#define GET_HEAP(address) ((((guint16)(address)) < HEAP_MIN)? \
KERNEL_OOPS("Heap address space violation on read")  \
: getHeapInt16(s->heap, ((guint16)(address))))
/* Reads a heap value if allowed */

#define UGET_HEAP(address) ((((guint16)(address)) < HEAP_MIN)? \
KERNEL_OOPS("Heap address space violation on read")  \
: getHeapUInt16(s->heap, ((guint16)(address))))
/* Reads a heap value if allowed */

#define PUT_HEAP(address, value) { if (((guint16)(address)) < HEAP_MIN) \
KERNEL_OOPS("Heap address space violation on write");        \
else { s->heap[((guint16)(address))] = (value) &0xff;               \
 s->heap[((guint16)(address)) + 1] = ((value) >> 8) & 0xff;}    \
if ((address) & 1)                                                 \
  sciprintf("Warning: Unaligned write to %04x\n", (address) & 0xffff); }
/* Sets a heap value if allowed */

static inline int
getHeapInt16(unsigned char *base, int address)
{
  if (address & 1)
    sciprintf("Warning: Unaligned read from %04x\n", (address) & 0xffff);

  return getInt16(base + address);
}

static inline unsigned int
getHeapUInt16(unsigned char *base, int address)
{
  if (address & 1)
    sciprintf("Warning: Unaligned unsigned read from %04x\n", (address) & 0xffff);

  return getUInt16(base + address);
}


/******************** Selector functionality ********************/

#define GET_SELECTOR(_object_, _selector_) read_selector(s, _object_, s->selector_map._selector_, __FILE__, \
							 __LINE__)
/* Retrieves a selector from an object
** Parameters: (heap_ptr) object: The address of the object which the selector should be read from
**             (selector_name) selector: The selector to read
** Returns   : (gint16) The selector value
** This macro halts on error. 'selector' must be a selector name registered in vm.h's
** selector_map_t and mapped in script.c.
*/


#define UGET_SELECTOR(_object_, _selector_) \
 ((guint16) read_selector(s, _object_, s->selector_map._selector_, __FILE__, __LINE__))
/* Retrieves an unsigned selector value from an object
** Parameters: (heap_ptr) object: The address of the object which the selector should be read from
**             (selector_name) selector: The selector to read
** Returns   : (guint16) The selector value
** This macro halts on error. 'selector' must be a selector name registered in vm.h's
** selector_map_t and mapped in script.c.
*/


#define PUT_SELECTOR(_object_, _selector_, _value_)\
 write_selector(s, _object_, s->selector_map._selector_, _value_, __FILE__, __LINE__)
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
/* Kludge for use with incoke_selector(). Used for compatibility with compilers that can't
** handle vararg macros.
*/


/* functions used internally by macros */
int
read_selector(struct _state *s, heap_ptr object, int selector_id, char *fname, int line);
void
write_selector(struct _state *s, heap_ptr object, int selector_id, int value, char *fname, int line);
int
invoke_selector(struct _state *s, heap_ptr object, int selector_id, int noinvalid, int kfunct,
		heap_ptr k_argp, int k_argc, char *fname, int line, int argc, ...);



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
listp(struct _state *s, heap_ptr address);
/* Determines whether the object at <address> is a list
** Parameters: (state_t *) s: The state to use
**             (heap_ptr) address: The address to check
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

#define SCI0_VIEW_PRIORITY(y) (((y) < s->priority_first)? 0 : (((y) >= s->priority_last)? 14 : 1\
	+ ((((y) - s->priority_first) * 14) / (s->priority_last - s->priority_first))))

#define SCI0_PRIORITY_BAND_FIRST(nr) ((((nr) == 0)? 0 :  \
        ((s->priority_first) + (((nr)-1) * (s->priority_last - s->priority_first)) / 14)))

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


/* List node addresses */
#define LIST_FIRST_NODE 0
#define LIST_PREVIOUS_NODE 0
#define LIST_LAST_NODE 2
#define LIST_NEXT_NODE 2
#define LIST_NODE_KEY 4
#define LIST_NODE_VALUE 6
/* 'previous' and 'next' nodes for lists mean 'first' and 'last' node, respectively */


/* Kernel optimization flags */
#define KERNEL_OPT_FLAG_GOT_EVENT (1<<0)
#define KERNEL_OPT_FLAG_GOT_2NDEVENT (1<<1)


/******************** Kernel functions ********************/

/* Generic description: */


typedef void kfunct(struct _state *s, int funct_nr, int argc, heap_ptr argv);

/* void
** kFunct(state_t *s, int funct_nr, int argc, heap_ptr argp);
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

void kLoad(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kUnLoad(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGameIsRestarting(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kNewList(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetSaveDir(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetCWD(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetCursor(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFindKey(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kNewNode(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kAddToFront(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kAddToEnd(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kShow(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kPicNotValid(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kRandom(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kAbs(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSqrt(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kOnControl(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kHaveMouse(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kJoystick(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetAngle(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetDistance(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kLastNode(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFirstNode(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kNextNode(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kPrevNode(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kNodeValue(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kClone(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDisposeClone(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kScriptID(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kMemoryInfo(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDrawPic(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDisposeList(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDisposeScript(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetPort(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetPort(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kNewWindow(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDisposeWindow(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kIsObject(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFormat(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDrawStatus(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDrawMenuBar(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kAddMenu(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetMenu(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kAddToPic(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCelWide(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCelHigh(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDisplay(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kAnimate(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetTime(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDeleteKey(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrLen(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetFarText(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrEnd(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrCat(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrCmp(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrCpy(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kStrAt(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kReadNumber(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDrawControl(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kNumCels(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kNumLoops(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kTextSize(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kInitBresen(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDoBresen(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCanBeHere(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDrawCel(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDirLoop(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCoordPri(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kPriCoord(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kValidPath(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kRespondsTo(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFOpen(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFPuts(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFGets(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFClose(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kTimesSin(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kTimesCos(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCosMult(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSinMult(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kTimesTan(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kTimesCot(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kMapKeyToDir(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGlobalToLocal(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kLocalToGlobal(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kWait(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCosDiv(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSinDiv(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kBaseSetter(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kParse(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kShakeScreen(struct _state *s, int funct_nr, int argc, heap_ptr argp);
#ifdef _WIN32
void kDeviceInfo_Win32(struct _state *s, int funct_nr, int argc, heap_ptr argp);
#else
void kDeviceInfo_Unix(struct _state *s, int funct_nr, int argc, heap_ptr argp);
#endif
void kHiliteControl(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kRestartGame(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSaid(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kEditControl(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDoSound(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetSynonyms(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGraph(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetEvent(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetMenu(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kMenuSelect(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCheckFreeSpace(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFlushResources(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kGetSaveFiles(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetDebug(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetJump(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kCheckSaveGame(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSaveGame(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kRestoreGame(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kEmptyList(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kAddAfter(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kSetNowSeen(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kDoAvoider(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kFileIO(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void kMemory(struct _state *s, int funct_nr, int argc, heap_ptr argp);
void k_Unknown(struct _state *s, int funct_nr, int argc, heap_ptr argp);
/* The Unknown/Unnamed kernel function */
void kstub(struct _state *s, int funct_nr, int argc, heap_ptr argp);
/* for unimplemented kernel functions */
void kNOP(struct _state *s, int funct_nr, int argc, heap_ptr argp);
/* for kernel functions that don't do anything */

typedef struct {
  char *functname; /* String representation of the kernel function as in script.999 */
  kfunct *kernel_function; /* The actual function */
} sci_kernel_function_t;

extern sci_kernel_function_t kfunct_mappers[];


#endif /* _SCI_KERNEL_H_ */
