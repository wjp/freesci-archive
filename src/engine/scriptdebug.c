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

#include <sciresource.h>
#include <engine.h>
#include <console.h>
#include <kdebug.h>
#include <vocabulary.h>
#include <kernel_compat.h>
#ifdef HAVE_UNISTD_H
/* Assume this is a sufficient precondition */
#  include <signal.h>
#endif

int _debugstate_valid = 0; /* Set to 1 while script_debug is running */
int _debug_step_running = 0; /* Set to >0 to allow multiple stepping */
int _debug_commands_not_hooked = 1; /* Commands not hooked to the console yet? */
int _debug_seeking = 0; /* Stepping forward until some special condition is met */
int _debug_seek_level = 0; /* Used for seekers that want to check their exec stack depth */
int _debug_seek_special = 0;  /* Used for special seeks(1) */
reg_t _debug_seek_reg = NULL_REG_INITIALIZER;  /* Used for special seeks(2) */

#define _DEBUG_SEEK_NOTHING 0
#define _DEBUG_SEEK_CALLK 1 /* Step forward until callk is found */
#define _DEBUG_SEEK_LEVEL_RET 2 /* Step forward until returned from this level */
#define _DEBUG_SEEK_SPECIAL_CALLK 3 /* Step forward until a /special/ callk is found */
#define _DEBUG_SEEK_SO 5 /* Step forward until specified PC (after the send command) and stack depth */

static reg_t *p_pc;
static stack_ptr_t *p_sp;
static stack_ptr_t *p_pp;
static reg_t *p_objp;
static unsigned int *p_restadjust;
static seg_id_t *p_var_segs;
static reg_t **p_vars;
static reg_t **p_var_base;
static int *p_var_max; /* May be NULL even in valid state! */

int _kdebug_cheap_event_hack = 0;
int _kdebug_cheap_soundcue_hack = -1;

char inputbuf[256] = "";

#define LOOKUP_SPECIES(species) (\
   (species >= 1000)? species : *(s->classtable[species].scriptposp) \
                                + s->classtable[species].class_offset)

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
c_segtable(state_t *s)
{
	int i;

	sciprintf("  ---- segment table ----\n");
	for (i = 0; i < s->seg_manager.heap_size; i++) {
		mem_obj_t *mobj = s->seg_manager.heap[i];
		if (mobj && mobj->type) {
			sciprintf(" [%04x] ", i);

			switch (mobj->type) {

			case MEM_OBJ_SCRIPT:
				sciprintf("S  script.%03d l:%d ",
					  mobj->data.script.nr,
					  mobj->data.script.lockers);
				break;

			case MEM_OBJ_CLONES:
				sciprintf("C  clones (%d allocd)", mobj->data.clones.clones_nr);
				break;

			case MEM_OBJ_LOCALS:
				sciprintf("L  locals %03d)",
					  mobj->data.locals.script_id);
				break;

			case MEM_OBJ_STACK:
				sciprintf("D  data stack (%d)",
					  mobj->data.stack.nr);
				break;

			case MEM_OBJ_SYS_STRINGS:
				sciprintf("Y  system string table");
				break;

			default:
				sciprintf("I  Invalid (type = %x)", mobj->type);
				break;
			}

			sciprintf("  seg_ID = %d \n", mobj->segmgr_id);
		}
	}
	sciprintf("\n");
	return 0;
}


static void
print_obj_head(state_t *s, object_t *obj)
{
	sciprintf(PREG" : %3d vars, %3d methods: %s\n",
		  PRINT_REG(obj->pos),
		  obj->variables_nr,
		  obj->methods_nr,
		  obj_get_name(s, obj->pos));
}

static void
_c_single_seg_info(state_t *s, mem_obj_t *mobj)
{
	switch (mobj->type) {

	case MEM_OBJ_SCRIPT: {
		int i;
		script_t *scr = &(mobj->data.script);
		sciprintf("script.%03d locked by %d, bufsize=%d (%x)\n",
			  scr->nr, scr->lockers, scr->buf_size, scr->buf_size);
		sciprintf("  Exports: %4d at %d\n",
			  scr->exports_nr,
			  ((byte *) scr->export_table) - ((byte *)scr->buf));
		sciprintf("  Synynms: %4d\n", scr->synonyms_nr);
		sciprintf("  Locals : %4d in segment 0x%x\n",
			  scr->locals_block->nr,
			  scr->locals_segment);
		sciprintf("  Objects: %4d\n",
			  scr->objects_nr);
		for (i = 0; i < scr->objects_nr; i++) {
			sciprintf("    ");
			print_obj_head(s, scr->objects + i);
		}
	}
		break;

	case MEM_OBJ_LOCALS: {
		local_variables_t *locals = &(mobj->data.locals);
		sciprintf("locals for script.%03d\n", locals->script_id);
		sciprintf("  %d (0x%x) locals\n", locals->nr, locals->nr);
	}
		break;

	case MEM_OBJ_STACK: {
		dstack_t *stack = &(mobj->data.stack);
		sciprintf("stack\n");
		sciprintf("  %d (0x%x) entries\n", stack->nr, stack->nr);
	}
		break;

	case MEM_OBJ_SYS_STRINGS: {
		sys_strings_t *strings = &(mobj->data.sys_strings);
		int i;

		sciprintf("system string table\n");
		for (i = 0; i < SYS_STRINGS_MAX; i++)
			if (strings->strings[i].name)
				sciprintf("  %s[%d]=\"%s\"\n",
					  strings->strings[i].name,
					  strings->strings[i].max_size,
					  strings->strings[i].value);
	}
		break;

	case MEM_OBJ_CLONES: {
		int i = 0;
		clone_table_t *ct =
			&(mobj->data.clones);

		sciprintf("clones\n");

		for (i = 0; i < ct->clones_nr; i++)
			if (ct->clones[i].next_free == CLONE_USED) {
				sciprintf("  [%04x] ", i);
				print_obj_head(s, &(ct->clones[i].obj));
			}
		     }
		break;

	default : sciprintf("Invalid type %d\n", mobj->type);
		break;
	}
}

int
c_seginfo(state_t *s)
{
	int i = 0;

	if (cmd_paramlength) {
		while (i < cmd_paramlength) {
			int nr = cmd_params[i++].val;
			if (nr < 0
			    || nr >= s->seg_manager.heap_size
			    || !s->seg_manager.heap[nr]) {
				sciprintf("Segment %04x does not exist\n", nr);
				return 1;
			}
			sciprintf("[%04x] ", nr);
			_c_single_seg_info(s, s->seg_manager.heap[nr]);
		}
	} else for (i = 0; i < s->seg_manager.heap_size; i++)
		if (s->seg_manager.heap[i]) {
			sciprintf("[%04x] ", i);
			_c_single_seg_info(s, s->seg_manager.heap[i]);
			sciprintf("\n");
		}
}

int
c_debuginfo(state_t *s)
{
	exec_stack_t *eframe = NULL;

	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (s->execution_stack && s->execution_stack_pos >= 0)
		eframe = s->execution_stack + s->execution_stack_pos;

	sciprintf("acc="PREG"[%04x] prev="PREG" &rest=%x\n",
		  PRINT_REG(s->r_acc), s->acc & 0xffff,
		  PRINT_REG(s->r_prev), *p_restadjust);
	if (eframe)
		sciprintf("pc="PREG" obj="PREG" fp="PSTK" sp="PSTK"\n",
			  PRINT_REG(*p_pc),
			  PRINT_REG(*p_objp),
			  PRINT_STK(*p_pp),
			  PRINT_STK(*p_sp));
	else
		sciprintf("<no execution stack: pc,obj,fp omitted>\n");
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

#warning "Re-implement con:so"
#if 0
int
c_stepover(state_t *s)
{
  int opcode, opnumber;

  if (!_debugstate_valid) {
    sciprintf("Not in debug state\n");
    return 1;
  }

  _debugstate_valid = 0;
  opcode = s->heap [*p_pc];
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
      _debug_seek_special = *p_pc + 5; break;

    case 0x44: /* callb W */
    case 0x47: /* calle B */
    case 0x56: /* super W */
      _debug_seek_special = *p_pc + 4; break;

    case 0x45: /* callb B */
    case 0x57: /* super B */
    case 0x4A: /* send W */
    case 0x54: /* self W */
      _debug_seek_special = *p_pc + 3; break;

    default:
      _debug_seek_special = *p_pc + 2;
    }
  }

  return 0;
}
#endif

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
  unsigned int i;
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

	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	sciprintf("Available classes:\n");
	for (i = 0; i < s->classtable_size; i++)
		if (s->classtable[i].reg.segment)
			sciprintf(" Class 0x%x at "PREG" (script 0x%x)\n",
				  i,
				  PRINT_REG(s->classtable[i].reg),
				  s->classtable[i].script);

  return 0;
}

