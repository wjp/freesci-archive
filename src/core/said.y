/***************************************************************************
 said.y Copyright (C) 1999 Christoph Reichenbach


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

%{

#include <engine.h>

#define SAID_BRANCH_NULL 0

#define MAX_SAID_TOKENS 32

#define YYDEBUG 1
#undef SAID_DEBUG

static char *said_parse_error;

static int said_token;
static int said_tokens_nr;
static int said_tokens[MAX_SAID_TOKENS];

static int said_blessed;  /* increminated by said_top_branch */

static int said_tree_pos; /* Set to 0 if we're out of space */
#define SAID_TREE_START 4; /* Reserve space for the 4 top nodes */

#define VALUE_IGNORE -424242

static parse_tree_node_t said_tree[VOCAB_TREE_NODES];

typedef int wgroup_t;
typedef int tree_t;
typedef int said_spec_t;

static tree_t
said_aug_branch(int, int, tree_t, tree_t);

static tree_t
said_attach_branch(tree_t, tree_t);

static tree_t
said_wgroup_branch(wgroup_t);

static said_spec_t
said_top_branch(tree_t);

static int
yylex(void);

static int
yyerror(char *s)
{
  said_parse_error = strdup(s);
  return 1; /* Abort */
}

%}

%token WGROUP /* Word group */
%token YY_COMMA     /* 0xf0 */
%token YY_AMP       /* 0xf1 */
%token YY_SLASH     /* 0xf2 */
%token YY_PARENO    /* 0xf3 */
%token YY_PARENC    /* 0xf4 */
%token YY_BRACKETSO /* 0xf5 */
%token YY_BRACKETSC /* 0xf6 */
%token YY_HASH      /* 0xf7 */
%left YY_LT         /* 0xf8 */
%token YY_GT        /* 0xf9 */

%%

said_spec:	  ne_subexpression more_after
			{ $$ = said_top_branch(said_attach_branch($1, said_aug_branch(0x142, 0x14a, SAID_BRANCH_NULL, $2))) }
		| ne_subexpression nested_before_first more_after
			{ $$ = said_top_branch(said_attach_branch($1, said_attach_branch($2, $3))) }
		| ne_subexpression nested_before_first nested_before_second more_after
			{ $$ = said_top_branch(said_attach_branch($1, said_attach_branch($2, said_attach_branch($3, $4)))) }
		;


subexpression:    /* empty */
			{ $$ = SAID_BRANCH_NULL }
		| ne_subexpression
			{ $$ = $1 }
		;


ne_subexpression:
		  expression YY_LT subexpression
			{ $$ = said_aug_branch(0x144, 0x14f, $1, $3) }
		| expression YY_LT subexpression YY_LT subexpression
			{ $$ = said_aug_branch(0x141, 0x144, $1, said_aug_branch(0x144, 0x14f, $3, $5)) }
		| expression YY_BRACKETSO YY_LT subexpression YY_BRACKETSC
			{ $$ = said_aug_branch(0x152, 0x144, $1, $4) }
		| expression
			{ $$ = $1 }
		;

expression:	  main_expression
			{ $$ = $1 }
		| main_expression YY_COMMA expression
			{ $$ = said_attach_branch($1, $3) }
		;

main_expression:  YY_BRACKETSO subexpression YY_BRACKETSC
			{ $$ = said_aug_branch(0x152, 0x14c, $2, SAID_BRANCH_NULL) }
		| YY_PARENO subexpression YY_PARENC
			{ $$ = said_aug_branch(0x141, 0x14c, $2, SAID_BRANCH_NULL) }
		| WGROUP
			{ $$ = said_wgroup_branch($1) }
		;

before_exp:	  /* empty */
			{ $$ = SAID_BRANCH_NULL }
		| YY_SLASH subexpression
			{ $$ = $2 }
		;

ne_before_exp:	YY_SLASH subexpression
			{ $$ = $2 }
		;

nested_before_first:	  ne_before_exp
			{ $$ = said_aug_branch(0x142, 0x14a, $1, SAID_BRANCH_NULL) }
		| YY_BRACKETSO before_exp YY_BRACKETSC
			{ $$ = said_aug_branch(0x152, 0x142, $2, SAID_BRANCH_NULL) }
		;

nested_before_second:	  ne_before_exp
			{ $$ = said_aug_branch(0x143, 0x14a, $1, SAID_BRANCH_NULL) }
		| YY_BRACKETSO before_exp YY_BRACKETSC
			{ $$ = said_aug_branch(0x152, 0x143, $2, SAID_BRANCH_NULL) }
		;

more_after:	  /* empty */
			{ $$ = SAID_BRANCH_NULL }
		| YY_GT
			{ $$ = said_aug_branch(0x14b, SAID_LONG(SAID_GT), SAID_BRANCH_NULL, SAID_BRANCH_NULL) }
		;



%%

int
parse_yy_token_lookup[] = {YY_COMMA, YY_AMP, YY_SLASH, YY_PARENO, YY_PARENC, YY_BRACKETSO, YY_BRACKETSC,
			   YY_HASH, YY_LT, YY_GT};

static int
yylex(void)
{
  int retval = said_tokens[said_token++];

  if (retval < SAID_LONG(SAID_FIRST)) {
    yylval = retval;
    retval = WGROUP;
  } else {
    retval >>= 8;

    if (retval == SAID_END)
      retval = 0;
    else {
      assert(retval >= SAID_FIRST);
      retval = parse_yy_token_lookup[retval - SAID_FIRST];
    }
  }

  return retval;
}

