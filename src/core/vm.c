/***************************************************************************
 vm.c Copyright (C) 1999 Christoph Reichenbach


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
#include <versions.h>
#include <kdebug.h>
#include <sys/resource.h>

/* #define VM_DEBUG_SEND */


int script_debug_flag = 0; /* Defaulting to running mode */
int script_abort_flag = 0; /* Set to 1 to abort execution */
int script_error_flag = 0; /* Set to 1 if an error occured, reset each round by the VM */
int script_checkloads_flag = 0; /* Print info when scripts get (un)loaded */
int script_step_counter = 0; /* Counts the number of steps executed */

extern int _debug_step_running; /* scriptdebug.c */
extern int _debug_seeking; /* scriptdebug.c */


static calls_struct_t *send_calls = NULL;
static int send_calls_allocated = 0;
static int bp_flag = 0;


int
script_error(state_t *s, char *file, int line, char *reason)
{
  sciprintf("Script error in file %s, line %d: %s\n", file, line, reason);
  script_debug_flag = script_error_flag = 1;
  return 0;
}

inline void putInt16(byte *addr, word value)
{
  addr[0] = value &0xff;
  addr[1] = value >> 8;
}

inline heap_ptr
get_class_address(state_t *s, int classnr)
{
  heap_ptr scriptpos = *(s->classtable[classnr].scriptposp);

  if (!scriptpos) {
    script_instantiate(s, s->classtable[classnr].script);
    scriptpos = *(s->classtable[classnr].scriptposp);
  }

  return scriptpos + s->classtable[classnr].class_offset;
}

/* Operating on the stack */
#define PUSH(v) putInt16(s->heap + (((xs->sp) += 2)) - 2, (v))
#define POP() ((gint16)(getInt16(s->heap + ((xs->sp)-=2))))

/* Getting instruction parameters */
#define GET_OP_BYTE() ((guint8) s->heap[(xs->pc)++])
#define GET_OP_WORD() (getUInt16(s->heap + ((xs->pc) += 2) - 2))
#define GET_OP_FLEX() ((opcode & 1)? GET_OP_BYTE() : GET_OP_WORD())
#define GET_OP_SIGNED_BYTE() ((gint8)(s->heap[(xs->pc)++]))
#define GET_OP_SIGNED_WORD() ((getInt16(s->heap + ((xs->pc) += 2) - 2)))
#define GET_OP_SIGNED_FLEX() ((opcode & 1)? GET_OP_SIGNED_BYTE() : GET_OP_SIGNED_WORD())

#define GET_HEAP(address) ((((guint16)(address)) < 800)? \
script_error(s, __FILE__, __LINE__, "Heap address space violation on read")  \
: getInt16(s->heap + ((guint16)(address))))
/* Reads a heap value if allowed */

#define UGET_HEAP(address) ((((guint16)(address)) < 800)? \
script_error(s, __FILE__, __LINE__, "Heap address space violation on read")   \
: getUInt16(s->heap + ((guint16)(address))))
/* Reads an unsigned heap value if allowed */

#define PUT_HEAP(address, value) if (((guint16)(address)) < 800) \
script_error(s, __FILE__, __LINE__, "Heap address space violation on write");        \
else { s->heap[(guint16)(address)] = (value) &0xff;               \
 s->heap[((guint16)(address)) + 1] = ((value) >> 8) & 0xff;}
/* Sets a heap value if allowed */

#define CLASS_ADDRESS(classnr) (((classnr < 0) || (classnr >= s->classtable_size)) ?              \
				0 /* Error condition */ : get_class_address(s, classnr))

#define OBJ_SPECIES(address) GET_HEAP((address) + SCRIPT_SPECIES_OFFSET)
/* Returns an object's species */

#define OBJ_SUPERCLASS(address) GET_HEAP((address) + SCRIPT_SUPERCLASS_OFFSET)
/* Returns an object's superclass */

#define OBJ_ISCLASS(address) (GET_HEAP((address) + SCRIPT_INFO_OFFSET) & SCRIPT_INFO_CLASS)
/* Is true if the object at the address is a class */


inline exec_stack_t *
execute_method(state_t *s, word script, word pubfunct, heap_ptr sp,
	       heap_ptr calling_obj, word argc, heap_ptr argp)
{

  heap_ptr tableaddress;
  heap_ptr scriptpos;
  int exports_nr;

  if (s->scripttable[script].heappos == 0) /* Script not present yet? */
      script_instantiate(s, script);

  scriptpos = s->scripttable[script].heappos;
  tableaddress = s->scripttable[script].export_table_offset;

  if (tableaddress == 0) {
    sciprintf("Error: Script 0x%x has no exports table; call[be] not possible\n", script);
    script_error_flag = script_debug_flag = 1;
    return NULL;
  }

  exports_nr = GET_HEAP(tableaddress);

  tableaddress += 2; /* Points to the first actual function pointer */
  if (pubfunct >= exports_nr) {
    sciprintf("Request for invalid exported function 0x%x of script 0x%x\n", pubfunct, script);
    script_error_flag = script_debug_flag = 1;
    return NULL;
  }

  /* Check if a breakpoint is set on this method */
  if (s->have_bp & BREAK_EXPORT)
  {
    breakpoint_t *bp;
    gint32 bpaddress;

    bpaddress = (script << 16 | pubfunct);

    bp = s->bp_list;
    while (bp)
    {
      if (bp->type == BREAK_EXPORT && bp->data.address == bpaddress)
      {
        sciprintf ("Break on script %d, export %d\n", script, pubfunct);
        script_debug_flag = 1;
        bp_flag = 1;
        break;
      }
      bp = bp->next;
    }
  }

  return 
    add_exec_stack_entry(s, scriptpos + GET_HEAP(tableaddress + (pubfunct * 2)), sp, 
			 calling_obj, argc, argp, -1, calling_obj, s->execution_stack_pos);
}



