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

#include <engine.h>
#include <math.h>
#include <ctype.h>
#include <kdebug.h>
#include <uinput.h>
#include <event.h>

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

extern DLLEXTERN int _debug_seeking;
extern DLLEXTERN int _debug_step_running;



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
if (address & 1)                                                 \
  sciprintf("Warning: Unaligned write to %04x\n", address & 0xffff); }
/* Sets a heap value if allowed */

static inline int
getHeapInt16(unsigned char *base, int address)
{
  if (address & 1)
    sciprintf("Warning: Unaligned read from %04x\n", address & 0xffff);

  return getInt16(base + address);
}

static inline unsigned int
getHeapUInt16(unsigned char *base, int address)
{
  if (address & 1)
    sciprintf("Warning: Unaligned unsigned read from %04x\n", address & 0xffff);

  return getUInt16(base + address);
}


/******************** Selector functionality ********************/

#define GET_SELECTOR(_object_, _selector_) read_selector(s, _object_, s->selector_map._selector_)
/* Retrieves a selector from an object
** Parameters: (heap_ptr) object: The address of the object which the selector should be read from
**             (selector_name) selector: The selector to read
** Returns   : (gint16) The selector value
** This macro halts on error. 'selector' must be a selector name registered in vm.h's
** selector_map_t and mapped in script.c.
*/


#define UGET_SELECTOR(_object_, _selector_) \
 ((guint16) read_selector(s, _object_, s->selector_map._selector_))
/* Retrieves an unsigned selector value from an object
** Parameters: (heap_ptr) object: The address of the object which the selector should be read from
**             (selector_name) selector: The selector to read
** Returns   : (guint16) The selector value
** This macro halts on error. 'selector' must be a selector name registered in vm.h's
** selector_map_t and mapped in script.c.
*/


#define PUT_SELECTOR(_object_, _selector_, _value_)\
 write_selector(s, _object_, s->selector_map._selector_, _value_)
/* Writes a selector value to an object
** Parameters: (heap_ptr) object: The address of the object which the selector should be written to
**             (selector_name) selector: The selector to read
**             (gint16) value: The value to write
** Returns   : (void)
** This macro halts on error. 'selector' must be a selector name registered in vm.h's
** selector_map_t and mapped in script.c.
*/


#define INV_SEL(_object_, _selector_, _noinvalid_) \
  s, ##_object_,  s->selector_map.##_selector_, ##_noinvalid_, funct_nr, argp, argc
/* Kludge for use with incoke_selector(). Used for compatibility with compilers that can't
** handle vararg macros.
*/


/* functions used internally by macros */
int
read_selector(state_t *s, heap_ptr object, int selector_id);
void
write_selector(state_t *s, heap_ptr object, int selector_id, int value);
int
invoke_selector(state_t *s, heap_ptr object, int selector_id, int noinvalid, int kfunct,
		heap_ptr k_argp, int k_argc, int argc, ...);



/******************** Text functionality ********************/
char *
kernel_lookup_text(state_t *s, int address, int index);
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
is_object(state_t *s, heap_ptr offset);
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
kernel_oops(state_t *s, char *file, int line, char *reason);
/* Halts script execution and informs the user about an internal kernel error or failed assertion
** Paramters: (state_t *) s: The state to use
**            (char *) file: The file the oops occured in
**            (int) line: The line the oops occured in
**            (char *) reason: Reason for the kernel oops
*/




/******************** Priority macros ********************/

#define VIEW_PRIORITY(y) (((y) < s->priority_first)? 0 : (((y) > s->priority_last)? 15 : 1\
	+ ((((y) - s->priority_first) * 15) / (s->priority_last - s->priority_first))))

#define PRIORITY_BAND_FIRST(nr) ((((nr) == 0)? 0 :  \
        ((s->priority_first) + ((nr) * (s->priority_last - s->priority_first)) / 15))-10)






/******************** Dynamic view list functions ********************/

void
_k_dyn_view_list_prepare_change(state_t *s);
     /* Removes all views in anticipation of a new window or text */
void
_k_dyn_view_list_accept_change(state_t *s);
     /* Redraws all views after a new window or text was added */