#define SAID_NEXT_NODE ((said_tree_pos == 0) || (said_tree_pos >= VOCAB_TREE_NODES))? said_tree_pos = 0 : said_tree_pos++

static inline int
said_leaf_node(tree_t pos, int value)
{
  said_tree[pos].type = PARSE_TREE_NODE_LEAF;

  if (value != VALUE_IGNORE)
    said_tree[pos].content.value = value;

  return pos;
}

static inline int
said_branch_node(tree_t pos, int left, int right)
{
  said_tree[pos].type = PARSE_TREE_NODE_BRANCH;

  if (left != VALUE_IGNORE)
    said_tree[pos].content.branches[0] = left;

  if (right != VALUE_IGNORE)
    said_tree[pos].content.branches[1] = right;

  return pos;
}

static tree_t
said_aug_branch(int n1, int n2, tree_t t1, tree_t t2)
{
  int retval;

  retval = said_branch_node(SAID_NEXT_NODE,
			    said_branch_node(SAID_NEXT_NODE,
					     said_leaf_node(SAID_NEXT_NODE, n1),
					     said_branch_node(SAID_NEXT_NODE,
							      said_leaf_node(SAID_NEXT_NODE, n2),
							      t1
							      )
					     ),
			    t2
			    );

#ifdef SAID_DEBUG
  fprintf(stderr,"AUG(0x%x, 0x%x, [%04x], [%04x]) = [%04x]\n", n1, n2, t1, t2, retval);
#endif

  return retval;
}

static tree_t
said_attach_branch(tree_t base, tree_t attacheant)
{
  int branchpos;
#ifdef SAID_DEBUG
  fprintf(stderr,"ATT2([%04x], [%04x]) = [%04x]\n", base, attacheant, base);
#endif
  if (!base)
    return 0; /* Happens if we're out of space */

  said_branch_node(base, VALUE_IGNORE, attacheant);

  return base;
}

static tree_t
said_wgroup_branch(wgroup_t wordgroup)
{
  int retval;

  retval =  said_branch_node(SAID_NEXT_NODE,
			     said_branch_node(SAID_NEXT_NODE,
					      said_leaf_node(SAID_NEXT_NODE, 0x141), /* Magic number #1 */
					      said_branch_node(SAID_NEXT_NODE,
							       said_leaf_node(SAID_NEXT_NODE, 0x153), /* Magic number #2 */
							       said_leaf_node(SAID_NEXT_NODE, wordgroup)
							       )
					      ),
			     SAID_BRANCH_NULL
			     ); /* I suppose I've been doing too much LISP lately. */

#ifdef SAID_DEBUG
  fprintf(stderr,"WGROUP(%03x) = [%04x]\n", wordgroup, retval);
#endif

  return retval;
}

static said_spec_t
said_top_branch(tree_t first)
{
#ifdef SAID_DEBUG
  fprintf(stderr, "TOP([%04x])\n", first);
#endif
  said_branch_node(0, 1, 2);
  said_leaf_node(1, 0x141); /* Magic number #1 */
  said_branch_node(2, 3, first);
  said_leaf_node(3, 0x13f); /* Magic number #2 */

  ++said_blessed;

  return 0;
}


int
said_parse_spec(state_t *s, byte *spec)
{
  int nextitem;

  said_parse_error = NULL;
  said_token = 0;
  said_tokens_nr = 0;
  said_blessed = 0;

  said_tree_pos = SAID_TREE_START;

  do {
    nextitem = *spec++;
    if (nextitem < SAID_FIRST)
      said_tokens[said_tokens_nr++] = nextitem << 8 | *spec++;
    else
      said_tokens[said_tokens_nr++] = SAID_LONG(nextitem);

  } while ((nextitem != SAID_END) && (said_tokens_nr < MAX_SAID_TOKENS));

  if (nextitem == SAID_END)
    yyparse();
  else {
    sciprintf("Error: SAID spec is too long\n");
    return 1;
  }

  if (said_parse_error) {
    sciprintf("Error while parsing SAID spec: %s\n", said_parse_error);
    free(said_parse_error);
    return 1;
  }

  if (said_tree_pos == 0) {
    sciprintf("Error: Out of tree space while parsing SAID spec\n");
    return 1;
  }

  if (said_blessed != 1) {
    sciprintf("Error: Found %d top branches\n");
    return 1;
  }

  return 0;
}


int
said(state_t *s, byte *spec)
{
  if (s->parser_valid) {

    if (said_parse_spec(s, spec)) {
      sciprintf("Offending spec was: ");
      vocab_decypher_said_block(s, spec);
      return 0;
    }

    vocab_dump_parse_tree(said_tree); /* Nothing better to do yet */
    return 0;

  }

  return 0;
}


#ifdef SAID_DEBUG_PROGRAM
int
main (int argc, char *argv)
{
  byte block[] = {0x01, 0x00, 0xf0, 0xf3, 0x01, 0x01, 0xf8, 0x01, 0x02, 0xf4, 0xf2, 0x01, 0x03, 0xff};
  state_t s;
  con_passthrough = 1;

  s.parser_valid = 1;
  said(&s, block);
}
#endif