exec_stack_t *
send_selector(state_t *s, heap_ptr send_obj, heap_ptr work_obj,
	      heap_ptr sp, int framesize, word restmod, heap_ptr argp)
     /* send_obj and work_obj are equal for anything but 'super' */
     /* Returns a pointer to the TOS exec_stack element */
{
  heap_ptr lookupresult;
  int selector;
  int argc;
  int i;
  int origin = s->execution_stack_pos; /* Origin: Used for debugging */
  exec_stack_t *retval = s->execution_stack + s->execution_stack_pos;
  /* We return a pointer to the new active exec_stack_t */

  /* The selector calls we catch are stored below: */
  int send_calls_nr = -1;

  framesize += restmod * 2;

  if (GET_HEAP(send_obj + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    sciprintf("Send: No object at %04x!\n", send_obj);
    script_error_flag = script_debug_flag = 1;
    return NULL;
  }

  while (framesize > 0) {

    selector = GET_HEAP(argp);

    argp += 2;
    argc = GET_HEAP(argp);

    if (argc > 0x500){ /* More arguments than the stack could possibly accomodate for */
      script_error(s, __FILE__, __LINE__, "More than 0x500 arguments to function call\n");
      return NULL;
    }

    /* Check if a breakpoint is set on this method */
    if (s->have_bp & BREAK_SELECTOR)
    {
      breakpoint_t *bp;
      char method_name [256];

      sprintf (method_name, "%s::%s",
        s->heap + getUInt16 (s->heap + send_obj + SCRIPT_NAME_OFFSET),
        s->selector_names [selector]);

      bp = s->bp_list;
      while (bp)
      {
        if (bp->type == BREAK_SELECTOR && !strcmp (bp->data.name, method_name))
        {
          sciprintf ("Break on %s\n", method_name);
          script_debug_flag = 1;
          bp_flag = 1;
          break;
        }
        bp = bp->next;
      }
    }

#ifdef VM_DEBUG_SEND
sciprintf("Send to selector %04x (%s):", selector, s->selector_names[selector]);
#endif /* VM_DEBUG_SEND */

    if (++send_calls_nr == (send_calls_allocated - 1))
      send_calls = g_realloc(send_calls, sizeof(calls_struct_t) * (send_calls_allocated *= 2));

    switch (lookup_selector(s, send_obj, selector, &lookupresult)) {

    case SELECTOR_NONE:
      sciprintf("Send to invalid selector 0x%x of object at 0x%x\n", 0xffff & selector, send_obj);
      script_error_flag = script_debug_flag = 1;
      --send_calls_nr;
      break;

    case SELECTOR_VARIABLE:

#ifdef VM_DEBUG_SEND
sciprintf("Varselector: ");
if (argc)
  sciprintf("Write %04x\n", UGET_HEAP(argp + 2));
else
  sciprintf("Read\n");
#endif /* VM_DEBUG_SEND */

      switch (argc) {
      case 0:   /* Read selector */
      case 1: { /* Argument is supplied -> Selector should be set */

	send_calls[send_calls_nr].address = lookupresult; /* register the call */
	send_calls[send_calls_nr].argp = argp;
	send_calls[send_calls_nr].argc = argc;
	send_calls[send_calls_nr].selector = selector;
	send_calls[send_calls_nr].type = EXEC_STACK_TYPE_VARSELECTOR; /* Register as a varselector */

      } break;
      default:
	--send_calls_nr;
	sciprintf("Send error: Variable selector %04x in %04x called with %04x params\n",
		  selector, send_obj, argc);
    }
      break;

    case SELECTOR_METHOD:
      argc += restmod;
      restmod = 0; /* Take care that the rest modifier is used only once */

#ifdef VM_DEBUG_SEND
sciprintf("Funcselector(");
 for (i = 0; i < argc; i++) {
   sciprintf("%04x", UGET_HEAP(argp + 2 + 2*i));
   if (i + 1 < argc)
     sciprintf(", ");
 }
sciprintf(")\n");
#endif /* VM_DEBUG_SEND */

      send_calls[send_calls_nr].address = lookupresult; /* register call */
      send_calls[send_calls_nr].argp = argp;
      send_calls[send_calls_nr].argc = argc;
      send_calls[send_calls_nr].selector = selector;
      send_calls[send_calls_nr].type = EXEC_STACK_TYPE_CALL;

      break;
    } /* switch(lookup_selector()) */


    framesize -= (4 + argc * 2);
    argp += argc * 2 + 2;
  }

  /* Iterate over all registered calls in the reverse order. This way, the first call is
  ** placed on the TOS; as soon as it returns, it will cause the second call to be executed.
  */
  for (; send_calls_nr >= 0; send_calls_nr--)
    if (send_calls[send_calls_nr].type == EXEC_STACK_TYPE_VARSELECTOR) /* Write/read variable? */
      retval = add_exec_stack_varselector(s, work_obj, send_calls[send_calls_nr].argc,
					  send_calls[send_calls_nr].argp,
					  send_calls[send_calls_nr].selector, 
					  send_calls[send_calls_nr].address, origin);

    else
      retval =
	add_exec_stack_entry(s, send_calls[send_calls_nr].address, sp, work_obj,
			     send_calls[send_calls_nr].argc, send_calls[send_calls_nr].argp,
			     send_calls[send_calls_nr].selector, send_obj, origin);


  /* Now check the TOS to execute all varselector entries */
  while (s->execution_stack[s->execution_stack_pos].type == EXEC_STACK_TYPE_VARSELECTOR) {

    /* varselector access? */
    if (s->execution_stack[s->execution_stack_pos].argc) { /* write? */

      word temp = GET_HEAP(s->execution_stack[s->execution_stack_pos].variables[VAR_PARAM] + 2);
      PUT_HEAP(s->execution_stack[s->execution_stack_pos].pc, temp);

    } else /* No, read */
      s->acc = GET_HEAP(s->execution_stack[s->execution_stack_pos].pc);

    --(s->execution_stack_pos);
  }

  retval = s->execution_stack + s->execution_stack_pos;
  return retval;
}


exec_stack_t *
add_exec_stack_varselector(state_t *s, heap_ptr objp, int argc, heap_ptr argp, int selector,
			   heap_ptr address, int origin)
{
  exec_stack_t *xstack = add_exec_stack_entry(s, address, 0, objp, argc, argp, selector,
					      objp, origin);
  /* Store selector address in pc */

  xstack->type = EXEC_STACK_TYPE_VARSELECTOR;

  return xstack;
}


exec_stack_t *
add_exec_stack_entry(state_t *s, heap_ptr pc, heap_ptr sp, heap_ptr objp, int argc, heap_ptr argp, 
		     int selector, heap_ptr sendp, int origin)
/* Returns new TOS element */
{
  exec_stack_t *xstack;

  if (!s->execution_stack)
    s->execution_stack = g_malloc(sizeof(exec_stack_t) * (s->execution_stack_size = 16));

  if (++(s->execution_stack_pos) == s->execution_stack_size) /* Out of stack space? */
    s->execution_stack = g_realloc(s->execution_stack,
				   sizeof(exec_stack_t) * (s->execution_stack_size += 8));

  /*  sciprintf("Exec stack: [%d/%d], origin %d, at %p\n", s->execution_stack_pos,
      s->execution_stack_size, origin, s->execution_stack); /* */

  xstack = s->execution_stack + s->execution_stack_pos;

  xstack->objp = objp;
  xstack->sendp = sendp;
  xstack->pc = pc;
  xstack->sp = sp;
  xstack->argc = argc;

  xstack->variables[VAR_GLOBAL] = s->global_vars; /* Global variables */

  if (objp) {
    xstack->variables[VAR_LOCAL] = /* Local variables */
      getUInt16(s->heap + objp + SCRIPT_LOCALVARPTR_OFFSET);
  } else
    xstack->variables[VAR_LOCAL] = 0; /* No object => no local variables */

  xstack->variables[VAR_TEMP] = sp; /* Temp variables */
  xstack->variables[VAR_PARAM] = argp; /* Parameters */

  PUT_HEAP(argp, argc); /* SCI code relies on the zeroeth argument to equal argc */

  /* Additional debug information */
  xstack->selector = selector;
  xstack->origin = origin;

  xstack->type = EXEC_STACK_TYPE_CALL; /* Normal call */

  return xstack;
}


void
run_vm(state_t *s, int restoring)
{
  gint16 temp, temp2, temp3;
  gint16 opparams[4]; /* opcode parameters */

  
  int restadjust = s->amp_rest; /* &rest adjusts the parameter count by this value */
  /* Current execution data: */
  int exec_stack_pos_temp; /* Used inside send-like commands */
  exec_stack_t *xs = s->execution_stack + s->execution_stack_pos;
  exec_stack_t *xs_new; /* Used during some operations */
  
  int old_execution_stack_base = s->execution_stack_base;

  if (restoring) {


  } else {

    s->execution_stack_base = s->execution_stack_pos;

  }
  
  /* SCI code reads the zeroeth argument to determine argc */
  PUT_HEAP(xs->variables[VAR_PARAM], xs->argc);

  while (1) {
    heap_ptr old_pc = xs->pc;
    heap_ptr old_sp = xs->sp;
    byte opcode, opnumber;
    int var_number; /* See description below */

    #ifdef _DOS
    /* poll the sound server! (dos needs this because it's singletasking) */
    if(s->sfx_driver) {
        if(s->sfx_driver->poll) {
            s->sfx_driver->poll();
        }
    }
    #endif

    if (s->execution_stack_pos_changed) {
      xs = s->execution_stack + s->execution_stack_pos;
      s->execution_stack_pos_changed = 0;
    }

    script_error_flag = 0; /* Set error condition to false */

    if (script_abort_flag)
      return; /* Emergency */

    if (script_debug_flag || sci_debug_flags)
    {
      script_debug(s, &(xs->pc), &(xs->sp), &(xs->variables[VAR_TEMP]),
		   &(xs->objp), &restadjust, bp_flag);
      bp_flag = 0;
    }
    /* Debug if this has been requested */

    opcode = GET_OP_BYTE(); /* Get opcode */

    if (xs->sp < s->stack_base)
      script_error(s, __FILE__, __LINE__, "Absolute stack underflow");

    if (xs->sp < xs->variables[VAR_TEMP])
      script_error(s, __FILE__, __LINE__, "Relative stack underflow");

    if (xs->sp >= s->stack_base + VM_STACK_SIZE)
      script_error(s, __FILE__, __LINE__, "Stack overflow");

    if (xs->pc < 800)
      script_error(s, __FILE__, __LINE__, "Program Counter gone astray");

    opnumber = opcode >> 1;

    for (temp = 0; formats[opnumber][temp]; temp++) /* formats comes from script.c */
      switch(formats[opnumber][temp]) {

      case Script_Byte: opparams[temp] = GET_OP_BYTE(); break;
      case Script_SByte: opparams[temp] = GET_OP_SIGNED_BYTE(); break;

      case Script_Word: opparams[temp] = GET_OP_WORD(); break;
      case Script_SWord: opparams[temp] = GET_OP_SIGNED_WORD(); break;

      case Script_Variable: 
      case Script_Property:
      case Script_Global:
      case Script_Param:
        opparams[temp] = GET_OP_FLEX(); break;

      case Script_SVariable: 
      case Script_SRelative:
        opparams[temp] = GET_OP_SIGNED_FLEX(); break;

      }

    switch (opnumber) {

    case 0x00: /* bnot */
      s->acc ^= 0xffff;
      break;

    case 0x01: /* add */
      s->acc = POP() + s->acc;
      break;

    case 0x02: /* sub */
      s->acc = POP() - s->acc;
      break;

    case 0x03: /* mul */
      s->acc = POP() * s->acc;
      break;

    case 0x04: /* div */
      s->acc = s->acc? POP() / s->acc : 0;
      break;

    case 0x05: /* mod */
      s->acc = (s->acc > 0)? POP() % s->acc : 0;
      break;

    case 0x06: /* shr */
      s->acc = ((guint16) POP()) >> s->acc;
      break;

    case 0x07: /* shl */
      s->acc = ((guint16) POP()) << s->acc;
      break;

    case 0x08: /* xor */
      s->acc = POP() ^ s->acc;
      break;

    case 0x09: /* and */
      s->acc = POP() & s->acc;
      break;

    case 0x0a: /* or */
      s->acc = POP() | s->acc;
      break;

    case 0x0b: /* neg */
      s->acc = -(s->acc);
      break;

    case 0x0c: /* not */
      s->acc = !s->acc;
      break;

    case 0x0d: /* eq? */
      s->prev = s->acc;
      s->acc = (POP() == s->acc);
      break;

    case 0x0e: /* ne? */
      s->prev = s->acc;
      s->acc = (POP() != s->acc);
      break;

    case 0x0f: /* gt? */
      s->prev = s->acc;
      s->acc = (POP() > s->acc);
      break;

    case 0x10: /* ge? */
      s->prev = s->acc;
      s->acc = (POP() >= s->acc);
      break;

    case 0x11: /* lt? */
      s->prev = s->acc;
      s->acc = (POP() < s->acc);
      break;

    case 0x12: /* le? */
      s->prev = s->acc;
      s->acc = (POP() <= s->acc);
      break;

    case 0x13: /* ugt? */
      s->prev = s->acc;
      s->acc = ((guint16)(POP()) > ((guint16)(s->acc)));
      break;

    case 0x14: /* uge? */
      s->prev = s->acc;
      s->acc = ((guint16)(POP()) >= ((guint16)(s->acc)));
      break;

    case 0x15: /* ult? */
      s->prev = s->acc;
      s->acc = ((guint16)(POP()) < ((guint16)(s->acc)));
      break;

    case 0x16: /* ule? */
      s->prev = s->acc;
      s->acc = ((guint16)(POP()) <= ((guint16)(s->acc)));
      break;

    case 0x17: /* bt */
      if (s->acc) xs->pc += opparams[0];
      break;

    case 0x18: /* bnt */
      if (!s->acc) xs->pc += opparams[0];
      break;

    case 0x19: /* jmp */
      xs->pc += opparams[0];
      break;

    case 0x1a: /* ldi */
      s->acc = opparams[0];
      break;

    case 0x1b: /* push */
      PUSH(s->acc);
      break;

    case 0x1c: /* pushi */
      PUSH(opparams[0]);
      break;

    case 0x1d: /* toss */
      POP();
      break;

    case 0x1e: /* dup */
      temp = POP();
      PUSH(temp);
      PUSH(temp); /* Gets compiler-optimized (I hope) */
      break;

    case 0x1f: /* link */
      xs->sp += (opparams[0]) * 2;
      break;

    case 0x20: /* call */
      temp = opparams[1] + 2 + (restadjust*2);
      temp2 = xs->sp;
      xs->sp -= temp;

      xs_new = add_exec_stack_entry(s, xs->pc + opparams[0], temp2, xs->objp,
				    GET_HEAP(xs->sp) + restadjust,
				    xs->sp, -1, xs->objp, s->execution_stack_pos);
      restadjust = 0; /* Used up the &rest adjustment */

      xs = xs_new;
      break;

    case 0x21: /* callk */
      xs->sp -= (opparams[1] + 2 + (restadjust * 2));
      s->amp_rest = 0; /* We just used up the restadjust, remember? */

      if (opparams[0] >= s->kernel_names_nr) {

	sciprintf("Invalid kernel function 0x%x requested\n", opparams[0]);
	script_debug_flag = script_error_flag = 1;

      } else {

	s->kfunct_table[opparams[0]]
	  (s, opparams[0], GET_HEAP(xs->sp) + restadjust, xs->sp + 2); /* Call kernel function */

	/* Calculate xs again: The kernel function might have spawned a new VM */
	xs = s->execution_stack + s->execution_stack_pos;
	restadjust = s->amp_rest;

      }

      break;

    case 0x22: /* callb */
      temp = xs->sp;
      xs->sp -= (opparams[1] + (restadjust * 2) + 2);
      xs_new = execute_method(s, 0, opparams[0], temp, xs->objp,
			      GET_HEAP(xs->sp) + restadjust,
			      xs->sp);
      restadjust = 0; /* Used up the &rest adjustment */

      if (xs_new) xs = xs_new;   /* in case of error, keep old stack */
      break;

    case 0x23: /* calle */
      temp = xs->sp;
      xs->sp -= opparams[2] + 2 + (restadjust*2);

      xs_new = execute_method(s, opparams[0], opparams[1], temp, xs->objp,
			      GET_HEAP(xs->sp) + restadjust, xs->sp);
      restadjust = 0; /* Used up the &rest adjustment */

      if (xs_new)
	xs = xs_new;   /* in case of error, keep old stack */
      break;

    case 0x24: /* ret */
      do {
	if (s->execution_stack_pos == s->execution_stack_base) { /* Have we reached the base? */

	  s->execution_stack_base = old_execution_stack_base; /* Restore stack base */
	  --(s->execution_stack_pos);

	  s->amp_rest = restadjust; /* Update &rest */
	  return; /* Hard return */
	}

	if (s->execution_stack[s->execution_stack_pos].type == EXEC_STACK_TYPE_VARSELECTOR)
	  /* varselector access? */
	  if (s->execution_stack[s->execution_stack_pos].argc) { /* write? */
	    word temp = GET_HEAP(s->execution_stack[s->execution_stack_pos].variables[VAR_PARAM] + 2);
	    PUT_HEAP(s->execution_stack[s->execution_stack_pos].pc, temp);
	  } else /* No, read */
	    s->acc = GET_HEAP(s->execution_stack[s->execution_stack_pos].pc);

	/* No we haven't, so let's do a soft return */
	--(s->execution_stack_pos);
	xs = s->execution_stack + s->execution_stack_pos;

      } while (s->execution_stack[s->execution_stack_pos].type == EXEC_STACK_TYPE_VARSELECTOR);
      /* Iterate over all varselector accesses */

      break;

    case 0x25: /* send */
      temp = xs->sp;
      xs->sp -= (opparams[0] + (restadjust * 2)); /* Adjust stack */

      xs_new = send_selector(s, s->acc, s->acc, temp, opparams[0],
			     restadjust, xs->sp);

      if (xs_new)
	xs = xs_new;

      restadjust = 0;

      break;

    case 0x28: /* class */
      s->acc = CLASS_ADDRESS(opparams[0]);
      break;

    case 0x2a: /* self */
      temp = xs->sp;
      xs->sp -= (opparams[0] + (restadjust * 2)); /* Adjust stack */

      xs_new = send_selector(s, xs->objp, xs->objp, temp,
			     opparams[0], restadjust, xs->sp);

      restadjust = 0;

      if (xs_new) xs = xs_new;
      break;

    case 0x2b: /* super */
      if ((opparams[0] < 0) || (opparams[0] >= s->classtable_size))
	script_error(s, __FILE__, __LINE__, "Invalid superclass in object");
      else {
	temp = xs->sp;
	xs->sp -= (opparams[1] + (restadjust * 2)); /* Adjust stack */

	xs_new = send_selector(s, *(s->classtable[opparams[0]].scriptposp)
			       + s->classtable[opparams[0]].class_offset, xs->objp,
			       temp, opparams[1], restadjust, xs->sp);
	restadjust = 0;

	if (xs_new) xs = xs_new;
      }

      break;

    case 0x2c: /* &rest */
      temp = opparams[0]; /* First argument */
      restadjust = xs->argc - temp + 1; /* +1 because temp counts the paramcount while argc doesn't */
      if (restadjust < 0)
	restadjust = 0;
      temp2 = xs->variables[VAR_PARAM] + (temp << 1);/* Pointer to the first argument to &restore */
      for (; temp <= xs->argc; temp++) {
	PUSH(getInt16(s->heap + temp2));
	temp2 += 2;
      }
      break;

    case 0x2d: /* lea */
      temp = opparams[0] >> 1;
      var_number = temp & 0x03; /* Get variable type */

      temp2 = xs->variables[var_number]; /* Get variable block offset */
      if (temp & 0x08)
	temp2 += s->acc; /* Add accumulator offset if requested */
      s->acc = temp2 + (opparams[1] << 1); /* Add index */
      break;

    case 0x2e: /* selfID */
      s->acc = xs->objp;
      break;

    case 0x30: /* pprev */
      PUSH(s->prev);
      break;

    case 0x31: /* pToa */
      s->acc = GET_HEAP(xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0]); 
      break;

    case 0x32: /* aTop */
      PUT_HEAP(xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0], s->acc);
      break;

    case 0x33: /* pTos */
      temp2 = GET_HEAP(xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0]);
      PUSH(temp2);
      break;

    case 0x34: /* sTop */
      temp = POP();
      PUT_HEAP(xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0], temp);
      break;

    case 0x35: /* ipToa */
      temp = xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0];
      s->acc = GET_HEAP(temp); 
      ++(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x36: /* dpToa */
      temp = xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0];
      s->acc = GET_HEAP(temp); 
      --(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x37: /* ipTos */
      temp2 = xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0];
      temp = GET_HEAP(temp2); 
      PUT_HEAP(temp2, temp + 1);
      break;

    case 0x38: /* dpTos */
      temp2 = xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0];
      temp = GET_HEAP(temp2);
      PUT_HEAP(temp2, temp - 1);
      break;

    case 0x39: /* lofsa */
      s->acc = opparams[0] + xs->pc;
      break;

    case 0x3a: /* lofss */
      PUSH(opparams[0] + xs->pc);
      break;

    case 0x3b: /* push0 */
      PUSH(0);
      break;

    case 0x3c: /* push1 */
      PUSH(1);
      break;

    case 0x3d: /* push2 */
      PUSH(2);
      break;

    case 0x3e: /* pushSelf */
      PUSH(xs->objp);
      break;

    case 0x40: /* lag */
    case 0x41: /* lal */
    case 0x42: /* lat */
    case 0x43: /* lap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + (opparams[0] << 1);
      s->acc = GET_HEAP(temp);
      break;

    case 0x44: /* lsg */
    case 0x45: /* lsl */
    case 0x46: /* lst */
    case 0x47: /* lsp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + (opparams[0] << 1);
      PUSH(GET_HEAP(temp));
      break;

    case 0x48: /* lagi */
    case 0x49: /* lali */
    case 0x4a: /* lati */
    case 0x4b: /* lapi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      s->acc = GET_HEAP(temp);
      break;

    case 0x4c: /* lsgi */
    case 0x4d: /* lsli */
    case 0x4e: /* lsti */
    case 0x4f: /* lspi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      PUSH(GET_HEAP(temp));
      break;

    case 0x50: /* sag */
    case 0x51: /* sal */
    case 0x52: /* sat */
    case 0x53: /* sap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + (opparams[0] << 1);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x54: /* ssg */
    case 0x55: /* ssl */
    case 0x56: /* sst */
    case 0x57: /* ssp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + (opparams[0] << 1);
      temp2 = POP();
      PUT_HEAP(temp, temp2);
      break;

    case 0x58: /* sagi */
    case 0x59: /* sali */
    case 0x5a: /* sati */
    case 0x5b: /* sapi */
      temp2 = POP();
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      PUT_HEAP(temp, temp2);
      s->acc = temp2;
      break;

    case 0x5c: /* ssgi */
    case 0x5d: /* ssli */
    case 0x5e: /* ssti */
    case 0x5f: /* sspi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      temp2 = POP();
      PUT_HEAP(temp, temp2);
      break;

    case 0x60: /* +ag */
    case 0x61: /* +al */
    case 0x62: /* +at */
    case 0x63: /* +ap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + (opparams[0] << 1);
      s->acc = GET_HEAP(temp);
      ++(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x64: /* +sg */
    case 0x65: /* +sl */
    case 0x66: /* +st */
    case 0x67: /* +sp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + (opparams[0] << 1);
      temp2 = GET_HEAP(temp);
      temp2++;
      PUT_HEAP(temp, temp2);
      PUSH(temp2);
      break;

    case 0x68: /* +agi */
    case 0x69: /* +ali */
    case 0x6a: /* +ati */
    case 0x6b: /* +api */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      s->acc = GET_HEAP(temp);
      ++(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x6c: /* +sgi */
    case 0x6d: /* +sli */
    case 0x6e: /* +sti */
    case 0x6f: /* +spi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      temp2 = GET_HEAP(temp);
      temp2++;
      PUT_HEAP(temp, temp2);
      PUSH(temp2);
      break;

    case 0x70: /* -ag */
    case 0x71: /* -al */
    case 0x72: /* -at */
    case 0x73: /* -ap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + (opparams[0] << 1);
      s->acc = GET_HEAP(temp);
      --(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x74: /* -sg */
    case 0x75: /* -sl */
    case 0x76: /* -st */
    case 0x77: /* -sp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + (opparams[0] << 1);
      temp2 = GET_HEAP(temp);
      temp2--;
      PUT_HEAP(temp, temp2);
      PUSH(temp2);
      break;

    case 0x78: /* -agi */
    case 0x79: /* -ali */
    case 0x7a: /* -ati */
    case 0x7b: /* -api */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      s->acc = GET_HEAP(temp);
      --(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x7c: /* -sgi */
    case 0x7d: /* -sli */
    case 0x7e: /* -sti */
    case 0x7f: /* -spi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      temp2 = GET_HEAP(temp);
      temp2--;
      PUT_HEAP(temp, temp2);
      PUSH(temp2);
      break;

    default:
      script_error(s, __FILE__, __LINE__, "Illegal opcode");

    } /* switch(opcode >> 1) */

    if (xs != s->execution_stack + s->execution_stack_pos) {
      sciprintf("Error: xs is stale; last command was %02x\n", opnumber);
    }

    if (script_error_flag) {
      _debug_step_running = 0; /* Stop multiple execution */
      _debug_seeking = 0; /* Stop special seeks */
      xs->pc = old_pc;
      xs->sp = old_sp;
    }
    else script_step_counter++;

  }
}


