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
/*
static tree_t
said_wgroup_branch(wgroup_t);
*/
static said_spec_t
said_top_branch(tree_t);

static tree_t
said_paren(tree_t, tree_t);

static tree_t
said_value(int, tree_t);

static tree_t
said_terminal(int);


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
%token YY_LT        /* 0xf8 */
%token YY_GT        /* 0xf9 */
%token YY_BRACKETSO_LT /* special token used to imitate LR(2) behaviour */

%%

saidspec :	  leftspec optcont
			{ $$ = said_top_branch(said_attach_branch($1, $2)) }
		| leftspec midspec optcont
			{ $$ = said_top_branch(said_attach_branch($1, said_attach_branch($2, $3))) }
		| leftspec midspec rightspec optcont
			{ $$ = said_top_branch(said_attach_branch($1, said_attach_branch($2, said_attach_branch($3, $4)))) }
		;


optcont :	 /* empty */
			{ $$ = SAID_BRANCH_NULL }
		| YY_GT
			{ $$ = said_paren(said_value(0x14b, said_value(0xf900, said_terminal(0xf900))), SAID_BRANCH_NULL) }
		;



leftspec :	/* empty */
			{ $$ = SAID_BRANCH_NULL }
		| expr
			{ $$ = said_paren(said_value(0x141, said_value(0x149, $1)), SAID_BRANCH_NULL) }
		;



midspec :	 YY_SLASH expr
			{ $$ = said_aug_branch(0x142, 0x14a, $2, SAID_BRANCH_NULL) }
		| YY_BRACKETSO YY_SLASH expr YY_BRACKETSC
			{ $$ = said_aug_branch(0x152, 0x142, said_aug_branch(0x142, 0x14a, $3, SAID_BRANCH_NULL), SAID_BRANCH_NULL) }
		| YY_SLASH
			{ $$ = SAID_BRANCH_NULL }
		;



rightspec :	 YY_SLASH expr
			{ $$ = said_aug_branch(0x143, 0x14a, $2, SAID_BRANCH_NULL) }
		| YY_BRACKETSO YY_SLASH expr YY_BRACKETSC
			{ $$ = said_aug_branch(0x152, 0x143, said_aug_branch(0x143, 0x14a, $3, SAID_BRANCH_NULL), SAID_BRANCH_NULL) }
		;



word :		 WGROUP
			{ $$ = said_paren(said_value(0x141, said_value(0x153, said_terminal($1))), SAID_BRANCH_NULL) }
		;


cwordset :	wordset
			{ $$ = said_aug_branch(0x141, 0x14f, $1, SAID_BRANCH_NULL) }
		| YY_BRACKETSO wordset YY_BRACKETSC
			{ $$ = said_aug_branch(0x141, 0x14f, said_aug_branch(0x152, 0x14c, said_aug_branch(0x141, 0x14f, $2, SAID_BRANCH_NULL), SAID_BRANCH_NULL), SAID_BRANCH_NULL) }
		;


wordset :	 word
			{ $$ = $1 }
		| YY_PARENO expr YY_PARENC
			{ $$ = said_aug_branch(0x141, 0x14c, $2, SAID_BRANCH_NULL) }
		| word YY_COMMA wordset
			{ $$ = said_attach_branch($1, $3) }
		;



expr :		 cwordset cwordrefset
			{ $$ = said_attach_branch($1, $2) }
		| cwordset
			{ $$ = $1 }
		| cwordrefset
			{ $$ = $1 }
		;



cwordrefset :	 wordrefset
			{ $$ = $1 }
		| YY_BRACKETSO_LT wordrefset YY_BRACKETSC
			{ $$ = said_aug_branch(0x152, 0x144, $2, SAID_BRANCH_NULL) }
		;



wordrefset :	YY_LT word recref
			{ $$ = said_aug_branch(0x144, 0x14f, $2, $3) }
		| YY_LT word
			{ $$ = said_aug_branch(0x144, 0x14f, $2, SAID_BRANCH_NULL) }
		;



recref :	YY_LT word recref
			{ $$ = said_aug_branch(0x141, 0x144, said_aug_branch(0x144, 0x14f, $2, SAID_BRANCH_NULL), $3) }
		| YY_LT word
			{ $$ = said_aug_branch(0x141, 0x144, said_aug_branch(0x144, 0x14f, $2, SAID_BRANCH_NULL), SAID_BRANCH_NULL) }
		;



%%

#if 0
((((((((((((((((((((((((((((((((((
))))))))))))))))))))))))))))))))))
//////////////////////////////////
[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#endif


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

    if (retval == SAID_TERM)
      retval = 0;
    else {
      assert(retval >= SAID_FIRST);
      retval = parse_yy_token_lookup[retval - SAID_FIRST];
      if (retval == YY_BRACKETSO
	  && ((said_tokens[said_token] >> 8) == SAID_LT))
	retval = YY_BRACKETSO_LT;
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
said_paren(tree_t t1, tree_t t2)
{
  if (t1)
    return said_branch_node(SAID_NEXT_NODE,
			    t1,
			    t2
			    );
  else
    return t2;
}

static tree_t
said_value(int val, tree_t t)
{
  return said_branch_node(SAID_NEXT_NODE,
			  said_leaf_node(SAID_NEXT_NODE, val),
			  t
			  );
			  
}

static tree_t
said_terminal(int val)
{
  return said_leaf_node(SAID_NEXT_NODE, val);
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

  if (!attacheant)
    return base;
  if (!base)
    return attacheant;

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

  } while ((nextitem != SAID_TERM) && (said_tokens_nr < MAX_SAID_TOKENS));

  if (nextitem == SAID_TERM)
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
said(state_t *s, byte *spec, int verbose)
{
  parse_tree_node_t *parse_tree = s->parser_nodes;

  if (s->parser_valid) {

    if (said_parse_spec(s, spec)) {
      sciprintf("Offending spec was: ");
      vocab_decypher_said_block(s, spec - s->heap);
      return 0;
    }

    if (verbose)
      vocab_dump_parse_tree("Said-tree", said_tree); /* Nothing better to do yet */
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
