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

#include <engine.h>
#include <console.h>
#include <kdebug.h>
#include <vocabulary.h>

int _debugstate_valid = 0; /* Set to 1 while script_debug is running */
int _debug_step_running = 0; /* Set to >0 to allow multiple stepping */
int _debug_commands_not_hooked = 1; /* Commands not hooked to the console yet? */
int _debug_seeking = 0; /* Stepping forward until some special condition is met */
int _debug_seek_level = 0; /* Used for seekers that want to check their exec stack depth */
int _debug_seek_special = 0; /* Used for special seeks */

static char oldcommand[SCI_CONSOLE_MAX_INPUT + 1] = ""; /* Stores the last command executed */

#define _DEBUG_SEEK_NOTHING 0
#define _DEBUG_SEEK_CALLK 1 /* Step forward until callk is found */
#define _DEBUG_SEEK_LEVEL_RET 2 /* Step forward until returned from this level */
#define _DEBUG_SEEK_SPECIAL_CALLK 3 /* Step forward until a /special/ callk is found */
#define _DEBUG_SEEK_SO 5 /* Step forward until specified PC (after the send command) and stack depth */

heap_ptr *_pc;
heap_ptr *_sp;
heap_ptr *_pp;
heap_ptr *_objp;
int *_restadjust;

int _kdebug_cheap_event_hack = 0;
int _kdebug_cheap_soundcue_hack = -1;

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
c_debuginfo(state_t *s)
{
  sciprintf("pc=%04x acc=%04x o=%04x fp=%04x sp=%04x\n", *_pc & 0xffff, s->acc & 0xffff,
	    *_objp & 0xffff, *_pp & 0xffff, *_sp & 0xffff);
  sciprintf("prev=%x sbase=%04x globls=%04x &restmod=%x\n",
	    s->prev & 0xffff, s->stack_base & 0xffff, s->global_vars & 0xffff,
	    *_restadjust & 0xffff);
  return 0;
}


int
c_step(state_t *s)
{
  _debugstate_valid = 0;
  if (cmd_paramlength && (cmd_params[0].val > 0))
    _debug_step_running = cmd_params[0].val;
  return 0;
}


int 
c_stepover(state_t *s)
{
  int opcode, opnumber;
  
  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  _debugstate_valid = 0;
  opcode = s->heap [*_pc];
  opnumber = opcode >> 1;
  if (opnumber == 0x22 /* callb */ || opnumber == 0x23 /* calle */ || 
      opnumber == 0x25 /* send */ || opnumber == 0x2a /* self */ || 
      opnumber == 0x2b /* super */) 
  {
    _debug_seeking = _DEBUG_SEEK_SO;
    _debug_seek_level = s->execution_stack_pos;
    /* Store in _debug_seek_special the offset of the next command after send */
    switch (opcode)
    {
    case 0x46: /* calle W */
      _debug_seek_special = *_pc + 5; break;

    case 0x44: /* callb W */
    case 0x47: /* calle B */
    case 0x56: /* super W */
      _debug_seek_special = *_pc + 4; break;

    case 0x45: /* callb B */
    case 0x57: /* super B */
    case 0x4A: /* send W */
    case 0x54: /* self W */
      _debug_seek_special = *_pc + 3; break;

    default:
      _debug_seek_special = *_pc + 2;
    }
  }

  return 0;
}

int
c_heapdump(state_t *s)
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

  sci_hexdump(s->heap + cmd_params[0].val, cmd_params[1].val, cmd_params[0].val);
  return 0;
}


int
c_sim_parse(state_t *s)
{
  int i;
  char *operators = ",&/()[]#<>";

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  if (cmd_paramlength == 0) {
    s->parser_valid = 0;
    return 0;
  }

  for (i = 0; i < cmd_paramlength; i++) {
    int flag = 0;
    char *token = cmd_params[i].str;

    if (strlen(token) == 1) {/* could be an operator */
      int j = 0;
      while (operators[j] && (operators[j] != token[0]))
	j++;
      if (operators[j]) {
	s->parser_nodes[i].type = 1;
	s->parser_nodes[i].content.value = j + 0xf0;
	flag = 1; /* found an operator */
      }
    }

    if (!flag) {
      char *openb = strchr(token, '['); /* look for opening braces */
      result_word_t *result;

      if (openb)
	*openb = 0; /* remove them and the rest */

      result = vocab_lookup_word(token, strlen(token),
				 s->parser_words, s->parser_words_nr,
				 s->parser_suffices, s->parser_suffices_nr);

      if (result) {
	s->parser_nodes[i].type = 0;
	s->parser_nodes[i].content.value = result->group;
	free(result);
      } else { /* group name was specified directly? */
	int val = strtol(token, NULL, 0); 
	if (val) {
	  s->parser_nodes[i].type = 0;
	  s->parser_nodes[i].content.value = val;
	} else { /* invalid and not matched */
	  sciprintf("Couldn't interpret '%s'\n", token);
	  s->parser_valid = 0;
	  return 1;
	}
      }
    }

  }

  s->parser_nodes[cmd_paramlength].type = -1; /* terminate */

  s->parser_valid = 2;
  return 0;
}


int
c_classtable(state_t *s)
{
  int i;
  heap_ptr spos; /* scriptpos */

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  sciprintf("Available classes:\n");
  for (i = 0; i < s->classtable_size; i++)
    if (s->classtable[i].scriptposp && (spos = *(s->classtable[i].scriptposp)))
      sciprintf(" Class 0x%x at %04x (script 0x%x)\n", i, spos + s->classtable[i].class_offset,
		s->classtable[i].script);

  return 0;
}

int
c_viewinfo(state_t *s)
{
	int view = cmd_params[0].val;
	int loops, i;

	if (!s) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	sciprintf("Resource view.%d ", view);

	loops = gfxop_lookup_view_get_loops(s->gfx_state, view);

	if (loops < 0)
		sciprintf("does not exist.\n");
	else {

		sciprintf("has %d loops:\n", loops);

		for (i = 0; i < loops; i++) {
			int j, cels;

			sciprintf("Loop %d: %d cels.\n", i, cels = gfxop_lookup_view_get_cels(s->gfx_state, view, i));
			for (j = 0; j < cels; j++) {
				int width;
				int height;
				point_t mod;

				gfxop_get_cel_parameters(s->gfx_state, view, i, j, &width, &height, &mod);

				sciprintf("   cel %d: size %dx%d, adj+(%d,%d)\n",j, width, height, mod.x, mod.y);
			}
		}
	}
	return 0;
}

int
c_list_sentence_fragments(state_t *s)
{
  int i;

  if (!s) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  for (i = 0; i < s->parser_branches_nr; i++) {
    int j = 0;

    sciprintf("R%02d: [%x] ->", i, s->parser_branches[i].id);
    while ((j < 10) && s->parser_branches[i].data[j]) {
      int dat = s->parser_branches[i].data[j++];

      switch (dat) {
      case VOCAB_TREE_NODE_COMPARE_TYPE:
	dat = s->parser_branches[i].data[j++];
	sciprintf(" C(%x)", dat);
	break;

      case VOCAB_TREE_NODE_COMPARE_GROUP:
	dat = s->parser_branches[i].data[j++];
	sciprintf(" WG(%x)", dat);
	break;

      case VOCAB_TREE_NODE_FORCE_STORAGE:
	dat = s->parser_branches[i].data[j++];
	sciprintf(" FORCE(%x)", dat);
	break;

      default:
	if (dat > VOCAB_TREE_NODE_LAST_WORD_STORAGE) {
	  int dat2 = s->parser_branches[i].data[j++];
	  sciprintf(" %x[%x]", dat, dat2);
	} else
	  sciprintf(" ?%x?", dat);
      }
    }
    sciprintf("\n");
  }

  sciprintf("%d rules.\n", s->parser_branches_nr);
  return 0;
}