int
_lookup_selector_functions(state_t *s, heap_ptr obj, int selectorid, heap_ptr *address)
{
  int superclass; /* Possibly required for recursion */
  word superclasspos;
  int methodselectors = obj + GET_HEAP(obj + SCRIPT_FUNCTAREAPTR_OFFSET)
    + SCRIPT_FUNCTAREAPTR_MAGIC;   /* First method selector */
  int methodselector_nr = GET_HEAP(methodselectors - 2); /* Number of methods is stored there */
  int i;

  if (methodselectors < 800 || methodselectors > 0xffff) {
    sciprintf("Lookup selector functions: Method selector offset %04x of object at %04x is invalid\n",
	      methodselectors, obj);
    script_debug_flag = script_error_flag = 1;
    return -1;
  }

  for (i = 0; i < methodselector_nr * 2; i += 2)
    if (GET_HEAP(methodselectors + i) == selectorid) { /* Found it? */
      if (address)
	*address = GET_HEAP(methodselectors + i + 2 + (methodselector_nr * 2)); /* Get address */
      return SELECTOR_METHOD;
    }

  /* If we got here, we didn't find it. */

  superclass = OBJ_SUPERCLASS(obj);

  if (superclass == -1)
    return SELECTOR_NONE; /* No success. Trust on calling function to report error. */

  superclasspos = CLASS_ADDRESS(superclass);
  /*fprintf(stderr,"superclass at %04x\n", superclasspos); */

  return
    _lookup_selector_functions(s, superclasspos, selectorid, address); /* Recurse to superclass */
}


