/***************************************************************************
 vm.c Copyright (C) 1999, 2000, 2001 Christoph Reichenbach

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


#include <sciresource.h>
#include <engine.h>
#include <versions.h>
#include <kdebug.h>

#if !defined (_WIN32) && !defined (__BEOS__)
#include <sys/resource.h>
#endif

#ifdef HAVE_SETJMP_H
#include <setjmp.h>
#endif


/*#define VM_DEBUG_SEND*/
#undef STRICT_SEND /* Disallows variable sends with more than one parameter */


int script_abort_flag = 0; /* Set to 1 to abort execution */
int script_error_flag = 0; /* Set to 1 if an error occured, reset each round by the VM */
int script_checkloads_flag = 0; /* Print info when scripts get (un)loaded */
int script_step_counter = 0; /* Counts the number of steps executed */

extern int _debug_step_running; /* scriptdebug.c */
extern int _debug_seeking; /* scriptdebug.c */



calls_struct_t *send_calls = NULL;
int send_calls_allocated = 0;
int bp_flag = 0;

/*-- validation functionality --*/

#ifndef DISABLE_VALIDATIONS

static inline stack_ptr_t
validate_stack_addr(state_t *s, stack_ptr_t sp)
{
	if (sp >= s->stack_base && sp < s->stack_top)
		return sp;

	script_debug_flag = script_error_flag = 1;
	sciprintf("Stack index %d out of valid range [%d..%d]\n",
		  sp, 0, s->stack_top - s->stack_base -1);
	return 0;
}

static inline int
validate_arithmetic(reg_t reg)
{
	if (reg.segment) {
		script_debug_flag = script_error_flag = 1;
		sciprintf("Attempt to read arithmetic value from non-zero segment [%04x]\n", reg.segment);
	}

	return reg.offset;
}

static inline int
validate_variable(reg_t *r, int type, int max, int index, int line)
{
	char *names[4] = {"global", "local", "temp", "param"};

	if (index < 0 || index >= max) {
		script_debug_flag = script_error_flag = 1;
		sciprintf("Attempt to read invalid %s variable %d ");
		if (max == 0)
			sciprintf("(variable type invalid)");
		else
			sciprintf("(out of range [%d..%d])", 0, max-1);
		sciprintf(" in %s, line %d", __FILE__, __LINE__);
		script_debug_flag = script_error_flag = 1;

		return 1;
	}

	return 0;
}

static inline reg_t
validate_read_var(reg_t *r, int type, int max, int index, int line)
{
	if (!validate_variable(r, type, max, index, line))
		return r[index];
	else
		return make_reg(0, 0);
}

static inline void
validate_write_var(reg_t *r, int type, int max, int index, int line, reg_t value)
{
	if (!validate_variable(r, type, max, index, line))
		r[index] = value;
}

#else
/*-- Non-validating alternatives -- */

#  define validate_stack_addr(s, sp) sp
#  define validate_arithmetic(r) ((r).offset)
#  define validate_variable(r, t, m, i, l)
#  define validate_read_var(r, t, m, i, l) ((r)[i])
#  define validate_write_var(r, t, m, i, l, v) ((r)[i] = (v))

#endif

#define READ_VAR(type, index) validate_read_var(variables[type], type, variables_max[type], index, __LINE__)
#define WRITE_VAR(type, index, value) validate_write_var(variables[type], type, variables_max[type], index, __LINE__, value)
#define WRITE_VAR16(type, index, value) WRITE_VAR(type, index, make_reg(0, value));

#define ACC_ARITHMETIC_L(op) make_reg(0, (op validate_arithmetic(s->r_acc)))
#define ACC_AUX_LOAD() aux_acc = validate_arithmetic(s->r_acc)
#define ACC_AUX_STORE() s->r_acc = make_reg(0, aux_acc)

/*==--------------------------==*/

void
check_heap_corruption(state_t *s, char *file, int line)
{
	byte *heap = s->_heap->start;
	int pos = s->_heap->first_free;

	fprintf(stderr, "\033[1m%s, L%d\033[0m|", file, line);
	while (pos != 0xffff) {
		int size = getUInt16(heap + pos);
		int next = getUInt16(heap + pos + 2);
		fprintf(stderr,"%04x(%04x) : ", pos, size);

		if (next < 1000 || next > 0xffff || size + pos > 0xffff) {
			fprintf(stderr, "\n\033[1m\033[31mHeap corrupt!\033[0m\n");
			fprintf(stderr, "Start = %04x, bad = %04x after %04x\n",
				s->_heap->first_free, next, pos);
			fprintf(stderr, "Totalsize = %x\n", size + pos);
			return;
		}
		pos = next;
	}
	fprintf(stderr,"\n");
}

#define CHECK_HEAP check_heap_corruption(s, __FILE__, __LINE__);

int
script_error(state_t *s, char *file, int line, char *reason)
{
  sciprintf("Script error in file %s, line %d: %s\n", file, line, reason);
  script_debug_flag = script_error_flag = 1;
  return 0;
}

inline heap_ptr
get_class_address(state_t *s, int classnr)
{
  if (NULL == s)
  {
    sciprintf("vm.c: get_class_address(): NULL passed for \"s\"\n");
    return 0;
  }

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
/* 16 bit: */
#define PUSH(v) PUSH32(make_reg(0, v))
#define POP() (validate_arithmetic(POP32()))
/* 32 bit: */
#define PUSH32(a) (*(validate_stack_addr(s, (xs->sp)++)) = (a))
#define POP32() (*(validate_stack_addr(s, --(xs->sp))))

/* Getting instruction parameters */
#define GET_OP_BYTE() ((guint8) code_buf[(pc_offset)++])
#define GET_OP_WORD() (getUInt16(code_buf + ((pc_offset) += 2) - 2))
#define GET_OP_FLEX() ((opcode & 1)? GET_OP_BYTE() : GET_OP_WORD())
#define GET_OP_SIGNED_BYTE() ((gint8)(code_buf[(pc_offset)++]))
#define GET_OP_SIGNED_WORD() ((getInt16(code_buf + ((pc_offset) += 2) - 2)))
#define GET_OP_SIGNED_FLEX() ((opcode & 1)? GET_OP_SIGNED_BYTE() : GET_OP_SIGNED_WORD())

#define CLASS_ADDRESS(classnr) (((classnr < 0) || (classnr >= s->classtable_size)) ?              \
				classnr /* parameter was parent object address */ : get_class_address(s, classnr))

#define OBJ_SPECIES(address) GET_HEAP((address) + SCRIPT_SPECIES_OFFSET)
/* Returns an object's species */

#define OBJ_SUPERCLASS(address) GET_HEAP((address) + SCRIPT_SUPERCLASS_OFFSET)
/* Returns an object's superclass */

