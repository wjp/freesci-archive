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



#include <engine.h>
#include <versions.h>
#include <kdebug.h>

#ifndef _WIN32
#include <sys/resource.h>
#endif

/*#define VM_DEBUG_SEND*/


int script_abort_flag = 0; /* Set to 1 to abort execution */
int script_error_flag = 0; /* Set to 1 if an error occured, reset each round by the VM */
int script_checkloads_flag = 0; /* Print info when scripts get (un)loaded */
int script_step_counter = 0; /* Counts the number of steps executed */

extern int _debug_step_running; /* scriptdebug.c */
extern int _debug_seeking; /* scriptdebug.c */


calls_struct_t *send_calls = NULL;
int send_calls_allocated = 0;
int bp_flag = 0;


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
  if (!s->classtable[classnr].scriptposp) {
    sciprintf("Attempt to dereference class %x, which doesn't exist\n", classnr);
    script_error_flag = script_debug_flag = 1;
    return 0;
  } else {
    heap_ptr scriptpos = *(s->classtable[classnr].scriptposp);

    if (!scriptpos) {
      script_instantiate(s, s->classtable[classnr].script, 1);
      scriptpos = *(s->classtable[classnr].scriptposp);
    }

    return scriptpos + s->classtable[classnr].class_offset;
  }
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

#define CLASS_ADDRESS(classnr) (((classnr < 0) || (classnr >= s->classtable_size)) ?              \
				0 /* Error condition */ : get_class_address(s, classnr))

#define OBJ_SPECIES(address) GET_HEAP((address) + SCRIPT_SPECIES_OFFSET)
/* Returns an object's species */

#define OBJ_SUPERCLASS(address) GET_HEAP((address) + SCRIPT_SUPERCLASS_OFFSET)
/* Returns an object's superclass */

#define OBJ_ISCLASS(address) (GET_HEAP((address) + SCRIPT_INFO_OFFSET) & SCRIPT_INFO_CLASS)
/* Is true if the object at the address is a class */


/*inline*/ exec_stack_t *
execute_method(state_t *s, word script, word pubfunct, heap_ptr sp,
	       heap_ptr calling_obj, word argc, heap_ptr argp)
{

  heap_ptr tableaddress;
  heap_ptr scriptpos;
  int exports_nr;
  int magic_ofs;

  if (s->scripttable[script].heappos == 0) /* Script not present yet? */
      script_instantiate(s, script, 1);

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

  if (s->version<SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
    magic_ofs=2; else
    magic_ofs=0;

  return 
    add_exec_stack_entry(s, scriptpos + GET_HEAP(tableaddress + (pubfunct * 2)) - magic_ofs, sp, 
			 calling_obj, argc, argp, -1, calling_obj, s->execution_stack_pos,
			 s->scripttable[script].localvar_offset);
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
			     send_calls[send_calls_nr].selector, send_obj, origin, 0);


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
					      objp, origin, 0);
  /* Store selector address in pc */

  xstack->type = EXEC_STACK_TYPE_VARSELECTOR;

  return xstack;
}


exec_stack_t *
add_exec_stack_entry(state_t *s, heap_ptr pc, heap_ptr sp, heap_ptr objp, int argc, heap_ptr argp, 
		     int selector, heap_ptr sendp, int origin, int localvarp)