int
lookup_selector(state_t *s, heap_ptr obj, int selectorid, heap_ptr *address)
{
  heap_ptr pc = 0;
  int species = OBJ_SPECIES(obj);
  int heappos = CLASS_ADDRESS(species);
  int varselector_nr = GET_HEAP(heappos + SCRIPT_SELECTORCTR_OFFSET);
  /* Number of variable selectors */

  int i;
  for (i = 0; i < varselector_nr * 2; i += 2)
    if (GET_HEAP(heappos + SCRIPT_SELECTOR_OFFSET + varselector_nr * 2 + i) == selectorid)
      { /* Found it? */
	if (address)
	  *address = obj + SCRIPT_SELECTOR_OFFSET + i; /* Get object- relative address */
	return SELECTOR_VARIABLE; /* report success */
      }

  return
    _lookup_selector_functions(s, obj, selectorid, address);
  /* Call recursive function selector seeker */
}


/* Detects early SCI versions by their different script header */
void script_detect_early_versions(state_t *s)
{
  int old_version = 0;
  int c;
  resource_t *script;

  for (c = 0; c < 1000; c++) {
    if (script = findResource(sci_script, c)) {

      int id = getInt16(script->data);

      if (id > 15) {
	version_require_earlier_than(s, SCI_VERSION_FTU_NEW_SCRIPT_HEADER);
	return;
      }
    }
  }

}