#define OBJ_ISCLASS(address) (GET_HEAP((address) + SCRIPT_INFO_OFFSET) & SCRIPT_INFO_CLASS)
/* Is true if the object at the address is a class */


/*inline*/ exec_stack_t *
execute_method(state_t *s, word script, word pubfunct, stack_ptr_t sp,
	       reg_t calling_obj, word argc, stack_ptr_t argp)
{
#warning "Re-enable execute_method"
#if 0
  heap_ptr tableaddress;
  heap_ptr scriptpos;
  int exports_nr;
  int magic_ofs;

  if (s->seg_manager.isloaded (&s->seg_manager, script_nr)) /* Script not present yet? */
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
    guint32 bpaddress;

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
    add_exec_stack_entry(s, (heap_ptr)(scriptpos + GET_HEAP(tableaddress + (pubfunct * 2)) - magic_ofs),
		 sp, calling_obj, (int)argc, argp, -1, calling_obj, s->execution_stack_pos);
#endif
}



exec_stack_t *
send_selector(state_t *s, reg_t send_obj, reg_t work_obj,
	      stack_ptr_t sp, int framesize, word restmod, stack_ptr_t argp)
     /* send_obj and work_obj are equal for anything but 'super' */
     /* Returns a pointer to the TOS exec_stack element */
{
#warning "Re-enable send_selector()"
#if 0
	heap_ptr lookupresult;
	int selector;
	int argc;
	int origin = s->execution_stack_pos; /* Origin: Used for debugging */
	exec_stack_t *retval = s->execution_stack + s->execution_stack_pos;
	int print_send_action = 0;
	/* We return a pointer to the new active exec_stack_t */

	/* The selector calls we catch are stored below: */
	int send_calls_nr = -1;

        if (NULL == s)
        {
                sciprintf("vm.c: exec_stack_t(): NULL passed for \"s\"\n");
                return NULL;
        }

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
		if (s->have_bp & BREAK_SELECTOR) {
			breakpoint_t *bp;
			char method_name [256];

			sprintf (method_name, "%s::%s",
				 s->heap + getUInt16 (s->heap + send_obj + SCRIPT_NAME_OFFSET),
				 s->selector_names [selector]);

			bp = s->bp_list;
			while (bp) {
				int cmplen = strlen(bp->data.name);
				if (bp->data.name[cmplen - 1] != ':')
					cmplen = 256;

				if (bp->type == BREAK_SELECTOR && !strncmp (bp->data.name, method_name, cmplen)) {
					sciprintf ("Break on %s (in [%04x])\n", method_name, send_obj);
					script_debug_flag = print_send_action = 1;
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
			send_calls = sci_realloc(send_calls, sizeof(calls_struct_t) * (send_calls_allocated *= 2));

		argc += restmod;
		restmod = 0; /* Take care that the rest modifier is used only once */

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
				if (print_send_action) {
					sciprintf("[read selector]\n");
					print_send_action = 0;
				}
				/* fallthrough */
			case 1:
#ifndef STRICT_SEND
			default:
#endif
				{ /* Argument is supplied -> Selector should be set */

				if (print_send_action) {
					int val = GET_HEAP(lookupresult);
					int uval = UGET_HEAP(lookupresult);
					int new = GET_HEAP(argp + 2);
					int unew = UGET_HEAP(argp + 2);

					sciprintf("[write to selector: change %d(0x%04x) to %d(0x%04x)]\n",
						  val, uval, new, unew);
					print_send_action = 0;
				}
				send_calls[send_calls_nr].address = lookupresult; /* register the call */
				send_calls[send_calls_nr].argp = argp;
				send_calls[send_calls_nr].argc = argc;
				send_calls[send_calls_nr].selector = selector;
				send_calls[send_calls_nr].type = EXEC_STACK_TYPE_VARSELECTOR; /* Register as a varselector */

			} break;
#ifdef STRICT_SEND
			default:
				--send_calls_nr;
				sciprintf("Send error: Variable selector %04x in %04x called with %04x params\n",
					  selector, send_obj, argc);
				script_debug_flag = 1; /* Enter debug mode */
				_debug_seeking = _debug_step_running = 0;

#endif
			}
			break;

		case SELECTOR_METHOD:

#ifdef VM_DEBUG_SEND
			sciprintf("Funcselector(");
			for (i = 0; i < argc; i++) {
				sciprintf("%04x", UGET_HEAP(argp + 2 + 2*i));
				if (i + 1 < argc)
					sciprintf(", ");
			}
			sciprintf(")\n");
#endif /* VM_DEBUG_SEND */
			if (print_send_action) {
				sciprintf("[invoke selector]\n");
				print_send_action = 0;
			}

			send_calls[send_calls_nr].address = lookupresult; /* register call */
			send_calls[send_calls_nr].argp = argp;
			send_calls[send_calls_nr].argc = argc;
			send_calls[send_calls_nr].selector = selector;
			send_calls[send_calls_nr].type = EXEC_STACK_TYPE_CALL;
			send_calls[send_calls_nr].sp = sp;
			sp = CALL_SP_CARRY; /* Destroy sp, as it will be carried over */

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
				add_exec_stack_entry(s, send_calls[send_calls_nr].address,
						     send_calls[send_calls_nr].sp, work_obj,
						     send_calls[send_calls_nr].argc,
						     send_calls[send_calls_nr].argp,
						     send_calls[send_calls_nr].selector,
						     send_obj, origin);

	/* Now check the TOS to execute all varselector entries */
	if (s->execution_stack_pos >= 0)
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
#endif
}


exec_stack_t *
add_exec_stack_varselector(state_t *s, reg_t objp, int argc, stack_ptr_t argp,
			   selector_t selector, reg_t *address, int origin)
{
	exec_stack_t *xstack = add_exec_stack_entry(s, NULL_REG, address, objp, argc, argp,
						    selector, objp, origin);
	/* Store selector address in sp */

	xstack->type = EXEC_STACK_TYPE_VARSELECTOR;

  return xstack;
}


exec_stack_t *
add_exec_stack_entry(state_t *s, reg_t pc, stack_ptr_t sp, reg_t objp, int argc,
		     stack_ptr_t argp, selector_t selector, reg_t sendp, int origin)
/* Returns new TOS element for the execution stack*/
{
	exec_stack_t *xstack = NULL;

	if (!s->execution_stack)
		s->execution_stack =
			sci_malloc(sizeof(exec_stack_t) * (s->execution_stack_size = 16));

	if (++(s->execution_stack_pos) == s->execution_stack_size) /* Out of stack space? */
		s->execution_stack = sci_realloc(s->execution_stack,
				     sizeof(exec_stack_t) * (s->execution_stack_size += 8));

  /*  sciprintf("Exec stack: [%d/%d], origin %d, at %p\n", s->execution_stack_pos,
      s->execution_stack_size, origin, s->execution_stack); */

	xstack = s->execution_stack + s->execution_stack_pos;

	xstack->objp = objp;
	xstack->sendp = sendp;
	xstack->pc = pc;
	xstack->sp = sp;
	xstack->argc = argc;

	xstack->variables_argp = argp; /* Parameters */

	*argp = make_reg(0, argc);  /* SCI code relies on the zeroeth argument to equal argc */

	/* Additional debug information */
	xstack->selector = selector;
	xstack->origin = origin;

	xstack->type = EXEC_STACK_TYPE_CALL; /* Normal call */

	return xstack;
}


static int jump_initialized = 0;
#ifdef HAVE_SETJMP_H
static jmp_buf vm_error_address;
#endif

void
vm_handle_fatal_error(state_t *s, int line, char *file)
{
	fprintf(stderr, "Fatal VM error in %s, L%d; aborting...\n", file, line);
#ifdef HAVE_SETJMP_H
	if (jump_initialized)
		longjmp(vm_error_address, 0);
	else
#endif
		{
			fprintf(stderr, "Could not recover, exitting...\n");
			exit(1);
		}
}

void
run_vm(state_t *s, int restoring)
{
	reg_t *variables[4]; /* global, local, temp, param, as immediate pointers */
#ifndef DISABLE_VALIDATIONS
	int variables_max[4]; /* Max. values for all variables */
#endif
	int temp;
	gint16 aux_acc; /* Auxiliary 16 bit accumulator */
	reg_t r_temp; /* Temporary register */
//	gint16 temp, temp2;
//	guint16 utemp, utemp2;
	gint16 opparams[4]; /* opcode parameters */

	unsigned int restadjust = s->r_amp_rest; /* &rest adjusts the parameter count
						 ** by this value  */
	/* Current execution data: */
	exec_stack_t *xs = s->execution_stack + s->execution_stack_pos;
	exec_stack_t *xs_new; /* Used during some operations */
	script_t *local_script = s->script_000;
	int pc_offset;
	byte *code_buf;

	if (!local_script) {
		script_error(s, __FILE__, __LINE__, "Program Counter gone astray");
		return;
	}
	
	pc_offset = xs->pc.offset;
	code_buf = local_script->buf;
//
//  int old_execution_stack_base = s->execution_stack_base;

	if (NULL == s) {
		sciprintf("vm.c: run_vm(): NULL passed for \"s\"\n");
		return;
	}

#ifdef HAVE_SETJMP_H
	setjmp(vm_error_address);
	jump_initialized = 1;
#endif
//
//
//  if (restoring) {
//
//
//  } else {
//
//    s->execution_stack_base = s->execution_stack_pos;
//
//  }

#ifndef DISABLE_VALIDATIONS
	/* Initialize maximum variable count */
	variables_max[VAR_GLOBAL] = s->script_000->locals_nr;
	variables_max[VAR_LOCAL] = local_script->locals_nr;
	variables_max[VAR_TEMP] = xs->sp - xs->fp;
	variables_max[VAR_PARAM] = xs->argc + 1;
#endif


	/* SCI code reads the zeroeth argument to determine argc */
	variables[VAR_GLOBAL] = s->script_000->locals;
	variables[VAR_LOCAL] = local_script->locals;
	variables[VAR_TEMP] = xs->fp;
	variables[VAR_PARAM] = xs->variables_argp;

	WRITE_VAR16(VAR_PARAM, 0, xs->argc);


	while (1) {
		byte opcode;
		reg_t old_pc = xs->pc;
		stack_ptr_t old_sp = xs->sp;
		byte opnumber;
		int var_type; /* See description below */
		int var_number;
//
//    if (s->execution_stack_pos_changed) {
//      xs = s->execution_stack + s->execution_stack_pos;
//      s->execution_stack_pos_changed = 0;
//    }
//
		script_error_flag = 0; /* Set error condition to false */

		if (script_abort_flag)
			return; /* Emergency */


		/* Debug if this has been requested: */
		if (script_debug_flag || sci_debug_flags) {
			script_debug(s, &(xs->pc), &(xs->sp), &(xs->fp),
				     &(xs->objp), &restadjust, bp_flag);
			bp_flag = 0;
		}

		opcode = GET_OP_BYTE(); /* Get opcode */


#ifndef DISABLE_VALIDATIONS
		if (xs->sp < xs->fp)
			script_error(s, "[VM] "__FILE__, __LINE__, "Stack underflow");

		variables_max[VAR_TEMP] = xs->sp - xs->fp;

		if (pc_offset < 0 || pc_offset >= local_script->buf_size)
			script_error(s, "[VM] "__FILE__, __LINE__, "Program Counter gone astray");
#endif

		opnumber = opcode >> 1;

		for (temp = 0; formats[opnumber][temp]; temp++) /* formats comes from script.c */
			switch(formats[opnumber][temp]) {

			case Script_Byte: opparams[temp] = GET_OP_BYTE(); break;
			case Script_SByte: opparams[temp] = GET_OP_SIGNED_BYTE(); break;

			case Script_Word: opparams[temp] = GET_OP_WORD(); break;
			case Script_SWord: opparams[temp] = GET_OP_SIGNED_WORD(); break;

			case Script_Variable:
			case Script_Property:

			case Script_Local:
			case Script_Temp:
			case Script_Global:
			case Script_Param:
				opparams[temp] = GET_OP_FLEX(); break;

			case Script_SVariable:
			case Script_SRelative:
				opparams[temp] = GET_OP_SIGNED_FLEX(); break;

			case Script_None:
			case Script_End:
				break;

			case Script_Invalid:
			default:
				sciprintf("opcode %02x: Invalid!", opcode);
				script_debug_flag = script_error_flag = 1;

			}

		switch (opnumber) {

		case 0x00: /* bnot */
			s->r_acc = ACC_ARITHMETIC_L (0xffff ^ /*acc*/);
			break;

		case 0x01: /* add */
			s->r_acc = ACC_ARITHMETIC_L (POP() + /*acc*/);
			break;

		case 0x02: /* sub */
			s->r_acc = ACC_ARITHMETIC_L (POP() - /*acc*/);
			break;

		case 0x03: /* mul */
			s->r_acc = ACC_ARITHMETIC_L (POP() * /*acc*/);
			break;

		case 0x04: /* div */
			ACC_AUX_LOAD();
			aux_acc = aux_acc? POP() / aux_acc : 0;
			ACC_AUX_STORE();
			break;

		case 0x05: /* mod */
			ACC_AUX_LOAD();
			aux_acc = (aux_acc > 0)? POP() % aux_acc : 0;
			ACC_AUX_STORE();
			break;

		case 0x06: /* shr */
			s->r_acc = ACC_ARITHMETIC_L(((guint16) POP()) >> /*acc*/);
			break;

		case 0x07: /* shl */
			s->r_acc = ACC_ARITHMETIC_L(((guint16) POP()) << /*acc*/);
			break;

		case 0x08: /* xor */
			s->r_acc = ACC_ARITHMETIC_L(POP() ^ /*acc*/);
			break;

		case 0x09: /* and */
			s->r_acc = ACC_ARITHMETIC_L(POP() & /*acc*/);
			break;

		case 0x0a: /* or */
			s->r_acc = ACC_ARITHMETIC_L(POP() | /*acc*/);
			break;

		case 0x0b: /* neg */
			s->r_acc = ACC_ARITHMETIC_L(-/*acc*/);
			break;

		case 0x0c: /* not */
			s->r_acc = ACC_ARITHMETIC_L(!/*acc*/);
			break;

		case 0x0d: /* eq? */
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L(POP() == /*acc*/);
			break;

		case 0x0e: /* ne? */
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L(POP() != /*acc*/);
			break;

		case 0x0f: /* gt? */
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L(POP() > /*acc*/);
			break;

		case 0x10: /* ge? */
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L(POP() >= /*acc*/);
			break;

		case 0x11: /* lt? */
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L(POP() < /*acc*/);
			break;

		case 0x12: /* le? */
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L(POP() <= /*acc*/);
			break;

		case 0x13: /* ugt? */
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L((guint16)(POP()) > (guint16)/*acc*/);
			break;

		case 0x14: /* uge? */
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L((guint16)(POP()) >= (guint16)/*acc*/);
			break;

		case 0x15: /* ult? */
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L((guint16)(POP()) < (guint16)/*acc*/);
			break;

		case 0x16: /* ule? */
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L((guint16)(POP()) <= (guint16)/*acc*/);
			break;

		case 0x17: /* bt */
			if (s->r_acc.offset || s->r_acc.segment) pc_offset += opparams[0];
			break;

		case 0x18: /* bnt */
			if (!(s->r_acc.offset || s->r_acc.segment)) pc_offset += opparams[0];
			break;

		case 0x19: /* jmp */
			xs->pc.offset += opparams[0];
			break;

		case 0x1a: /* ldi */
			s->r_acc = make_reg(0, opparams[0]);
			break;

		case 0x1b: /* push */
			PUSH32(s->r_acc);
			break;

		case 0x1c: /* pushi */
			PUSH(opparams[0]);
			break;

		case 0x1d: /* toss */
			xs->sp--;
			break;

		case 0x1e: /* dup */
			PUSH32(xs->sp[-1]);
			break;

		case 0x1f: /* link */
			xs->sp += opparams[0];
			break;

		case 0x20: { /* call */
			int argc = (opparams[1] >> 1) /* Given as offset, but we need count */
				+ 1 + restadjust;
			stack_ptr_t call_base = xs->sp - argc;

			xs_new = add_exec_stack_entry(s, make_reg(xs->pc.segment,
								  xs->pc.offset + opparams[0]),
						      xs->sp, xs->objp,
						      (validate_arithmetic(*call_base) >> 1)
						      	+ restadjust,
						      call_base, NULL_SELECTOR, xs->objp,
						      s->execution_stack_pos);
			restadjust = 0; /* Used up the &rest adjustment */

			xs = xs_new;
			break;
		}

		case 0x21: /* callk */
#warning "Disabled VM operations: fixme"
#if 0
			xs->sp -= opparams[1]+2;
    if (s->version>=SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
    {
      xs->sp -= restadjust * 2;
      s->amp_rest = 0; /* We just used up the restadjust, remember? */
    }
      if (opparams[0] >= s->kernel_names_nr) {

	sciprintf("Invalid kernel function 0x%x requested\n", opparams[0]);
	script_debug_flag = script_error_flag = 1;

      } else {

	if (s->version>=SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
	    s->kfunct_table[opparams[0]](s, opparams[0], GET_HEAP(xs->sp) + restadjust, xs->sp + 2); /* Call kernel function */ else
	    s->kfunct_table[opparams[0]](s, opparams[0], GET_HEAP(xs->sp), xs->sp + 2); /* Call kernel function */

	/* Calculate xs again: The kernel function might have spawned a new VM */
	xs = s->execution_stack + s->execution_stack_pos;

	if (s->version>=SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
	  restadjust = s->amp_rest;

      }
#endif
      fprintf(stderr, "Not implemented yet: callk\n");
      exit(1);

      break;

//    case 0x22: /* callb */
//      temp = xs->sp;
//      xs->sp -= (opparams[1] + (restadjust * 2) + 2);
//      xs_new = execute_method(s, 0, opparams[0], temp, xs->objp,
//			      GET_HEAP(xs->sp) + restadjust,
//			      xs->sp);
//      restadjust = 0; /* Used up the &rest adjustment */
//
//      if (xs_new) xs = xs_new;   /* in case of error, keep old stack */
//      break;
//
//    case 0x23: /* calle */
//      temp = xs->sp;
//      xs->sp -= opparams[2] + 2 + (restadjust*2);
//
//      xs_new = execute_method(s, opparams[0], opparams[1], temp, xs->objp,
//			      GET_HEAP(xs->sp) + restadjust, xs->sp);
//      restadjust = 0; /* Used up the &rest adjustment */
//
//      if (xs_new)
//	xs = xs_new;   /* in case of error, keep old stack */
//      break;
//
//    case 0x24: /* ret */
//      do {
//	heap_ptr old_sp = xs->sp;
//	heap_ptr old_fp = xs->variables[VAR_TEMP];
//
//	if (s->execution_stack_pos == s->execution_stack_base) { /* Have we reached the base? */
//
//	  s->execution_stack_base = old_execution_stack_base; /* Restore stack base */
//
//	  --(s->execution_stack_pos);
//
//	  s->amp_rest = restadjust; /* Update &rest */
//	  return; /* Hard return */
//	}
//
//	if (s->execution_stack[s->execution_stack_pos].type == EXEC_STACK_TYPE_VARSELECTOR) {
//	  /* varselector access? */
//	  if (s->execution_stack[s->execution_stack_pos].argc) { /* write? */
//	    word temp = GET_HEAP(s->execution_stack[s->execution_stack_pos].variables[VAR_PARAM] + 2);
#if 0
      /* adjusted for GLUTTON usage */
      *(s->execution_stack[s->execution_stack_pos].sp) = VALUE_TO_STORE;
#endif
//	  } else /* No, read */
//	    s->acc = GET_HEAP(s->execution_stack[s->execution_stack_pos].pc);
//	}
//
//	/* No we haven't, so let's do a soft return */
//	--(s->execution_stack_pos);
//	xs = s->execution_stack + s->execution_stack_pos;
//
//	if (xs->sp == CALL_SP_CARRY) { /* Used in sends to 'carry' the stack pointer */
//	  xs->sp = old_sp;
//	  xs->variables[VAR_TEMP] = old_fp;
//	}
//
//      } while (s->execution_stack[s->execution_stack_pos].type == EXEC_STACK_TYPE_VARSELECTOR);
//      /* Iterate over all varselector accesses */
//
//      break;
//
//    case 0x25: /* send */
//      temp = xs->sp;
//      xs->sp -= (opparams[0] + (restadjust * 2)); /* Adjust stack */
//
//      xs_new = send_selector(s, s->acc, s->acc, temp, (int)opparams[0],
//			     (word)restadjust, xs->sp);
//
//      if (xs_new)
//	xs = xs_new;
//
//      restadjust = 0;
//
//      break;
//
//    case 0x28: /* class */
//      s->acc = CLASS_ADDRESS(opparams[0]);
//      break;
//
//    case 0x2a: /* self */
//      temp = xs->sp;
//      xs->sp -= (opparams[0] + (restadjust * 2)); /* Adjust stack */
//
//      xs_new = send_selector(s, xs->objp, xs->objp, temp,
//			     (int)opparams[0], (word)restadjust, xs->sp);
//
//      restadjust = 0;
//
//      if (xs_new) xs = xs_new;
//      break;
//
//    case 0x2b: /* super */
//      if ((opparams[0] < 0) || (opparams[0] >= s->classtable_size))
//	script_error(s, __FILE__, __LINE__, "Invalid superclass in object");
//      else {
//        int kludge;
//
//	temp = xs->sp;
//	xs->sp -= (opparams[1] + (restadjust * 2)); /* Adjust stack */
//        kludge = get_class_address(s, opparams[0]);
//	/* kludge necessary due to compiler bugs (egcs 2.91.66, at least) */
//	xs_new = send_selector(s, (heap_ptr)kludge, xs->objp, temp, (int)opparams[1], (word)restadjust, xs->sp);
//	restadjust = 0;
//
//	if (xs_new) xs = xs_new;
//      }
//
//      break;
//
//    case 0x2c: /* &rest */
//      utemp = opparams[0]; /* First argument */
//      restadjust = xs->argc - utemp + 1; /* +1 because utemp counts the paramcount while argc doesn't */
//      if (restadjust < 0)
//	restadjust = 0;
//      utemp2 = xs->variables[VAR_PARAM] + (utemp << 1);/* Pointer to the first argument to &restore */
//      for (; utemp <= xs->argc; utemp++) {
//	PUSH(getInt16(s->heap + utemp2));
//	utemp2 += 2;
//      }
//
//      break;
//
//    case 0x2d: /* lea */
//      utemp = opparams[0] >> 1;
//      var_number = utemp & 0x03; /* Get variable type */
//
//      utemp2 = xs->variables[var_number]; /* Get variable block offset */
//      if (utemp & 0x08)
//	utemp2 += (s->acc << 1); /* Add accumulator offset if requested */
//      s->acc = utemp2 + (opparams[1] << 1); /* Add index */
//      break;

		case 0x2e: /* selfID */
			s->r_acc = xs->objp;
			break;

		case 0x30: /* pprev */
			PUSH32(s->r_prev);
			break;

//    case 0x31: /* pToa */
//      s->acc = GET_HEAP(xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0]);
//      break;
//
//    case 0x32: /* aTop */
//      PUT_HEAP(xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0], s->acc);
//      break;
//
//    case 0x33: /* pTos */
//      temp2 = GET_HEAP(xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0]);
//      PUSH(temp2);
//      break;
//
//    case 0x34: /* sTop */
//      temp = POP();
//      PUT_HEAP(xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0], temp);
//      break;
//
//    case 0x35: /* ipToa */
//      temp = xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0];
//      s->acc = GET_HEAP(temp);
//      ++(s->acc);
//      PUT_HEAP(temp, s->acc);
//      break;
//
//    case 0x36: /* dpToa */
//      temp = xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0];
//      s->acc = GET_HEAP(temp);
//      --(s->acc);
//      PUT_HEAP(temp, s->acc);
//      break;
//
//    case 0x37: /* ipTos */
//      temp2 = xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0];
//      temp = GET_HEAP(temp2);
//      PUT_HEAP(temp2, temp + 1);
//      break;
//
//    case 0x38: /* dpTos */
//      temp2 = xs->objp + SCRIPT_SELECTOR_OFFSET + opparams[0];
//      temp = GET_HEAP(temp2);
//      PUT_HEAP(temp2, temp - 1);
//      break;
//
//    case 0x39: /* lofsa */
//      s->acc = opparams[0] + xs->pc;
//      if (opparams[0]+xs->pc>=0xFFFE)
//      {
//        sciprintf("VM: lofsa operation overflowed: 0x%x + 0x%x=0x%x\n", opparams[0], xs->pc,
//	          opparams[0]+xs->pc);
//	script_error_flag = script_debug_flag = 1;
//      }
//      break;
//
//    case 0x3a: /* lofss */
//      PUSH(opparams[0] + xs->pc);
//      if (opparams[0]+xs->pc>=0xFFFE)
//      {
//        sciprintf("VM: lofss operation overflowed: %x + %x=%x\n", opparams[0], xs->pc,
//	          opparams[0]+xs->pc);
//	script_error_flag = script_debug_flag = 1;
//      }
//      break;
//
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
			PUSH32(xs->objp);
			break;

		case 0x40: /* lag */
		case 0x41: /* lal */
		case 0x42: /* lat */
		case 0x43: /* lap */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0];
			s->r_acc = READ_VAR(var_type, var_number);
			break;

		case 0x44: /* lsg */
		case 0x45: /* lsl */
		case 0x46: /* lst */
		case 0x47: /* lsp */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0];
			PUSH32(READ_VAR(var_type, var_number));
			break;

		case 0x48: /* lagi */
		case 0x49: /* lali */
		case 0x4a: /* lati */
		case 0x4b: /* lapi */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0] + validate_arithmetic(s->r_acc);
			s->r_acc = READ_VAR(var_type, var_number);
			break;

		case 0x4c: /* lsgi */
		case 0x4d: /* lsli */
		case 0x4e: /* lsti */
		case 0x4f: /* lspi */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0] + validate_arithmetic(s->r_acc);
			PUSH32(READ_VAR(var_type, var_number));
			break;

		case 0x50: /* sag */
		case 0x51: /* sal */
		case 0x52: /* sat */
		case 0x53: /* sap */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0];
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x54: /* ssg */
		case 0x55: /* ssl */
		case 0x56: /* sst */
		case 0x57: /* ssp */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0];
			WRITE_VAR(var_type, var_number, POP32());
			break;

		case 0x58: /* sagi */
		case 0x59: /* sali */
		case 0x5a: /* sati */
		case 0x5b: /* sapi */
			/* Special semantics because it wouldn't really make a whole lot
			** of sense otherwise, with acc being used for two things
			** simultaneously... */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0] + validate_arithmetic(s->r_acc);
			WRITE_VAR(var_type, var_number, s->r_acc = POP32());
			break;

		case 0x5c: /* ssgi */
		case 0x5d: /* ssli */
		case 0x5e: /* ssti */
		case 0x5f: /* sspi */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0] + validate_arithmetic(s->r_acc);
			WRITE_VAR(var_type, var_number, POP32());
			break;

		case 0x60: /* +ag */
		case 0x61: /* +al */
		case 0x62: /* +at */
		case 0x63: /* +ap */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0];
			s->r_acc = make_reg(0,
					    1 + validate_arithmetic(READ_VAR(var_type,
									     var_number)));
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x64: /* +sg */
		case 0x65: /* +sl */
		case 0x66: /* +st */
		case 0x67: /* +sp */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0];
			r_temp = make_reg(0,
					  1 + validate_arithmetic(READ_VAR(var_type,
									   var_number)));
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		case 0x68: /* +agi */
		case 0x69: /* +ali */
		case 0x6a: /* +ati */
		case 0x6b: /* +api */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0] + validate_arithmetic(s->r_acc);
			s->r_acc = make_reg(0,
					    1 + validate_arithmetic(READ_VAR(var_type,
									     var_number)));
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x6c: /* +sgi */
		case 0x6d: /* +sli */
		case 0x6e: /* +sti */
		case 0x6f: /* +spi */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0] + validate_arithmetic(s->r_acc);
			r_temp = make_reg(0,
					  1 + validate_arithmetic(READ_VAR(var_type,
									   var_number)));
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		case 0x70: /* -ag */
		case 0x71: /* -al */
		case 0x72: /* -at */
		case 0x73: /* -ap */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0];
			s->r_acc = make_reg(0,
					    -1 + validate_arithmetic(READ_VAR(var_type,
									     var_number)));
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x74: /* -sg */
		case 0x75: /* -sl */
		case 0x76: /* -st */
		case 0x77: /* -sp */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0];
			r_temp = make_reg(0,
					  -1 + validate_arithmetic(READ_VAR(var_type,
									   var_number)));
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		case 0x78: /* -agi */
		case 0x79: /* -ali */
		case 0x7a: /* -ati */
		case 0x7b: /* -api */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0] + validate_arithmetic(s->r_acc);
			s->r_acc = make_reg(0,
					    -1 + validate_arithmetic(READ_VAR(var_type,
									     var_number)));
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x7c: /* -sgi */
		case 0x7d: /* -sli */
		case 0x7e: /* -sti */
		case 0x7f: /* -spi */
			var_type = (opcode >> 1) & 0x3; /* Gets the variable type: g, l, t or p */
			var_number = opparams[0] + validate_arithmetic(s->r_acc);
			r_temp = make_reg(0,
					  -1 + validate_arithmetic(READ_VAR(var_type,
									   var_number)));
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		default:
			script_error(s, __FILE__, __LINE__, "Illegal opcode");

		} /* switch(opcode >> 1) */