enum {
  _parse_eoi,
  _parse_token_pareno,
  _parse_token_parenc,
  _parse_token_nil,
  _parse_token_number
};

int
_parse_getinp(int *i, int *nr)
{
  char *token;

  if (*i == cmd_paramlength)
    return _parse_eoi;

  token = cmd_params[(*i)++].str;

  if (!strcmp(token,"("))
    return _parse_token_pareno;

  if (!strcmp(token,")"))
    return _parse_token_parenc;

  if (!strcmp(token,"nil"))
    return _parse_token_nil;

  *nr = strtol(token, NULL, 0);
  return _parse_token_number;
}

int
_parse_nodes(state_t *s, int *i, int *pos, int type, int nr)
{
  int nexttk, nextval, newpos, oldpos;

  if (type == _parse_token_nil)
    return 0;

  if (type == _parse_token_number) {
    s->parser_nodes[*pos += 1].type = PARSE_TREE_NODE_LEAF;
    s->parser_nodes[*pos].content.value = nr; 
    return *pos;
  }
  if (type == _parse_eoi) {
    sciprintf("Unbalanced parentheses\n");
    return -1;
  }
  if (type == _parse_token_parenc) {
    sciprintf("Syntax error at token %d\n", *i);
    return -1;
  }
  s->parser_nodes[oldpos = ++(*pos)].type = PARSE_TREE_NODE_BRANCH;
  
  nexttk = _parse_getinp(i, &nextval);
  if ((newpos = s->parser_nodes[oldpos].content.branches[0] = _parse_nodes(s, i, pos, nexttk, nextval)) == -1)
    return -1;

  nexttk = _parse_getinp(i, &nextval);
  if ((newpos = s->parser_nodes[oldpos].content.branches[1] = _parse_nodes(s, i, pos, nexttk, nextval)) == -1)
    return -1;

  if (_parse_getinp(i, &nextval) != _parse_token_parenc)
    sciprintf("Expected ')' at token %d\n", *i);
  return oldpos;
}

int
c_set_parse_nodes(state_t *s)
{
  int i = 0;
  int foo, bar;
  int pos = -1;

  if (!s) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  bar = _parse_getinp(&i, &foo);
  if (_parse_nodes(s, &i, &pos, bar, foo) == -1)
    return 1;

  vocab_dump_parse_tree("debug-parse-tree", s->parser_nodes);
  return 0;
}

/* from grammar.c: */
int
vocab_gnf_parse(parse_tree_node_t *nodes, result_word_t *words, int words_nr,
		parse_tree_branch_t *branch0, parse_rule_list_t *tlist, int verbose);
/* parses with a GNF rule set */

int
c_parse(state_t *s)
{
  result_word_t *words;
  int words_nr;
  char *error;
  char *string;

  if (!s) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  string = cmd_params[0].str;
  sciprintf("Parsing '%s'\n", string);
  words = vocab_tokenize_string(string, &words_nr,
				s->parser_words, s->parser_words_nr,
				s->parser_suffices, s->parser_suffices_nr,
				&error);
  if (words) {

    int i, syntax_fail = 0;

    vocab_synonymize_tokens(words, words_nr, s->synonyms, s->synonyms_nr);

    sciprintf("Parsed to the following blocks:\n");

    for (i = 0; i < words_nr; i++)
      sciprintf("   Type[%04x] Group[%04x]\n", words[i].class, words[i].group);

    if (vocab_gnf_parse(&(s->parser_nodes[0]), words, words_nr, s->parser_branches,
			s->parser_rules, 1))
      syntax_fail = 1; /* Building a tree failed */

    free(words);

    if (syntax_fail)
      sciprintf("Building a tree failed.\n");
    else
      vocab_dump_parse_tree("debug-parse-tree", s->parser_nodes);

  } else {
    
      sciprintf("Unknown word: '%s'\n", error);
      free(error);
  }
  return 0;
}

int
c_save_game(state_t *s)
{
	int omit_check = cmd_params[0].str[0] == '_';
	int i;

	if (!s) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	s->amp_rest = *_restadjust;

	if (!omit_check) {
		int result = 0;
		for (i = 0; i < s->file_handles_nr; i++)
			if (s->file_handles[i])
				result++;
		
		if (result) {
			sciprintf("Game state has %d open file handles.\n", result);
			sciprintf("Save to '_%s' to ignore this check.\nGame was NOT saved.\n", cmd_params[0].str);
			return 1;
		}
	}

	if (gamestate_save(s, cmd_params[0].str)) {
		sciprintf("Saving the game state to '%s' failed\n", cmd_params[0].str);
	}

	return 0;
}


int
c_restore_game(state_t *s)
{
	state_t *newstate;
  
	if (!s) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	newstate = gamestate_restore(s, cmd_params[0].str);

	if (newstate) {

		s->successor = newstate; /* Set successor */

		script_abort_flag = 1; /* Abort game */
		_debugstate_valid = 0;

		return 0;
	} else {
		sciprintf("Restoring gamestate '%s' failed.\n", cmd_params[0].str);
		return 1;
	}
}


int
c_restart_game(state_t *s)
{
  int i;

  if (!s) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  for (i = 0; i < cmd_paramlength; i++) {
    if ((strcmp(cmd_params[0].str, "-r") == 0)
	|| (strcmp(cmd_params[0].str, "--replay") == 0))
      s->restarting_flags |= SCI_GAME_WAS_RESTARTED_AT_LEAST_ONCE;
    else
      if ((strcmp(cmd_params[0].str, "-p") == 0)
	  || (strcmp(cmd_params[0].str, "--play") == 0))
	s->restarting_flags &= ~SCI_GAME_WAS_RESTARTED_AT_LEAST_ONCE;
      else {
	sciprintf("Invalid parameter '%s'\n", cmd_params[0].str);
	return 1;
      }
  }

  sciprintf("Restarting\n");

  s->restarting_flags |= SCI_GAME_IS_RESTARTING_NOW;

  script_abort_flag = 1;
  _debugstate_valid = 0;
  return 0;
}


int
c_stack(state_t *s)
{
  int i;

  if (!s) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  for (i = cmd_params[0].val ; i > 0; i--)
    sciprintf("[sp-%04x] = %04x\n", i * 2, 0xffff & getInt16(s->heap + *_sp - i*2));

  return 0;
}

int
c_scriptinfo(state_t *s)
{
  int i;

  if (!s) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  sciprintf("Available scripts:\n");
  for (i = 0; i < 1000; i++)
    if (s->scripttable[i].heappos)
      sciprintf("%03x at %04x, locals at %04x\n   exports at %04x, %d lockers\n",
		i, s->scripttable[i].heappos, s->scripttable[i].localvar_offset,
		s->scripttable[i].export_table_offset, s->scripttable[i].lockers);

  return 0;
}

char *selector_name(state_t *s, int selector)
{
  return s->selector_names[selector];
}