int
script_init_state(state_t *s, sci_version_t version)
{
  resource_t *vocab996 = findResource(sci_vocab, 996);
  int i;
  int scriptnr;
  int seeker;
  int classnr;
  int size;
  resource_t *script;
  int magic_offset; /* For strange scripts in older SCI versions */

  s->max_version = SCI_VERSION(9,999,999); /* :-) */
  s->min_version = 0; /* Set no real limits */
  s->version = SCI_VERSION_DEFAULT_SCI0;

  script_detect_early_versions(s);

  s->_heap = heap_new();
  s->heap = s->_heap->start;
  s->acc = s->amp_rest = s->prev = 0;

  s->execution_stack = NULL;    /* Start without any execution stack */
  s->execution_stack_base = -1; /* No vm is running yet */
  s->execution_stack_pos = -1;   /* Start at execution stack position 0 */

  s->global_vars = 0; /* Set during launch time */

  if (!version) {
    s->version_lock_flag = 0;
  } else {
    s->version = version;
    s->version_lock_flag = 1; /* Lock version */
  }

  if (!vocab996)
    s->classtable_size = 20;
  else
    s->classtable_size = vocab996->length >> 2;

  s->classtable = g_new0(class_t, s->classtable_size);

  for (scriptnr = 0; scriptnr < 1000; scriptnr++) {
    int objtype;
    resource_t *script = findResource(sci_script, scriptnr);

    if (script) {

      size = getInt16(script->data);
      if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
        magic_offset = seeker = 2;
      else
	magic_offset = seeker = 0;

      do {

	while ((objtype = getInt16(script->data + seeker)) && (objtype != sci_obj_class)
	       && (seeker < script->length))
	  seeker += getInt16(script->data + seeker + 2);

	if (objtype) { /* implies sci_obj_class */

	  seeker -= SCRIPT_OBJECT_MAGIC_OFFSET; /* Adjust position; script home is base +8 bytes */

	  classnr = getInt16(script->data + seeker + 4 + SCRIPT_SPECIES_OFFSET);
	  if (classnr >= s->classtable_size) {

	    if (classnr >= SCRIPT_MAX_CLASSTABLE_SIZE) {
	      fprintf(stderr,"Invalid class number 0x%x in script.%d(0x%x), offset %04x\n",
		      classnr, scriptnr, scriptnr, seeker);
	      return 1;
	    }

	    s->classtable = g_realloc(s->classtable, sizeof(class_t) * (classnr + 1));
	    memset(&(s->classtable[s->classtable_size]), 0,
		   sizeof(class_t) * (1 + classnr - s->classtable_size)); /* Clear after resize */

	    s->classtable_size = classnr + 1; /* Adjust maximum number of entries */
	  }

	  s->classtable[classnr].class_offset = seeker + 4 - magic_offset;
	  s->classtable[classnr].script = scriptnr;
	  s->classtable[classnr].scriptposp = &(s->scripttable[scriptnr].heappos);

	  seeker += SCRIPT_OBJECT_MAGIC_OFFSET; /* Re-adjust position */

	  seeker += getInt16(script->data + seeker + 2); /* Move to next */
	}

      } while (objtype != sci_obj_terminator);

    }
  }

  return 0;
}


