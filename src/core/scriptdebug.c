/***************************************************************************
 scriptdebug.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Script debugger functionality. Absolutely not threadsafe. */

#include <resource.h>
#include <vm.h>
#include <console.h>

int _debugstate_valid = 0; /* Set to 1 while script_debug is running */
int _debug_step_running = 0; /* Set to >0 to allow multiple stepping */
int _debug_commands_not_hooked = 1; /* Commands not hooked to the console yet? */
int _debug_seeking = 0; /* Stepping forward until some special condition is met */

#define _DEBUG_SEEK_NOTHING 0
#define _DEBUG_SEEK_CALLK 1 /* Step forward until callk is found */

state_t *_s;
heap_ptr *_pc;
heap_ptr *_sp;
heap_ptr *_pp;
heap_ptr *_objp;
int *_restadjust;


char inputbuf[256] = "";

char *
_debug_get_input_default()
{
  char newinpbuf[256];

  printf("> ");
  fgets(newinpbuf, 254, stdin);

  if (strlen(newinpbuf) != 0)
    memcpy(inputbuf, newinpbuf, 256);

  return inputbuf;
}

char *(*_debug_get_input)(void) = _debug_get_input_default;


int
c_debuginfo()
{
  sciprintf("pc=%04x acc=%04x o=%04x fp=%04x sp=%04x\n", *_pc & 0xffff, _s->acc & 0xffff,
	    *_objp & 0xffff, *_pp & 0xffff, *_sp & 0xffff);
  sciprintf("prev=%x sbase=%04x globls=%04x &restmod=%x\n",
	    _s->prev & 0xffff, _s->stack_base & 0xffff, _s->global_vars & 0xffff,
	    *_restadjust & 0xffff);
}

int
c_step()
{
  _debugstate_valid = 0;
  if (cmd_paramlength && (cmd_params[0].val > 0))
    _debug_step_running = cmd_params[0].val;
}

int
c_heapdump()
{
  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  if (cmd_params[1].val < 0) {
    cmd_params[0].val += cmd_params[1].val;
    cmd_params[0].val = -cmd_params[0].val;
  }
  if (cmd_params[0].val < 0)
    cmd_params[0].val = 0;
  if (cmd_params[0].val > 0xffff) {
    sciprintf("Heap index out of range\n");
    return 1;
  }
  if (cmd_params[0].val + cmd_params[1].val > 0xffff) {
    cmd_params[1].val = 0xffff - cmd_params[0].val;
  }

  sci_hexdump(_s->heap + cmd_params[0].val, cmd_params[1].val, cmd_params[0].val);
}


int
c_classtable()
{
  int i;
  heap_ptr spos; /* scriptpos */

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  sciprintf("Available classes:\n");
  for (i = 0; i < _s->classtable_size; i++)
    if (_s->classtable[i].scriptposp && (spos = *(_s->classtable[i].scriptposp)))
      sciprintf(" Class 0x%x at %04x (script 0x%x)\n", i, spos + _s->classtable[i].class_offset,
		_s->classtable[i].script);

}

int
c_stack()
{
  int i;

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  for (i = cmd_params[0].val ; i > 0; i--)
    sciprintf("[sp-%04x] = %04x\n", i * 2, 0xffff & getInt16(_s->heap + *_sp - i*2));

}

int
c_scriptinfo()
{
  int i;

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  sciprintf("Available scripts:\n");
  for (i = 0; i < 1000; i++)
    if (_s->scripttable[i].heappos)
      sciprintf("%03x at %04x, locals at %04x\n   exports at %04x, %d lockers\n",
		i, _s->scripttable[i].heappos, _s->scripttable[i].localvar_offset,
		_s->scripttable[i].export_table_offset, _s->scripttable[i].lockers);
}