int prop_ofs_to_id(state_t *s, int prop_ofs, int objp)
{
    word species = getInt16(s->heap + objp + SCRIPT_SPECIES_OFFSET);
    word superclass = getInt16(s->heap + objp + SCRIPT_SUPERCLASS_OFFSET);
    word type = getInt16(s->heap + objp + SCRIPT_INFO_OFFSET);
    int i;
    byte *selectorIDoffset;

    int selectors = getInt16(s->heap + objp + SCRIPT_SELECTORCTR_OFFSET);

    byte *selectoroffset = (byte *) s->heap + objp + SCRIPT_SELECTOR_OFFSET;

    if (type & SCRIPT_INFO_CLASS)
      selectorIDoffset = selectoroffset + selectors * 2;
    else
      selectorIDoffset =
	s->heap
	+ *(s->classtable[species].scriptposp)
	+ s->classtable[species].class_offset
	+ SCRIPT_SELECTOR_OFFSET
	+ selectors * 2;

      return (getInt16(selectorIDoffset + prop_ofs));

}  

void
print_objname(state_t *s, heap_ptr pos)
{
  if ((pos < 4) || (pos > 0xfff0))
    sciprintf("!invalid");
  else {
    if ((getInt16(s->heap + pos + SCRIPT_OBJECT_MAGIC_OFFSET)) != SCRIPT_OBJECT_MAGIC_NUMBER)
      sciprintf("!non-object");
    else {
      word namepos = getUInt16(s->heap + pos + SCRIPT_NAME_OFFSET);
      word type = getUInt16(s->heap + pos + SCRIPT_INFO_OFFSET);

      if (type & SCRIPT_INFO_CLONE)
	sciprintf("*");
      else if (type & SCRIPT_INFO_CLASS)
	sciprintf("%%");
      else if (type != 0)
	sciprintf("?[%04x]", type);

      if (!namepos || !(*(s->heap + namepos)))
	sciprintf("<???>");
      else sciprintf(s->heap + namepos);
    }
  }

  sciprintf(" @%04x\n", pos);
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

  sciprintf("%04X: [%c] %s", pos, opsize? 'B' : 'W', s->opcodes[opcode].name);

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
    case Script_Property:
    case Script_Global:
    case Script_Param:
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

    case Script_SRelative:
      if (opsize)
	param_value = s->heap[retval++];
      else {
	param_value = 0xffff & (s->heap[retval] | (s->heap[retval+1] << 8));
	retval += 2;
      }
      sciprintf (opsize? " %02x  [%04x]" : " %04x  [%04x]", param_value, pos+(short) param_value);
      break;

    case Script_End: retval = 0;
      break;

    default:
      sciprintf("Internal assertion failed in 'disassemble', %s, L%d\n", __FILE__, __LINE__);

    }

  if (pos == *_pc) /* Extra information if debugging the current opcode */

    if ((opcode == op_pTos)||(opcode == op_sTop)||
        (opcode == op_pToa)||(opcode == op_aTop)||
	(opcode == op_dpToa)||(opcode == op_ipToa)||
	(opcode == op_dpTos)||(opcode == op_ipTos))
    {
      int prop_ofs = s->heap[retval - 1];
      int prop_id = prop_ofs_to_id(s, prop_ofs, *_objp & 0xffff);
      
      sciprintf("	(%s)", selector_name(s, prop_id));
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
		  ? "<invalid>" : selector_name(s,selector));

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
c_dumpnodes(state_t *s)
{
  int end = MIN(cmd_params[0].val, VOCAB_TREE_NODES);
  int i;


  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  for (i = 0; i < end; i++) {
    sciprintf(" Node %03x: ", i);
    if (s->parser_nodes[i].type == PARSE_TREE_NODE_LEAF)
      sciprintf("Leaf: %04x\n", s->parser_nodes[i].content.value);
    else
      sciprintf("Branch: ->%04x, ->%04x\n", s->parser_nodes[i].content.branches[0],
		s->parser_nodes[i].content.branches[1]);
  }

  return 0;
}

static char *varnames[] = {"global", "local", "temp", "param"};
static char *varabbrev = "gltp";

int
c_vmvarlist(state_t *s)
{
      exec_stack_t *stack = s->execution_stack+s->execution_stack_pos;
      int i;
      for (i=0;i<4;i++) 
	sciprintf("%s vars at %04x\n", varnames[i], stack->variables[i]);
      return 0;
}
 
int
c_vmvars(state_t *s)
{
  exec_stack_t *stack = s->execution_stack+s->execution_stack_pos;
  
  switch(cmd_paramlength) {
  case 2:
    {
      int vartype = strchr(varabbrev, *cmd_params[0].str)-varabbrev;
      sciprintf("%s var %d == %d\n", varnames[vartype], cmd_params[1].val, GET_HEAP(stack->variables[vartype]+(cmd_params[1].val<<1)));
      break;
    }
  case 3:
    {
      int vartype = strchr(varabbrev, *cmd_params[0].str)-varabbrev;
      PUT_HEAP(stack->variables[vartype]+(cmd_params[1].val<<1),cmd_params[2].val);
      break;
    }
  }
  return 0;
}
int
c_backtrace(state_t *s)
{
  int i;

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  sciprintf("Call stack (current base: %x):\n", s->execution_stack_base);
  for (i = 0; i <= s->execution_stack_pos; i++) {
    exec_stack_t *call = &(s->execution_stack[i]);
    heap_ptr namepos;
    int paramc, totalparamc;

    switch (call->type) {

    case EXEC_STACK_TYPE_CALL: {/* Normal function */
      namepos = getInt16(s->heap + call->sendp + SCRIPT_NAME_OFFSET);
      sciprintf(" %x:[%x]  %s::%s(", i, call->origin,
		s->heap + namepos, (call->selector == -1)? "<call[be]?>":
		selector_name(s,call->selector));
    }
    break;

    case EXEC_STACK_TYPE_KERNEL: /* Kernel function */
      sciprintf(" %x:[%x]  k%s(", i, call->origin, s->kernel_names[-(call->selector)-42]);
      break;

    case EXEC_STACK_TYPE_VARSELECTOR:
      namepos = getInt16(s->heap + call->sendp + SCRIPT_NAME_OFFSET);
      sciprintf(" %x:[%x] vs%s %s::%s (", i, call->origin, (call->argc)? "write" : "read",
		s->heap + namepos, s->selector_names[call->selector]);
      break;
    } /* switch */

    totalparamc = call->argc;

    if (totalparamc > 16)
      totalparamc = 16;

    for (paramc = 1; paramc <= totalparamc; paramc++) {
      sciprintf("%04x", getUInt16(s->heap + call->variables[VAR_PARAM] + paramc * 2));

      if (paramc < call->argc)
	sciprintf(", ");
    }

    if (call->argc > 16)
      sciprintf("...");

    if (call->objp == 0)
      sciprintf(")\n");
    else {
      if (call->sp != CALL_SP_CARRY)
	sciprintf(")\n    obj@%04x pc=%04x sp=%04x fp=%04x\n", call->objp, call->pc,
		  call->sp, call->variables[VAR_TEMP]);
      else
	sciprintf(")\n    obj@%04x pc=%04x sp:carry fp=%04x\n", call->objp, call->pc,
		  call->variables[VAR_TEMP]);
    }
  }
  return 0;
}

int
c_redraw_screen(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	s->visual->draw(GFXW(s->visual), gfx_point(0,0));
	gfxop_update_box(s->gfx_state, gfx_rect(0, 0, 320, 200));
	return 0;
}