void
script_free_state(state_t *s)
{
  int i;

  /* FIXME: file handles will NOT be closed under DOS. DJGPP generates an
     exception fault whenever you try to close a never-opened file */

  sciprintf("Freeing state-dependant data\n");
  for (i = 0; i < MAX_HUNK_BLOCKS; i++)
      if (s->hunk[i].size) {
	  g_free(s->hunk[i].data);
	  s->hunk[i].size = 0;
      }

  for (i = 3; i < MAX_PORTS; i++) /* Ports 0,1,2 are fixed */
    if (s->ports[i]) {
      g_free(s->ports[i]);
      s->ports[i] = 0;
    }

  if (s->pic_views_nr)
  {
    g_free(s->pic_views);
    s->pic_views = NULL;
  }
  if (s->dyn_views_nr)
  {
    g_free(s->dyn_views);
    s->dyn_views = NULL;
  }

  /* Close all opened file handles */
  for (i = 1; i < s->file_handles_nr; i++)
    if (s->file_handles[i])
    #ifndef _DOS
      fclose(s->file_handles[i]);
    #endif

  g_free(s->file_handles);

  heap_del(s->_heap);

  menubar_free(s->menubar);

  g_free(s->classtable);
}


heap_ptr
script_instantiate(state_t *s, int script_nr)
{
  resource_t *script = findResource(sci_script, script_nr);
  heap_ptr pos;
  int objtype;
  unsigned int objlength;
  heap_ptr script_basepos;
  int magic_pos_adder; /* Usually 0; 2 for older SCI versions */
  if (!script) {
    sciprintf("Script 0x%x requested but not found\n", script_nr);
    /*    script_debug_flag = script_error_flag = 1; */
    return 0;
  }

  if (s->scripttable[script_nr].heappos) { /* Is it already loaded? */
    s->scripttable[script_nr].lockers++; /* Required by another script */
    return s->scripttable[script_nr].heappos;
  }

  script_basepos = heap_allocate(s->_heap, script->length);

  if (!script_basepos) {
    sciprintf("Not enough heap space for script size 0x%x of script 0x%x, has 0x%x\n",
	      script->length, script_nr, heap_largest(s->_heap));
    script_debug_flag = script_error_flag = 1;
    return 0;
  }

  s->scripttable[script_nr].heappos = script_basepos + 2;
  /* Set heap position (beyond the size word) */
  s->scripttable[script_nr].lockers = 1; /* Locked by one */
  s->scripttable[script_nr].export_table_offset = 0;
  s->scripttable[script_nr].synonyms_offset = 0;
  s->scripttable[script_nr].synonyms_nr = 0;
  s->scripttable[script_nr].localvar_offset = 0;

  if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER) {
    int locals_size = getUInt16(script->data)*2;
    s->scripttable[script_nr].localvar_offset=heap_allocate(s->_heap,locals_size);
    sciprintf( "Old SCI version; assuming locals size %d\n", locals_size); 
    /* There won't be a localvar block in this case */
    memcpy(s->heap + script_basepos + 2, script->data + 2, script->length -2);
    pos = script_basepos + 2;
    magic_pos_adder = 2;
  } else {
    memcpy(s->heap + script_basepos + 2, script->data, script->length); /* Copy the script */
    script_basepos += 2; 
    magic_pos_adder = 0;
    pos = script_basepos;
  }

  /* Now do a first pass through the script objects to find the
  ** export table and local variable block 
  */

  objlength = 0;

  do {
    pos += objlength; /* Step over the last checked object */

    objtype = GET_HEAP(pos);
    objlength = GET_HEAP(pos+2);

    if (objtype == sci_obj_exports)
      s->scripttable[script_nr].export_table_offset = pos + 4; /* +4 is to step over the header */
    
    if (objtype == sci_obj_synonyms) {
      s->scripttable[script_nr].synonyms_offset = pos + 4; /* +4 is to step over the header */
      s->scripttable[script_nr].synonyms_nr = (objlength - 4) / 4;
    }
    
    if (objtype == sci_obj_localvars)
      s->scripttable[script_nr].localvar_offset = pos + 4; /* +4 is to step over the header */

  } while (objtype != 0);


  /* And now a second pass to adjust objects and class pointers, and the general pointers */
  pos = script_basepos + magic_pos_adder;

  objlength = 0;

  do {
    pos += objlength; /* Step over the last checked object */

    objtype = GET_HEAP(pos);
    objlength = GET_HEAP(pos+2);

    pos += 4; /* Step over header */

    if ((objtype == sci_obj_object) || (objtype == sci_obj_class)) { /* object or class? */
      int i;
      heap_ptr functarea;
      int functions_nr;
      int superclass;
      heap_ptr name_addr;

      pos -= SCRIPT_OBJECT_MAGIC_OFFSET; /* Get into home position */

      functarea = pos + GET_HEAP(pos + SCRIPT_FUNCTAREAPTR_OFFSET)
	+ SCRIPT_FUNCTAREAPTR_MAGIC;
      functions_nr = GET_HEAP(functarea - 2); /* Number of functions */
      superclass = OBJ_SUPERCLASS(pos); /* Get superclass */

      /*      fprintf(stderr,"functs (%x) area @ %x\n", functions_nr, functarea); */

      PUT_HEAP(pos + SCRIPT_LOCALVARPTR_OFFSET, s->scripttable[script_nr].localvar_offset);
      /* This sets the local variable pointer */

      functarea += 2 + functions_nr * 2;
      /* Move over the selector IDs to the actual addresses */

      for (i = 0; i < functions_nr * 2; i += 2) {
	heap_ptr functpos = GET_HEAP(functarea + i);
	PUT_HEAP(functarea + i, functpos + script_basepos); /* Adjust function pointer addresses */
      }

      if (superclass >= 0)
	script_instantiate(s, s->classtable[superclass].script);
      /* Recurse to assure that the superclass is available */

      pos += SCRIPT_OBJECT_MAGIC_OFFSET; /* Step back from home to base */
      
    } /* if object or class */
    else if (objtype == sci_obj_pointers) { /* A relocation table */
      int pointerc = GET_HEAP(pos);
      int i;

      for (i = 0; i < pointerc; i++) {
	int new_address = ((guint16) GET_HEAP(pos + 2 + i*2)) + script_basepos;
	int old_indexed_pointer;
	PUT_HEAP(pos + 2 + i*2, new_address); /* Adjust pointers. Not sure if this is needed. */
	old_indexed_pointer = ((guint16) GET_HEAP(new_address));
	PUT_HEAP(new_address, old_indexed_pointer + script_basepos);
	/* Adjust indexed pointer. */

      } /* For all indexed pointers pointers */

    }

    pos -= 4; /* Step back on header */

  } while ((objtype != 0) && ((pos - script_basepos) < script->length - 2));

  /*    if (script_nr == 0)   sci_hexdump(s->heap + script_basepos +2, script->length-2, script_basepos);*/
  return s->scripttable[script_nr].heappos;

}


