/***************************************************************************
 grammar.c Copyright (C) 2000 Christoph Reichenbach


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

***************************************************************************/

/* Functionality to transform the context-free SCI grammar rules into
** strict Greibach normal form (strict GNF), and to test SCI input against
** that grammar, writing an appropriate node tree if successful.
*/

#include <resource.h>
#include <vocabulary.h>
#include <stdarg.h>
#include <string.h>

#define TOKEN_OPAREN 0xff000000
#define TOKEN_CPAREN 0xfe000000
#define TOKEN_TERMINAL_CLASS 0x10000
#define TOKEN_TERMINAL_GROUP 0x20000
#define TOKEN_STUFFING_WORD 0x40000
#define TOKEN_NON_NT (TOKEN_OPAREN | TOKEN_TERMINAL_CLASS | TOKEN_TERMINAL_GROUP | TOKEN_STUFFING_WORD)
#define TOKEN_TERMINAL (TOKEN_TERMINAL_CLASS | TOKEN_TERMINAL_GROUP)

static int _allocd_rules = 0;

typedef struct {
  int id; /* non-terminal ID */
  int first_special; /* first terminal or non-terminal */
  int length;
  int data[0]; /* actual data */
} parse_rule_t;


typedef struct _parse_rule_list {
  int terminal; /* Terminal character this rule matches against or 0 for a non-terminal rule */
  parse_rule_t *rule;
  struct _parse_rule_list *next;
} parse_rule_list_t;


static void
vocab_print_rule(parse_rule_t *rule)
{
  int i;
  int wspace = 0;

  if (!rule)
    sciprintf("NULL rule");

  sciprintf("[%03x] -> ", rule->id);

  if (!rule->length)
    sciprintf("e");

  for(i = 0; i < rule->length; i++) {
    int token = rule->data[i];

    if (token == TOKEN_OPAREN) {

      if (i == rule->first_special)
	sciprintf("_");

      sciprintf("(");
      wspace = 0;
    } else if (token == TOKEN_CPAREN) {

      if (i == rule->first_special)
	sciprintf("_");

      sciprintf(")");
      wspace = 0;
    } else {
      if (wspace)
	sciprintf(" ");

      if (i == rule->first_special)
	sciprintf("_");
      if (token & TOKEN_TERMINAL_CLASS)
	sciprintf("C(%04x)", token & 0xffff);
      else if (token & TOKEN_TERMINAL_GROUP)
	sciprintf("G(%04x)", token & 0xffff);
      else if (token & TOKEN_STUFFING_WORD)
	sciprintf("%03x", token & 0xffff);
      else
	sciprintf("[%03x]", token); /* non-terminal */
      wspace = 1;
    }

    if (i == rule->first_special)
      sciprintf("_");
  }
}


static void
_vfree(parse_rule_t *rule)
{
  free(rule);
  --_allocd_rules;
  rule = NULL;
}

static parse_rule_t *
_vbuild(int id, int argc, ...)
{
  va_list args;
  int i;
  parse_rule_t *rule = malloc(sizeof(int) * (argc + 3));

  ++_allocd_rules;
  rule->id = id;
  rule->first_special = 0;
  rule->length = argc;
  va_start(args, argc);
  for (i = 0; i < argc; i++) {
    int v;
    rule->data[i] = v = va_arg(args, int);
    if ((!rule->first_special)
	&& ((v & TOKEN_TERMINAL)
	    || !(v & TOKEN_NON_NT)))
      rule->first_special = i;
  }
  va_end(args);
  return rule;
}

static parse_rule_t *
_vcat(int id, parse_rule_t *a, parse_rule_t *b)
{
  int i;
  parse_rule_t *rule = malloc(sizeof(int) * (a->length + b->length + 3));

  rule->id = id;
  rule->length = a->length + b->length;
  rule->first_special = a->first_special;
  ++_allocd_rules;

  memcpy(rule->data, a->data, sizeof(int) * a->length);
  memcpy(&(rule->data[a->length]), b->data, sizeof(int) * b->length);

  return rule;
}