int
c_visible_map(state_t *s)
{
  if (!s) {
    sciprintf("Not in debug state\n");
    return 1;
  }
WARNING(fixme!)
#if 0
  if (s->onscreen_console)
    con_restore_screen(s, s->osc_backup);

  if (cmd_params[0].val <= 3) s->pic_visible_map = cmd_params[0].val;
  c_redraw_screen(s);
  
  if (s->onscreen_console)
    s->osc_backup = con_backup_screen(s);
#endif
  return 0;
}


int
c_showfont(state_t *s)
{
WARNING(fixme!)
#if 0
  resource_t *font;
  port_t port;
  int characters;
  int fontheight;
  char textbuf[12];
  int fontwidth = 18;
  sci_event_t e;
  int i, y, x;

  if (!s) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  font = findResource(sci_font, cmd_params[0].val);

  if (!font) {
    sciprintf("font.%03d not found\n", cmd_params[0].val);
    return 1;
  }

  
  if (s->onscreen_console) {
    con_restore_screen(s, s->osc_backup); /* Restore screen... */
    s->osc_backup = con_backup_screen(s); /* and grab it for later use */
  }

  characters = get_font_entries_nr(font->data);
  fontheight = get_font_height(font->data);

  sciprintf("font.%03d: %d characters, %d pixels high\n", cmd_params[0].val, characters, fontheight);
  sprintf(textbuf, "font.%03d", cmd_params[0].val);

  port.ymin = 30;
  port.xmin = 160 - (fontwidth * 9);
  port.ymax = 34 + (fontheight + 3) * (1+((characters + 0xf) / 0x10));
  port.xmax = 160 + (fontwidth * 9);
  port.flags = WINDOW_FLAG_TITLE;
  port.priority = -1;
  port.color = 15;
  port.bgcolor = 0;
  port.font = font->data;
  port.alignment = ALIGN_TEXT_CENTER;
  draw_window(s->pic, &port, port.bgcolor, port.priority, textbuf, font->data, port.flags);

  for (i = 0; i < 0x10; i++) {
    sprintf(textbuf, "%x", i);
    draw_text0(s->pic, &port, (i+2) * fontwidth, 3, textbuf, font->data, 0xc);
  }

  fontheight += 3;
  y = 3;
  for (i = 0; i < characters; i++) {
    if (!(i & 0xf)) {
      x = fontwidth << 1;
      y += fontheight;
      sprintf(textbuf, "%x0", i >> 4);
      draw_text0(s->pic, &port, 3, y, textbuf, font->data, 0xc);
      textbuf[1] = 0;
    }

    textbuf[0] = i;
    draw_text0_without_newline(s->pic, &port, x, y, textbuf, font->data, 0xf);
    x += fontwidth;
  }

  graph_update_box(s, 0, 0, 320, 200);

  do {
    e = getEvent(s);
    s->gfx_driver->Wait(s, 25000);
  } while ((e.type != SCI_EVT_KEYBOARD) && (e.type != SCI_EVT_MOUSE_RELEASE));

  if (s->onscreen_console) {
    con_restore_screen(s, s->osc_backup); /* Restore screen again... */
    s->osc_backup = con_backup_screen(s); /* ...grab for onscreen display removal */
  }
#endif
  return 0;
}


int
c_gfx_current_port(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (!s->port)
		sciprintf("none.\n");
	else
		sciprintf("%d\n", s->port->ID);
	return 0;
}

int
c_gfx_print_port(state_t *s)
{
	gfxw_port_t *port;

	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	port = s->port;

	if (cmd_paramlength > 0) {
		if (s->visual) {
			port = gfxw_find_port(s->visual, cmd_params[0].val);
		} else {
			sciprintf("visual is uninitialized.\n");
			return 1;
		}
	}

	if (port)
		port->print(GFXW(port), 0);
	else
		sciprintf("No such port.\n");

	return 0;
}

c_gfx_priority(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (cmd_paramlength) {
		int zone = cmd_params[0].val;
		if (zone < 0)
			zone = 0;
		if (zone > 15) zone = 15;

		sciprintf("Zone %x starts at y=%d\n", zone, PRIORITY_BAND_FIRST(zone));
	} else {
		sciprintf("Priority bands start at y=%d\nThey end at y=%d\n", s->priority_first, s->priority_last);
	}

	return 0;
}

int
c_gfx_print_visual(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (s->visual)
		s->visual->print(GFXW(s->visual), 0);
	else
		sciprintf("visual is uninitialized.\n");

	return 0;
}

c_gfx_print_dynviews(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (!s->dyn_views)
		sciprintf("No dynview list active.\n");
	else
		s->dyn_views->print(GFXW(s->dyn_views), 0);

	return 0;
}

c_gfx_drawpic(state_t *s)
{
	int flags = 1, default_palette = 0;

	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (cmd_paramlength > 1) {
		default_palette = cmd_params[1].val;

		if (cmd_paramlength > 2)
			flags = cmd_params[2].val;
	}

	gfxop_new_pic(s->gfx_state, cmd_params[0].val, flags, default_palette);
	gfxop_clear_box(s->gfx_state, gfx_rect(0, 0, 320, 200));
	gfxop_update(s->gfx_state);
	return 0;
}

#ifdef GFXW_DEBUG_WIDGETS
extern gfxw_widget_t *debug_widgets[];
extern int debug_widget_pos;
c_gfx_print_widget(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (cmd_paramlength) {
		int i;
		for (i = 0; i < cmd_paramlength ; i++) {
			int widget_nr = cmd_params[i].val;

			sciprintf("===== Widget #%d:\n", widget_nr);
			debug_widgets[widget_nr]->print(debug_widgets[widget_nr], 0);
		}

	} else if(debug_widget_pos>1)
		sciprintf("Widgets 0-%d are active\n", debug_widget_pos-1);
	else if (debug_widget_pos == 1)
		sciprintf("Widget 0 is active\n");
	else
		sciprintf("No widgets are active\n");

	return 0;
}
#endif

int
c_gfx_show_map(state_t *s)
{
	int map = cmd_params[0].val;
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	gfxop_set_clip_zone(s->gfx_state, gfx_rect_fullscreen);

	switch (map) {
	case 0:
		s->visual->add_dirty_abs(GFXWC(s->visual), gfx_rect(0, 0, 320, 200), 0);
		s->visual->draw(GFXW(s->visual), gfx_point(0, 0));
		break;

	case 1:
		gfx_xlate_pixmap(s->gfx_state->pic->priority_map, s->gfx_state->driver->mode, 0);
		gfxop_draw_pixmap(s->gfx_state, s->gfx_state->pic->priority_map, gfx_rect(0, 0, 320, 200), gfx_point(0, 0));
		break;

	case 2:
		gfx_xlate_pixmap(s->gfx_state->control_map, s->gfx_state->driver->mode, 0);
		gfxop_draw_pixmap(s->gfx_state, s->gfx_state->control_map, gfx_rect(0, 0, 320, 200), gfx_point(0, 0));
		break;

	default:
		sciprintf("Map %d is not available.\n", map);
		return 1;
	}

	gfxop_update(s->gfx_state);
	return 0;
}