int
c_viewinfo(state_t *s)
{
	int view = cmd_params[0].val;
	int loops, i;
	gfxr_view_t *view_pixmaps = NULL;

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

				if (con_can_handle_pixmaps()) {
					view_pixmaps = gfxr_get_view(s->gfx_state->resstate,
								     view, &i, &j);
					con_insert_pixmap(gfx_clone_pixmap(view_pixmaps->loops[i].cels[j],
									   s->gfx_state->driver->mode));
				}

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

  if ((unsigned)*i == cmd_paramlength)
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
      sciprintf("   Type[%04x] Group[%04x]\n", words[i].w_class, words[i].group);

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
#if 0
int
c_save_game(state_t *s)
{
	int omit_check = cmd_params[0].str[0] == '_';
	int i;

	if (!s) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	s->amp_rest = *p_restadjust;

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
#endif

int
c_restart_game(state_t *s)
{
  unsigned int i;

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
	exec_stack_t *xs;

	if (!s) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (s->execution_stack_pos >= 0)
		xs = s->execution_stack + s->execution_stack_pos;
	else {
		sciprintf("No exec stack!");
		return 1;
	}

	for (i = cmd_params[0].val ; i > 0 && (xs->sp - xs->fp - i) >= 0; i--)
		sciprintf("[SP:%04x] = "PREG"\n",
			  xs->sp - xs->fp - i, PRINT_REG(xs->sp[-i]));

	return 0;
}

#warning "Re-implement con:scriptinfo"
#if 0
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
#endif

char *selector_name(state_t *s, int selector)
{
	if (selector >= 0 && selector < s->selector_names_nr)
		return s->selector_names[selector];
	else
		return "--INVALID--";
}

int prop_ofs_to_id(state_t *s, int prop_ofs, int objp)
{
	return -1;
#warning "Re-implement prop_ofs_to_id"
#if 0
	word species = getInt16(s->heap + objp + SCRIPT_SPECIES_OFFSET);
	word type = getInt16(s->heap + objp + SCRIPT_INFO_OFFSET);
	byte *selectorIDoffset;

	int selectors = getInt16(s->heap + objp + SCRIPT_SELECTORCTR_OFFSET);

	byte *selectoroffset = (byte *) s->heap + objp + SCRIPT_SELECTOR_OFFSET;

	if (type & SCRIPT_INFO_CLASS)
		selectorIDoffset = selectoroffset + selectors * 2;
	else
		selectorIDoffset =
			s->heap
			+ LOOKUP_SPECIES(species)
			+ SCRIPT_SELECTOR_OFFSET
			+ selectors * 2;

	return (getInt16(selectorIDoffset + prop_ofs));
#endif
}

void
print_objname_wrapper(int foo)
{
	if (foo == -1)
		sciprintf("!invalid");
	else if (foo == -2)
		sciprintf("!non-object");}

int
print_objname(state_t *s, heap_ptr pos, int address)
{
	if ((pos < 4) || (pos > 0xfff0))
		return -1;
	else {
		if ((getInt16(s->heap + pos + SCRIPT_OBJECT_MAGIC_OFFSET)) != SCRIPT_OBJECT_MAGIC_NUMBER)
			return -2;
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
			else sciprintf((char *) s->heap + namepos);
		}
	}

	if (address)
		sciprintf(" @%04x\n", pos);
        return 0;
}

reg_t
disassemble(state_t *s, reg_t pos, int print_bw_tag, int print_bytecode)
/* Disassembles one command from the heap, returns address of next command or 0 if a ret was
** encountered.
*/
{
	mem_obj_t *memobj = GET_SEGMENT(s->seg_manager, pos.segment, MEM_OBJ_SCRIPT);
	script_t *script_entity = NULL;
	byte *scr;
	int scr_size;
	reg_t retval = make_reg(pos.segment, pos.offset + 1);
	word param_value;
	int opsize;
	int opcode;
	int bytecount = 1;
	int i = 0;

	if (!memobj) {
		sciprintf("Disassembly failed: Segment %04x non-existant or not a script\n",
			  pos.segment);
		return retval;
	} else script_entity = &(memobj->data.script);

	scr = script_entity->buf;
	scr_size = script_entity->buf_size;

	if (pos.offset >= scr_size) {
		sciprintf("Trying to disassemble beyond end of script\n");
		return pos;
	}

	opsize = scr[pos.offset];
	opcode = opsize >> 1;

	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return retval;
	}

	opsize &= 1; /* byte if true, word if false */

  
	sciprintf("%04x: ", pos.offset);

	if (print_bytecode) {
		while (formats[opcode][i]) {
			switch (formats[opcode][i++]) {

			case Script_SByte:
			case Script_Byte: bytecount++;
				break;

			case Script_Word:
			case Script_SWord: bytecount += 2;
				break;

			case Script_SVariable:
			case Script_Variable:
			case Script_Property:
			case Script_Global:
			case Script_Local:
			case Script_Temp:
			case Script_Param:
			case Script_SRelative:
				if (opsize)
					bytecount ++;
				else
					bytecount += 2;
				break;

			default:
				break;
			}
		}

		if (pos.offset + bytecount > scr_size) {
			sciprintf("Operation arguments extend beyond end of script\n");
			return retval;
		}

		for (i = 0; i < bytecount; i++)
			sciprintf("%02x ", scr[pos.offset + i]);

		for (i = bytecount; i < 5; i++)
			sciprintf("   ");

	}

	if (print_bw_tag)
		sciprintf("[%c] ", opsize? 'B' : 'W');
	sciprintf("%s", s->opcodes[opcode].name);

	i = 0;
	while (formats[opcode][i])

		switch (formats[opcode][i++]) {

		case Script_Invalid: sciprintf("-Invalid operation-"); break;

		case Script_SByte:
		case Script_Byte: sciprintf(" %02x", scr[retval.offset++]); break;

		case Script_Word:
		case Script_SWord:
			sciprintf(" %04x", 0xffff & (scr[retval.offset]
						     | (scr[retval.offset+1] << 8)));
			retval.offset += 2;
			break;

		case Script_SVariable:
		case Script_Variable:
		case Script_Property:
		case Script_Global:
		case Script_Local:
		case Script_Temp:
		case Script_Param:
			if (opsize)
				param_value = scr[retval.offset++];
			else {
				param_value = 0xffff & (scr[retval.offset]
							| (scr[retval.offset+1] << 8));
				retval.offset += 2;
			}

			if (opcode == op_callk)
				sciprintf(" %s[%x]", (param_value < s->kernel_names_nr)
					  ? s->kernel_names[param_value] : "<invalid>", param_value);
			else sciprintf(opsize? " %02x" : " %04x", param_value);

			break;

		case Script_SRelative:
			if (opsize)
				param_value = scr[retval.offset++];
			else {
				param_value = 0xffff & (scr[retval.offset]
							| (scr[retval.offset+1] << 8));
				retval.offset += 2;
			}
			sciprintf (opsize? " %02x  [%04x]" : " %04x  [%04x]", param_value, (0xffff) & (retval.offset + param_value));
			break;

		case Script_End: retval = NULL_REG;
			break;

		default:
			sciprintf("Internal assertion failed in 'disassemble', %s, L%d\n", __FILE__, __LINE__);

		}

#warning "Re-enable extra disasm information"
#if 0
	if (pos == *p_pc) /* Extra information if debugging the current opcode */

		if ((opcode == op_pTos)||(opcode == op_sTop)||
		    (opcode == op_pToa)||(opcode == op_aTop)||
		    (opcode == op_dpToa)||(opcode == op_ipToa)||
		    (opcode == op_dpTos)||(opcode == op_ipTos)) {
			int prop_ofs = s->heap[retval - 1];
			int prop_id = prop_ofs_to_id(s, prop_ofs, *p_objp & 0xffff);

			sciprintf("	(%s)", selector_name(s, prop_id));
		}
#endif

	sciprintf("\n");

#warning "Re-enable extra disasm information"
#if 0
	if (pos == *p_pc) { /* Extra information if debugging the current opcode */

		if (opcode == op_callk) {
			int stackframe = s->heap[retval - 1] + (*p_restadjust * 2);
			int argc = getInt16(s->heap + *p_sp - stackframe - 2);
			int i;

			sciprintf(" Kernel params: (");

			for (i = 0; i < argc; i++) {
				sciprintf("%04x", getUInt16(s->heap + *p_sp - stackframe + i*2));
				if (i+1 < argc)
					sciprintf(", ");
			}
			sciprintf(")\n");

		} else

			if ((opcode == op_send) || (opcode == op_self)) {
				int restmod = *p_restadjust;
				int stackframe = s->heap[retval - 1] + (restmod * 2);
				word selector;
				heap_ptr selector_addr;

				while (stackframe > 0) {
					int argc = getInt16(s->heap + *p_sp - stackframe + 2);
					heap_ptr nameaddress = 0;
					heap_ptr called_obj_addr;
					char *name;

					if (opcode == op_send)
						called_obj_addr = s->acc;
					else if (opcode == op_self)
						called_obj_addr = *p_objp;
					else { /* op_super */
						called_obj_addr = s->heap[retval - 2];

						called_obj_addr = (s->classtable[called_obj_addr].scriptposp)?
							0 :
							*(s->classtable[called_obj_addr].scriptposp)
							+ s->classtable[called_obj_addr].class_offset;
					}

					selector = getInt16(s->heap + *p_sp - stackframe);

					if (called_obj_addr > 100) /* If we are in valid heap space */
						if (getInt16(s->heap + called_obj_addr + SCRIPT_OBJECT_MAGIC_OFFSET)
						    == SCRIPT_OBJECT_MAGIC_NUMBER)
							nameaddress = getUInt16(s->heap + called_obj_addr + SCRIPT_NAME_OFFSET);

					if (nameaddress)
						name = (char *) s->heap + nameaddress;
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

						sciprintf("%04x", 0xffff & getUInt16(s->heap + *p_sp - stackframe + 4));
						if (argc) sciprintf(", ");
						stackframe -= 2;

					}

					sciprintf(")\n");

					stackframe -= 4;
				} /* while (stackframe > 0) */

			} /* Send-like opcodes */

	} /* (heappos == *p_pc) */