static parse_rule_t *
_vinsert(parse_rule_t *turkey, parse_rule_t *stuffing)
{
  int firstnt = turkey->first_special;
  parse_rule_t *rule;

  while ((firstnt < turkey->length)
	 && (turkey->data[firstnt] & TOKEN_NON_NT))
    firstnt++;

  if ((firstnt == turkey->length)
      || (turkey->data[firstnt] != stuffing->id))
    return NULL;

  rule = malloc(sizeof(int) * (turkey->length - 1 + stuffing->length + 3));
  rule->id = turkey->id;
  rule->first_special = firstnt + stuffing->first_special;
  rule->length = turkey->length - 1 + stuffing->length;
  ++_allocd_rules;

  if (firstnt > 0)
    memcpy(rule->data, turkey->data, sizeof(int) * firstnt);
  memcpy(&(rule->data[firstnt]), stuffing->data, sizeof(int) * stuffing->length);
  if (firstnt < turkey->length - 1)
    memcpy(&(rule->data[firstnt + stuffing->length]), &(turkey->data[firstnt + 1]),
	   sizeof(int) * (turkey->length - firstnt - 1));

  return rule;
}


static int
_greibach_rule_p(parse_rule_t *rule)
{
  int pos = rule->first_special;
  while (pos < rule->length
	 && (rule->data[pos] & TOKEN_NON_NT)
	 && !(rule->data[pos] & TOKEN_TERMINAL))
    ++pos;

  if (pos == rule->length)
    return 0;

  return (rule->data[pos] & TOKEN_TERMINAL);
}

static parse_rule_t *
_vbuild_rule(parse_tree_branch_t *branch)
{
  parse_rule_t *rule;
  int tokens = 0, tokenpos = 0, i;

  while (tokenpos < 10 && branch->data[tokenpos]) {
    int type = branch->data[tokenpos];
    tokenpos += 2;

    if ((type == VOCAB_TREE_NODE_COMPARE_TYPE)
	|| (type == VOCAB_TREE_NODE_COMPARE_GROUP)
	|| (type == VOCAB_TREE_NODE_FORCE_STORAGE))
      ++tokens;
    else if (type > VOCAB_TREE_NODE_LAST_WORD_STORAGE)
      tokens += 5;
    else return NULL; /* invalid */
  }

  rule = malloc(sizeof(int) * (3 + tokens));
  ++_allocd_rules;
  rule->id = branch->id;
  rule->length = tokens;
  rule->first_special = 0;

  tokens = 0;
  for (i = 0; i < tokenpos; i += 2) {
    int type = branch->data[i];
    int value = branch->data[i + 1];

    if (type == VOCAB_TREE_NODE_COMPARE_TYPE)
      rule->data[tokens++] = value | TOKEN_TERMINAL_CLASS;
    else if (type == VOCAB_TREE_NODE_COMPARE_GROUP)
      rule->data[tokens++] = value | TOKEN_TERMINAL_GROUP;
    else if (type == VOCAB_TREE_NODE_FORCE_STORAGE)
      rule->data[tokens++] = value | TOKEN_STUFFING_WORD;
    else { /* normal inductive rule */
      rule->data[tokens++] = TOKEN_OPAREN;
      rule->data[tokens++] = type | TOKEN_STUFFING_WORD;
      rule->data[tokens++] = value | TOKEN_STUFFING_WORD;

      if (i == 0)
	rule->first_special = tokens;

      rule->data[tokens++] = value; /* The non-terminal */
      rule->data[tokens++] = TOKEN_CPAREN;
    }
  }

  return rule;
}

/************** Rule lists **************/
/* Slow recursive implementations ahead */
/****************************************/

void
vocab_free_rule_list(parse_rule_list_t *list)
{
  if (list) {
    _vfree(list->rule);
    vocab_free_rule_list(list->next); /* Yep, this is slow and memory-intensive. */
    free(list);
  }
}

static inline int
_rules_equal_p(parse_rule_t *r1, parse_rule_t *r2)
{
  if ((r1->id != r2->id)
      || (r1->length != r2->length)
      || (r1->first_special != r2->first_special))
    return 0;

  return !(memcmp(r1->data, r2->data, sizeof(int) * r1->length));
}