int
c_gfx_draw_cel(state_t *s)
{
	int view = cmd_params[0].val;
	int loop = cmd_params[1].val;
	int cel = cmd_params[2].val;

	if (!s) {
		sciprintf("Not in debug state!\n");
		return 1;
	}

	gfxop_set_clip_zone(s->gfx_state, gfx_rect_fullscreen);
	gfxop_draw_cel(s->gfx_state, view, loop, cel, gfx_point(160, 100),
		       s->ega_colors[0]);
	gfxop_update(s->gfx_state);

	return 0;
}

int
c_gfx_fill_screen(state_t *s)
{
	int col = cmd_params[0].val;

	if (!s) {
		sciprintf("Not in debug state!\n");
		return 1;
	}

	if (col < 0 || col > 15)
		col = 0;

	gfxop_set_clip_zone(s->gfx_state, gfx_rect_fullscreen);
	gfxop_fill_box(s->gfx_state, gfx_rect_fullscreen, s->ega_colors[col]);
	gfxop_update(s->gfx_state);

	return 0;
}



int
c_disasm(state_t *s)
{
	int vpc = cmd_params[0].val;
	int op_count;

	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (vpc <= 0)
		vpc = *_pc + vpc;

	if (cmd_paramlength > 1)
		{
			if (cmd_params[1].val < 0)
				return 0;
			op_count = cmd_params[1].val;
		}
	else
		op_count = 1;

	do {
		vpc = disassemble(s, vpc);
	} while ((vpc > 0) && (vpc < 0xfff2) && (cmd_paramlength > 1) && (--op_count));
	return 0;
}


int
c_snk(state_t *s)
{
	int callk_index;
	char *endptr;
  
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (cmd_paramlength > 0) {
		/* Try to convert the parameter to a number. If the conversion stops
		   before end of string, assume that the parameter is a function name
		   and scan the function table to find out the index. */
		callk_index = strtoul (cmd_params [0].str, &endptr, 0);
		if (*endptr != '\0')
			{
				int i;

				callk_index = -1;
				for (i = 0; i < s->kernel_names_nr; i++)
					if (!strcmp (cmd_params [0].str, s->kernel_names [i]))
						{
							callk_index = i;
							break;
						}

				if (callk_index == -1)
					{
						sciprintf ("Unknown kernel function '%s'\n", cmd_params [0].str);
						return 1;
					}
			}

		_debug_seeking = _DEBUG_SEEK_SPECIAL_CALLK;
		_debug_seek_special = callk_index;
		_debugstate_valid = 0;
	} else {
		_debug_seeking = _DEBUG_SEEK_CALLK;
		_debugstate_valid = 0;
	}
	return 0;
}


int
c_sret(state_t *s)
{
	_debug_seeking = _DEBUG_SEEK_LEVEL_RET;
	_debug_seek_level = s->execution_stack_pos;
	_debugstate_valid = 0;
	return 0;
}

int
c_go(state_t *s)
{
	_debug_seeking = 0;
	_debugstate_valid = 0;
	script_debug_flag = 0;
	return 0;
}


int
c_set_acc(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	s->acc = cmd_params[0].val;
	return 0;
}

int
c_resource_id(state_t *s)
{
	int id = cmd_params[0].val;

	sciprintf("%s.%d (0x%x)\n", Resource_Types[id >> 11], id &0x7ff, id & 0x7ff);
	return 0;
}

int
c_heap_free(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	heap_dump_free(s->_heap);
	return 0;
}

int
c_listclones(state_t *s)
{
	int i, j = 0;
	sciprintf("Listing all logged clones:\n");
	for (i = 0; i < SCRIPT_MAX_CLONES; i++)
		if (s->clone_list[i]) {
			sciprintf("  Clone at %04x\n", s->clone_list[i]);
			j++;
		}

	sciprintf("Total of %d clones.\n", j);
	return 0;
}

void
set_debug_mode (struct _state *s, int mode, char *areas)
{
	char modechars[] = "ulgcmfbadspMSFt"; /* Valid parameter chars */
	char *parser;
	int seeker;
	char frob;

	parser = areas;
	while ((frob = *parser)) {
		seeker = 0;

		if (frob == '*') { /* wildcard */
			if (mode) /* set all */
				s->debug_mode = 0xffffffff;
			else /* unset all */
				s->debug_mode = 0;

			parser++;
			continue;
		}

		while (modechars[seeker] && (modechars[seeker] != frob))
			seeker++;

		if (modechars[seeker] == '\0') {
			sciprintf("Invalid log mode parameter: %c\n", frob);
			return;
		}

		if (mode) /* Set */
			s->debug_mode |= (1 << seeker);
		else /* UnSet */
			s->debug_mode &= ~(1 << seeker);

		parser++;
	}
}

int
c_debuglog(state_t *s)
{
	int i;

	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (cmd_paramlength == 0) {
		for (i = 0; i < SCIk_DEBUG_MODES; i++)
			if (s->debug_mode & (1 << i))
				sciprintf(" Logging %s\n", SCIk_Debug_Names[i]);

		return 0;
	}

	for (i = 0; i < cmd_paramlength; i++) {
		int mode;

		if (cmd_params[i].str[0] == '+')
			mode = 1;
		else
			if (cmd_params[i].str[0] == '-')
				mode = 0;
			else {
				sciprintf("Parameter '%s' should start with + or -\n", cmd_params[i].str);
				return 1;
			}

		set_debug_mode(s, mode, cmd_params [i].str + 1);
	}

	return 0;
}


int
show_list(state_t *s, heap_ptr list)
{
	heap_ptr prevnode = 0, node;
	int nodectr = 0;

	sciprintf("List at %04x:\n", list);
	node = getInt16(s->heap + list );

	while (node) {
		nodectr++;
		sciprintf(" - Node at %04x: Key=%04x Val=%04x\n",node,
			  getUInt16(s->heap + node + 4),getUInt16(s->heap + node + 6));
		prevnode = node;
		node = getUInt16(s->heap + node + 2); /* Next node */
	}

	if (prevnode != getUInt16(s->heap + list + 2)) /* Lastnode != registered lastnode? */
		sciprintf("WARNING: List.LastNode %04x != last node %04x\n",
			  getUInt16(s->heap + list + 2), prevnode);

	if (nodectr)
		sciprintf("%d registered nodes.\n", nodectr);
	else
		sciprintf("List is empty.\n");
	return 0;
}


int
c_dump_words(state_t *s)
{
	int i;

	if (!s) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (!s->parser_words) {
		sciprintf("No words.\n");
		return 0;
	}

	for (i = 0; i < s->parser_words_nr; i++) {
		word_t *word = s->parser_words[i];
		sciprintf("%s: C %03x G %03x\n", word->word, word->class, word->group);
	}
	sciprintf("%d words\n", s->parser_words_nr);

	return 0;
}

int
c_show_list(state_t *s)
{
  heap_ptr list = cmd_params[0].val;

  if (!s) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  if (getInt16(s->heap + list - 2) != 6) { /* List is hmalloc()'d to size 6 */
    sciprintf("No list at %04x.\n", list);
    return 1;
  }

  return 
    show_list(s, list);
}


int
c_mem_info(state_t *s)
{
  int i, cnt = 0, allocd = 0;
  if (!s) {
    sciprintf("Not in debug state!\n");
    return 1;
  }

  heap_meminfo(s->_heap);
  sciprintf("Heap: Free:%04x  Max:%04x\n", heap_meminfo(s->_heap) & 0xffff, heap_largest(s->_heap) & 0xffff);

  sciprintf("Hunk:\n");
  for (i = 0; i < MAX_HUNK_BLOCKS; i++)
    if (s->hunk[i].size) {
      sciprintf("#%d: %d bytes\n", i, s->hunk[i].size);
      ++cnt;
      allocd += s->hunk[i].size;
    }

  sciprintf("Hunk handles: %d (%d bytes total)\n", cnt, allocd);

  return 0;
}


