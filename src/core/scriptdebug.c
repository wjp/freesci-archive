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
#include <kdebug.h>

int _debugstate_valid = 0; /* Set to 1 while script_debug is running */
int _debug_step_running = 0; /* Set to >0 to allow multiple stepping */
int _debug_commands_not_hooked = 1; /* Commands not hooked to the console yet? */
int _debug_seeking = 0; /* Stepping forward until some special condition is met */
int _debug_seek_level = 0; /* Used for seekers that want to check their exec stack depth */
int _debug_seek_special = 0; /* Used for special seeks */

#define _DEBUG_SEEK_NOTHING 0
#define _DEBUG_SEEK_CALLK 1 /* Step forward until callk is found */
#define _DEBUG_SEEK_LEVEL_RET 2 /* Step forward until returned from this level */
#define _DEBUG_SEEK_SPECIAL_CALLK 3 /* Step forward until a /special/ callk is found */

state_t *_s;
heap_ptr *_pc;
heap_ptr *_sp;
heap_ptr *_pp;
heap_ptr *_objp;
int *_restadjust;


char inputbuf[256] = "";

char *
_debug_get_input_default(void)
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
c_debuginfo(void)
{
  sciprintf("pc=%04x acc=%04x o=%04x fp=%04x sp=%04x\n", *_pc & 0xffff, _s->acc & 0xffff,
	    *_objp & 0xffff, *_pp & 0xffff, *_sp & 0xffff);
  sciprintf("prev=%x sbase=%04x globls=%04x &restmod=%x\n",
	    _s->prev & 0xffff, _s->stack_base & 0xffff, _s->global_vars & 0xffff,
	    *_restadjust & 0xffff);
  return 0;
}

int
c_step(void)
{
  _debugstate_valid = 0;
  if (cmd_paramlength && (cmd_params[0].val > 0))
    _debug_step_running = cmd_params[0].val;
  return 0;
}

int
c_heapdump(void)
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
  return 0;
}


int
c_classtable(void)
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

  return 0;
}

int
c_stack(void)
{
  int i;

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  for (i = cmd_params[0].val ; i > 0; i--)
    sciprintf("[sp-%04x] = %04x\n", i * 2, 0xffff & getInt16(_s->heap + *_sp - i*2));

  return 0;
}

int
c_scriptinfo(void)
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

  return 0;
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

    case Script_SByte:
    case Script_Byte: sciprintf(" %02x", s->heap[retval++]); break;

    case Script_Word:
    case Script_SWord:
      sciprintf(" %04x", 0xffff & (s->heap[retval] | (s->heap[retval+1] << 8)));
      retval += 2;
      break;

    case Script_SVariable:
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

    if (opcode == op_callk) {
      int stackframe = s->heap[retval - 1] + (*_restadjust * 2);
      int argc = getInt16(s->heap + *_sp - stackframe - 2);
      int i;

      sciprintf(" Kernel params: (");

      for (i = 0; i < argc; i++) {
	sciprintf("%04x", getUInt16(s->heap + *_sp - stackframe + i*2));
	if (i+1 < argc)
	  sciprintf(", ");
      }
      sciprintf(")\n");

    } else

    if ((opcode == op_send) || (opcode == op_super) || (opcode == op_self)) {
      int restmod = *_restadjust;
      int stackframe = s->heap[retval - 1] + (restmod * 2);
      word selector;
      heap_ptr selector_addr;

      while (stackframe > 0) {
	int argc = getInt16(s->heap + *_sp - stackframe + 2);
	heap_ptr nameaddress = 0;
	heap_ptr called_obj_addr;
	char *name;

	if (opcode == op_send)
	  called_obj_addr = s->acc;
	else if (opcode == op_self)
	  called_obj_addr = *_objp;
	else { /* op_super */
	  called_obj_addr = s->heap[retval - 2];

	  called_obj_addr = *(s->classtable[called_obj_addr].scriptposp)
	    + s->classtable[called_obj_addr].class_offset;
	}

	selector = getInt16(s->heap + *_sp - stackframe);
      
	if (called_obj_addr > 100) /* If we are in valid heap space */
	  if (getInt16(s->heap + called_obj_addr + SCRIPT_OBJECT_MAGIC_OFFSET)
	      == SCRIPT_OBJECT_MAGIC_NUMBER)
	    nameaddress = getUInt16(s->heap + called_obj_addr + SCRIPT_NAME_OFFSET);

	if (nameaddress)
	  name = s->heap + nameaddress;
	else
	  name = "<invalid>";

	sciprintf("  %s::%s[", name, (selector > s->selector_names_nr)
		  ? "<invalid>" : s->selector_names[selector]);

	switch (lookup_selector(s, called_obj_addr, selector, &selector_addr)) {
	case SELECTOR_METHOD:
	  sciprintf("FUNCT");
	  argc += restmod;
	  restmod = 0;
	  break;
	case SELECTOR_VARIABLE:
	  sciprintf("VAR");
	  break;
	case SELECTOR_NONE:
	  sciprintf("INVALID");
	  break;
	}

	sciprintf("](");

	while (argc--) {

	  sciprintf("%04x", 0xffff & getUInt16(s->heap + *_sp - stackframe + 4));
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
c_backtrace(void)
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
      sciprintf("%04x", getUInt16(_s->heap + *(call->argpp) + paramc * 2));

      if (paramc < *(call->argcp))
	sciprintf(", ");
    }

    if (*(call->argcp) > 16)
      sciprintf("...");

    sciprintf(")\n    obj@%04x pc=%04x sp=%04x fp=%04x\n", *(call->objpp), *(call->pcp),
	      *(call->spp), *(call->ppp));
  }
  return 0;
}