#endif
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
      int i;

      for (i=0;i<4;i++)
	      sciprintf("%s vars at "PREG"\n",
			varnames[i],
			make_reg(p_var_segs[i], p_vars[i] - p_var_base[i]));
      return 0;
}

int
c_vmvars(state_t *s)
{
	char *vartype_pre = strchr(varabbrev, *cmd_params[0].str);
	int vartype;
	int idx = cmd_params[1].val;

	if (!vartype_pre) {
		sciprintf("Invalid variable type '%c'\n",
			  *cmd_params[0].str);
		return 1;
	}
	vartype = vartype_pre - varabbrev;

	if (idx < 0) {
		sciprintf("Invalid: negative index\n");
		return 1;
	}
	if ((p_var_max)
		&& (p_var_max[vartype] <= idx)) {
		sciprintf("Max. index is %d (0x%x)\n",
			  p_var_max[vartype], p_var_max[vartype]);
		return 1;
	}

	switch(cmd_paramlength) {
	case 2:
		sciprintf("%s var %d == "PREG"\n", varnames[vartype], idx,
			  PRINT_REG(p_vars[vartype][idx]));
		break;

	case 3:
		p_vars[vartype][idx] = cmd_params[2].reg;
		break;

	default:
		sciprintf("Too many arguments\n");
	}
	return 0;
}

#ifdef HAVE_FORK
static int _codebug_pid = 0;
static int _codebug_stdin[2];
static int _codebug_stdout[2];
static int _codebug_commands[2]; /* sends commands to intermediate process */

/* Codebugging uses two child processes:
** The first one only performs output "coloring", its child process
** is an actual secondary freesci process
*/

#define CODEBUG_BUFSIZE 512
static char *_codebug_colstr = "\033[31m\033[1m";
static char *_codebug_uncolstr = "\033[0m";

static void
codebug_send_command(char *cmd)
{
	if (_codebug_pid)
		write(_codebug_commands[1], cmd, strlen(cmd));
}

static void
_print_colored(int from_fd)
{
	char buf[CODEBUG_BUFSIZE + 64];
	char *buf_offset;
	int br;
	int length_increment;

	strcpy(buf, _codebug_colstr);
	length_increment = strlen(_codebug_colstr);
	buf_offset = buf + length_increment;
	length_increment += strlen(_codebug_uncolstr);

	do {
		br = read(from_fd, buf_offset, CODEBUG_BUFSIZE);
		if (br > 0) {
			strcpy(buf_offset + br, _codebug_uncolstr);
			/* Atomic write with colors to nullify risk of
			** interference from fg process  */
			write(1, buf, br + length_increment);
		}
	} while (br == CODEBUG_BUFSIZE);
}

void
_codebug_kill_children()
{
	if (_codebug_pid)
		kill(_codebug_pid, SIGTERM);
}

void
_codebug_sighandler(int i)
{
	if (i == SIGPIPE || i == SIGTERM)
		kill(_codebug_pid, SIGTERM);

	write(1, _codebug_uncolstr, strlen(_codebug_uncolstr));
	fprintf(stderr, "Child process failed, aborting!\n");
	fprintf(stderr, "(if you're using the UNIX sound server, you'll probably get spurious"
		" warnings regarding the sound server now...)\n");
	exit(1);
}

static void
do_codebug(char *path, char *datapath)
{
	pipe(_codebug_stdin);
	pipe(_codebug_stdout);
	close(_codebug_commands[1]);

	_codebug_pid = fork();
	if (_codebug_pid) {
		fd_set fs;
		int max_n;
		
		/* parent process */
		close(_codebug_stdin[0]);
		close(_codebug_stdout[1]);

		max_n = _codebug_commands[0];
		if (max_n < _codebug_stdout[0])
			max_n = _codebug_stdout[0];

		signal(SIGCHLD, _codebug_sighandler);
		signal(SIGPIPE, _codebug_sighandler);
		signal(SIGTERM, _codebug_sighandler);

		while (1) { /* Until sigchild */
			FD_ZERO(&fs);
			FD_SET(_codebug_commands[0], &fs);
			FD_SET(_codebug_stdout[0], &fs);

			fflush(NULL);
			select(max_n + 1, &fs, NULL, NULL, NULL);

			if (FD_ISSET(_codebug_commands[0], &fs)) {
				char buf[CODEBUG_BUFSIZE];
				int br;
				do {
					br = read(_codebug_commands[0], buf, CODEBUG_BUFSIZE);
					if (br > 0)
						write(_codebug_stdin[1], buf, br);
				} while (br == CODEBUG_BUFSIZE);
			}

			if (FD_ISSET(_codebug_stdout[0], &fs))
				_print_colored(_codebug_stdout[0]);

		}
	} else {
		/* child */
		close(_codebug_stdin[1]);
		close(_codebug_stdout[0]);

		/* Re-associate stdin, stdout, stderr with i/o pipes */
		dup2(_codebug_stdin[0], 0);
		dup2(_codebug_stdout[1], 1);
		dup2(_codebug_stdout[1], 2);

		printf("Calling '%s' on data directory '%s'...\n", path, datapath);
		execlp(path, path, "-D", "-gnull", "-Pnull", "-Onull", "-Mmt32gm",
		       "--disable-readline", "-d", datapath, NULL);
		perror("execlp of FreeSCI");
		exit(1);
	}
}

static int
c_codebug(state_t *s)
{
	char *path = cmd_params[0].str;

	if (_codebug_pid) {
		sciprintf("Already codebugging!\n");
		return 1;
	}

	pipe(_codebug_commands);
	_codebug_pid = fork();

	if (!_codebug_pid)
		do_codebug(path, s->resource_dir); /* Do codebugging in a child process */
	else {
		close(_codebug_commands[0]);
		sleep(1); /* Yield to the scheduler, at least a bit */
		atexit(_codebug_kill_children);
	}
}
#endif