heap_ptr
disassemble(state_t *s, heap_ptr pos)
/* Disassembles one command from the heap, returns address of next command or 0 if a ret was
** encountered.
*/
{
  heap_ptr retval = pos + 1;
  word param_value;
  int opsize = s->heap[pos];
  int opcode = opsize >> 1;
  int i = 0;

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  opsize &= 1; /* byte if true, word if false */

  sciprintf(" [%c] %s", opsize? 'B' : 'W', s->opcodes[opcode].name);

  while (formats[opcode][i])

    switch (formats[opcode][i++]) {

    case Script_Invalid: sciprintf("-Invalid operation-"); break;

    case Script_Byte: sciprintf(" %02x", s->heap[retval++]); break;

    case Script_Word:
    case Script_SWord:
      sciprintf(" %04x", 0xffff & (s->heap[retval] | (s->heap[retval+1] << 8)));
      retval += 2;
      break;

    case Script_Variable:
      if (opsize)
	param_value = s->heap[retval++];
      else {
	param_value = 0xffff & (s->heap[retval] | (s->heap[retval+1] << 8));
	retval += 2;
      }

      if (opcode == op_callk)
	sciprintf(" %s[%x]", (param_value < s->kernel_names_nr)
		  ? s->kernel_names[param_value] : "<invalid>", param_value);
      else sciprintf(opsize? " %02x" : " %04x", param_value);

      break;

    case Script_End: retval = 0;

    }

  
  sciprintf("\n");

  if (pos == *_pc) { /* Extra information if debugging the current opcode */

    if ((opcode == op_send) || (opcode == op_super) || (opcode == op_self)) {
      int stackframe = s->heap[retval - 1] + (*_restadjust * 2);
      word selector;

      while (stackframe > 0) {
	int argc = getInt16(s->heap + *_sp - stackframe + 2);

	selector = getInt16(s->heap + *_sp - stackframe);
      
	sciprintf("  %s(", (selector > s->selector_names_nr)
		  ? "<invalid>" : s->selector_names[selector]);

	while (argc--) {

	  sciprintf("%04x", getInt16(s->heap + *_sp - stackframe));
	  if (argc) sciprintf(", ");
	  stackframe -= 2;

	}

	sciprintf(")\n");

	stackframe -= 4;
      } /* while (stackframe > 0) */

    } /* Send-like opcodes */

  } /* (heappos == *_pc) */

  return retval;
}


int
c_backtrace()
{
  int i;

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  sciprintf("Call stack:\n");
  for (i = 0; i <= script_exec_stackpos; i++) {
    script_exec_stack_t *call = &(script_exec_stack[i]);
    heap_ptr namepos = getInt16(_s->heap + *(call->objpp) + SCRIPT_NAME_OFFSET);
    int paramc, totalparamc;

    sciprintf(" %x:  %s::%s(", i, _s->heap + namepos, (call->selector == -1)? "<call[be]?>":
	      _s->selector_names[call->selector]);

    totalparamc = *(call->argcp);

    if (totalparamc > 16)
      totalparamc = 16;

    for (paramc = 1; paramc <= totalparamc; paramc++) {
      sciprintf("%04x", getInt16(_s->heap + *(call->argpp) + paramc * 2));

      if (paramc < *(call->argcp))
	sciprintf(", ");
    }

    if (*(call->argcp) > 16)
      sciprintf("...");

    sciprintf(")\n    obj@%04x pc=%04x sp=%04x fp=%04x\n", *(call->objpp), *(call->pcp),
	      *(call->spp), *(call->ppp));
  }
}

int
c_disasm()
{
  int vpc = cmd_params[0].val;

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  if (vpc < 0)
    return;

  if (cmd_paramlength > 1)
    if (cmd_params[1].val < 0)
      return;

  do {
    vpc = disassemble(_s, vpc);
  } while ((vpc < 0xfff2) && (cmd_paramlength > 1) && (cmd_params[1].val--));
}


int
c_snk()
{
  _debug_seeking = _DEBUG_SEEK_CALLK;
  _debugstate_valid = 0;
}

int
objinfo(heap_ptr pos)
{
  word type;

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  if ((pos < 4) || (pos > 0xfff0)) {
    sciprintf("Invalid address.\n");
    return 1;
  }

  if ((getInt16(_s->heap + pos + SCRIPT_OBJECT_MAGIC_OFFSET)) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    sciprintf("Not an object.\n");
    return 0;
  }

  type = getInt16(_s->heap + pos + SCRIPT_OBJECT_MAGIC_OFFSET - 4);

  if (type == sci_obj_object)
    sciprintf("Object");
  else if (type == sci_obj_class)
    sciprintf("Class");
  else {
    sciprintf("Not an object.\n");
    return 0;
  }

  {
    word selectors, functions;
    byte* selectorIDoffset;
    byte* selectoroffset;
    byte* functIDoffset;
    byte* functoffset;
    word localvarptr = getInt16(_s->heap + pos + SCRIPT_LOCALVARPTR_OFFSET);
    word species = getInt16(_s->heap + pos + SCRIPT_SPECIES_OFFSET);
    word superclass = getInt16(_s->heap + pos + SCRIPT_SUPERCLASS_OFFSET);
    word namepos = getInt16(_s->heap + pos + SCRIPT_NAME_OFFSET);
    int i;

    sciprintf(" %s\n", _s->heap + namepos);
    sciprintf("Species=%04x, Superclass=%04x\n", species, superclass);
    sciprintf("Local variables @ 0x%04x\n", localvarptr);
    
    selectors = getInt16(_s->heap + pos + SCRIPT_SELECTORCTR_OFFSET);

    selectoroffset = _s->heap + pos + SCRIPT_SELECTOR_OFFSET;

    if (type == sci_obj_class)
      selectorIDoffset = selectoroffset + selectors * 2;
    else
      selectorIDoffset =
	_s->heap
	+ *(_s->classtable[species].scriptposp)
	+ _s->classtable[species].class_offset
	+ SCRIPT_SELECTOR_OFFSET
	+ selectors * 2;

    functIDoffset = _s->heap + pos + SCRIPT_FUNCTAREAPTR_MAGIC
      + getInt16(_s->heap + pos + SCRIPT_FUNCTAREAPTR_OFFSET);

    functions = getInt16(functIDoffset - 2);

    functoffset = functIDoffset + 2 + functions * 2;

    if (selectors > 0) {
      sciprintf("Variable selectors:\n");

      for (i = 0; i < selectors; i++) {
	word selectorID = selectorIDoffset? getInt16(selectorIDoffset + i*2) : i;

	sciprintf("  %s[%04x] = %04x\n", selectorIDoffset? _s->selector_names[selectorID] : "<?>",
		  selectorID, 0xffff & getInt16(selectoroffset + i*2));
      }
    } /* if variable selectors are present */


    if (functions > 0) {
      sciprintf("Method selectors:\n");

      for (i = 0; i < functions; i++) {
	word selectorID = getInt16(functIDoffset + i*2);

	sciprintf("  %s[%04x] at %04x\n", _s->selector_names[selectorID], selectorID,
		  0xffff & getInt16(functoffset + i*2));
      }
    } /* if function selectors are present */
  }
  return 0;
}