void
script_uninstantiate(state_t *s, int script_nr)
{
  heap_ptr handle = s->scripttable[script_nr].heappos;
  heap_ptr pos;
  int objtype, objlength;

  if (!handle) {
    /*    sciprintf("Warning: unloading script 0x%x requested although not loaded\n", script_nr); */
    /* This is perfectly valid SCI behaviour */
    return;
  }

#if 0
  /* Make a pass over the object in order uninstantiate all superclasses */
  pos = handle;

  objlength = 0;

  sciprintf("Unlocking script.0x%x\n", script_nr);
  if (s->scripttable[script_nr].lockers == 0)
    return; /* This can happen during recursion */

  --(s->scripttable[script_nr].lockers); /* One less locker */

  do {
    pos += objlength; /* Step over the last checked object */

    objtype = GET_HEAP(pos);
    objlength = UGET_HEAP(pos+2);

    pos += 4; /* Step over header */

    if ((objtype == sci_obj_object) || (objtype == sci_obj_class)) { /* object or class? */
      int superclass;

      pos -= SCRIPT_OBJECT_MAGIC_OFFSET;

      superclass = OBJ_SUPERCLASS(pos); /* Get superclass */

      fprintf(stderr, "SuperClass = %04x from pos %04x\n", superclass, pos);

      if (superclass >= 0) {
	int superclass_script = s->classtable[superclass].script;

	if (superclass_script == script_nr) {
	  if (s->scripttable[script_nr].lockers)
	    --(s->scripttable[script_nr].lockers); /* Decrease lockers if this is us ourselves */
	} else
	  script_uninstantiate(s, superclass_script);
	/* Recurse to assure that the superclass lockers number gets decreased */
      }
      
      pos += SCRIPT_OBJECT_MAGIC_OFFSET;
    } /* if object or class */

    pos -= 4; /* Step back on header */
    
  } while (objtype != 0);

  if (s->scripttable[script_nr].lockers)
    return; /* if xxx.lockers > 0 */

  /* Otherwise unload it completely */
#endif /* 0 */
  /* Explanation: I'm starting to believe that this work is done by SCI itself. */

  s->scripttable[script_nr].heappos = 0;
  s->scripttable[script_nr].localvar_offset = 0;
  s->scripttable[script_nr].export_table_offset = 0;

  heap_free(s->_heap, handle - 2); /* Unallocate script on the heap */

  if (script_checkloads_flag)
    sciprintf("Unloaded script 0x%x.\n", script_nr);

  return;
}