int
c_refresh_screen(void)
{
  _s->graphics_callback(_s, GRAPHICS_CALLBACK_REDRAW_ALL,0,0,0,0);
  return 0;
}

int
c_redraw_screen(void)
{
  graph_update_box(_s, 0, 0, 320, 200);
  return 0;
}

int
c_disasm(void)
{
  int vpc = cmd_params[0].val;

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  if (vpc < 0)
    return 0;

  if (cmd_paramlength > 1)
    if (cmd_params[1].val < 0)
      return 0;

  do {
    vpc = disassemble(_s, vpc);
  } while ((vpc < 0xfff2) && (cmd_paramlength > 1) && (cmd_params[1].val--));
  return 0;
}


int
c_snk(void)
{
  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  if (cmd_paramlength > 0) {
    _debug_seeking = _DEBUG_SEEK_SPECIAL_CALLK;
    _debug_seek_special = cmd_params[0].val;
    _debugstate_valid = 0;
  } else {
    _debug_seeking = _DEBUG_SEEK_CALLK;
    _debugstate_valid = 0;
  }
  return 0;
}

int
c_sret(void)
{
  _debug_seeking = _DEBUG_SEEK_LEVEL_RET;
  _debug_seek_level = script_exec_stackpos;
  _debugstate_valid = 0;
  return 0;
}


int
c_set_acc(void)
{
  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  _s->acc = cmd_params[0].val;
  return 0;
}

int
c_resource_id(void)
{
  int id = cmd_params[0].val;

  sciprintf("%s.%d (0x%x)\n", Resource_Types[id >> 11], id &0x7ff, id & 0x7ff);
  return 0;
}

int
c_heap_free(void)
{
  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  heap_dump_free(_s->_heap);
  return 0;
}

int
c_listclones(void)
{
  int i, j = 0;
  sciprintf("Listing all logged clones:\n");
  for (i = 0; i < SCRIPT_MAX_CLONES; i++)
    if (_s->clone_list[i]) {
      sciprintf("  Clone at %04x\n", _s->clone_list[i]);
      j++;
    }

  sciprintf("Total of %d clones.\n", j);
  return 0;
}


int
c_debuglog(void)
{
  int i;
  char *parser;
  char modechars[] = "ulgcmf"; /* Valid parameter chars */

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  if (cmd_paramlength == 0) {
    for (i = 0; i < SCIk_DEBUG_MODES; i++)
      if (_s->debug_mode & (1 << i))
	sciprintf(" Logging %s\n", SCIk_Debug_Names[i]);

    return 0;
  }

  for (i = 0; i < cmd_paramlength; i++) {
    int mode;
    int seeker;
    char frob;

    if (cmd_params[i].str[0] == '+')
      mode = 1;
    else
      if (cmd_params[i].str[0] == '-')
	mode = 0;
      else {
	sciprintf("Parameter '%s' should start with + or -\n", cmd_params[i].str);
	return 1;
      }

    parser = cmd_params[i].str + 1;
    while (frob = *parser) {
      seeker = 0;

      if (frob == '*') { /* wildcard */
	if (mode) /* set all */
	  _s->debug_mode = 0xffffffff;
	else /* unset all */
	  _s->debug_mode = 0;

	parser++;
	continue;
      }

      while (modechars[seeker] && (modechars[seeker] != frob))
	seeker++;

      if (modechars[seeker] == '\0') {
	sciprintf("Invalid log mode parameter: %c\n", frob);
	return 1;
      }

      if (mode) /* Set */
	_s->debug_mode |= (1 << seeker);
      else /* UnSet */
	_s->debug_mode &= ~(1 << seeker);

      parser++;
    }
  }

  return 0;
}