int
c_heapobj()
{
  return
    objinfo(cmd_params[0].val);
}

int
c_obj()
{
  return
    objinfo(*_objp);
}

int
c_accobj()
{
  return
    objinfo(_s->acc);
}

void
script_debug(state_t *s, heap_ptr *pc, heap_ptr *sp, heap_ptr *pp, heap_ptr *objp, int *restadjust)
{

  if (_debug_seeking) { /* Are we looking for something special? */
    int op = s->heap[*pc] >> 1;

    switch (_debug_seeking) {


    case _DEBUG_SEEK_CALLK: {
      if (op != op_callk) return;
      break;
    }

    } /* switch(_debug_seeking) */

    _debug_seeking = _DEBUG_SEEK_NOTHING; /* OK, found whatever we were looking for */

  } /* if (_debug_seeking) */


  _debugstate_valid = (_debug_step_running == 0);

  if (_debugstate_valid) {
    _s = s;
    _pc = pc;
    _sp = sp;
    _pp = pp;
    _objp = objp;
    _restadjust = restadjust;

    c_debuginfo();
    sciprintf("Step #%d\n", script_step_counter);
    disassemble(s, *pc);

    if (_debug_commands_not_hooked) {

      _debug_commands_not_hooked = 0;

      cmdHook(c_debuginfo, "registers", "", "Displays all current register values");
      cmdHook(c_step, "s", "i*", "Executes one or several operations\n\nEXAMPLES\n\n"
	      "    s 4\n\n  Execute 4 commands\n\n    s\n\n  Execute next command");
      cmdHook(c_heapdump, "heapdump", "ii", "Dumps data from the heap\n"
	      "\nEXAMPLE\n\n    heapdump 0x1200 42\n\n  Dumps 42 bytes starting at heap address\n"
	      "  0x1200");
      cmdHook(c_disasm, "disasm", "ii*", "Disassembles one or more commands\n\n"
	      "USAGE\n\n  disasm [startaddr] <number of commands to disassemble>");
      cmdHook(c_scriptinfo, "scriptinfo", "", "Displays information about all\n  loaded scripts");
      cmdHook(c_heapobj, "heapobj", "i", "Displays information about an\n  object or class on the\n"
	      "specified heap address.\n\nSEE ALSO\n\n  obj, accobj");
      cmdHook(c_obj, "obj", "", "Displays information about the\n  currently active object/class.\n"
	      "\n\nSEE ALSO\n\n  heapobj, accobj");
      cmdHook(c_accobj, "accobj", "", "Displays information about an\n  object or class at the\n"
	      "address indexed by acc.\n\nSEE ALSO\n\n  obj, heapobj");
      cmdHook(c_classtable, "classtable", "", "Lists all available classes");
      cmdHook(c_stack, "stack", "i", "Dumps the specified number of stack elements");
      cmdHook(c_backtrace, "bt", "", "Dumps the send/self/super/call/calle/callb stack");
      cmdHook(c_snk, "snk", "", "Steps forward until it hits the next\n  callk operation");

      cmdHookInt(&script_exec_stackpos, "script_exec_stackpos", "Position on the execution stack\n");
      cmdHookInt(&script_debug_flag, "script_debug_flag", "Set != 0 to enable debugger\n");
      cmdHookInt(&script_checkloads_flag, "script_checkloads_flag", "Set != 0 to display information\n"
		 "  when scripts are loaded or unloaded");
      cmdHookInt(&script_abort_flag, "script_abort_flag", "Set != 0 to abort execution\n");
      cmdHookInt(&script_step_counter, "script_step_counter", "# of executed SCI operations\n");

    } /* If commands were not hooked up */

  }

  if (_debug_step_running)
    _debug_step_running--;

  while (_debugstate_valid) {
    cmdParse(_debug_get_input());
    sciprintf("\n");
  }
}