int
game_init(state_t *s)
{
  heap_ptr stack_handle = heap_allocate(s->_heap, VM_STACK_SIZE);
  heap_ptr parser_handle = heap_allocate(s->_heap, PARSE_HEAP_SIZE);
  heap_ptr script0;
  heap_ptr game_obj; /* Address of the game object */
  heap_ptr game_init; /* Address of the init() method */
  heap_ptr functarea;
  resource_t *resource;
  int i, font_nr;

  s->synonyms = NULL;
  s->synonyms_nr = 0; /* No synonyms */

  /* Initialize script table */
  for (i = 0; i < 1000; i++)
    s->scripttable[i].heappos = 0;
  /* Initialize hunk data */
  for (i = 0; i < MAX_HUNK_BLOCKS; i++)
    s->hunk[i].size = 0;
  /* Initialize ports */
  memset(s->ports, 0, sizeof(port_t *) * MAX_PORTS);
  /* Initialize clone list */
  memset(&(s->clone_list), 0, sizeof(heap_ptr) * SCRIPT_MAX_CLONES);
  /* Initialize send_calls buffer */

  if (!send_calls_allocated)
    send_calls = g_new(calls_struct_t, send_calls_allocated = 16);

  if (!stack_handle) {
    sciprintf("game_init(): Insufficient heap space for stack\n");
    return 1;
  }

  if (!parser_handle) {
    sciprintf("script_init(): Insufficient heap space for parser word error block\n");
    return 1;
  }

  if (!(script0 = script_instantiate(s, 0))) {
    sciprintf("script_init(): Could not instantiate script 0\n");
    return 1;
  }

  s->successor = NULL; /* No successor */
  s->status_bar_text = NULL; /* Status bar is blank */

  fprintf(stderr," Script 0 at %04x\n", script0);

  /* Init parser */
  if (s->parser_words = vocab_get_words(&(s->parser_words_nr))) {
    s->parser_suffices = vocab_get_suffices(&(s->parser_suffices_nr));
    s->parser_branches = vocab_get_branches(&(s->parser_branches_nr));

    /* Mark parse tree as unused */
    s->parser_nodes[0].type = PARSE_TREE_NODE_LEAF;
    s->parser_nodes[0].content.value = 0;
  } else
    sciprintf("Assuming that this game does not use a parser.\n");

  s->restarting_flags = SCI_GAME_IS_NOT_RESTARTING;

  s->stack_base = stack_handle + 2;
  s->parser_base = parser_handle + 2;
  s->global_vars = s->scripttable[0].localvar_offset;
  /* Global variables are script 0's local variables */

  s->kernel_names = vocabulary_get_knames(&s->kernel_names_nr);
  s->opcodes = vocabulary_get_opcodes();

  if (!(s->selector_names = vocabulary_get_snames(NULL))) {
    sciprintf("script_init(): Could not retreive selector names (vocab.997)!\n");
    return 1;
  }

  for (s->selector_names_nr = 0; s->selector_names[s->selector_names_nr]; s->selector_names_nr++);
  /* Counts the number of selector names */

  script_map_selectors(s, &(s->selector_map));
  /* Maps a few special selectors for later use */

  s->file_handles_nr = 5;
  s->file_handles = g_new0(FILE *, s->file_handles_nr);
  /* Allocate memory for file handles */

  script_map_kernel(s);
  /* Maps the kernel functions */

  s->menubar = menubar_new(); /* Create menu bar */

  g_get_current_time(&(s->game_start_time)); /* Get start time */
  memcpy(&(s->last_wait_time), &(s->game_start_time), sizeof(GTimeVal));
  /* Use start time as last_wait_time */

  s->mouse_pointer = NULL; /* No mouse pointer */
  s->mouse_pointer_nr = -1; /* No mouse pointer resource */
  s->pointer_x = (320 / 2); /* With centered x coordinate */
  s->pointer_y = 150; /* And an y coordinate somewhere down the screen */
  s->last_pointer_x = 0;
  s->last_pointer_y = 0;
  s->last_pointer_size_x = 0;
  s->last_pointer_size_y = 0; /* No previous pointer */

  s->pic = alloc_empty_picture(SCI_RESOLUTION_320X200, SCI_COLORDEPTH_8BPP);
  s->pic_not_valid = 1; /* Picture is invalid */
  s->pic_is_new = 0;
  s->pic_visible_map = 0; /* Other values only make sense for debugging */
  s->animation_delay = 500; /* Used in kAnimate for pic openings */

  s->pic_views_nr = s->dyn_views_nr = 0;
  s->pic_views = 0; s->dyn_views = 0; /* No PicViews, no DynViews */

  memset(s->ports, sizeof(s->ports), 0); /* Set to no ports */

  s->wm_port.ymin = 10; s->wm_port.ymax = 199;
  s->wm_port.xmin = 0; s->wm_port.xmax = 319;
  s->wm_port.priority = 11;
  s->ports[0] = &(s->wm_port); /* Window Manager port */

  s->titlebar_port.ymin = 0; s->titlebar_port.ymax = 9;
  s->titlebar_port.xmin = 0; s->titlebar_port.xmax = 319;
  s->ports[1] = &(s->titlebar_port);

  s->picture_port.ymin = 10; s->picture_port.ymax = 199;
  s->picture_port.xmin = 0; s->picture_port.xmax = 319;
  s->ports[2] = &(s->picture_port); /* Background picture port */

  s->view_port = 0; /* Currently using the window manager port */

  s->priority_first = 42; /* Priority zone 0 ends here */
  s->priority_last = 200; /* The highest priority zone (15) starts here */

  s->debug_mode = 0x0; /* Disable all debugging */
  s->onscreen_console = 0; /* No onscreen console unless explicitly requested */

  s->bp_list = NULL; /* No breakpoints defined */
  s->have_bp = 0;

  srand(time(NULL)); /* Initialize random number generator */

  font_nr = -1;
  do {
    resource = findResource(sci_font, ++font_nr);
  } while ((!resource) && (font_nr < 999));

  if (!resource) {
    sciprintf("No text font was found.\n");
    return 1;
  }
  for (i = 0; i < 3; i++) {
      s->ports[i]->font = resource->data; /* let all ports default to the 'system' font */
      s->ports[i]->gray_text = 0;
      s->ports[i]->font_nr = font_nr;
      s->ports[i]->color = 0;
      s->ports[i]->bgcolor = -1; /* All ports should be transparent */
  }

  memset(s->hunk, sizeof(s->hunk), 0); /* Sets hunk to be unused */
  memset(s->clone_list, sizeof(s->clone_list), 0); /* No clones */

  s->save_dir = heap_allocate(s->_heap, MAX_HOMEDIR_SIZE + strlen(FREESCI_GAMEDIR)
			      + MAX_GAMEDIR_SIZE + 4); /* +4 for the three slashes and trailing \0 */

  game_obj = script0 + GET_HEAP(s->scripttable[0].export_table_offset + 2);
  /* The first entry in the export table of script 0 points to the game object */

  if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
        game_obj -= 2; /* Adjust for alternative header */

  if (GET_HEAP(game_obj + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    sciprintf("script_init(): Game object is not at 0x%x\n", game_obj);
    return 1;
  }

  s->game_name = s->heap + GET_HEAP(game_obj + SCRIPT_NAME_OFFSET);

  sciprintf(" Game designation is \"%s\"\n", s->game_name);

  if (strlen(s->game_name) >= MAX_GAMEDIR_SIZE) {

    s->game_name[MAX_GAMEDIR_SIZE - 1] = 0; /* Fix length with brute force */
    sciprintf(" Designation too long; was truncated to \"%s\"\n", s->game_name);

  }

  s->game_obj = game_obj;
  s->stack_handle = stack_handle;

  return 0;
}
  
int
game_run(state_t **_s)
{
  state_t *successor = NULL;
  state_t *s = *_s;
  int game_is_finished = 0;

  sciprintf(" Calling %s::play()\n", s->game_name);
  putInt16(s->heap + s->stack_base, s->selector_map.play); /* Call the play selector... */
  putInt16(s->heap + s->stack_base + 2, 0);                    /* ... with 0 arguments. */

  /* Now: Register the first element on the execution stack- */
  send_selector(s, s->game_obj, s->game_obj, s->stack_base + 2, 4, 0, s->stack_base);
  /* and ENGAGE! */

  do {
    save_ff(s->_heap); /* Save heap state */
    run_vm(s, (successor)? 1 : 0);

    if (s->restarting_flags & SCI_GAME_IS_RESTARTING_NOW) { /* Restart was requested? */

      g_free(s->execution_stack);
      s->execution_stack = NULL;
      s->execution_stack_pos = -1;
      s->execution_stack_pos_changed = 0;
      restore_ff(s->_heap); /* Restore old heap state */

      game_exit(s);
      script_free_state(s);
      script_init_state(s, s->version);
      game_init(s);

      sciprintf(" Restarting game\n");
      /*      putInt16(s->heap + s->stack_base, s->selector_map.replay); /* Call the replay selector */
      putInt16(s->heap + s->stack_base + 2, 0);
      send_selector(s, s->game_obj, s->game_obj, s->stack_base + 2, 4, 0, s->stack_base);

      script_abort_flag = 0;

      s->restarting_flags = SCI_GAME_WAS_RESTARTED;

    } else
      
      if (successor = s->successor) {
	script_abort_flag = 0;
	free(s);
	*_s = s = successor;
      } else

	game_is_finished = 1;
  } while (!game_is_finished);

  sciprintf(" Game::play() finished.\n");
  return 0;
}

int
game_exit(state_t *s)
{
  int i;
  breakpoint_t *bp, *bp_next;

  if (s->execution_stack)
    g_free(s->execution_stack);

  sciprintf("Freeing vocabulary...\n");
  vocabulary_free_snames(s->selector_names);
  vocabulary_free_knames(s->kernel_names);
  g_free(s->opcodes);
  g_free(s->kfunct_table);

  s->selector_names = NULL;
  s->kernel_names = NULL;
  s->opcodes = NULL;
  s->kfunct_table = NULL;
  /* Make sure to segfault if any of those are dereferenced */

  if (s->synonyms_nr) {
    free(s->synonyms);
    s->synonyms = NULL;
    s->synonyms_nr = 0;
  }

  sciprintf("Freeing graphics data...\n");
  free_picture(s->pic);

  sciprintf("Freeing miscellaneous data...\n");

  if (s->parser_words) {
    vocab_free_words(s->parser_words, s->parser_words_nr);
    vocab_free_suffices(s->parser_suffices, s->parser_suffices_nr);
    vocab_free_branches(s->parser_branches);
  }

  heap_free(s->_heap, s->stack_handle);
  heap_free(s->_heap, s->save_dir);
  heap_free(s->_heap, s->parser_base - 2);

  /* Free breakpoint list */
  bp = s->bp_list;
  while (bp)
  {
    bp_next = bp->next;
    if (bp->type == BREAK_SELECTOR) g_free (bp->data.name);
    g_free (bp);
    bp = bp_next;
  }
  s->bp_list = NULL;

  if (send_calls_allocated) {
    g_free(send_calls);
    send_calls_allocated = 0;
  }

  return 0;
}


void
version_require_earlier_than(state_t *s, sci_version_t version)
{
  if (s->version_lock_flag)
    return;

  if (version <= s->min_version) {
    sciprintf("Version autodetect conflict: Less than %d.%03d.%03d was requested, but "
	      "%d.%03d.%03d is the current minimum\n",
	      SCI_VERSION_MAJOR(version), SCI_VERSION_MINOR(version), SCI_VERSION_PATCHLEVEL(version),
	      SCI_VERSION_MAJOR(s->min_version), SCI_VERSION_MINOR(s->min_version),
	      SCI_VERSION_PATCHLEVEL(s->min_version));
    return;
  }
  else if (version < s->max_version) {
    s->max_version = version -1;
    if (s->max_version < s->version)
      s->version = s->max_version;
  }
}


void
version_require_later_than(state_t *s, sci_version_t version)
{
  if (s->version_lock_flag)
    return;

  if (version > s->max_version) {
    sciprintf("Version autodetect conflict: More than %d.%03d.%03d was requested, but less than"
	      "%d.%03d.%03d is required ATM\n",
	      SCI_VERSION_MAJOR(version), SCI_VERSION_MINOR(version), SCI_VERSION_PATCHLEVEL(version),
	      SCI_VERSION_MAJOR(s->max_version), SCI_VERSION_MINOR(s->max_version),
	      SCI_VERSION_PATCHLEVEL(s->max_version));
    return;
  }
  else if (version > s->min_version) {
    s->min_version = version;
    if (s->min_version > s->version)
      s->version = s->min_version;
  }
}

sci_version_t
version_parse(char *vn)
{
  int major = *vn - '0'; /* One version digit */
  int minor = atoi(vn + 2);
  int patchlevel = atoi(vn + 6);

  return SCI_VERSION(major, minor, patchlevel);
}