#warning "Exec stack assertion: fixme"
//
//    if (xs != s->execution_stack + s->execution_stack_pos) {
//      sciprintf("Error: xs is stale; last command was %02x\n", opnumber);
//    }
//
		if (script_error_flag) {
			_debug_step_running = 0; /* Stop multiple execution */
			_debug_seeking = 0; /* Stop special seeks */
			xs->pc = old_pc;
			xs->sp = old_sp;
		} else
			++script_step_counter;
	}
}


int
_lookup_selector_functions(state_t *s, heap_ptr obj, int selectorid, heap_ptr *address, heap_ptr speciespos)
{
#warning "Re-implement selector lookup"
#if 0
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

	superclasspos = (speciespos)? speciespos : (CLASS_ADDRESS(superclass));

	return
		_lookup_selector_functions(s, superclasspos, selectorid, address, 0); /* Recurse to superclass */
#endif
}


int
lookup_selector(state_t *s, heap_ptr obj, int selectorid, heap_ptr *address)
{
#warning "Re-implement selector lookup"
#if 0
	int species = OBJ_SPECIES(obj);
	int heappos = CLASS_ADDRESS(species);
	int varselector_nr = GET_HEAP(heappos + SCRIPT_SELECTORCTR_OFFSET);
	int speciespos = 0; /* ALWAYS zero ATM... */
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
		_lookup_selector_functions(s, obj, selectorid, address, (heap_ptr)speciespos);
	/* Call recursive function selector seeker */
#endif
}