int
c_backtrace(state_t *s)
{
	int i;

	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	sciprintf("Call stack (current base: 0x%x):\n", s->execution_stack_base);
	for (i = 0; i <= s->execution_stack_pos; i++) {
		exec_stack_t *call = &(s->execution_stack[i]);
		char *objname = obj_get_name(s, call->sendp);
		int paramc, totalparamc;

		switch (call->type) {

		case EXEC_STACK_TYPE_CALL: {/* Normal function */
			sciprintf(" %x:[%x]  %s::%s(", i, call->origin,
				  objname, (call->selector == -1)? "<call[be]?>":
				  selector_name(s,call->selector));
		}
			break;

		case EXEC_STACK_TYPE_KERNEL: /* Kernel function */
			sciprintf(" %x:[%x]  k%s(", i, call->origin, s->kernel_names[-(call->selector)-42]);
			break;

		case EXEC_STACK_TYPE_VARSELECTOR:
			sciprintf(" %x:[%x] vs%s %s::%s (", i, call->origin, (call->argc)? "write" : "read",
				  objname, s->selector_names[call->selector]);
			break;
		} /* switch */

		totalparamc = call->argc;

		if (totalparamc > 16)
			totalparamc = 16;

		for (paramc = 1; paramc <= totalparamc; paramc++) {
			sciprintf(PREG, call->variables_argp[paramc]);

			if (paramc < call->argc)
				sciprintf(", ");
		}

		if (call->argc > 16)
			sciprintf("...");

		sciprintf(")\n    obj@"PREG,
			  PRINT_REG(call->objp));
		if (call->type == EXEC_STACK_TYPE_CALL)
			sciprintf(" pc="PREG,
				  PRINT_REG(call->addr.pc));
		else
			sciprintf(" pc:none");

		if (call->sp == CALL_SP_CARRY)
			sciprintf(" sp:carry");
		else
			sciprintf(" sp="PSTK,
				  PRINT_STK(call->sp));

		sciprintf(" fp="PSTK,
			  PRINT_STK(call->fp));
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
	gfxop_update(s->gfx_state);
	gfxop_usleep(s->gfx_state, 10);

	return 0;
}


int
c_clear_screen(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	gfxop_clear_box(s->gfx_state, gfx_rect(0, 0, 320, 200));
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

int
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

int
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

int
c_gfx_print_dropviews(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (!s->drop_views)
		sciprintf("No dropped dynview list active.\n");
	else
		s->drop_views->print(GFXW(s->drop_views), 0);

	return 0;
}

int
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
	gfxop_usleep(s->gfx_state, 0);
	return 0;
}

#ifdef GFXW_DEBUG_WIDGETS
extern gfxw_widget_t *debug_widgets[];
extern int debug_widget_pos;

int
c_gfx_print_widget(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (cmd_paramlength) {
		unsigned int i;
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
c_gfx_draw_rect(state_t *s)
{
	int col = cmd_params[4].val;

	if (!s) {
		sciprintf("Not in debug state!\n");
		return 1;
	}

	if (col < 0 || col > 15)
		col = 0;

	gfxop_set_clip_zone(s->gfx_state, gfx_rect_fullscreen);
	gfxop_fill_box(s->gfx_state, gfx_rect(cmd_params[0].val, cmd_params[1].val, cmd_params[2].val, cmd_params[3].val), s->ega_colors[col]);
	gfxop_update(s->gfx_state);

	return 0;
}

int
c_gfx_propagate_rect(state_t *s)
{
	int map = cmd_params[4].val;
	rect_t rect;

	if (!s) {
		sciprintf("Not in debug state!\n");
		return 1;
	}

	if (map < 0 || map > 1)
		map = 0;

	gfxop_set_clip_zone(s->gfx_state, gfx_rect_fullscreen);

	rect = gfx_rect(cmd_params[0].val,
			cmd_params[1].val,
			cmd_params[2].val,
			cmd_params[3].val);

	if (map == 1)
		gfxop_clear_box(s->gfx_state, rect);
	else
		gfxop_update_box(s->gfx_state, rect);
	gfxop_update(s->gfx_state);
	gfxop_usleep(s->gfx_state, 10);

	return 0;
}

#define GETRECT(ll, rr, tt, bb) \
ll = GET_SELECTOR(pos, ll); \
rr = GET_SELECTOR(pos, rr); \
tt = GET_SELECTOR(pos, tt); \
bb = GET_SELECTOR(pos, bb);

int
c_gfx_draw_viewobj(state_t *s)
{
#warning "Re-implement con:gfx_draw_viewobj"
#if 0
	heap_ptr pos = (heap_ptr) (cmd_params[0].val);
	int is_view;
	int x, y, priority;
	int nsLeft, nsRight, nsBottom, nsTop;
	int brLeft, brRight, brBottom, brTop;

	if (!s) {
		sciprintf("Not in debug state!\n");
		return 1;
	}

	if ((pos < 4) || (pos > 0xfff0)) {
		sciprintf("Invalid address.\n");
		return 1;
	}

	if ((getInt16(s->heap + pos + SCRIPT_OBJECT_MAGIC_OFFSET)) != SCRIPT_OBJECT_MAGIC_NUMBER) {
		sciprintf("Not an object.\n");
		return 0;
	}


	is_view =
		(lookup_selector(s, pos, s->selector_map.x, NULL) == SELECTOR_VARIABLE)
		&&
		(lookup_selector(s, pos, s->selector_map.brLeft, NULL) == SELECTOR_VARIABLE)
		&&
		(lookup_selector(s, pos, s->selector_map.signal, NULL) == SELECTOR_VARIABLE)
		&&
		(lookup_selector(s, pos, s->selector_map.nsTop, NULL) == SELECTOR_VARIABLE);

	if (!is_view) {
		sciprintf("Not a dynamic View object.\n");
		return 0;
	}

	x = GET_SELECTOR(pos, x);
	y = GET_SELECTOR(pos, y);
	priority = GET_SELECTOR(pos, priority);
	GETRECT(brLeft, brRight, brBottom, brTop);
	GETRECT(nsLeft, nsRight, nsBottom, nsTop);
	gfxop_set_clip_zone(s->gfx_state, gfx_rect_fullscreen);

	brTop += 10;
	brBottom += 10;
	nsTop += 10;
	nsBottom += 10;

	gfxop_fill_box(s->gfx_state, gfx_rect(nsLeft, nsTop,
					      nsRight - nsLeft + 1,
					      nsBottom - nsTop + 1),
		       s->ega_colors[2]);

	gfxop_fill_box(s->gfx_state, gfx_rect(brLeft, brTop,
					      brRight - brLeft + 1,
					      brBottom - brTop + 1),
		       s->ega_colors[1]);


	gfxop_fill_box(s->gfx_state, gfx_rect(x-1, y-1,
					      3, 3),
		       s->ega_colors[0]);

	gfxop_fill_box(s->gfx_state, gfx_rect(x-1, y,
					      3, 1),
		       s->ega_colors[priority]);

	gfxop_fill_box(s->gfx_state, gfx_rect(x, y-1,
					      1, 3),
		       s->ega_colors[priority]);

	gfxop_update(s->gfx_state);

	return 0;
#endif
}


int
c_gfx_flush_resources(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	gfxop_set_pointer_cursor(s->gfx_state, GFXOP_NO_POINTER);
	sciprintf("Flushing resources...\n");
	s->visual->widfree(GFXW(s->visual));
	gfxr_free_all_resources(s->gfx_state->driver, s->gfx_state->resstate);
	s->visual = NULL;
	return 0;
}

c_gfx_update_zone(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	return s->gfx_state->driver->update(s->gfx_state->driver,
				     gfx_rect(cmd_params[0].val,
					      cmd_params[1].val,
					      cmd_params[2].val,
					      cmd_params[3].val),
				     gfx_point(cmd_params[0].val,
					       cmd_params[1].val),
					    GFX_BUFFER_FRONT
					    );

}

#if 0
#warning "Re-implement con:disasm"
int
c_disasm(state_t *s)
{
	int vpc = cmd_params[0].val;
	int op_count = 1;
	int do_bwc = 0;
	int do_bytes = 0;
	int i;
	int invalid = 0;

	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (vpc <= 0)
		vpc = *p_pc + vpc;

	for (i = 1; i < cmd_paramlength; i++) {
			if (!strcasecmp(cmd_params[i].str, "bwt"))
				do_bwc = 1;
			else if (!strcasecmp(cmd_params[i].str, "bc"))
				do_bytes = 1;
			else if (toupper(cmd_params[i].str[0]) == 'C')
				op_count = atoi(cmd_params[i].str + 1);
			else {
				invalid = 1;
				sciprintf("Invalid option '%s'\n", cmd_params[i].str);
			}
	}

	if (invalid || op_count < 0)
		return invalid;

	do {
		vpc = disassemble(s, (heap_ptr)vpc, do_bwc, do_bytes);
	} while ((vpc > 0) && (vpc < 0xfff2) && (cmd_paramlength > 1) && (--op_count));
	return 0;
}
#endif


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

	s->acc = (gint16) (cmd_params[0].val);
	return 0;
}

int
c_resource_id(state_t *s)
{
	int id = cmd_params[0].val;

	sciprintf("%s.%d (0x%x)\n", sci_resource_types[id >> 11], id &0x7ff, id & 0x7ff);
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
c_heap_dump_all(state_t *s)
{
	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	heap_dump_all(s->_heap);
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
	char modechars[] = "ulgcmfbadspMSFtr"; /* Valid parameter chars */
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
	unsigned int i;

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

#define GFX_DEBUG_MODES 4

int
c_gfx_debuglog(state_t *s)
{
	unsigned int i;
	gfx_driver_t *drv = s->gfx_state->driver;
	struct {
		char *name;
		char id;
		int flag;
	} gfx_debug_mode[GFX_DEBUG_MODES] = {
		{ "Mouse Pointer", 'p', GFX_DEBUG_POINTER},
		{ "Updates", 'u', GFX_DEBUG_UPDATES},
		{ "Pixmap operations", 'x', GFX_DEBUG_PIXMAPS},
		{ "Basic operations", 'b', GFX_DEBUG_BASIC},
	};

	if (!_debugstate_valid) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	if (cmd_paramlength == 0) {
		int j;

		sciprintf("Logging in GFX driver:\n");
		if (!drv->debug_flags)
			sciprintf("  (nothing)\n");

		for (j = 0; j < GFX_DEBUG_MODES; j++)
			if (drv->debug_flags
			    & gfx_debug_mode[j].flag) {
				sciprintf("  - %s (%c)\n",
					  gfx_debug_mode[j].name,
					  gfx_debug_mode[j].id);
			}

	} else for (i = 0; i < cmd_paramlength; i++) {
		int mode;
		int j = 0;
		int flags = 0;

		if (cmd_params[i].str[0] == '-')
			mode = 0;
		else if (cmd_params[i].str[0] == '+')
			mode = 1;
		else {
			sciprintf("Mode spec must start with '+' or '-'\n");
			return 1;
		}

		while (cmd_params[i].str[++j]) {
			int k;
			int flag = 0;

			for (k = 0; !flag && k < GFX_DEBUG_MODES; k++)
				if (gfx_debug_mode[k].id == cmd_params[i].str[j])
					flag = gfx_debug_mode[k].flag;

			if (!flag) {
				sciprintf("Invalid/unknown mode flag '%c'\n",
					  cmd_params[i].str[j]);
				return 1;
			}
			flags |= flag;
		}

		if (mode) /* + */
			drv->debug_flags |= flags;
		else /* - */
			drv->debug_flags &= ~flags;
	}

	return 0;
}


int
show_list(state_t *s, heap_ptr list)
{
	heap_ptr prevnode = 0, node;
	int nodectr = 0;

	sciprintf("List at %04x:\n", list);
	node = getUInt16(s->heap + list );

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
		sciprintf("%s: C %03x G %03x\n", word->word, word->w_class, word->group);
	}
	sciprintf("%d words\n", s->parser_words_nr);

	return 0;
}

int
c_show_list(state_t *s)
{
	heap_ptr list = (guint16)cmd_params[0].val;
	int size;

	if (!s) {
	  sciprintf("Not in debug state\n");
	  return 1;
	}

	size = getUInt16(s->heap + list - 2);
	if (size != 6 && size != 8) {
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

#warning "Make mem_info a little more useful"

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
					print_objname_wrapper(print_objname
							      (s, (heap_ptr)(seeker - SCRIPT_OBJECT_MAGIC_OFFSET + 4)
							       /* to compensate for the header */, 1));

				seeker += size;
			} while (segment && (seeker < 0xffff ));
		}

	for (i = 0; i < SCRIPT_MAX_CLONES; i++)
		if (s->clone_list[i])
			print_objname_wrapper(print_objname(s, s->clone_list[i], 1));

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


#define ASSERT_PARAMS(number) \
   if (cmd_paramlength <= number) {\
     sciprintf("Operation '%s' needs %d parameters\n", op, number); \
     return 1;\
   }

int
c_snd(state_t *s)
{
	char *op = cmd_params[0].str;

	if (!s) {
		sciprintf("Must have state to do this!");
		return 1;
	}

	if (!strcmp(op, "stop")) {
		sound_command_default(s, SOUND_COMMAND_SUSPEND_ALL, 0, 0);
	} else if (!strcmp(op, "resume")) {
		sound_command_default(s, SOUND_COMMAND_RESUME_ALL, 0, 0);
	} else if (!strcmp(op, "play")) {
		ASSERT_PARAMS(1);
		if (!sound_command_default(s, SOUND_COMMAND_INIT_HANDLE, 42, cmd_params[1].val)); {
			sound_command_default(s, SOUND_COMMAND_PLAY_HANDLE, 42, 1000);
		}
	} else if (!strcmp(op, "mute_channel")) {
		ASSERT_PARAMS(1);
		sound_command_default(s, SOUND_COMMAND_MUTE_CHANNEL, 0, cmd_params[1].val);
	} else if (!strcmp(op, "unmute_channel")) {
		ASSERT_PARAMS(1);
		sound_command_default(s, SOUND_COMMAND_UNMUTE_CHANNEL, 0, cmd_params[1].val);
	} else if (!strcmp(op, "mute")) {
		int i;

		for (i = 0; i < 16; i++)
			sound_command_default(s, SOUND_COMMAND_MUTE_CHANNEL, 0, i);
	} else if (!strcmp(op, "unmute")) {
		int i;

		for (i = 0; i < 16; i++)
			sound_command_default(s, SOUND_COMMAND_UNMUTE_CHANNEL, 0, i);
	} else if (!strcmp(op, "solo")) {
		int i;

		ASSERT_PARAMS(1);
		for (i = 0; i < 16; i++)
			sound_command_default(s, i == cmd_params[1].val?
				      SOUND_COMMAND_UNMUTE_CHANNEL :
				      SOUND_COMMAND_MUTE_CHANNEL, 0, i);
	} else if (!strcmp(op, "printmap")) {
		sound_command_default(s, SOUND_COMMAND_PRINT_MAPPING, 0, 0);
	} else if (!strcmp(op, "printchannels")) {
		sound_command_default(s, SOUND_COMMAND_PRINT_CHANNELS, 0, 0);
	} else if (!strcmp(op, "songid")) {
		sound_command_default(s, SOUND_COMMAND_PRINT_SONG_INFO, 0, 0);
	} else {
		sciprintf("Invalid sound command %s\n", op);
		return 1;
	}

	sci_sched_yield();

	process_sound_events(s);
	return 0;
}

c_sndmap(state_t *s)
{
	char *op = cmd_params[0].str;
	int instr = cmd_params[1].val;

	if (!s) {
		sciprintf("Must have state to do this!");
		return 1;
	}

	if (!strcmp(op, "mute")) {
		sound_command_default(s, SOUND_COMMAND_IMAP_SET_INSTRUMENT, instr, NOMAP);
	} else if (!strcmp(op, "percussion")) {
		ASSERT_PARAMS(2);
		sound_command_default(s, SOUND_COMMAND_IMAP_SET_INSTRUMENT, instr, RHYTHM);
		sound_command_default(s, SOUND_COMMAND_IMAP_SET_PERCUSSION, instr, cmd_params[2].val);
	} else if (!strcmp(op, "instrument")) {
		ASSERT_PARAMS(2);
		sound_command_default(s, SOUND_COMMAND_IMAP_SET_INSTRUMENT, instr, cmd_params[2].val);
	} else if (!strcmp(op, "shift")) {
		ASSERT_PARAMS(2);
		sound_command_default(s, SOUND_COMMAND_IMAP_SET_KEYSHIFT, instr, cmd_params[2].val);
	} else if (!strcmp(op, "finetune")) {
		ASSERT_PARAMS(2);
		sound_command_default(s, SOUND_COMMAND_IMAP_SET_FINETUNE, instr, cmd_params[2].val);
	} else if (!strcmp(op, "bender")) {
		ASSERT_PARAMS(2);
		sound_command_default(s, SOUND_COMMAND_IMAP_SET_BENDER_RANGE, instr, cmd_params[2].val);
	} else if (!strcmp(op, "volume")) {
		ASSERT_PARAMS(2);
		sound_command_default(s, SOUND_COMMAND_IMAP_SET_VOLUME, instr, cmd_params[2].val);
	} else {
		sciprintf("Invalid sound mapping command %s\n", op);
		sciprintf("Valid commands: mute, percussion, instrument, shift, finetune,\n"
			  "  bender, volume\n");
		return 1;
	}

	sci_sched_yield();
	gfxop_usleep(s->gfx_state, 350000);

	process_sound_events(s);
	return 0;
}

#undef ASSERT_PARAMS


#define GETRECT(ll, rr, tt, bb) \
ll = GET_SELECTOR(pos, ll); \
rr = GET_SELECTOR(pos, rr); \
tt = GET_SELECTOR(pos, tt); \
bb = GET_SELECTOR(pos, bb);

static void
viewobjinfo(state_t *s, heap_ptr pos)
{
#if 0
#warning "Re-implement viewobjinfo"
	char *signals[16] = {
		"stop_update",
		"updated",
		"no_update",
		"hidden",
		"fixed_priority",
		"always_update",
		"force_update",
		"remove",
		"frozen",
		"is_extra",
		"hit_obstacle",
		"doesnt_turn",
		"no_cycler",
		"ignore_horizon",
		"ignore_actor",
		"dispose!"
	};

	int x, y, z, priority;
	int cel, loop, view, signal;
	int nsLeft, nsRight, nsBottom, nsTop;
	int lsLeft, lsRight, lsBottom, lsTop;
	int brLeft, brRight, brBottom, brTop;
	int i;
	int have_rects = 0;
	abs_rect_t nsrect, nsrect_clipped, brrect;

	if (lookup_selector(s, pos, s->selector_map.nsBottom, NULL) == SELECTOR_VARIABLE) {
		GETRECT(nsLeft, nsRight, nsBottom, nsTop);
		GETRECT(lsLeft, lsRight, lsBottom, lsTop);
		GETRECT(brLeft, brRight, brBottom, brTop);
		have_rects = 1;
	}

	GETRECT(view, loop, signal, cel);

	sciprintf("\n-- View information:\ncel %d/%d/%d at ", view, loop, cel);

	x = GET_SELECTOR(pos, x);
	y = GET_SELECTOR(pos, y);
	priority = GET_SELECTOR(pos, priority);
	if (s->selector_map.z > 0) {
		z = GET_SELECTOR(pos, z);
		sciprintf("(%d,%d,%d)\n", x, y, z);
	} else sciprintf("(%d,%d)\n", x, y);

	if (priority == -1)
		sciprintf("No priority.\n\n");
	else
		sciprintf("Priority = %d (band starts at %d)\n\n",
			  priority, PRIORITY_BAND_FIRST(priority));

	if (have_rects) {
		sciprintf("nsRect: [%d..%d]x[%d..%d]\n", nsLeft, nsRight, nsTop, nsBottom);
		sciprintf("lsRect: [%d..%d]x[%d..%d]\n", lsLeft, lsRight, lsTop, lsBottom);
		sciprintf("brRect: [%d..%d]x[%d..%d]\n", brLeft, brRight, brTop, brBottom);
	}

	nsrect = get_nsrect(s, pos, 0);
	nsrect_clipped = get_nsrect(s, pos, 1);
	brrect = set_base(s, pos);
	sciprintf("new nsRect: [%d..%d]x[%d..%d]\n", nsrect.x, nsrect.xend,
		  nsrect.y, nsrect.yend);
	sciprintf("new clipped nsRect: [%d..%d]x[%d..%d]\n", nsrect_clipped.x,
		  nsrect_clipped.xend, nsrect_clipped.y, nsrect_clipped.yend);
	sciprintf("new brRect: [%d..%d]x[%d..%d]\n", brrect.x, brrect.xend,
		  brrect.y, brrect.yend);
	sciprintf("\n signals = %04x:\n", signal);

	for (i = 0; i < 16; i++)
		if (signal & (1 << i))
			sciprintf("  %04x: %s\n", 1 << i, signals[i]);
#endif
}

#undef GETRECT


int
c_vo(state_t *s)
{
	reg_t pos = cmd_params[0].reg;
	object_t *obj = obj_get(s, pos);
	int i;

	sciprintf("["PREG"]: ", PRINT_REG(pos));
	if (!obj) {
		sciprintf("Not an object.");
		return 1;
	}

	print_obj_head(s, obj);

	sciprintf("  -- member variables:\n");
	for (i = 0; i < obj->variables_nr; i++)

		sciprintf("    [%03x] %s = "PREG"\n",
			  VM_OBJECT_GET_VARSELECTOR(obj, i),
			  selector_name(s, VM_OBJECT_GET_VARSELECTOR(obj, i)),
			  PRINT_REG(obj->variables[i]));
	sciprintf("  -- methods:\n");
	for (i = 0; i < obj->methods_nr; i++) {
		reg_t fptr = VM_OBJECT_READ_FUNCTION(obj, i);
		sciprintf("    [%03x] %s = "PREG"\n",
			  VM_OBJECT_GET_FUNCSELECTOR(obj, i),
			  selector_name(s, VM_OBJECT_GET_FUNCSELECTOR(obj, i)),
			  PRINT_REG(fptr));
	}
}

int
objinfo(state_t *s, reg_t pos)
{
#warning "Fix objinfo()"
#if 0
	word type;
	int is_view;

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
				+ LOOKUP_SPECIES(species)
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
				word value = 0xffff & getUInt16(selectoroffset + i*2);
				int svalue = getInt16(selectoroffset + i*2);

				sciprintf("  %s[%04x] = %04x ", selectorIDoffset?
					  selector_name(s, selectorID) : "<?>",
					  selectorID, value);
				if (value < 0x1000 || (value > 0xf000))
					sciprintf("(%d)", svalue);
				else {
					stack_ptr_t stackend =
						s->execution_stack[s->execution_stack_pos].sp;

					if (value > s->stack_base && value < stackend)
						sciprintf("<stack addr>");
					else if (listp(s, value))
						sciprintf("<list>");
					else print_objname(s, value, 0);
				}
				sciprintf("\n");
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
	is_view =
		(lookup_selector(s, pos, s->selector_map.x, NULL) == SELECTOR_VARIABLE)
		&&
		(lookup_selector(s, pos, s->selector_map.y, NULL) == SELECTOR_VARIABLE)
		&&
		(lookup_selector(s, pos, s->selector_map.signal, NULL) == SELECTOR_VARIABLE)
		&&
		(lookup_selector(s, pos, s->selector_map.cel, NULL) == SELECTOR_VARIABLE);

	if (is_view)
		viewobjinfo(s, pos);

	return 0;
#endif
}

int
c_heapobj(state_t *s)
{
	return
		objinfo(s, cmd_params[0].reg);
}

int
c_obj(state_t *s)
{
	return
		objinfo(s, *p_objp);
}

int
c_accobj(state_t *s)
{
	return
		objinfo(s, s->r_acc);
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
    bp->next = (breakpoint_t *) sci_malloc (sizeof (breakpoint_t));
    bp = bp->next;
  }
  else {
    s->bp_list = (breakpoint_t *) sci_malloc (sizeof (breakpoint_t));
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
  bp->data.name = sci_malloc (strlen (cmd_params [0].str)+1);
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
	if (type == BREAK_SELECTOR) sci_free (bp->data.name);
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

int
c_statusbar(state_t *s)
{
	if (!s) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	s->titlebar_port->color=s->ega_colors[cmd_params[0].val];
	s->titlebar_port->bgcolor=s->ega_colors[cmd_params[1].val];

	s->status_bar_foreground=cmd_params[0].val;
	s->status_bar_background=cmd_params[1].val;

	sciw_set_status_bar(s, s->titlebar_port, s->status_bar_text,
			    s->status_bar_foreground,
			    s->status_bar_background);
	gfxop_update(s->gfx_state);
	return 0;
}

int
c_sci_version(state_t *s)
{
	if (!s) {
		sciprintf("Not in debug state\n");
		return 1;
	}

	sciprintf("Emulating SCI version %d.%03d.%03d\n",
		  SCI_VERSION_MAJOR(s->version),
		  SCI_VERSION_MINOR(s->version),
		  SCI_VERSION_PATCHLEVEL(s->version));

	return 0;
}

void
script_debug(state_t *s, reg_t *pc, stack_ptr_t *sp, stack_ptr_t *pp, reg_t *objp,
	     unsigned int *restadjust, 
	     seg_id_t *segids, reg_t **variables,
	     reg_t **variables_base, int *variables_nr,
	     int bp)
{
	int have_windowed = s->gfx_state->driver->capabilities & GFX_CAPABILITY_WINDOWED;
	/* Do we support a separate console? */

#ifndef WANT_CONSOLE
	if (!have_windowed) {
		script_debug_flag = sci_debug_flags = 0;
		fprintf(stderr, "On-screen console disabled and driver claims not to support windowed mode.\n");
		if (script_error_flag) {
			fprintf(stderr, "Aborting...\n");
			exit(1);
		} else
			fprintf(stderr, "Continuing...\n");
		return;
	}
#endif


	if (sci_debug_flags & _DEBUG_FLAG_LOGGING) {
		int old_debugstate = _debugstate_valid;

		p_var_segs = segids;
		p_vars = variables;
		p_var_max = variables_nr;
		p_var_base = variables_base;
		p_pc = pc;
		p_sp = sp;
		p_pp = pp;
		p_objp = objp;
		p_restadjust = restadjust;
		sciprintf("acc="PREG"  ", PRINT_REG(s->r_acc));
		_debugstate_valid = 1;
		disassemble(s, *pc, 0, 1);

		_debugstate_valid = old_debugstate;

		if (!script_debug_flag)
			return;
	}

	if (_debug_seeking && !bp) { /* Are we looking for something special? */
#warning "FIXME: Determine opcode correctly"
		int op = 0/*s->heap[*pc] >> 1*/;
		int paramb1 = 0/*s->heap[*pc + 1]*/; /* Careful with that ! */

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
			if (!REG_EQ(*pc,_debug_seek_reg) ||
			    s->execution_stack_pos != _debug_seek_level)
				return;
			break;

		} /* switch(_debug_seeking) */

		_debug_seeking = _DEBUG_SEEK_NOTHING; /* OK, found whatever we were looking for */

	} /* if (_debug_seeking) */


	_debugstate_valid = (_debug_step_running == 0);

	if (_debugstate_valid) {
		p_pc = pc;
		p_sp = sp;
		p_pp = pp;
		p_objp = objp;
		p_restadjust = restadjust;
		p_var_segs = segids;
		p_vars = variables;
		p_var_max = variables_nr;
		p_var_base = variables_base;

		c_debuginfo(s);
		sciprintf("Step #%d\n", script_step_counter);
		disassemble(s, *pc, 0, 1);

		if (_debug_commands_not_hooked) {

			_debug_commands_not_hooked = 0;

			con_hook_command(c_debuginfo, "registers", "", "Displays all current register values");
			con_hook_command(c_vmvars, "vmvars", "!sia*", "Displays or changes variables in the VM\n\nFirst parameter is either g(lobal), l(ocal), t(emp) or p(aram).\nSecond parameter is the var number\nThird parameter (if specified) is the value to set the variable to");
			con_hook_command(c_sci_version, "sci_version", "", "Prints the SCI version currently being emulated");
			con_hook_command(c_vmvarlist, "vmvarlist", "!", "Displays the addresses of variables in the VM");
			con_hook_command(c_step, "s", "i*", "Executes one or several operations\n\nEXAMPLES\n\n"
					 "    s 4\n\n  Execute 4 commands\n\n    s\n\n  Execute next command");
#warning "Re-enable con:so hook"
#if 0
			con_hook_command(c_stepover, "so", "", "Executes one operation skipping over sends");
#endif
			con_hook_command(c_heapdump, "heapdump", "ii", "Dumps data from the heap\n"
					 "\nEXAMPLE\n\n    heapdump 0x1200 42\n\n  Dumps 42 bytes starting at heap address\n"
					 "  0x1200");
#warning "Re-enable con:disasm hook"
#if 0
			con_hook_command(c_disasm, "disasm", "is*", "Disassembles one or more commands\n\n"
					 "USAGE\n\n  disasm [startaddr] <options>\n\n"
					 "  Valid options are:\n"
					 "  bwt  : Print byte/word tag\n"
					 "  c<x> : Disassemble <x> bytes\n"
					 "  bc   : Print bytecode\n\n"
					 "  If the start address is specified to be zero, the\n"
					 "  current program counter is used instead.\n\n");
#endif
#warning "Re-enable con:scriptinfo hook"
#if 0
			con_hook_command(c_scriptinfo, "scripttable", "", "Displays information about all\n  loaded scripts");
#endif
#warning "Make con:heapobj expect an 'a' type argument"
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
			con_hook_command(c_heap_dump_all, "heapdump_all", "", "Shows allocated/free zones on the\n  heap");
			con_hook_command(c_sret, "sret", "", "Steps forward until ret is called\n  on the current execution"
					 " stack\n  level.");
			con_hook_command(c_resource_id, "resource_id", "i", "Identifies a resource number by\n"
					 "  splitting it up in resource type\n  and resource number.");
			con_hook_command(c_clear_screen, "clear_screen", "", "Clears the screen, shows the\n  background pic and picviews");
			con_hook_command(c_redraw_screen, "redraw_screen", "", "Redraws the screen");
			con_hook_command(c_show_list, "listinfo", "i", "Examines the list at the specified\n  heap address");
			con_hook_command(c_mem_info, "meminfo", "", "Displays heap memory information");
			con_hook_command(c_debuglog, "debuglog", "s*", "Sets the debug log modes.\n  Possible parameters:\n"
					 "  +x (sets debugging for x)\n  -x (unsets debugging for x)\n\nPossible values"
					 " for x:\n  u: Unimpl'd/stubbed stuff\n  l: Lists and nodes\n  g: Graphics\n"
					 "  c: Character handling\n  m: Memory management\n  f: Function call checks\n"
					 "  b: Bresenham details\n  a: Audio\n  d: System gfx management\n  s: Base setter"
					 "\n  p: Parser\n  M: The menu system\n  S: Said specs\n  F: File I/O\n  t: GetTime"
					 "\n  r: Room numbers\n"
					 "  *: Everything\n\n"
					 "  If invoked withour parameters,\n  it will list all activated\n  debug options.\n\n"
					 "SEE ALSO\n"
					 "  gfx_debuglog\n");
			con_hook_command(c_visible_map, "set_vismap", "i", "Sets the visible map.\n  Default is 0 (visual).\n"
					 "  Other useful values are:\n  1: Priority\n  2: Control\n  3: Auxiliary\n");
			con_hook_command(c_simkey, "simkey", "i", "Simulates a keypress with the\n  specified scancode.\n");
			con_hook_command(c_statusbar, "statusbar", "ii", "Sets the colors of the status bar. Also controllable from the script.\n");
			con_hook_command(c_bpx, "bpx", "s", "Sets a breakpoint on the execution of\n  the specified method.\n\n  EXAMPLE:\n"
					 "  bpx ego::doit\n\n  May also be used to set a breakpoint\n  that applies whenever an object\n"
					 "  of a specific type is touched:\n  bpx foo::\n");
			con_hook_command(c_bpe, "bpe", "ii", "Sets a breakpoint on the execution of specified"
					 " exported function.\n");
			con_hook_command(c_bplist, "bplist", "", "Lists all breakpoints.\n");
			con_hook_command(c_bpdel, "bpdel", "i", "Deletes a breakpoint with specified index.");
			con_hook_command(c_go, "go", "", "Executes the script.\n");
			con_hook_command(c_dumpnodes, "dumpnodes", "i", "shows the specified number of nodes\n"
					 "  from the parse node tree");
#warning "Re-enable savegames"
#if 0
			con_hook_command(c_save_game, "save_game", "s", "Saves the current game state to\n  the hard disk");
			con_hook_command(c_restore_game, "restore_game", "s", "Restores a saved game from the\n  hard disk");
#endif
			con_hook_command(c_restart_game, "restart", "s*", "Restarts the game.\n\nUSAGE\n\n  restart [-r] [-p]"
					 " [--play] [--replay]\n\n  There are two ways to restart an SCI\n  game:\n"
					 "  play (-p) calls the game object's play()\n    method\n  replay (-r) calls the replay() method");
#warning "Re-enable con:viewinfo"
#if 0
			con_hook_command(c_viewinfo, "viewinfo", "i", "Displays the number of loops\n  and cels of each loop"
					 " for the\n  specified view resource.\n\n  Output:\n    C(x): Check word type against x\n"
					 "    WG(x): Check word group against mask x\n    FORCE(x): Force storage node x\n");
#endif
			con_hook_command(c_list_sentence_fragments, "list_sentence_fragments", "", "Lists all sentence fragments (which\n"
					 "  are used to build Parse trees).");
			con_hook_command(c_parse, "parse", "s", "Parses a sequence of words and prints\n  the resulting parse tree.\n"
					 "  The word sequence must be provided as a\n  single string.");
			con_hook_command(c_gnf, "gnf", "", "Displays the Parse grammar\n  in strict GNF");
			con_hook_command(c_set_parse_nodes, "set_parse_nodes", "s*", "Sets the contents of all parse nodes.\n"
					 "  Input token must be separated by\n  blanks.");
			con_hook_command(c_objs, "objs", "", "Lists all objects and classes from all\n  currently loaded scripts, plus\n"
					 "  all clones.\n  Objects are not marked; clones are marked\n  with a preceeding '*', and classes\n"
					 "  with a preceeding '%'\n"
					 "\n\nSEE ALSO\n  clonetable\n");
			con_hook_command(c_snd, "snd", "si*",
					 "Executes a sound command\n\n"
					 "USAGE\n\n"
					 "  snd <command> [param]\n\n"
					 "  snd stop\n"
					 "    Suspends audio output.\n\n"
					 "  snd resume\n"
					 "    Resumes audio output.\n\n"
					 "  snd play <song>\n"
					 "    Plays a song with a high priority\n\n"
					 "  snd mute_channel <channel>\n"
					 "    Mutes one output channel\n\n"
					 "  snd unmute_channel <channel>\n"
					 "    Unmutes one output channel\n\n"
					 "  snd mute\n"
					 "    Mutes all channels\n\n"
					 "  snd unmute\n"
					 "    Unmutes all channels\n\n"
					 "  snd solo <channel>\n"
					 "    Mutes all channels except for one\n\n"
					 "  snd printchannels\n"
					 "    Prints all channels and instruments\n\n"
					 "  snd printmap\n"
					 "    Prints all channels and instruments,\n"
					 "    including their General MIDI map-\n"
					 "    pings\n\n"
					 "  snd songid\n"
					 "    Prints the ID (heap address) of the\n"
					 "    song currently playing. '42' is used\n"
					 "    by songs run from the command line.\n"
					 "SEE ALSO\n"
					 "  sndmap\n");
			con_hook_command(c_sndmap, "sndmap", "sii*",
					 "Changes MT32->GM mappings\n\n"
					 "USAGE\n\n"
					 "  sndmap <command> <instr> [param]\n\n"
					 "  sndmap mute <instr>\n"
					 "    Mutes the specified instrument\n\n"
					 "  sndmap percussion <instr> <gm-perc>\n"
					 "    Maps the instrument to a GM percussion\n"
					 "    instrument\n\n"
					 "  sndmap instrument <instr> <gm-inst>\n"
					 "    Maps the instrument to a GM instrument\n\n"
					 "  sndmap shift <instr> <shift>\n"
					 "    Selects the instrument shfit\n\n"
					 "  sndmap finetune <instr> <val>\n"
					 "    Fine-tunes the instrument\n\n"
					 "  sndmap bender <instr> <range>\n"
					 "    Selects a bender range\n\n"
					 "  sndmap volume <instr> <vol>\n"
					 "    Specifies a relative volume for the\n"
					 "    instrument (0-128)\n\n"
					 "SEE ALSO\n"
					 "  snd\n");
			con_hook_command(c_gfx_debuglog, "gfx_debuglog", "s*",
					 "Sets or prints the gfx driver's debug\n"
					 "settings\n\n"
					 "USAGE\n\n"
					 "  gfx_debuglog {[+|-][p|u|x|b]+}*\n\n"
					 "  gfx_debuglog\n\n"
					 "    Prints current settings\n\n"
					 "  gfx_debuglog +X\n\n"
					 "    Activates all debug features listed in X\n\n"
					 "  gfx_debuglog -X\n\n"
					 "    Deactivates the debug features listed in X\n\n"
					 "  Debug features:\n"
					 "    p: Pointer\n"
					 "    u: Updates\n"
					 "    x: Pixmaps\n"
					 "    b: Basic features\n\n"
					 "SEE ALSO\n"
					 "  debuglog\n");

#ifdef SCI_SIMPLE_SAID_CODE
			con_hook_command(c_sim_parse, "simparse", "s*", "Simulates a parsed entity.\n\nUSAGE\n  Call this"
					 " function with a list of\n  Said operators, words, and word group"
					 "\n  numbers to match Said() specs\n  that look identical.\n"
					 "\n  Note that opening braces and\n  everything behind them are\n"
					 "\n  removed from all non-operator\n  parameter tokens.\n"
					 "\n  simparse without parameters\n  removes the entity.\n");
#endif /* SCI_SIMPLE_SAID_CODE */
#ifdef GFXW_DEBUG_WIDGETS
			con_hook_command(c_gfx_print_widget, "gfx_print_widget", "i*", "If called with no parameters, it\n  shows which widgets are active.\n"
					 "  With parameters, it lists the\n  widget corresponding to the\n  numerical index specified (for\n  each parameter).");
#endif
			con_hook_command(c_gfx_flush_resources, "gfx_free_widgets", "", "Frees all dynamically allocated\n  widgets (for memory profiling).\n");
			con_hook_command(c_gfx_current_port, "gfx_current_port", "", "Determines the current port number");
			con_hook_command(c_gfx_print_port, "gfx_print_port", "i*", "Displays all information about the\n  specified port,"
					 " or the current port\n  if no port was specified.");
			con_hook_command(c_gfx_print_visual, "gfx_print_visual", "", "Displays all information about the\n  current widget state");
			con_hook_command(c_gfx_print_dynviews, "gfx_print_dynviews", "", "Shows the dynview list");
			con_hook_command(c_gfx_print_dropviews, "gfx_print_dropviews", "", "Shows the list of dropped\n  dynviews");
			con_hook_command(c_gfx_drawpic, "gfx_drawpic", "ii*", "Draws a pic resource\n\nUSAGE\n  gfx_drawpic <nr> [<pal> [<fl>]]\n"
					 "  where <nr> is the number of the pic resource\n  to draw\n  <pal> is the optional default\n  palette for the pic (0 is"
					 "\n  assumed if not specified)\n  <fl> are any pic draw flags (default\n  is 1)");
			con_hook_command(c_dump_words, "dumpwords", "", "Lists all parser words");
			con_hook_command(c_gfx_show_map, "gfx_show_map", "i", "Shows one of the screen maps\n  Semantics of the int parameter:\n"
					 "    0: visual map (back buffer)\n    1: priority map (back buf.)\n    2: control map (static buf.)");
			con_hook_command(c_gfx_fill_screen, "gfx_fill_screen", "i", "Fills the screen with one\n  of the EGA colors\n");
			con_hook_command(c_gfx_draw_rect, "gfx_draw_rect", "iiiii", "Draws a rectangle to the screen\n  with one of the EGA colors\n\nUSAGE\n\n"
					 "  gfx_draw_rect <x> <y> <xl> <yl> <color>");
			con_hook_command(c_gfx_propagate_rect,
					 "gfx_propagate_rect",
					 "iiiii",
					 "Propagates a lower gfx buffer to a\n"
					 "  higher gfx buffer.\n\nUSAGE\n\n"
					 "  gfx_propagate_rect <x> <y> <xl> <yl> <buf>\n");
			con_hook_command(c_gfx_update_zone, "gfx_update_zone", "iiii", "Propagates a rectangular area from\n  the back buffer to the front buffer"
					 "\n\nUSAGE\n\n"
					 "  gfx_update_zone <x> <y> <xl> <yl>");
#warning "Re-enable con:draw_viewobj"
#if 0
			con_hook_command(c_gfx_draw_viewobj, "draw_viewobj", "i", "Draws the nsRect and brRect of a\n  dynview object.\n\n  nsRect is green, brRect\n"
					 "  is blue.\n");
#endif
			con_hook_command(c_gfx_draw_cel, "gfx_draw_cel", "iii", "Draws a single view\n  cel to the center of the\n  screen\n\n"
					 "USAGE\n  gfx_draw_cel <view> <loop> <cel>\n");
			con_hook_command(c_gfx_priority, "gfx_priority", "i*", "Prints information about priority\n  bands\nUSAGE\n\n  gfx_priority\n\n"
					 "  will print the min and max values\n  for the priority bands\n\n  gfx_priority <val>\n\n  Print start of the priority\n"
					 "  band for the specified\n  priority\n");
			con_hook_command(c_segtable, "segtable", "!",
					 "Gives a short listing of all segments\n\n"
					 "SEE ALSO\n\n"
					 "  seginfo.1");
			con_hook_command(c_seginfo, "seginfo", "!i*",
					 "Explains the specified segment\n\n"
					 "USAGE\n\n"
					 "  seginfo\n"
					 "  seginfo <nr>\n"
					 "  Either explains all active segments\n"
					 "  (no parameter) or the specified one.\n\n"
					 "SEE ALSO\n\n"
					 "  segtable.1");
			con_hook_command(c_vo, "vo", "!a",
					 "Examines an object\n\n"
					 "SEE ALSO\n\n"
					 "  addresses.3");
#ifdef HAVE_FORK
			con_hook_command(c_codebug, "codebug", "!s",
					 "Starts codebugging mode\n\nUSAGE\n\n"
					 "  codebug path/to/old/freesci\n\n"
					 "  A different version of FreeSCI may be\n"
					 "  spawned as a child process; all commands\n"
					 "  send to this debugger will also be sent to\n"
					 "  to the child process.\n\nSEE ALSO\n\n  codebugging.3");
#endif


			con_hook_int(&script_debug_flag, "script_debug_flag", "Set != 0 to enable debugger\n");
			con_hook_int(&script_checkloads_flag, "script_checkloads_flag", "Set != 0 to display information\n"
				     "  when scripts are loaded or unloaded");
			con_hook_int(&script_abort_flag, "script_abort_flag", "Set != 0 to abort execution\n");
			con_hook_int(&script_step_counter, "script_step_counter", "# of executed SCI operations\n");
			con_hook_int(&sci_debug_flags, "debug_flags", "Debug flags:\n  0x0001: Log each command executed\n"
				     "  0x0002: Break on warnings\n");

			con_hook_page("codebugging",
				      "Co-debugging allows to run two (sufficiently\n"
				      "  recent) versions of FreeSCI concurrently,\n"
				      "  with one acting as a client of the other.\n"
				      "    Co-debugging can be started by calling\n"
				      "  'codebug' (see codebug.1); note that the\n"
				      "  argument passed to it must be a version of\n"
				      "  FreeSCI that performs fflush(NULL) before\n"
				      "  each read; only late 0.3.3-devel and later\n"
				      "  have this property.\n\n"
				      "  In co-debug mode, all commands are sent to\n"
				      "  both programs, UNLESS one of the following\n"
				      "  prefixes is used:\n\n"
				      "    '.' : Only sends to the foreground version\n"
				      "    ':' : Only sends to tbe background version\n\n"
				      "  For example, when running 0.3.3 from within\n"
				      "  0.6.0, \".version\" would determine the version\n"
				      "  as 0.6.0, and \"0.3.3\" would be returned for\n"
				      "  \":version\". Both versions would be print\n"
				      "  if only \"version\" was invoked, each result\n"
				      "  coming from a different process.\n\n"
				      "COLORS\n\n"
				      "  Whenever possible, the background process will\n"
				      "  have its output marked by a non-default color\n"
				      "  (usually red).\n\n"
				      "TROUBLESHOOTING\n\n"
				      "  If the background version appears to be silent,\n"
				      "  make sure it is calling fflush(NULL) before\n"
				      "  reading input.\n\n"
				      "SEE ALSO\n\n"
				      "  codebug.1");

		} /* If commands were not hooked up */

	}

	if (_debug_step_running)
		_debug_step_running--;

	/* Suspend music playing */
	if (s->sound_server)
		(s->sound_server->suspend)(s);

	while (_debugstate_valid) {
		int skipfirst = 0;
		char *commandstring;
#ifdef WANT_CONSOLE
#  ifndef FORCE_CONSOLE
		if (!have_windowed) {
#  endif
			char *input;
			con_gfx_show(s->gfx_state);
			input = con_gfx_read(s->gfx_state);
			con_gfx_hide(s->gfx_state);
			commandstring = input;
			sciprintf("> %s\n", commandstring);
#  ifndef FORCE_CONSOLE
		} else
#  endif
#endif
#ifndef FORCE_CONSOLE
			commandstring = _debug_get_input();

		/* Check if a specific destination has been given */
		if (commandstring
		    && (commandstring[0] == '.'
			|| commandstring[0] == ':'))
			skipfirst = 1;

#endif
#ifdef HAVE_FORK
		if (commandstring[0] != '.') {
			codebug_send_command(commandstring + skipfirst);
			codebug_send_command("\n");
		}
#endif
		if (commandstring[0] != ':')
			con_parse(s, commandstring + skipfirst);
		sciprintf("\n");
	}

/* Resume music playing */
	if (s->sound_server)
		(s->sound_server->resume)(s);
}