/* Returns new TOS element */
{
  exec_stack_t *xstack;

  if (!s->execution_stack)
    s->execution_stack = g_malloc(sizeof(exec_stack_t) * (s->execution_stack_size = 16));

  if (++(s->execution_stack_pos) == s->execution_stack_size) /* Out of stack space? */
    s->execution_stack = g_realloc(s->execution_stack,
				   sizeof(exec_stack_t) * (s->execution_stack_size += 8));

  /*  sciprintf("Exec stack: [%d/%d], origin %d, at %p\n", s->execution_stack_pos,
      s->execution_stack_size, origin, s->execution_stack); */

  xstack = s->execution_stack + s->execution_stack_pos;

  xstack->objp = objp;
  xstack->sendp = sendp;
  xstack->pc = pc;
  xstack->sp = sp;
  xstack->argc = argc;

  xstack->variables[VAR_GLOBAL] = s->global_vars; /* Global variables */

  if (localvarp) {
    xstack->variables[VAR_LOCAL] = localvarp;
  } else
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
  gint16 temp, temp2;
  guint16 utemp, utemp2;
  gint16 opparams[4]; /* opcode parameters */

  
  int restadjust = s->amp_rest; /* &rest adjusts the parameter count by this value */
  /* Current execution data: */
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
				    xs->sp, -1, xs->objp, s->execution_stack_pos, 0);
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

	if (s->execution_stack[s->execution_stack_pos].type == EXEC_STACK_TYPE_VARSELECTOR) {
	  /* varselector access? */
	  if (s->execution_stack[s->execution_stack_pos].argc) { /* write? */
	    word temp = GET_HEAP(s->execution_stack[s->execution_stack_pos].variables[VAR_PARAM] + 2);
	    PUT_HEAP(s->execution_stack[s->execution_stack_pos].pc, temp);
	  } else /* No, read */
	    s->acc = GET_HEAP(s->execution_stack[s->execution_stack_pos].pc);
	}

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
      utemp = opparams[0]; /* First argument */
      restadjust = xs->argc - utemp + 1; /* +1 because utemp counts the paramcount while argc doesn't */
      if (restadjust < 0)
	restadjust = 0;
      utemp2 = xs->variables[VAR_PARAM] + (utemp << 1);/* Pointer to the first argument to &restore */
      for (; utemp <= xs->argc; utemp++) {
	PUSH(getInt16(s->heap + utemp2));
	utemp2 += 2;
      }
      break;

    case 0x2d: /* lea */
      utemp = opparams[0] >> 1;
      var_number = utemp & 0x03; /* Get variable type */

      utemp2 = xs->variables[var_number]; /* Get variable block offset */
      if (utemp & 0x08)
	utemp2 += (s->acc << 1); /* Add accumulator offset if requested */
      s->acc = utemp2 + (opparams[1] << 1); /* Add index */
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
      utemp = xs->variables[var_number] + (opparams[0] << 1);
      s->acc = GET_HEAP(utemp);
      break;

    case 0x44: /* lsg */
    case 0x45: /* lsl */
    case 0x46: /* lst */
    case 0x47: /* lsp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + (opparams[0] << 1);
      PUSH(GET_HEAP(utemp));
      break;

    case 0x48: /* lagi */
    case 0x49: /* lali */
    case 0x4a: /* lati */
    case 0x4b: /* lapi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      s->acc = GET_HEAP(utemp);
      break;

    case 0x4c: /* lsgi */
    case 0x4d: /* lsli */
    case 0x4e: /* lsti */
    case 0x4f: /* lspi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      PUSH(GET_HEAP(utemp));
      break;

    case 0x50: /* sag */
    case 0x51: /* sal */
    case 0x52: /* sat */
    case 0x53: /* sap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + (opparams[0] << 1);
      PUT_HEAP(utemp, s->acc);
      break;

    case 0x54: /* ssg */
    case 0x55: /* ssl */
    case 0x56: /* sst */
    case 0x57: /* ssp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + (opparams[0] << 1);
      utemp2 = POP();
      PUT_HEAP(utemp, utemp2);
      break;

    case 0x58: /* sagi */
    case 0x59: /* sali */
    case 0x5a: /* sati */
    case 0x5b: /* sapi */
      utemp2 = POP();
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      PUT_HEAP(utemp, utemp2);
      s->acc = utemp2;
      break;

    case 0x5c: /* ssgi */
    case 0x5d: /* ssli */
    case 0x5e: /* ssti */
    case 0x5f: /* sspi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      utemp2 = POP();
      PUT_HEAP(utemp, utemp2);
      break;

    case 0x60: /* +ag */
    case 0x61: /* +al */
    case 0x62: /* +at */
    case 0x63: /* +ap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + (opparams[0] << 1);
      s->acc = GET_HEAP(utemp);
      ++(s->acc);
      PUT_HEAP(utemp, s->acc);
      break;

    case 0x64: /* +sg */
    case 0x65: /* +sl */
    case 0x66: /* +st */
    case 0x67: /* +sp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + (opparams[0] << 1);
      utemp2 = GET_HEAP(utemp);
      utemp2++;
      PUT_HEAP(utemp, utemp2);
      PUSH(utemp2);
      break;

    case 0x68: /* +agi */
    case 0x69: /* +ali */
    case 0x6a: /* +ati */
    case 0x6b: /* +api */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      s->acc = GET_HEAP(utemp);
      ++(s->acc);
      PUT_HEAP(utemp, s->acc);
      break;

    case 0x6c: /* +sgi */
    case 0x6d: /* +sli */
    case 0x6e: /* +sti */
    case 0x6f: /* +spi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      utemp2 = GET_HEAP(utemp);
      utemp2++;
      PUT_HEAP(utemp, utemp2);
      PUSH(utemp2);
      break;

    case 0x70: /* -ag */
    case 0x71: /* -al */
    case 0x72: /* -at */
    case 0x73: /* -ap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + (opparams[0] << 1);
      s->acc = GET_HEAP(utemp);
      --(s->acc);
      PUT_HEAP(utemp, s->acc);
      break;

    case 0x74: /* -sg */
    case 0x75: /* -sl */
    case 0x76: /* -st */
    case 0x77: /* -sp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + (opparams[0] << 1);
      utemp2 = GET_HEAP(utemp);
      utemp2--;
      PUT_HEAP(utemp, utemp2);
      PUSH(utemp2);
      break;

    case 0x78: /* -agi */
    case 0x79: /* -ali */
    case 0x7a: /* -ati */
    case 0x7b: /* -api */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      s->acc = GET_HEAP(utemp);
      --(s->acc);
      PUT_HEAP(utemp, s->acc);
      break;

    case 0x7c: /* -sgi */
    case 0x7d: /* -sli */
    case 0x7e: /* -sti */
    case 0x7f: /* -spi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      utemp = xs->variables[var_number] + ((opparams[0] + s->acc) << 1);
      utemp2 = GET_HEAP(utemp);
      utemp2--;
      PUT_HEAP(utemp, utemp2);
      PUSH(utemp2);
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

  if (methodselectors < 800 || methodselectors > 0xffff || methodselector_nr > 0x1000) {
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
  int species = OBJ_SPECIES(obj);
  int heappos = CLASS_ADDRESS(species);
  int varselector_nr = GET_HEAP(heappos + SCRIPT_SELECTORCTR_OFFSET);
  /* Number of variable selectors */

  int i;
  
  if (s->version<SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
    selectorid&=~1; /* Low bit in this case is read/write toggle */
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
  int c;
  resource_t *script;

  for (c = 0; c < 1000; c++) {
    if ((script = findResource(sci_script, c))) {

      int id = getInt16(script->data);

      if (id > 15) {
	version_require_earlier_than(s, SCI_VERSION_FTU_NEW_SCRIPT_HEADER);
	return;
      }
    }
  }

}