int
c_objs(state_t *s)
{
  int i;

  if (!s) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  for (i = 0; i < 1000; i++)
    if (s->scripttable[i].heappos) {
      int seeker = s->scripttable[i].heappos;
      int segment, size;

      do {
	segment = GET_HEAP(seeker);
	size = GET_HEAP(seeker + 2);

	if (segment == sci_obj_object || segment == sci_obj_class)
	  print_objname(s, seeker - SCRIPT_OBJECT_MAGIC_OFFSET + 4 /* to compensate for the header */);

	seeker += size;
      } while (segment && (seeker < 0xffff ));
    }

  for (i = 0; i < SCRIPT_MAX_CLONES; i++)
    if (s->clone_list[i])
      print_objname(s, s->clone_list[i]);

  return 0;
}


int
c_simkey(state_t *s)
{
  _kdebug_cheap_event_hack = cmd_params[0].val;
  return 0;
}

int
c_simsoundcue(state_t *s)
{
  _kdebug_cheap_soundcue_hack = cmd_params[0].val;
  return 0;
}

int
objinfo(state_t *s, heap_ptr pos)
{
  word type;

  if ((pos < 4) || (pos > 0xfff0)) {
    sciprintf("Invalid address.\n");
    return 1;
  }

  if ((getInt16(s->heap + pos + SCRIPT_OBJECT_MAGIC_OFFSET)) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    sciprintf("Not an object.\n");
    return 0;
  }

  type = getInt16(s->heap + pos + SCRIPT_INFO_OFFSET);

  if (type & SCRIPT_INFO_CLONE)
    sciprintf("Clone");
  else if (type & SCRIPT_INFO_CLASS)
    sciprintf("Class");
  else if (type == 0)
    sciprintf("Object");
  else
    sciprintf("Weird object");

  {
    word selectors, functions;
    byte* selectorIDoffset;
    byte* selectoroffset;
    byte* functIDoffset;
    byte* functoffset;
    word localvarptr = getInt16(s->heap + pos + SCRIPT_LOCALVARPTR_OFFSET);
    word species = getInt16(s->heap + pos + SCRIPT_SPECIES_OFFSET);
    word superclass = getInt16(s->heap + pos + SCRIPT_SUPERCLASS_OFFSET);
    word namepos = getInt16(s->heap + pos + SCRIPT_NAME_OFFSET);
    int i;

    sciprintf(" %s\n", s->heap + namepos);
    sciprintf("Species=%04x, Superclass=%04x\n", species, superclass);
    sciprintf("Local variables @ 0x%04x\n", localvarptr);
    
    selectors = getInt16(s->heap + pos + SCRIPT_SELECTORCTR_OFFSET);

    selectoroffset = s->heap + pos + SCRIPT_SELECTOR_OFFSET;

    if (type & SCRIPT_INFO_CLASS)
      selectorIDoffset = selectoroffset + selectors * 2;
    else
      selectorIDoffset =
	s->heap
	+ *(s->classtable[species].scriptposp)
	+ s->classtable[species].class_offset
	+ SCRIPT_SELECTOR_OFFSET
	+ selectors * 2;

    functIDoffset = s->heap + pos + SCRIPT_FUNCTAREAPTR_MAGIC
      + getUInt16(s->heap + pos + SCRIPT_FUNCTAREAPTR_OFFSET);

    functions = getInt16(functIDoffset - 2);

    functoffset = functIDoffset + 2 + functions * 2;

    if (selectors > 0) {
      sciprintf("Variable selectors:\n");

      for (i = 0; i < selectors; i++) {
	word selectorID = selectorIDoffset? getInt16(selectorIDoffset + i*2) : i;

	sciprintf("  %s[%04x] = %04x\n", selectorIDoffset? selector_name(s, selectorID) : "<?>",
		  selectorID, 0xffff & getInt16(selectoroffset + i*2));
      }
    } /* if variable selectors are present */


    if (functions > 0) {
      sciprintf("Method selectors:\n");

      for (i = 0; i < functions; i++) {
	word selectorID = getInt16(functIDoffset + i*2);

	if (selectorID > s->selector_names_nr) {
	  sciprintf("Invalid selector number: %04x!\n", selectorID);
	  return 0;
	}

	sciprintf("  %s[%04x] at %04x\n", s->selector_names[selectorID], selectorID,
		  0xffff & getInt16(functoffset + i*2));
      }
    } /* if function selectors are present */
  }
  return 0;
}

int
c_heapobj(state_t *s)
{
  return
    objinfo(s, cmd_params[0].val);
}

int
c_obj(state_t *s)
{
  return
    objinfo(s, *_objp);
}

int
c_accobj(state_t *s)
{
  return
    objinfo(s, s->acc);
}

/*** Breakpoint commands ***/

static breakpoint_t *
bp_alloc(state_t *s)
{
  breakpoint_t *bp;

  if (s->bp_list)
  {
    bp = s->bp_list;
    while (bp->next)
      bp = bp->next;
    bp->next = (breakpoint_t *) malloc (sizeof (breakpoint_t));
    bp = bp->next;
  }
  else {
    s->bp_list = (breakpoint_t *) malloc (sizeof (breakpoint_t));
    bp = s->bp_list;
  }

  bp->next = NULL;

  return bp;
}

int
c_bpx(state_t *s)
{
  breakpoint_t *bp;

  /* Note: We can set a breakpoint on a method that has not been loaded yet.
     Thus, we can't check whether the command argument is a valid method name.
     A breakpoint set on an invalid method name will just never trigger. */

  bp=bp_alloc (s);

  bp->type = BREAK_SELECTOR;
  bp->data.name = malloc (strlen (cmd_params [0].str)+1);
  strcpy (bp->data.name, cmd_params [0].str);
  s->have_bp |= BREAK_SELECTOR;

  return 0;
}

int
c_bpe(state_t *s)
{
  breakpoint_t *bp;

  bp=bp_alloc (s);

  bp->type = BREAK_EXPORT;
  bp->data.address = (cmd_params [0].val << 16 | cmd_params [1].val);
  s->have_bp |= BREAK_EXPORT;

  return 0;
}

int
c_bplist(state_t *s)
{
  breakpoint_t *bp;
  int i = 0;
  long bpdata;

  bp = s->bp_list;
  while (bp)
  {
    sciprintf ("  #%i: ", i);
    switch (bp->type)
    {
    case BREAK_SELECTOR:
      sciprintf ("Execute %s\n", bp->data.name);
      break;
    case BREAK_EXPORT:
      bpdata = bp->data.address;
      sciprintf ("Execute script %d, export %d\n", bpdata >> 16, bpdata & 0xFFFF);
      break;
    }
    
    bp = bp->next;
    i++;
  }

  return 0;
}

