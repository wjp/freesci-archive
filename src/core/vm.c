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


/* #define VM_DEBUG_SEND */


int script_debug_flag = 1; /* Defaulting to debug mode */
int script_abort_flag = 0; /* Set to 1 to abort execution */
int script_exec_stackpos = -1; /* 0 means that one script is active */
int script_error_flag = 0; /* Set to 1 if an error occured, reset each round by the VM */
int script_checkloads_flag = 0; /* Print info when scripts get (un)loaded */
int script_step_counter = 0; /* Counts the number of executed steps */

script_exec_stack_t script_exec_stack[SCRIPT_MAX_EXEC_STACK];

extern int _debug_step_running; /* scriptdebug.c */
extern int _debug_seeking; /* scriptdebug.c */

int
script_error(state_t *s, char *reason)
{
  sciprintf("Script error: %s\n", reason);
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

/* Operating on the steck */
#define PUSH(v) putInt16(s->heap + (sp += 2) - 2, (v))
#define POP() ((gint16)(getInt16(s->heap + (sp-=2))))

/* Getting instruction parameters */
#define GET_OP_BYTE() ((guint8) s->heap[pc++])
#define GET_OP_WORD() (getUInt16(s->heap + (pc += 2) - 2))
#define GET_OP_FLEX() ((opcode & 1)? GET_OP_BYTE() : GET_OP_WORD())
#define GET_OP_SIGNED_BYTE() ((gint8)(s->heap[pc++]))
#define GET_OP_SIGNED_WORD() ((getInt16(s->heap + (pc += 2) - 2)))
#define GET_OP_SIGNED_FLEX() ((opcode & 1)? GET_OP_SIGNED_BYTE() : GET_OP_SIGNED_WORD())

#define GET_HEAP(address) ((((guint16)(address)) < 800)? \
script_error(s, "Heap address space violation on read")  \
: getInt16(s->heap + ((guint16)(address))))
/* Reads a heap value if allowed */

#define UGET_HEAP(address) ((((guint16)(address)) < 800)? \
script_error(s, "Heap address space violation on read")   \
: getUInt16(s->heap + ((guint16)(address))))
/* Reads an unsigned heap value if allowed */

#define PUT_HEAP(address, value) if (((guint16)(address)) < 800) \
script_error(s, "Heap address space violation on write");        \
else { s->heap[(guint16) address] = (value) &0xff;               \
 s->heap[((guint16)address) + 1] = ((value) >> 8) & 0xff;}
/* Sets a heap value if allowed */

#define CLASS_ADDRESS(classnr) (((classnr < 0) || (classnr >= s->classtable_size)) ?              \
				0 /* Error condition */ : get_class_address(s, classnr))

#define OBJ_SPECIES(address) GET_HEAP((address) + SCRIPT_SPECIES_OFFSET)
/* Returns an object's species */

#define OBJ_SUPERCLASS(address) GET_HEAP((address) + SCRIPT_SUPERCLASS_OFFSET)
/* Returns an object's superclass */

#define OBJ_ISCLASS(address) (GET_HEAP((address) + SCRIPT_INFO_OFFSET) & SCRIPT_INFO_CLASS)
/* Is true if the object at the address is a class */


inline void
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
    return;
  }

  exports_nr = GET_HEAP(tableaddress);

  tableaddress += 2; /* Points to the first actual function pointer */
  if (pubfunct >= exports_nr) {
    sciprintf("Request for invalid exported function 0x%x of script 0x%x\n", pubfunct, script);
    script_error_flag = script_debug_flag = 1;
    return;
  }

  execute(s, scriptpos + GET_HEAP(tableaddress + (pubfunct * 2)), sp, 
    calling_obj, argc, argp, -1, calling_obj);
}