/******************** Sound functions ********************/

void
process_sound_events(state_t *s); /* Get all sound events, apply their changes to the heap */




/******************** Constants ********************/

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


/* Sound status */
#define _K_SOUND_STATUS_STOPPED 0
#define _K_SOUND_STATUS_INITIALIZED 1
#define _K_SOUND_STATUS_PLAYING 2
#define _K_SOUND_STATUS_PAUSED 3


/* List node addresses */
#define LIST_FIRST_NODE 0
#define LIST_PREVIOUS_NODE 0
#define LIST_LAST_NODE 2
#define LIST_NEXT_NODE 2
#define LIST_NODE_KEY 4
#define LIST_NODE_VALUE 6
/* 'previous' and 'next' nodes for lists mean 'first' and 'last' node, respectively */

/******************** Kernel functions ********************/

/* Generic description: */

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

void kLoad(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kUnLoad(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGameIsRestarting(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kNewList(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGetSaveDir(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGetCWD(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSetCursor(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kFindKey(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kNewNode(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kAddToFront(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kAddToEnd(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kShow(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kPicNotValid(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kRandom(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kAbs(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSqrt(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kOnControl(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kHaveMouse(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGetAngle(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGetDistance(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kLastNode(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kFirstNode(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kNextNode(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kPrevNode(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kNodeValue(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kClone(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDisposeClone(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kScriptID(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kMemoryInfo(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDrawPic(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDisposeList(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDisposeScript(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGetPort(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSetPort(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kNewWindow(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDisposeWindow(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kIsObject(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kFormat(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDrawStatus(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDrawMenuBar(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kAddMenu(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSetMenu(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kAddToPic(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kCelWide(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kCelHigh(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDisplay(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kAnimate(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGetTime(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDeleteKey(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kStrLen(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGetFarText(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kStrEnd(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kStrCat(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kStrCmp(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kStrCpy(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kStrAt(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kReadNumber(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDrawControl(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kNumCels(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kNumLoops(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kTextSize(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kInitBresen(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDoBresen(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kCanBeHere(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDrawCel(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDirLoop(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kCoordPri(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kPriCoord(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kValidPath(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kRespondsTo(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kFOpen(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kFPuts(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kFGets(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kFClose(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kTimesSin(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kTimesCos(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kCosMult(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSinMult(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kMapKeyToDir(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGlobalToLocal(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kLocalToGlobal(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kWait(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kCosDiv(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSinDiv(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kBaseSetter(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kParse(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kShakeScreen(state_t *s, int funct_nr, int argc, heap_ptr argp);
#ifdef _WIN32
void kDeviceInfo_Win32(state_t *s, int funct_nr, int argc, heap_ptr argp);
#else
void kDeviceInfo_Unix(state_t *s, int funct_nr, int argc, heap_ptr argp);
#endif
void kHiliteControl(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kRestartGame(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSaid(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kEditControl(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kDoSound(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSetSynonyms(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGraph(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGetEvent(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGetMenu(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kMenuSelect(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kCheckFreeSpace(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kFlushResources(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kGetSaveFiles(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSetDebug(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSetJump(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kCheckSaveGame(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSaveGame(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kRestoreGame(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kEmptyList(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kAppendAfter(state_t *s, int funct_nr, int argc, heap_ptr argp);
void kSetNowSeen(state_t *s, int funct_nr, int argc, heap_ptr argp);
void k_Unknown(state_t *s, int funct_nr, int argc, heap_ptr argp);
/* The Unknown/Unnamed kernel function */
void kstub(state_t *s, int funct_nr, int argc, heap_ptr argp);
/* for unimplemented kernel functions */
void kNOP(state_t *s, int funct_nr, int argc, heap_ptr argp);
/* for kernel functions that don't do anything */

typedef struct {
  char *functname; /* String representation of the kernel function as in script.999 */
  kfunct *kernel_function; /* The actual function */
} sci_kernel_function_t;

extern sci_kernel_function_t kfunct_mappers[];


/******************** Portability kludges ********************/

#ifdef _WIN32
int sci_ffs(int _mask);
#else
#define sci_ffs ffs
#endif


#endif /* _SCI_KERNEL_H_ */