int c_bpdel(state_t *s)
{
  breakpoint_t *bp, *bp_next, *bp_prev;
  int i = 0, found = 0;
  int type;

  /* Find breakpoint with given index */
  bp_prev = NULL;
  bp = s->bp_list;
  while (bp && i < cmd_params [0].val)
  {
    bp_prev = bp;
    bp = bp->next;
    i++;
  }
  if (!bp)
  {
    sciprintf ("Invalid breakpoint index %i\n", cmd_params [0].val);
    return 1;
  }

  /* Delete it */
  bp_next = bp->next;
  type = bp->type;
  if (type == BREAK_SELECTOR) free (bp->data.name);
  free (bp);
  if (bp_prev)
    bp_prev->next = bp_next;
  else
    s->bp_list = bp_next;

  /* Check if there are more breakpoints of the same type. If not, clear
     the respective bit in s->have_bp. */
  for (bp = s->bp_list; bp; bp=bp->next)
    if (bp->type == type)
    {
      found = 1;
      break;
    }

  if (!found) s->have_bp &= ~type;

  return 0;
}

int
c_gnf(state_t *s)
{
  if (!s) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  vocab_gnf_dump(s->parser_branches, s->parser_branches_nr);
  return 0;
}

int
c_se(state_t *s)
{
  stop_on_event=1;
  _debugstate_valid = script_debug_flag = script_error_flag = 0;
  return 0;
}

void
script_debug(state_t *s, heap_ptr *pc, heap_ptr *sp, heap_ptr *pp, heap_ptr *objp,
	     int *restadjust, int bp)
{

	if (sci_debug_flags & _DEBUG_FLAG_LOGGING) {
		int old_debugstate = _debugstate_valid;

		_pc = pc;
		_sp = sp;
		_pp = pp;
		_objp = objp;
		_restadjust = restadjust;
		sciprintf("acc=%04x  ", s->acc & 0xffff);
		_debugstate_valid = 1;
		disassemble(s, *pc);

		_debugstate_valid = old_debugstate;

		if (!script_debug_flag)
			return;
	}

	if (_debug_seeking && !bp) { /* Are we looking for something special? */
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
			if ((op != op_ret) || (_debug_seek_level < s->execution_stack_pos)) return;
			break;
		}

		case _DEBUG_SEEK_SO:
			if (*pc != _debug_seek_special || s->execution_stack_pos != _debug_seek_level) return;
			break;

		} /* switch(_debug_seeking) */

		_debug_seeking = _DEBUG_SEEK_NOTHING; /* OK, found whatever we were looking for */

	} /* if (_debug_seeking) */


	_debugstate_valid = (_debug_step_running == 0);

	if (_debugstate_valid) {
		_pc = pc;
		_sp = sp;
		_pp = pp;
		_objp = objp;
		_restadjust = restadjust;

		c_debuginfo(s);
		sciprintf("Step #%d\n", script_step_counter);
		disassemble(s, *pc);

		if (_debug_commands_not_hooked) {

			_debug_commands_not_hooked = 0;

			con_hook_command(c_debuginfo, "registers", "", "Displays all current register values");
			con_hook_command(c_vmvars, "vmvars", "si*", "Displays or changes variables in the VM\n\nFirst parameter is either g(lobal), l(ocal), t(emp) or p(aram).\nSecond parameter is the var number\nThird parameter (if specified) is the value to set the variable to");
			con_hook_command(c_vmvarlist, "vmvarlist", "", "Displays the addresses of variables in the VM");
			con_hook_command(c_step, "s", "i*", "Executes one or several operations\n\nEXAMPLES\n\n"
					 "    s 4\n\n  Execute 4 commands\n\n    s\n\n  Execute next command");
			con_hook_command(c_stepover, "so", "", "Executes one operation skipping over sends");
			con_hook_command(c_heapdump, "heapdump", "ii", "Dumps data from the heap\n"
					 "\nEXAMPLE\n\n    heapdump 0x1200 42\n\n  Dumps 42 bytes starting at heap address\n"
					 "  0x1200");
			con_hook_command(c_disasm, "disasm", "ii*", "Disassembles one or more commands\n\n"
					 "USAGE\n\n  disasm [startaddr] <number of commands to disassemble>");
			con_hook_command(c_scriptinfo, "scripttable", "", "Displays information about all\n  loaded scripts");
			con_hook_command(c_heapobj, "heapobj", "i", "Displays information about an\n  object or class on the\n"
					 "specified heap address.\n\nSEE ALSO\n\n  obj, accobj");
			con_hook_command(c_obj, "obj", "", "Displays information about the\n  currently active object/class.\n"
					 "\n\nSEE ALSO\n\n  heapobj, accobj");
			con_hook_command(c_accobj, "accobj", "", "Displays information about an\n  object or class at the\n"
					 "address indexed by acc.\n\nSEE ALSO\n\n  obj, heapobj");
			con_hook_command(c_classtable, "classtable", "", "Lists all available classes");
			con_hook_command(c_stack, "stack", "i", "Dumps the specified number of stack elements");
			con_hook_command(c_backtrace, "bt", "", "Dumps the send/self/super/call/calle/callb stack");
			con_hook_command(c_snk, "snk", "s*", "Steps forward until it hits the next\n  callk operation.\n"
					 "  If invoked with a parameter, it will\n  look for that specific callk.\n");
			con_hook_command(c_se, "se", "", "Steps forward until an SCI event is received.\n");      
			con_hook_command(c_listclones, "clonetable", "", "Lists all registered clones");
			con_hook_command(c_set_acc, "set_acc", "i", "Sets the accumulator");
			con_hook_command(c_heap_free, "heapfree", "", "Shows the free heap");
			con_hook_command(c_sret, "sret", "", "Steps forward until ret is called\n  on the current execution"
					 " stack\n  level.");
			con_hook_command(c_resource_id, "resource_id", "i", "Identifies a resource number by\n"
					 "  splitting it up in resource type\n  and resource number.");
			con_hook_command(c_redraw_screen, "redraw_screen", "", "Redraws the screen");
			con_hook_command(c_show_list, "listinfo", "i", "Examines the list at the specified\n  heap address");
			con_hook_command(c_mem_info, "meminfo", "", "Displays heap memory information");
			con_hook_command(c_debuglog, "debuglog", "s*", "Sets the debug log modes.\n  Possible parameters:\n"
					 "  +x (sets debugging for x)\n  -x (unsets debugging for x)\n\nPossible values"
					 " for x:\n  u: Unimpl'd/stubbed stuff\n  l: Lists and nodes\n  g: Graphics\n"
					 "  c: Character handling\n  m: Memory management\n  f: Function call checks\n"
					 "  b: Bresenham details\n  a: Audio\n  d: System gfx management\n  s: Base setter"
					 "\n  p: Parser\n  M: The menu system\n  \n  S: Said specs\n  F: File I/O\n  *: Everything\n\n"
					 "  If invoked withour parameters,\n  it will list all activated\n  debug options.");
			con_hook_command(c_visible_map, "set_vismap", "i", "Sets the visible map.\n  Default is 0 (visual).\n"
					 "  Other useful values are:\n  1: Priority\n  2: Control\n  3: Auxiliary\n");
			con_hook_command(c_simkey, "simkey", "i", "Simulates a keypress with the\n  specified scancode.\n");
			con_hook_command(c_bpx, "bpx", "s", "Sets a breakpoint on the execution of specified method.\n");
			con_hook_command(c_bpe, "bpe", "ii", "Sets a breakpoint on the execution of specified"
					 " exported function.\n");
			con_hook_command(c_bplist, "bplist", "", "Lists all breakpoints.\n");
			con_hook_command(c_bpdel, "bpdel", "i", "Deletes a breakpoint with specified index.");
			con_hook_command(c_go, "go", "", "Executes the script.\n");
			con_hook_command(c_dumpnodes, "dumpnodes", "i", "shows the specified number of nodes\n"
					 "  from the parse node tree");
			con_hook_command(c_save_game, "save_game", "s", "Saves the current game state to\n  the hard disk");
			con_hook_command(c_restore_game, "restore_game", "s", "Restores a saved game from the\n  hard disk");
			con_hook_command(c_restart_game, "restart", "s*", "Restarts the game.\n\nUSAGE\n\n  restart [-r] [-p]"
					 " [--play] [--replay]\n\n  There are two ways to restart an SCI\n  game:\n"
					 "  play (-p) calls the game object's play()\n    method\n  replay (-r) calls the replay() method");
			con_hook_command(c_viewinfo, "viewinfo", "i", "Displays the number of loops\n  and cels of each loop"
					 " for the\n  specified view resource.\n\n  Output:\n    C(x): Check word type against x\n"
					 "    WG(x): Check word group against mask x\n    FORCE(x): Force storage node x\n");
			con_hook_command(c_list_sentence_fragments, "list_sentence_fragments", "", "Lists all sentence fragments (which\n"
					 "  are used to build Parse trees).");
			con_hook_command(c_parse, "parse", "s", "Parses a sequence of words and prints\n  the resulting parse tree.\n"
					 "  The word sequence must be provided as a\n  single string.");
			con_hook_command(c_gnf, "gnf", "", "Displays the Parse grammar\n  in strict GNF");
			con_hook_command(c_set_parse_nodes, "set_parse_nodes", "s*", "Sets the contents of all parse nodes.\n"
					 "  Input token must be separated by\n  blanks.");
			con_hook_command(c_showfont, "showfont", "i", "Displays all characters of the specified font");
			con_hook_command(c_objs, "objs", "", "Lists all objects and classes from all\n  currently loaded scripts, plus\n"
					 "  all clones.\n  Objects are not marked; clones are marked\n  with a preceeding '*', and classes\n"
					 "  with a preceeding '%'\n"
					 "\n\nSEE ALSO\n  clonetable\n");
#ifdef SCI_SIMPLE_SAID_CODE
			con_hook_command(c_sim_parse, "simparse", "s*", "Simulates a parsed entity.\n\nUSAGE\n  Call this"
					 " function with a list of\n  Said operators, words, and word group"
					 "\n  numbers to match Said() specs\n  that look identical.\n"
					 "\n  Note that opening braces and\n  everything behind them are\n"
					 "\n  removed from all non-operator\n  parameter tokens.\n"
					 "\n  simparse without parameters\n  removes the entity.\n");
#endif SCI_SIMPLE_SAID_CODE
#ifdef GFXW_DEBUG_WIDGETS
			con_hook_command(c_gfx_print_widget, "gfx_print_widget", "i*", "If called with no parameters, it\n  shows which widgets are active.\n"
					 "  With parameters, it lists the\n  widget corresponding to the\n  numerical index specified (for\n  each parameter).");
#endif
			con_hook_command(c_gfx_current_port, "gfx_current_port", "", "Determines the current port number");
			con_hook_command(c_gfx_print_port, "gfx_print_port", "i*", "Displays all information about the\n  specified port,"
					 " or the current port\n  if no port was specified.");
			con_hook_command(c_gfx_print_visual, "gfx_print_visual", "", "Displays all information about the\n  current widget state");
			con_hook_command(c_gfx_print_dynviews, "gfx_print_dynviews", "", "Shows the dynview list");
			con_hook_command(c_gfx_drawpic, "gfx_drawpic", "ii*", "Draws a pic resource\n\nUSAGE\n  gfx_drawpic <nr> [<pal> [<fl>]]\n"
					 "  where <nr> is the number of the pic resource\n  to draw\n  <pal> is the optional default\n  palette for the pic (0 is"
					 "\n  assumed if not specified)\n  <fl> are any pic draw flags (default\n  is 1)");
			con_hook_command(c_dump_words, "dumpwords", "", "Lists all parser words");
			con_hook_command(c_gfx_show_map, "gfx_show_map", "i", "Shows one of the screen maps\n  Semantics of the int parameter:\n"
					 "    0: visual map (back buffer)\n    1: priority map (back buf.)\n    2: control map (static buf.)");
			con_hook_command(c_gfx_fill_screen, "gfx_fill_screen", "i", "Fills the screen with one\n  of the EGA colors\n");
			con_hook_command(c_gfx_draw_cel, "gfx_draw_cel", "iii", "Draws a single view\n  cel to the center of the\n  screen\n\n"
					 "USAGE\n  gfx_draw_cel <view> <loop> <cel>\n");
			con_hook_command(c_gfx_priority, "gfx_priority", "i*", "Prints information about priority\n  bands\nUSAGE\n\n  gfx_priority\n\n"
					 "  will print the min and max values\n  for the priority bands\n\n  gfx_priority <val>\n\n  Print start of the priority\n"
					 "  band for the specified\n  priority\n");


			con_hook_int(&script_debug_flag, "script_debug_flag", "Set != 0 to enable debugger\n");
			con_hook_int(&script_checkloads_flag, "script_checkloads_flag", "Set != 0 to display information\n"
				     "  when scripts are loaded or unloaded");
			con_hook_int(&script_abort_flag, "script_abort_flag", "Set != 0 to abort execution\n");
			con_hook_int(&script_step_counter, "script_step_counter", "# of executed SCI operations\n");
			con_hook_int(&sci_debug_flags, "debug_flags", "Debug flags:\n  0x0001: Log each command executed\n"
				     "  0x0002: Break on warnings\n");

		} /* If commands were not hooked up */

	}

	if (_debug_step_running)
		_debug_step_running--;

	/* Suspend music playing */
	if (s->sfx_driver)
		(s->sfx_driver->suspend)(s);