parse_rule_list_t *
vocab_add_rule(parse_rule_list_t *list, parse_rule_t *rule)
{
  parse_rule_list_t *new_elem = malloc(sizeof(parse_rule_list_t));
  int term = rule->data[rule->first_special];

  new_elem->rule = rule;
  new_elem->next = NULL;
  new_elem->terminal = term = ((term & TOKEN_TERMINAL)? term : 0);

  if (!list)
    return new_elem;
  else if (term < list->terminal) {
      new_elem->next = list;
      return new_elem;
  } else {
    parse_rule_list_t *seeker = list;

    while (seeker->next && seeker->next->terminal <= term) {
      if (seeker->next->terminal == term)
	if (_rules_equal_p(seeker->next->rule, rule)) {
	  _vfree(rule);
	  return list; /* No duplicate rules */
	}
      seeker = seeker->next;
    }

    new_elem->next = seeker->next;
    seeker->next = new_elem;
    return list;
  }
}

static void
_vprl(parse_rule_list_t *list, int pos)
{
  if (list) {
    sciprintf("R%03d: ", pos);
    vocab_print_rule(list->rule);
    sciprintf("\n");
    _vprl(list->next, pos+1);
  } else {
    sciprintf("%d rules total.\n", pos);
  }
}

void
vocab_print_rule_list(parse_rule_list_t *list)
{
  _vprl(list, 0);
}

static parse_rule_list_t *
_vocab_split_rule_list(parse_rule_list_t *list)
{
  if (!list->next
      || (list->next->terminal)) {
    parse_rule_list_t *tmp = list->next;
    list->next = NULL;
    return tmp;
  }
  else return _vocab_split_rule_list(list->next);
}

static parse_rule_list_t *
_vocab_merge_rule_lists(parse_rule_list_t *l1, parse_rule_list_t *l2)
{
  parse_rule_list_t *retval = l1, *seeker = l2;
  while (seeker) {
    retval = vocab_add_rule(retval, seeker->rule);
    seeker = seeker->next;
  }

  return retval;
}

static int
_vocab_rule_list_length(parse_rule_list_t *list)
{
  return ((list)? _vocab_rule_list_length(list->next) + 1 : 0);
}

void
vocab_gnf_foo(parse_tree_branch_t *branches, int branches_nr)
{
  int i;
  int iterations = 0;
  int last_termrules, termrules = 0;
  int ntrules_nr;
  parse_rule_list_t *ntlist = NULL;
  parse_rule_list_t *tlist, *new_tlist;
  parse_rule_t *r1, *r2, *r3;

  for (i = 1; i < branches_nr; i++) { /* branch rule 0 is treated specially */
    parse_rule_t *rule = _vbuild_rule(branches + i);
    //    vocab_print_rule(rule);sciprintf("\n");
    ntlist = vocab_add_rule(ntlist, rule);
  }

  tlist = _vocab_split_rule_list(ntlist);
  ntrules_nr = _vocab_rule_list_length(ntlist);
  sciprintf("(%d non-GNF rules)\n", ntrules_nr);

  new_tlist = tlist;
  tlist = NULL;

  do {
    parse_rule_list_t *new_new_tlist = NULL;
    parse_rule_list_t *ntseeker, *tseeker;
    last_termrules = termrules;

    ntseeker = ntlist;
    while (ntseeker) {
      tseeker = new_tlist;

      while (tseeker) {
	parse_rule_t *newrule = _vinsert(ntseeker->rule, tseeker->rule);
	if (newrule)
	  new_new_tlist = vocab_add_rule(new_new_tlist, newrule);
	tseeker = tseeker->next;
      }

      ntseeker = ntseeker->next;
    }

    tlist = _vocab_merge_rule_lists(tlist, new_tlist);
    new_tlist = new_new_tlist;

    termrules = _vocab_rule_list_length(new_new_tlist);
    sciprintf("After iteration #%d: %d new term rules\n", ++iterations, termrules);
  } while (termrules && (iterations < 30));

  vocab_free_rule_list(ntlist);

  sciprintf("\nGNF rules:\n");
  vocab_print_rule_list(tlist);

  sciprintf("%d allocd rules\n", _allocd_rules);
  vocab_free_rule_list(tlist);
}