int
show_list(heap_ptr list)
{
  heap_ptr prevnode = 0, node;
  int nodectr = 0;

  sciprintf("List at %04x:\n", list);
  node = getInt16(_s->heap + list );

  while (node) {
    nodectr++;
    sciprintf(" - Node at %04x: Key=%04x Val=%04x\n",node,
	      getUInt16(_s->heap + node + 4),getUInt16(_s->heap + node + 6));
    prevnode = node;
    node = getUInt16(_s->heap + node + 2); /* Next node */
  }

  if (prevnode != getUInt16(_s->heap + list + 2)) /* Lastnode != registered lastnode? */
    sciprintf("WARNING: List.LastNode %04x != last node %04x\n",
	      getUInt16(_s->heap + list + 2), prevnode);

  if (nodectr)
    sciprintf("%d registered nodes.\n", nodectr);
  else
    sciprintf("List is empty.\n");
  return 0;
}


int
c_show_list(void)
{
  heap_ptr list = cmd_params[0].val;

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  if (getInt16(_s->heap + list - 2) != 6) { /* List is hmalloc()'d to size 6 */
    sciprintf("No list at %04x.\n", list);
    return 1;
  }

  return 
    show_list(list);
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

  type = getInt16(_s->heap + pos + SCRIPT_INFO_OFFSET);

  if (type & SCRIPT_INFO_CLONE)
    sciprintf("Clone");
  else if (type & SCRIPT_INFO_CLASS)
    sciprintf("Class");
  else if (type == 0)
    sciprintf("Object");
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

    if (type & SCRIPT_INFO_CLASS)
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
c_heapobj(void)
{
  return
    objinfo(cmd_params[0].val);
}

int
c_obj(void)
{
  return
    objinfo(*_objp);
}

int
c_accobj(void)
{
  return
    objinfo(_s->acc);
}

void
script_debug(state_t *s, heap_ptr *pc, heap_ptr *sp, heap_ptr *pp, heap_ptr *objp, int *restadjust)
{

  if (_debug_seeking) { /* Are we looking for something special? */
    int op = s->heap[*pc] >> 1;
    int paramb1 = s->heap[*pc + 1]; /* Careful with that ! */

    switch (_debug_seeking) {

    case _DEBUG_SEEK_SPECIAL_CALLK:
      if (paramb1 != _debug_seek_special)
	return;

    case _DEBUG_SEEK_CALLK: {
      if (op != op_callk) return;
      break;
    }

    case _DEBUG_SEEK_LEVEL_RET: {
      if ((op != op_ret) || (_debug_seek_level < script_exec_stackpos)) return;
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
      cmdHook(c_scriptinfo, "scripttable", "", "Displays information about all\n  loaded scripts");
      cmdHook(c_heapobj, "heapobj", "i", "Displays information about an\n  object or class on the\n"
	      "specified heap address.\n\nSEE ALSO\n\n  obj, accobj");
      cmdHook(c_obj, "obj", "", "Displays information about the\n  currently active object/class.\n"
	      "\n\nSEE ALSO\n\n  heapobj, accobj");
      cmdHook(c_accobj, "accobj", "", "Displays information about an\n  object or class at the\n"
	      "address indexed by acc.\n\nSEE ALSO\n\n  obj, heapobj");
      cmdHook(c_classtable, "classtable", "", "Lists all available classes");
      cmdHook(c_stack, "stack", "i", "Dumps the specified number of stack elements");
      cmdHook(c_backtrace, "bt", "", "Dumps the send/self/super/call/calle/callb stack");
      cmdHook(c_snk, "snk", "i*", "Steps forward until it hits the next\n  callk operation.\n"
	      "  If invoked with a parameter, it will\n  look for that specific callk.\n");
      cmdHook(c_listclones, "clonetable", "", "Lists all registered clones");
      cmdHook(c_set_acc, "set_acc", "i", "Sets the accumulator");
      cmdHook(c_heap_free, "heapfree", "", "Shows the free heap");
      cmdHook(c_sret, "sret", "", "Steps forward until ret is called\n  on the current execution stack"
	      "\n  level.");
      cmdHook(c_resource_id, "resource_id", "i", "Identifies a resource number by\n"
	      "  splitting it up in resource type\n  and resource number.");
      cmdHook(c_refresh_screen, "refresh_screen", "", "Redraws the screen");
      cmdHook(c_redraw_screen, "redraw_screen", "", "Reloads and redraws the screen");
      cmdHook(c_show_list, "listinfo", "i", "Examines the list at the specified\n  heap address");
      cmdHook(c_debuglog, "debuglog", "s*", "Sets the debug log modes.\n  Possible parameters:\n"
	      "  +x (sets debugging for x)\n  -x (unsets debugging for x)\n\nPossible values for x:\n"
	      "  u: Unimpl'd/stubbed stuff\n  l: Lists and nodes\n  g: Graphics\n  c: Character"
	      " handling\n  m: Memory management\n  f: Function call checks\n  *: Everything\n\n"
	      "  If invoked withour parameters,\n  it will list all activated\n  debug options.");

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