WARNING(fixme!)
#if 0
		if (s->onscreen_console) {
			s->osc_backup = con_backup_screen(s);
			con_visible_rows = 20;

			con_draw(s, s->osc_backup);

			while (_debugstate_valid) {
				sci_event_t event = (gfxop_get_event(s->gfx_state));
				int redraw_console = 0;
				char *commandbuf;

				if ((event.type & SCI_EVT_KEYBOARD)
				    || (s->pointer_x != s->last_pointer_x)
				    || (s->pointer_y != s->last_pointer_y))
					redraw_console = 1; /* Redraw on keypress or mouse movement */

				if ((event.buckybits & SCI_EVM_CTRL) && (event.data == '`')) /* UnConsole command? */
					_debugstate_valid = 0;
				else
					if ((commandbuf = con_input(&event))) {

						sciprintf(" >%s\n", commandbuf);

						if (strlen(commandbuf) == 0) /* Repeat old command? */
							strcpy(commandbuf, oldcommand);
						else /* No, a new valid command */
							strcpy(oldcommand, commandbuf);

						con_parse(s, commandbuf);

						redraw_console = 1;
					}

				if (redraw_console)
					con_draw(s, s->osc_backup);
			}

			con_restore_screen(s, s->osc_backup);

		} else {
#endif

			while (_debugstate_valid) {
				con_parse(s, _debug_get_input());
				sciprintf("\n");
			}
#if 0
		}
#endif
/* Resume music playing */
	if (s->sfx_driver)
		(s->sfx_driver->resume)(s);
}