void
send_selector(state_t *s, heap_ptr send_obj, heap_ptr work_obj,
	      heap_ptr sp, int framesize, word restmod, heap_ptr argp)
     /* send_obj and work_obj are equal for anything but 'super' */
{
  heap_ptr lookupresult;
  int selector;
  int argc;
  int i;

  framesize += restmod * 2;

  if (GET_HEAP(send_obj + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    sciprintf("Send: No object at %04x!\n", send_obj);
    script_error_flag = script_debug_flag = 1;
    return;
  }

  while (framesize > 0) {

    selector = GET_HEAP(argp);

    argp += 2;
    argc = GET_HEAP(argp);

    if (argc > 512){ /* More arguments than the stack could possibly accomodate for */
      script_error(s, "More than 512 arguments to function call\n");
      return;
    }

#ifdef VM_DEBUG_SEND
sciprintf("Send to selector %04x (%s):", selector, s->selector_names[selector]);
#endif /* VM_DEBUG_SEND */

    switch (lookup_selector(s, send_obj, selector, &lookupresult)) {

    case SELECTOR_NONE:
      sciprintf("Send to invalid selector 0x%x of object at 0x%x\n", selector, send_obj);
      script_error_flag = script_debug_flag = 1;
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
      case 0: s->acc = GET_HEAP(lookupresult); break;
      case 1: { /* Argument is supplied -> Selector should be set */
	word temp = GET_HEAP(argp + 2);
	PUT_HEAP(lookupresult, temp);
      } break;
      default:
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
      execute(s, lookupresult, sp, work_obj, argc, argp, selector, send_obj);
      break;
    } /* switch(lookup_selector()) */


    framesize -= (4 + argc * 2);
    argp += argc * 2 + 2;
  }
}

void
execute(state_t *s, heap_ptr pc, heap_ptr sp, heap_ptr objp, int argc, heap_ptr argp, 
        int selector, heap_ptr sendp)
{
  gint16 temp, temp2, temp3;
  gint16 opparams[4]; /* Opcode parameters */
  heap_ptr fp = sp;
  int restadjust = 0; /* &rest adjusts the parameter count by this value */
  heap_ptr local_vars = getUInt16(s->heap + objp + SCRIPT_LOCALVARPTR_OFFSET);
  int bp_flag = 0;

  heap_ptr variables[4] =
  { s->global_vars, local_vars, fp, argp }; /* Offsets of global, local, temp, and param variables */

  heap_ptr selector_offset = objp + SCRIPT_SELECTOR_OFFSET;

  PUT_HEAP(argp, argc); /* SCI code reads the zeroeth argument to determine argc */

  /* Start entering debug information into call stack debug blocks */
  ++script_exec_stackpos;
  script_exec_stack[script_exec_stackpos].objpp = &objp;
  script_exec_stack[script_exec_stackpos].sendpp = &sendp;
  script_exec_stack[script_exec_stackpos].pcp = &pc;
  script_exec_stack[script_exec_stackpos].spp = &sp;
  script_exec_stack[script_exec_stackpos].ppp = &fp;
  script_exec_stack[script_exec_stackpos].argcp = &argc;
  script_exec_stack[script_exec_stackpos].argpp = &argp;
  script_exec_stack[script_exec_stackpos].selector = selector;

  /* Check if a breakpoint is set on this method */
  if (s->have_bp & BREAK_EXECUTE && selector != -1)
  {
    breakpoint_t *bp;
    char method_name [256];

    sprintf (method_name, "%s::%s",
      s->heap + getUInt16 (s->heap + sendp + SCRIPT_NAME_OFFSET),
      s->selector_names [selector]);

    bp = s->bp_list;
    while (bp)
    {
      if (bp->type == BREAK_EXECUTE && !strcmp ((char *) bp->data, method_name))
      {
        sciprintf ("Break on %s\n", method_name);
        script_debug_flag = 1;
        bp_flag = 1;
        break;
      }
      bp = bp->next;
    }
  }

  while (1) {
    heap_ptr old_pc = pc;
    heap_ptr old_sp = sp;
    byte opcode, opnumber;
    int var_number; /* See description below */

    script_error_flag = 0; /* Set error condition to false */

    if (script_abort_flag)
      return; /* Emergency */

    if (script_debug_flag)
    {
      script_debug(s, &pc, &sp, &fp, &objp, &restadjust, bp_flag);
      bp_flag = 0;
    }
    /* Debug if this has been requested */

    opcode = GET_OP_BYTE(); /* Get opcode */

    if (sp < s->stack_base)
      script_error(s, "Absolute stack underflow");

    if (sp < fp)
      script_error(s, "Relative stack underflow");

    if (sp >= s->stack_base + VM_STACK_SIZE)
      script_error(s, "Stack overflow");

    if (pc < 800)
      script_error(s, "Program Counter gone astray");

    opnumber = opcode >> 1;

    for (temp = 0; formats[opnumber][temp]; temp++) /* formats comes from script.c */
      switch(formats[opnumber][temp]) {

      case Script_Byte: opparams[temp] = GET_OP_BYTE(); break;
      case Script_SByte: opparams[temp] = GET_OP_SIGNED_BYTE(); break;

      case Script_Word: opparams[temp] = GET_OP_WORD(); break;
      case Script_SWord: opparams[temp] = GET_OP_SIGNED_WORD(); break;

      case Script_Variable: opparams[temp] = GET_OP_FLEX(); break;
      case Script_SVariable: opparams[temp] = GET_OP_SIGNED_FLEX(); break;

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
      if (s->acc) pc += opparams[0];
      break;

    case 0x18: /* bnt */
      if (!s->acc) pc += opparams[0];
      break;

    case 0x19: /* jmp */
      pc += opparams[0];
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
      sp += (opparams[0]) * 2;
      break;

    case 0x20: /* call */
      temp = opparams[1] + 2 + (restadjust*2);
      execute(s, pc + opparams[0], sp, objp, GET_HEAP(sp - temp) + restadjust,
	      sp - temp, -1, objp);
      sp -= temp;
      restadjust = 0; /* Used up the &rest adjustment */
      break;

    case 0x21: /* callk */
      sp -= (opparams[1] + 2 + (restadjust * 2));

      if (opparams[0] >= s->kernel_names_nr) {

	sciprintf("Invalid kernel function 0x%x requested\n", opparams[0]);
	script_debug_flag = script_error_flag = 1;

      } else {

	s->kfunct_table[opparams[0]]
	  (s, opparams[0], GET_HEAP(sp) + restadjust, sp + 2); /* Call kernel function */
	restadjust = 0;

      }

      break;

    case 0x22: /* callb */
      execute_method(s, 0, opparams[0], sp, objp,
		     GET_HEAP(sp - opparams[1] - 2 - (restadjust*2)) + restadjust, sp - opparams[1] - 2
		     - restadjust * 2);
      sp -= (opparams[1] + (restadjust * 2) + 2);
      restadjust = 0; /* Used up the &rest adjustment */
      break;

    case 0x23: /* calle */
      temp = opparams[2] + 2 + (restadjust*2);
      execute_method(s, opparams[0], opparams[1], sp, objp,
		     GET_HEAP(sp - temp) + restadjust, sp - temp);
      sp -= temp;
      restadjust = 0; /* Used up the &rest adjustment */
      break;

    case 0x24: /* ret */
      --script_exec_stackpos; /* Go back one exec stack level */
      ++script_step_counter; /* We skip the final 'else', so this must be done here */
      return; /* Hard return */
      break;

    case 0x25: /* send */
      send_selector(s, s->acc, s->acc, sp, opparams[0], restadjust, sp - opparams[0] - restadjust * 2);
      sp -= (opparams[0] + (restadjust * 2)); /* Adjust stack */
      restadjust = 0;
      break;

    case 0x28: /* class */
      s->acc = CLASS_ADDRESS(opparams[0]);
      break;

    case 0x2a: /* self */
      send_selector(s, objp, objp, sp, opparams[0], restadjust, sp - opparams[0] - restadjust * 2);
      sp -= (opparams[0] + (restadjust * 2)); /* Adjust stack */
      restadjust = 0;
      break;

    case 0x2b: /* super */

      if ((opparams[0] < 0) || (opparams[0] >= s->classtable_size))
	script_error(s, "Invalid superclass in object");
      else {
	send_selector(s, *(s->classtable[opparams[0]].scriptposp)
		      + s->classtable[opparams[0]].class_offset, objp,
		      sp, opparams[1], restadjust, sp - opparams[1] - restadjust * 2);
	sp -= (opparams[1] + (restadjust * 2)); /* Adjust stack */
	restadjust = 0;
      }

      break;

    case 0x2c: /* &rest */
      temp = opparams[0]; /* First argument */
      restadjust = argc - temp + 1; /* +1 because temp counts the paramcount while argc doesn't */
      if (restadjust < 0)
	restadjust = 0;
      temp2 = argp + (temp << 1); /* Pointer to the first argument to &restore */
      for (; temp <= argc; temp++) {
	PUSH(getInt16(s->heap + temp2));
	temp2 += 2;
      }
      break;

    case 0x2d: /* lea */
      temp = opparams[0] >> 1;
      var_number = temp & 0x03; /* Get variable type */

      temp2 = variables[var_number]; /* Get variable block offset */
      if (temp & 0x08)
	temp2 += s->acc; /* Add accumulator offset if requested */
      s->acc = temp2 + (opparams[1] << 1); /* Add index */
      break;

    case 0x2e: /* selfID */
      s->acc = objp;
      break;

    case 0x30: /* pprev */
      PUSH(s->prev);
      break;

    case 0x31: /* pToa */
      s->acc = GET_HEAP(selector_offset + opparams[0]); 
      break;

    case 0x32: /* aTop */
      PUT_HEAP(selector_offset + opparams[0], s->acc);
      break;

    case 0x33: /* pTos */
      temp2 = GET_HEAP(selector_offset + opparams[0]);
      PUSH(temp2);
      break;

    case 0x34: /* sTop */
      temp = POP();
      PUT_HEAP(selector_offset + opparams[0], temp);
      break;

    case 0x35: /* ipToa */
      temp = selector_offset + opparams[0];
      s->acc = GET_HEAP(temp); 
      ++(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x36: /* dpToa */
      temp = selector_offset + opparams[0];
      s->acc = GET_HEAP(temp); 
      --(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x37: /* ipTos */
      temp2 = selector_offset + opparams[0];
      temp = GET_HEAP(temp2); 
      PUT_HEAP(temp2, temp + 1);
      break;

    case 0x38: /* dpTos */
      temp2 = selector_offset + opparams[0];
      temp = GET_HEAP(temp2);
      PUT_HEAP(temp2, temp - 1);
      break;

    case 0x39: /* lofsa */
      s->acc = opparams[0] + pc;
      break;

    case 0x3a: /* lofss */
      PUSH(opparams[0] + pc);
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
      PUSH(objp);
      break;

    case 0x40: /* lag */
    case 0x41: /* lal */
    case 0x42: /* lat */
    case 0x43: /* lap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (opparams[0] << 1);
      s->acc = GET_HEAP(temp);
      break;

    case 0x44: /* lsg */
    case 0x45: /* lsl */
    case 0x46: /* lst */
    case 0x47: /* lsp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (opparams[0] << 1);
      PUSH(GET_HEAP(temp));
      break;

    case 0x48: /* lagi */
    case 0x49: /* lali */
    case 0x4a: /* lati */
    case 0x4b: /* lapi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + ((opparams[0] + s->acc) << 1);
      s->acc = GET_HEAP(temp);
      break;

    case 0x4c: /* lsgi */
    case 0x4d: /* lsli */
    case 0x4e: /* lsti */
    case 0x4f: /* lspi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + ((opparams[0] + s->acc) << 1);
      PUSH(GET_HEAP(temp));
      break;

    case 0x50: /* sag */
    case 0x51: /* sal */
    case 0x52: /* sat */
    case 0x53: /* sap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (opparams[0] << 1);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x54: /* ssg */
    case 0x55: /* ssl */
    case 0x56: /* sst */
    case 0x57: /* ssp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (opparams[0] << 1);
      temp2 = POP();
      PUT_HEAP(temp, temp2);
      break;

    case 0x58: /* sagi */
    case 0x59: /* sali */
    case 0x5a: /* sati */
    case 0x5b: /* sapi */
      temp2 = POP();
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + ((opparams[0] + s->acc) << 1);
      PUT_HEAP(temp, temp2);
      s->acc = temp2;
      break;

    case 0x5c: /* ssgi */
    case 0x5d: /* ssli */
    case 0x5e: /* ssti */
    case 0x5f: /* sspi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + ((opparams[0] + s->acc) << 1);
      temp2 = POP();
      PUT_HEAP(temp, temp2);
      break;

    case 0x60: /* +ag */
    case 0x61: /* +al */
    case 0x62: /* +at */
    case 0x63: /* +ap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (opparams[0] << 1);
      s->acc = GET_HEAP(temp);
      ++(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x64: /* +sg */
    case 0x65: /* +sl */
    case 0x66: /* +st */
    case 0x67: /* +sp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (opparams[0] << 1);
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
      temp = variables[var_number] + ((opparams[0] + s->acc) << 1);
      s->acc = GET_HEAP(temp);
      ++(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x6c: /* +sgi */
    case 0x6d: /* +sli */
    case 0x6e: /* +sti */
    case 0x6f: /* +spi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + ((opparams[0] + s->acc) << 1);
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
      temp = variables[var_number] + (opparams[0] << 1);
      s->acc = GET_HEAP(temp);
      --(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x74: /* -sg */
    case 0x75: /* -sl */
    case 0x76: /* -st */
    case 0x77: /* -sp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (opparams[0] << 1);
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
      temp = variables[var_number] + ((opparams[0] + s->acc) << 1);
      s->acc = GET_HEAP(temp);
      --(s->acc);
      PUT_HEAP(temp, s->acc);
      break;

    case 0x7c: /* -sgi */
    case 0x7d: /* -sli */
    case 0x7e: /* -sti */
    case 0x7f: /* -spi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + ((opparams[0] + s->acc) << 1);
      temp2 = GET_HEAP(temp);
      temp2--;
      PUT_HEAP(temp, temp2);
      PUSH(temp2);
      break;

    default:
      script_error(s, "Illegal opcode");

    } /* switch(opcode >> 1) */

    if (script_error_flag) {
      _debug_step_running = 0; /* Stop multiple execution */
      _debug_seeking = 0; /* Stop special seeks */
      pc = old_pc;
      sp = old_sp;
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



int
script_init_state(state_t *s, sci_version_t version)
{
  resource_t *vocab996 = findResource(sci_vocab, 996);
  int scriptnr;
  int seeker;
  int classnr;
  int size;
  resource_t *script;

  s->_heap = heap_new();
  s->heap = s->_heap->start;
  s->acc = s->prev = 0;

  s->global_vars = 0; /* Set during launch time */

  if (!version)
    s->version = SCI_VERSION_DEFAULT_SCI0;
  else
    s->version = version;

  if (!vocab996)
    s->classtable_size = 20;
  else
    s->classtable_size = vocab996->length >> 2;

  s->classtable = calloc(s->classtable_size, sizeof(class_t));

  for (scriptnr = 0; scriptnr < 1000; scriptnr++) {
    int objtype;
    resource_t *script = findResource(sci_script, scriptnr);

    if (script) {

      size = getInt16(script->data);
      if (size == script->length)
	seeker = 2;
      else if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
        seeker = 2;
      else
	seeker = 0;


      do {

	while ((objtype = getInt16(script->data + seeker)) && (objtype != sci_obj_class))
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

	    s->classtable = realloc(s->classtable, sizeof(class_t) * (classnr + 1));
	    memset(&(s->classtable[s->classtable_size]), 0,
		   sizeof(class_t) * (1 + classnr - s->classtable_size)); /* Clear after resize */

	    s->classtable_size = classnr + 1; /* Adjust maximum number of entries */
	  }

	  s->classtable[classnr].class_offset = seeker + 4;
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
  heap_del(s->_heap);
  free(s->classtable);
}


heap_ptr
script_instantiate(state_t *s, int script_nr)
{
  resource_t *script = findResource(sci_script, script_nr);
  heap_ptr pos;
  int objtype;
  unsigned int objlength;
  heap_ptr handle;

  if (!script) {
    sciprintf("Script 0x%x requested but not found\n", script_nr);
    /*    script_debug_flag = script_error_flag = 1; */
    return 0;
  }

  if (s->scripttable[script_nr].heappos) { /* Is it already loaded? */
    s->scripttable[script_nr].lockers++; /* Required by another script */
    return s->scripttable[script_nr].heappos;
  }

  handle = heap_allocate(s->_heap, script->length);

  if (!handle) {
    sciprintf("Not enough heap space for script size 0x%x of script 0x%x\n",
	      script->length, script_nr);
    script_debug_flag = script_error_flag = 1;
    return 0;
  }

  handle += 2; /* Get beyond the type word */
  if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
    handle += 2; /* Get beyond unknown word */

  s->scripttable[script_nr].heappos = handle; /* Set heap position */
  s->scripttable[script_nr].lockers = 1; /* Locked by one */
  s->scripttable[script_nr].export_table_offset = 0;
  s->scripttable[script_nr].localvar_offset = 0;

  objlength = getInt16(script->data);
  if (objlength != script->length)
    memcpy(s->heap + handle, script->data, script->length); /* Copy the script */
  else
    memcpy(s->heap + handle, script->data + 2, script->length -2);

  /* Now do a first pass through the script objects to find the
  ** export table and local variable block 
  */
  pos = handle;

  objlength = 0;

  do {
    pos += objlength; /* Step over the last checked object */

    objtype = GET_HEAP(pos);
    objlength = GET_HEAP(pos+2);

    if (objtype == sci_obj_exports)
      s->scripttable[script_nr].export_table_offset = pos + 4; /* +4 is to step over the header */
    
    if (objtype == sci_obj_localvars)
      s->scripttable[script_nr].localvar_offset = pos + 4; /* +4 is to step over the header */

  } while (objtype != 0);


  /* And now a second pass to adjust objects and class pointers, and the general pointers */
  pos = handle;

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
	PUT_HEAP(functarea + i, functpos + handle); /* Adjust function pointer addresses */
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
	int new_address = ((guint16) GET_HEAP(pos + 2 + i*2)) + handle;
	int old_indexed_pointer;
	PUT_HEAP(pos + 2 + i*2, new_address); /* Adjust pointers. Not sure if this is needed. */
	old_indexed_pointer = ((guint16) GET_HEAP(new_address));
	PUT_HEAP(new_address, old_indexed_pointer + handle);
	/* Adjust indexed pointer. */

      } /* For all indexed pointers pointers */

    }

    pos -= 4; /* Step back on header */

    
  } while (objtype != 0);

  if (script_checkloads_flag)
    sciprintf("Script 0x%x was loaded to %04x.\n", script_nr, handle);

  return handle;

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
  int i;

  if (!stack_handle) {
    sciprintf("script_init(): Insufficient heap space for stack\n");
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

  fprintf(stderr," Script 0 at %04x\n", script0);

  /* Init parser */
  s->parser_words = vocab_get_words(&(s->parser_words_nr));
  s->parser_suffices = vocab_get_suffices(&(s->parser_suffices_nr));

  s->restarting_flag = 0; /* We're not restarting here */

  s->stack_base = stack_handle + 2;
  s->parser_base = parser_handle + 2;
  s->global_vars = s->scripttable[0].localvar_offset;
  /* Global variables are script 0's local variables */

  s->kernel_names = vocabulary_get_knames(&s->kernel_names_nr);
  s->opcodes = vocabulary_get_opcodes();
  s->selector_names = vocabulary_get_snames();
  for (s->selector_names_nr = 0; s->selector_names[s->selector_names_nr]; s->selector_names_nr++);
  /* Counts the number of selector names */

  script_map_selectors(s, &(s->selector_map));
  /* Maps a few special selectors for later use */

  s->file_handles_nr = 5;
  s->file_handles = calloc(s->file_handles_nr, sizeof(FILE *));
  /* Allocate memory for file handles */

  script_map_kernel(s);
  /* Maps the kernel functions */

  s->menubar = menubar_new(); /* Create menu bar */

  g_get_current_time(&(s->game_start_time)); /* Get start time */
  memcpy(&(s->last_wait_time), &(s->game_start_time), sizeof(GTimeVal));
  /* Use start time as last_wait_time */

  s->mouse_pointer = NULL; /* No mouse pointer */
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

  s->bp_list = NULL; /* No breakpoints defined */
  s->have_bp = 0;

  srand(time(NULL)); /* Initialize random number generator */

  i = 0;
  do {
    resource = findResource(sci_font, i++);
  } while ((!resource) && (i < 1000));

  if (!resource) {
    sciprintf("No text font was found.\n");
    return 1;
  }
  for (i = 0; i < 3; i++) {
      s->ports[i]->font = resource->data; /* let all ports default to the 'system' font */
      s->ports[i]->bgcolor = -1; /* All ports should be transparent */
  }

  memset(s->hunk, sizeof(s->hunk), 0); /* Sets hunk to be unused */
  memset(s->clone_list, sizeof(s->clone_list), 0); /* No clones */

  s->save_dir = heap_allocate(s->_heap, MAX_HOMEDIR_SIZE + strlen(FREESCI_GAMEDIR)
			      + MAX_GAMEDIR_SIZE + 4); /* +4 for the three slashes and trailing \0 */

  game_obj = script0 + GET_HEAP(s->scripttable[0].export_table_offset + 2);
  /* The first entry in the export table of script 0 points to the game object */

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
game_run(state_t *s)
{
  sciprintf(" Calling %s::play()\n", s->game_name);
  putInt16(s->heap + s->stack_base, s->selector_map.play); /* Call the play selector... */
  putInt16(s->heap + s->stack_base + 2, 0);                    /* ... with 0 arguments. */
  send_selector(s, s->game_obj, s->game_obj, s->stack_base + 2, 4, 0, s->stack_base); /* Engage! */
  sciprintf(" Game::play() finished.\n");
  return 0;
}

int
game_exit(state_t *s)
{
  int i;
  breakpoint_t *bp, *bp_next;

  sciprintf("Freeing vocabulary...\n");
  vocabulary_free_snames(s->selector_names);
  vocabulary_free_knames(s->kernel_names);
  free(s->opcodes);
  free(s->kfunct_table);

  s->selector_names = NULL;
  s->kernel_names = NULL;
  s->opcodes = NULL;
  s->kfunct_table = NULL;
  /* Make sure to segfault if any of those are dereferenced */

  sciprintf("Freeing graphics data...\n");
  free_picture(s->pic);

  for (i = 0; i < MAX_HUNK_BLOCKS; i++)
      if (s->hunk[i].size) {
	  free(s->hunk[i].data);
	  s->hunk[i].size = 0;
      }

  for (i = 3; i < MAX_PORTS; i++) /* Ports 0,1,2 are fixed */
    if (s->ports[i]) {
      free(s->ports[i]);
      s->ports[i] = 0;
    }

  if (s->pic_views)
    free(s->pic_views);
  if (s->dyn_views)
    free(s->dyn_views);

  sciprintf("Freeing miscellaneous data...\n");
  free(s->file_handles);

  menubar_free(s->menubar);

  vocab_free_words(s->parser_words, s->parser_words_nr);
  vocab_free_suffices(s->parser_suffices, s->parser_suffices_nr);

  heap_free(s->_heap, s->stack_handle);
  heap_free(s->_heap, s->save_dir);
  heap_free(s->_heap, s->parser_base - 2);

  /* Free breakpoint list */
  bp = s->bp_list;
  while (bp)
  {
    bp_next = bp->next;
    if (bp->type == BREAK_EXECUTE) free (bp->data);
    free (bp);
    bp = bp_next;
  }
  s->bp_list = NULL;

  return 0;
}