/* Detects early SCI versions by their different script header */
void script_detect_early_versions(state_t *s)
{
	int c;
	resource_t *script = {0};

	for (c = 0; c < 1000; c++) {
		if ((script = scir_find_resource(s->resmgr, sci_script, c, 0))) {

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
#warning "Fix script instantiation"
#if 1
	resource_t *script = scir_find_resource(s->resmgr, sci_script, script_nr, 0);
	heap_ptr pos;
	int objtype;
	unsigned int objlength;
	heap_ptr script_basepos;
	reg_t reg;
	reg_t reg_tmp;
	int seg_id;
	int magic_pos_adder; /* Usually 0; 2 for older SCI versions */
	

	if (!script) {
		sciprintf("Script 0x%x requested but not found\n", script_nr);
		/*    script_debug_flag = script_error_flag = 1; */
		return 0;
	}

        if (NULL == s)
	{
                sciprintf("vm.c: script_instantiate(): NULL passed for \"s\"\n");
		return 0;
	}

	if (s->seg_manager.isloaded (&s->seg_manager, script_nr, SCRIPT_ID)) { /* Is it already loaded? */
		int seg = s->seg_manager.seg_get ( &s->seg_manager, script_nr );
		s->seg_manager.increment_lockers( &s->seg_manager, seg, SEG_ID); /* Required by another script */
		return seg;
	}

/* fprintf(stderr,"Allocing %d(0x%x)\n", script->size, script->size); */
	if (!s->seg_manager.allocate( &s->seg_manager, s, script_nr, &seg_id )) { /* ALL YOUR SCRIPT BASE ARE BELONG TO US */
		sciprintf("Not enough heap space for script size 0x%x of script 0x%x, has 0x%x\n",
			  script->size, script_nr, heap_largest(s->_heap));
		script_debug_flag = script_error_flag = 1;
		return 0;
	}
	reg.segment = seg_id;
	
	script_basepos = s->seg_manager.get_heappos( &s->seg_manager, reg.segment, SEG_ID);	// this is zero

	recursive = 1;
	/* Set heap position (beyond the size word) */
	s->seg_manager.set_lockers( &s->seg_manager, 1, reg.segment, SEG_ID );
	s->seg_manager.set_export_table_offset( &s->seg_manager, 0, reg.segment, SEG_ID );
	s->seg_manager.set_synonyms_offset( &s->seg_manager, 0, reg.segment, SEG_ID );
	s->seg_manager.set_synonyms_nr( &s->seg_manager, 0, reg.segment, SEG_ID );
	s->seg_manager.set_localvar_offset( &s->seg_manager, 0, reg.segment, SEG_ID );
	
	if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER) {
		int locals_size = getUInt16(script->data)*2;
		int locals = (locals_size)? script->size : 0;

		s->seg_manager.set_localvar_offset( &s->seg_manager, locals, reg.segment, SEG_ID );

		if (locals)
			s->seg_manager.mset( &s->seg_manager, locals, 0, locals_size, reg.segment, SEG_ID);
			//memset(s->heap+locals,0,locals_size);
		/* Old script block */
		/* There won't be a localvar block in this case */
/* HEAP CORRUPTOR! */
/*
 fprintf(stderr,"script of size %d(+2) -> %04x\n", script->size, script_basepos);
 fprintf(stderr,"   -- memcpying [%04x..%04x]\n", script_basepos + 2, script_basepos + script->size);
*/
		s->seg_manager.mcpy_in_out( &s->seg_manager, 0, script->data + 2, script->size - 2, reg.segment, SEG_ID);
		reg.offset = script_basepos;
		magic_pos_adder = 2;
	} else {
		s->seg_manager.mcpy_in_out( &s->seg_manager, 0, script->data, script->size, reg.segment, SEG_ID);
		magic_pos_adder = 0;
		reg.offset = script_basepos;
	}

	/* Now do a first pass through the script objects to find the
	** export table and local variable block
	*/

	objlength = 0;
	reg_tmp = reg;
	do {
		reg.offset += objlength; /* Step over the last checked object */

		objtype = GET_HEAP(s, reg, SCRIPT_BUFFER);
		reg_tmp.offset = reg.offset + 2;
		objlength = GET_HEAP(s, reg_tmp, SCRIPT_BUFFER);

		if (objtype == sci_obj_exports)
			s->seg_manager.set_export_table_offset( &s->seg_manager, reg.offset + 4, reg.segment, SEG_ID ); /* +4 is to step over the header */

		else if (objtype == sci_obj_synonyms) {
			s->seg_manager.set_synonyms_offset( &s->seg_manager, reg.offset + 4, reg.segment, SEG_ID ); /* +4 is to step over the header */
			s->seg_manager.set_synonyms_nr( &s->seg_manager, (objlength) / 4, reg.segment, SEG_ID );
			reg_tmp.offset = s->seg_manager.get_synonyms_offset( &s->seg_manager, reg.segment, SEG_ID ) +
				     ((s->seg_manager.get_synonyms_nr( &s->seg_manager, reg.segment, SEG_ID )) << 2);
				     
			if (GET_HEAP(s, reg_tmp, SCRIPT_BUFFER) < 0)
				/* Adjust for "terminal" synonym entries */
				s->seg_manager.set_synonyms_nr( &s->seg_manager, (objlength) / 4 - 1, reg.segment, SEG_ID );

		} else if (objtype == sci_obj_class) {
			heap_ptr classpos = reg.offset - SCRIPT_OBJECT_MAGIC_OFFSET + 4/* Header */;
			int species = 0;// OBJ_SPECIES(classpos);
#warning fix the OBJ_SPECIES!!
			if (species < 0 || species >= s->classtable_size) {
				sciprintf("Invalid species %d(0x%x) not in interval "
					  "[0,%d) while instantiating script %d\n",
					  species, species, s->classtable_size,
					  script_nr);
				script_debug_flag = script_error_flag = 1;
				return 1;
			}

			s->classtable[species].class_offset = classpos - magic_pos_adder - script_basepos; // ?????
			s->classtable[species].script = script_nr;
			s->classtable[species].scriptposp = &(s->scripttable[script_nr].heappos); //??????????? !!!!!!

		}

		if (objtype == sci_obj_localvars)
			s->seg_manager.set_localvar_offset( &s->seg_manager, reg.offset + 4, reg.segment, SEG_ID );/* +4 is to step over the header */

	} while (objtype != 0);


	/* And now a second pass to adjust objects and class pointers, and the general pointers */
	reg.offset = s->seg_manager.get_heappos( &s->seg_manager, reg.segment, SEG_ID) + magic_pos_adder;

	objlength = 0;
	reg_tmp = reg;
	do {
		pos += objlength; /* Step over the last checked object */

		objtype = GET_HEAP(s, reg, SCRIPT_BUFFER);
		reg_tmp.offset = reg.offset + 2;
		objlength = GET_HEAP(s, reg_tmp, SCRIPT_BUFFER);

		reg.offset += 4; /* Step over header */

		if ((objtype == sci_obj_object) || (objtype == sci_obj_class)) { /* object or class? */
			int i;
			heap_ptr functarea;
			int functions_nr;
			int superclass;
			int species;

			reg.offset -= SCRIPT_OBJECT_MAGIC_OFFSET; /* Get into home position */

			reg_tmp.offset = reg.offset + SCRIPT_FUNCTAREAPTR_OFFSET;
			functarea = reg.offset + GET_HEAP(s, reg_tmp, SCRIPT_BUFFER)
				+ SCRIPT_FUNCTAREAPTR_MAGIC;
			reg_tmp.offset = functarea - 2;
			functions_nr = GET_HEAP(s, reg_tmp, SCRIPT_BUFFER); /* Number of functions */
			superclass = 0;// OBJ_SUPERCLASS(reg.offset); /* Get superclass... */
#warning fix OBJ_SUPERCLASS!!!
			species = 0; //OBJ_SPECIES(reg.offset); /* ...and species */
#warning fix OBJ_SPECIES!!!!
			/*      fprintf(stderr,"functs (%x) area @ %x\n", functions_nr, functarea); */

			/* This sets the local variable pointer: */
			reg_tmp.offset = reg.offset + SCRIPT_LOCALVARPTR_OFFSET;
			PUT_HEAP(s, reg_tmp, s->seg_manager.get_localvar_offset( &s->seg_manager, reg.segment, SEG_ID ), SCRIPT_BUFFER);
			/* Now set the superclass address: */
			if (superclass > -1) {
				reg_tmp.offset = reg.offset + SCRIPT_SUPERCLASS_OFFSET;
				PUT_HEAP(s, reg_tmp, get_class_address(s, superclass), SCRIPT_BUFFER);
			}
			
			reg_tmp.offset = reg.offset + SCRIPT_SPECIES_OFFSET;
			PUT_HEAP(s, reg_tmp, get_class_address(s, species), SCRIPT_BUFFER);

			functarea += 2 + functions_nr * 2;
			/* Move over the selector IDs to the actual addresses */

			for (i = 0; i < functions_nr * 2; i += 2) {
				heap_ptr functpos;
				reg_tmp.offset = functarea + i;
				functpos = GET_HEAP(s, reg_tmp, SCRIPT_BUFFER);
				reg_tmp.offset = functarea + i;
				PUT_HEAP(s, reg_tmp, functpos + script_basepos, SCRIPT_BUFFER); /* Adjust function pointer addresses */
			}

			if ((superclass >= 0) && recursive)
				script_instantiate(s, s->classtable[superclass].script, 1);
			/* Recurse to assure that the superclass is available */

			reg.offset += SCRIPT_OBJECT_MAGIC_OFFSET; /* Step back from home to base */

		} /* if object or class */
		else if (objtype == sci_obj_pointers) { /* A relocation table */
			int pointerc = GET_HEAP(s, reg, SCRIPT_BUFFER);
			int i;

			for (i = 0; i < pointerc; i++) {
				int new_address;
				int old_indexed_pointer;
				reg_tmp.offset = reg.offset + 2 + i*2;
				new_address = ((guint16) GET_HEAP(s, reg_tmp, SCRIPT_BUFFER)) + script_basepos;
				PUT_HEAP(s, reg_tmp, new_address, SCRIPT_BUFFER); /* Adjust pointers. Not sure if this is needed. */
				reg_tmp.offset = new_address;
				old_indexed_pointer = ((guint16) GET_HEAP(s, reg_tmp, SCRIPT_BUFFER));
				reg_tmp.offset = new_address;
				PUT_HEAP(s, reg_tmp, old_indexed_pointer + script_basepos, SCRIPT_BUFFER);
				/* Adjust indexed pointer. */

			} /* For all indexed pointers pointers */

		}

		reg.offset -= 4; /* Step back on header */

	} while ((objtype != 0) && (((unsigned)reg.offset - script_basepos) < script->size - 2));

	/*    if (script_nr == 0)   sci_hexdump(s->heap + script_basepos +2, script->size-2, script_basepos);*/
	return s->scripttable[script_nr].heappos;	//???
#endif

}


void
script_uninstantiate(state_t *s, int script_nr)
{
#warning "Fix script uninstantiation"
#if 0
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

	if ((s->scripttable[script_nr].localvar_offset)&&
	    (s->version<SCI_VERSION_FTU_NEW_SCRIPT_HEADER))
		heap_free(s->_heap, s->scripttable[script_nr].localvar_offset-2);

	s->scripttable[script_nr].localvar_offset = 0;
	s->scripttable[script_nr].export_table_offset = 0;

	heap_free(s->_heap, handle - 2); /* Unallocate script on the heap */

	if (script_checkloads_flag)
		sciprintf("Unloaded script 0x%x.\n", script_nr);

	return;
#endif
}


static void
_init_stack_base_with_selector(state_t *s, selector_t selector)
{
	s->stack_base[0] = make_reg(0, (word) selector);
	s->stack_base[1] = NULL_REG;
}

static state_t *
_game_run(state_t *s, int restoring)
{
	state_t *successor = NULL;
	int game_is_finished = 0;
	do {
		s->execution_stack_pos_changed = 0;
		run_vm(s, (successor || restoring)? 1 : 0);
		if (s->restarting_flags & SCI_GAME_IS_RESTARTING_NOW) { /* Restart was requested? */

			sci_free(s->execution_stack);
			s->execution_stack = NULL;
			s->execution_stack_pos = -1;
			s->execution_stack_pos_changed = 0;

			game_exit(s);
			game_init(s);
			_init_stack_base_with_selector(s, s->selector_map.play);
			/* Call the play selector */

			send_selector(s, s->game_obj, s->game_obj,
				      s->stack_base, 2, 0, s->stack_base);

			script_abort_flag = 0;
			s->restarting_flags = SCI_GAME_WAS_RESTARTED |
				SCI_GAME_WAS_RESTARTED_AT_LEAST_ONCE;

		}
		else {
			successor = s->successor;
			if (successor) {
				game_exit(s);
				script_free_vm_memory(s);
				sci_free(s);
				s = successor;

				if (!send_calls_allocated)
					send_calls = sci_calloc(sizeof(calls_struct_t),
								send_calls_allocated = 16);

				if (script_abort_flag == SCRIPT_ABORT_WITH_REPLAY) {
					sciprintf("Restarting with replay()\n");
					s->execution_stack_pos = -1; /* Resatart with replay */
					
					_init_stack_base_with_selector(s, s->selector_map.replay);
					/* Call the replay selector */

					send_selector(s, s->game_obj, s->game_obj,
						      s->stack_base, 2,
						      0, s->stack_base);
				}

				script_abort_flag = 0;

				SCI_MEMTEST;
			} else
				game_is_finished = 1;
		}
	} while (!game_is_finished);

	return s;
}

int
game_run(state_t **_s)
{
	state_t *s = *_s;

	sciprintf(" Calling %s::play()\n", s->game_name);
	_init_stack_base_with_selector(s, s->selector_map.play); /* Call the play selector */
	

	/* Now: Register the first element on the execution stack- */
	if (!send_selector(s, s->game_obj, s->game_obj,
			   s->stack_base, 2, 0,
			   s->stack_base) || script_error_flag) {
		sciprintf("Failed to run the game! Aborting...\n");
		return 1;
	}
	/* and ENGAGE! */
	*_s = s = _game_run(s, 0);

	sciprintf(" Game::play() finished.\n");
	return 0;
}

int
game_restore(state_t **_s, char *game_name)
{
	state_t *s;
	int debug_state = _debugstate_valid;

	sciprintf("Restoring savegame '%s'...\n", game_name);
	fprintf(stderr, "Quick-restore disabled until VM reconstruction is complete!");
	return 0;
#if 0
	s = gamestate_restore(*_s, game_name);
#endif

	if (!s) {
		sciprintf("Restoring gamestate '%s' failed.\n", game_name);
		return 1;
	}
	_debugstate_valid = debug_state;
	script_abort_flag = 0;
	s->restarting_flags = 0;

	*_s = s = _game_run(s, 1);

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


void
quit_vm()
{
	script_abort_flag = 1; /* Terminate VM */
	_debugstate_valid = 0;
	_debug_seeking = 0;
	_debug_step_running = 0;
}