heap_ptr
script_instantiate(state_t *s, int script_nr, int recursive)
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
    sciprintf( "Loading script %d: Old SCI version; assuming locals size %d\n", script_nr, locals_size); 
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

      if ((superclass >= 0) && recursive)
	script_instantiate(s, s->classtable[superclass].script, 1);
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
#if 0
  heap_ptr pos;
  int objtype, objlength;
#endif

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
    run_vm(s, (successor)? 1 : 0);

    if (s->restarting_flags & SCI_GAME_IS_RESTARTING_NOW) { /* Restart was requested? */

      g_free(s->execution_stack);
      s->execution_stack = NULL;
      s->execution_stack_pos = -1;
      s->execution_stack_pos_changed = 0;

      game_exit(s);
      game_init(s);

      sciprintf(" Restarting flags=%02x\n", s->restarting_flags);

      sciprintf(" Restarting game with ");

      if (s->restarting_flags & SCI_GAME_WAS_RESTARTED_AT_LEAST_ONCE) {
	sciprintf("replay()\n");
	putInt16(s->heap + s->stack_base, s->selector_map.replay); /* Call the replay selector */
      } else {
	sciprintf("play()\n");
	putInt16(s->heap + s->stack_base, s->selector_map.play); /* Call the play selector */
      }
      putInt16(s->heap + s->stack_base + 2, 0);
      send_selector(s, s->game_obj, s->game_obj, s->stack_base + 2, 4, 0, s->stack_base);

      script_abort_flag = 0;
      s->restarting_flags = SCI_GAME_WAS_RESTARTED | SCI_GAME_WAS_RESTARTED_AT_LEAST_ONCE;

    } 
    else {
      successor = s->successor;
      if (successor) {
	script_abort_flag = 0;
	game_exit(s);
	free(s);
	*_s = s = successor;
	if (!send_calls_allocated)
	  send_calls = g_new(calls_struct_t, send_calls_allocated = 16);
	SCI_MEMTEST;
      } else
	game_is_finished = 1;
    }
  } while (!game_is_finished);

  sciprintf(" Game::play() finished.\n");
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
