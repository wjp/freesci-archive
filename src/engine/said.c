/* A Bison parser, made by GNU Bison 1.875a.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     WGROUP = 258,
     YY_COMMA = 259,
     YY_AMP = 260,
     YY_SLASH = 261,
     YY_PARENO = 262,
     YY_PARENC = 263,
     YY_BRACKETSO = 264,
     YY_BRACKETSC = 265,
     YY_HASH = 266,
     YY_LT = 267,
     YY_GT = 268,
     YY_BRACKETSO_LT = 269,
     YY_BRACKETSO_SLASH = 270,
     YY_LT_BRACKETSO = 271,
     YY_LT_PARENO = 272
   };
#endif
#define WGROUP 258
#define YY_COMMA 259
#define YY_AMP 260
#define YY_SLASH 261
#define YY_PARENO 262
#define YY_PARENC 263
#define YY_BRACKETSO 264
#define YY_BRACKETSC 265
#define YY_HASH 266
#define YY_LT 267
#define YY_GT 268
#define YY_BRACKETSO_LT 269
#define YY_BRACKETSO_SLASH 270
#define YY_LT_BRACKETSO 271
#define YY_LT_PARENO 272




/* Copy the first part of user declarations.  */
#line 28 "said.y"


#include <engine.h>

#define SAID_BRANCH_NULL 0

#define MAX_SAID_TOKENS 128

/* Maximum number of words to be expected in a parsed sentence */
#define AUGMENT_MAX_WORDS 64


#define ANYWORD 0xfff

#define WORD_TYPE_BASE 0x141
#define WORD_TYPE_REF 0x144
#define WORD_TYPE_SYNTACTIC_SUGAR 0x145

#define AUGMENT_SENTENCE_PART_BRACKETS 0x152

/* Minor numbers */
#define AUGMENT_SENTENCE_MINOR_MATCH_PHRASE 0x14c
#define AUGMENT_SENTENCE_MINOR_MATCH_WORD 0x153
#define AUGMENT_SENTENCE_MINOR_RECURSE 0x144
#define AUGMENT_SENTENCE_MINOR_PARENTHESES 0x14f


#undef YYDEBUG /*1*/
/*#define SAID_DEBUG*/
/*#define SCI_DEBUG_PARSE_TREE_AUGMENTATION*/ /* uncomment to debug parse tree augmentation*/


#ifdef SCI_DEBUG_PARSE_TREE_AUGMENTATION
#define scidprintf sciprintf
#else
#define scidprintf if (0) sciprintf
#endif


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
	said_parse_error = sci_strdup(s);
	return 1; /* Abort */
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 213 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  23
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   69

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  18
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  13
/* YYNRULES -- Number of rules. */
#define YYNRULES  33
/* YYNRULES -- Number of states. */
#define YYNSTATES  62

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   272

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     6,    10,    15,    16,    18,    19,    21,
      24,    29,    31,    34,    39,    41,    43,    45,    49,    51,
      55,    59,    65,    68,    70,    72,    74,    78,    83,    87,
      92,    95,   100,   104
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      19,     0,    -1,    21,    20,    -1,    21,    22,    20,    -1,
      21,    22,    23,    20,    -1,    -1,    13,    -1,    -1,    27,
      -1,     6,    27,    -1,    15,     6,    27,    10,    -1,     6,
      -1,     6,    27,    -1,    15,     6,    27,    10,    -1,     6,
      -1,     3,    -1,    26,    -1,     9,    26,    10,    -1,    24,
      -1,     7,    27,     8,    -1,    26,     4,    26,    -1,    26,
       4,     9,    26,    10,    -1,    25,    28,    -1,    25,    -1,
      28,    -1,    29,    -1,    14,    29,    10,    -1,    29,    14,
      29,    10,    -1,    12,    24,    30,    -1,    17,     7,    26,
       8,    -1,    12,    26,    -1,    16,     9,    26,    10,    -1,
      12,    26,    30,    -1,    12,    26,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned char yyrline[] =
{
       0,   138,   138,   140,   142,   148,   149,   156,   157,   163,
     165,   167,   173,   175,   177,   183,   188,   190,   195,   197,
     199,   201,   207,   209,   211,   217,   219,   221,   227,   229,
     231,   233,   239,   241
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "WGROUP", "YY_COMMA", "YY_AMP", "YY_SLASH", 
  "YY_PARENO", "YY_PARENC", "YY_BRACKETSO", "YY_BRACKETSC", "YY_HASH", 
  "YY_LT", "YY_GT", "YY_BRACKETSO_LT", "YY_BRACKETSO_SLASH", 
  "YY_LT_BRACKETSO", "YY_LT_PARENO", "$accept", "saidspec", "optcont", 
  "leftspec", "midspec", "rightspec", "word", "cwordset", "wordset", 
  "expr", "cwordrefset", "wordrefset", "recref", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    18,    19,    19,    19,    20,    20,    21,    21,    22,
      22,    22,    23,    23,    23,    24,    25,    25,    26,    26,
      26,    26,    27,    27,    27,    28,    28,    28,    29,    29,
      29,    29,    30,    30
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     2,     3,     4,     0,     1,     0,     1,     2,
       4,     1,     2,     4,     1,     1,     1,     3,     1,     3,
       3,     5,     2,     1,     1,     1,     3,     4,     3,     4,
       2,     4,     3,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       7,    15,     0,     0,     0,     0,     0,     0,     0,     5,
      18,    23,    16,     8,    24,    25,     0,     0,    18,    30,
       0,     0,     0,     1,    11,     6,     0,     2,     5,    22,
       0,     0,    19,    17,     0,    28,    26,     0,     0,     9,
       0,    14,     0,     3,     5,     0,    20,     0,    33,    31,
      29,     0,    12,     0,     4,     0,    27,    32,    10,     0,
      21,    13
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     8,    27,     9,    28,    44,    10,    11,    12,    13,
      14,    15,    35
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -22
static const yysigned_char yypact[] =
{
       1,   -22,     1,     9,     9,    37,     0,     4,     5,    13,
     -22,    31,    46,   -22,   -22,    27,    47,    20,    45,    46,
      49,     9,     9,   -22,     1,   -22,    52,   -22,    19,   -22,
      28,    37,   -22,   -22,     9,   -22,   -22,    32,    48,   -22,
       1,     1,    54,   -22,    50,     9,    46,    51,     2,   -22,
     -22,    55,   -22,     1,   -22,    36,   -22,   -22,   -22,    56,
     -22,   -22
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -22,   -22,   -21,   -22,   -22,   -22,    58,   -22,    -1,    -2,
      53,    -4,    21
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      16,    20,    17,    19,     1,    23,    30,    43,     2,    21,
       3,    22,     1,     4,    34,     5,     2,     6,     7,    24,
      37,    38,    39,    54,    30,    41,    25,    47,    26,    46,
      33,     1,    25,    48,    42,     2,    30,    45,    51,    52,
      30,    31,    49,     4,    55,     5,    60,     6,     7,     4,
      30,    59,    30,     6,     7,    32,    50,    34,    40,    36,
      53,    56,    18,    25,    29,    58,    61,     0,     0,    57
};

static const yysigned_char yycheck[] =
{
       2,     5,     3,     4,     3,     0,     4,    28,     7,     9,
       9,     7,     3,    12,    12,    14,     7,    16,    17,     6,
      21,    22,    24,    44,     4,     6,    13,    31,    15,    30,
      10,     3,    13,    34,    15,     7,     4,     9,    40,    41,
       4,    14,    10,    12,    45,    14,    10,    16,    17,    12,
       4,    53,     4,    16,    17,     8,     8,    12,     6,    10,
       6,    10,     4,    13,    11,    10,    10,    -1,    -1,    48
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     7,     9,    12,    14,    16,    17,    19,    21,
      24,    25,    26,    27,    28,    29,    27,    26,    24,    26,
      29,     9,     7,     0,     6,    13,    15,    20,    22,    28,
       4,    14,     8,    10,    12,    30,    10,    26,    26,    27,
       6,     6,    15,    20,    23,     9,    26,    29,    26,    10,
       8,    27,    27,     6,    20,    26,    10,    30,    10,    27,
      10,    10
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 139 "said.y"
    { yyval = said_top_branch(said_attach_branch(yyvsp[-1], yyvsp[0])); }
    break;

  case 3:
#line 141 "said.y"
    { yyval = said_top_branch(said_attach_branch(yyvsp[-2], said_attach_branch(yyvsp[-1], yyvsp[0]))); }
    break;

  case 4:
#line 143 "said.y"
    { yyval = said_top_branch(said_attach_branch(yyvsp[-3], said_attach_branch(yyvsp[-2], said_attach_branch(yyvsp[-1], yyvsp[0])))); }
    break;

  case 5:
#line 148 "said.y"
    { yyval = SAID_BRANCH_NULL; }
    break;

  case 6:
#line 150 "said.y"
    { yyval = said_paren(said_value(0x14b, said_value(0xf900, said_terminal(0xf900))), SAID_BRANCH_NULL); }
    break;

  case 7:
#line 156 "said.y"
    { yyval = SAID_BRANCH_NULL; }
    break;

  case 8:
#line 158 "said.y"
    { yyval = said_paren(said_value(0x141, said_value(0x149, yyvsp[0])), SAID_BRANCH_NULL); }
    break;

  case 9:
#line 164 "said.y"
    { yyval = said_aug_branch(0x142, 0x14a, yyvsp[0], SAID_BRANCH_NULL); }
    break;

  case 10:
#line 166 "said.y"
    { yyval = said_aug_branch(0x152, 0x142, said_aug_branch(0x142, 0x14a, yyvsp[-1], SAID_BRANCH_NULL), SAID_BRANCH_NULL); }
    break;

  case 11:
#line 168 "said.y"
    { yyval = SAID_BRANCH_NULL; }
    break;

  case 12:
#line 174 "said.y"
    { yyval = said_aug_branch(0x143, 0x14a, yyvsp[0], SAID_BRANCH_NULL); }
    break;

  case 13:
#line 176 "said.y"
    { yyval = said_aug_branch(0x152, 0x143, said_aug_branch(0x143, 0x14a, yyvsp[-1], SAID_BRANCH_NULL), SAID_BRANCH_NULL); }
    break;

  case 14:
#line 178 "said.y"
    { yyval = SAID_BRANCH_NULL; }
    break;

  case 15:
#line 184 "said.y"
    { yyval = said_paren(said_value(0x141, said_value(0x153, said_terminal(yyvsp[0]))), SAID_BRANCH_NULL); }
    break;

  case 16:
#line 189 "said.y"
    { yyval = said_aug_branch(0x141, 0x14f, yyvsp[0], SAID_BRANCH_NULL); }
    break;

  case 17:
#line 191 "said.y"
    { yyval = said_aug_branch(0x141, 0x14f, said_aug_branch(0x152, 0x14c, said_aug_branch(0x141, 0x14f, yyvsp[-1], SAID_BRANCH_NULL), SAID_BRANCH_NULL), SAID_BRANCH_NULL); }
    break;

  case 18:
#line 196 "said.y"
    { yyval = yyvsp[0]; }
    break;

  case 19:
#line 198 "said.y"
    { yyval = said_aug_branch(0x141, 0x14c, yyvsp[-1], SAID_BRANCH_NULL); }
    break;

  case 20:
#line 200 "said.y"
    { yyval = said_attach_branch(yyvsp[-2], yyvsp[0]); }
    break;

  case 21:
#line 202 "said.y"
    { yyval = said_attach_branch(yyvsp[-4], yyvsp[-2]); }
    break;

  case 22:
#line 208 "said.y"
    { yyval = said_attach_branch(yyvsp[-1], yyvsp[0]); }
    break;

  case 23:
#line 210 "said.y"
    { yyval = yyvsp[0]; }
    break;

  case 24:
#line 212 "said.y"
    { yyval = yyvsp[0]; }
    break;

  case 25:
#line 218 "said.y"
    { yyval = yyvsp[0]; }
    break;

  case 26:
#line 220 "said.y"
    { yyval = said_aug_branch(0x152, 0x144, yyvsp[-1], SAID_BRANCH_NULL); }
    break;

  case 27:
#line 222 "said.y"
    { yyval = said_attach_branch(yyvsp[-3], said_aug_branch(0x152, 0x144, yyvsp[-1], SAID_BRANCH_NULL)); }
    break;

  case 28:
#line 228 "said.y"
    { yyval = said_aug_branch(0x144, 0x14f, yyvsp[-1], yyvsp[0]); }
    break;

  case 29:
#line 230 "said.y"
    { yyval = said_aug_branch(0x144, 0x14c, yyvsp[-1], SAID_BRANCH_NULL); }
    break;

  case 30:
#line 232 "said.y"
    { yyval = said_aug_branch(0x144, 0x14f, yyvsp[0], SAID_BRANCH_NULL); }
    break;

  case 31:
#line 234 "said.y"
    { yyval = said_aug_branch(0x152, 0x144, said_aug_branch(0x144, 0x14f, yyvsp[-1], SAID_BRANCH_NULL), SAID_BRANCH_NULL); }
    break;

  case 32:
#line 240 "said.y"
    { yyval = said_aug_branch(0x141, 0x144, said_aug_branch(0x144, 0x14f, yyvsp[-1], SAID_BRANCH_NULL), yyvsp[0]); }
    break;

  case 33:
#line 242 "said.y"
    { yyval = said_aug_branch(0x141, 0x144, said_aug_branch(0x144, 0x14f, yyvsp[0], SAID_BRANCH_NULL), SAID_BRANCH_NULL); }
    break;


    }

/* Line 999 of yacc.c.  */
#line 1295 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 247 "said.y"



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
			if (retval == YY_BRACKETSO) {
				if ((said_tokens[said_token] >> 8) == SAID_LT)
					retval = YY_BRACKETSO_LT;
				else
					if ((said_tokens[said_token] >> 8) == SAID_SLASH)
						retval = YY_BRACKETSO_SLASH;
			} else if (retval == YY_LT && (said_tokens[said_token] >> 8) == SAID_BRACKO) {
				retval = YY_LT_BRACKETSO;
			} else if (retval == YY_LT && (said_tokens[said_token] >> 8) == SAID_PARENO) {
				retval = YY_LT_PARENO;
			}
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

/**********************/
/**** Augmentation ****/
/**********************/


/** primitive functions **/

#define AUG_READ_BRANCH(a, br, p) \
  if (tree[p].type != PARSE_TREE_NODE_BRANCH) \
    return 0; \
  a = tree[p].content.branches[br];

#define AUG_READ_VALUE(a, p) \
  if (tree[p].type != PARSE_TREE_NODE_LEAF) \
    return 0; \
  a = tree[p].content.value;

#define AUG_ASSERT(i) \
  if (!i) return 0;

static int
aug_get_next_sibling(parse_tree_node_t *tree, int pos, int *first, int *second)
     /* Returns the next sibling relative to the specified position in 'tree',
     ** sets *first and *second to its augment node values, returns the new position
     ** or 0 if there was no next sibling
     */
{
	int seek, valpos;

	AUG_READ_BRANCH(pos, 1, pos);
	AUG_ASSERT(pos);
	AUG_READ_BRANCH(seek, 0, pos);
	AUG_ASSERT(seek);

	/* Now retreive first value */
	AUG_READ_BRANCH(valpos, 0, seek);
	AUG_ASSERT(valpos);
	AUG_READ_VALUE(*first, valpos);

	/* Get second value */
	AUG_READ_BRANCH(seek, 1, seek);
	AUG_ASSERT(seek);
	AUG_READ_BRANCH(valpos, 0, seek);
	AUG_ASSERT(valpos);
	AUG_READ_VALUE(*second, valpos);

	return pos;
}


static int
aug_get_wgroup(parse_tree_node_t *tree, int pos)
     /* Returns 0 if pos in tree is not the root of a 3-element list, otherwise
     ** it returns the last element (which, in practice, is the word group
     */
{
	int val;

	AUG_READ_BRANCH(pos, 0, pos);
	AUG_ASSERT(pos);
	AUG_READ_BRANCH(pos, 1, pos);
	AUG_ASSERT(pos);
	AUG_READ_BRANCH(pos, 1, pos);
	AUG_ASSERT(pos);
	AUG_READ_VALUE(val, pos);

	return val;
}


static int
aug_get_base_node(parse_tree_node_t *tree)
{
	int startpos = 0;
	AUG_READ_BRANCH(startpos, 1, startpos);
	return startpos;
}


/** semi-primitive functions **/


static int
aug_get_first_child(parse_tree_node_t *tree, int pos, int *first, int *second)
     /* like aug_get_next_sibling, except that it recurses into the tree and
     ** finds the first child (usually *not* Ayanami Rei) of the current branch
     ** rather than its next sibling.
     */
{
	AUG_READ_BRANCH(pos, 0, pos);
	AUG_ASSERT(pos);
	AUG_READ_BRANCH(pos, 1, pos);
	AUG_ASSERT(pos);

	return aug_get_next_sibling(tree, pos, first, second);
}

static void
aug_find_words_recursively(parse_tree_node_t *tree, int startpos,
			   int *base_words, int *base_words_nr,
			   int *ref_words, int *ref_words_nr,
			   int maxwords, int refbranch)
     /* Finds and lists all base (141) and reference (144) words */
{
	int major, minor;
	int word;
	int pos = aug_get_first_child(tree, startpos, &major, &minor);

	/*	if (major == WORD_TYPE_REF)
		refbranch = 1;*/

	while (pos) {
		if ((word = aug_get_wgroup(tree, pos))) { /* found a word */

			if (!refbranch && major == WORD_TYPE_BASE) {	
				if ((*base_words_nr) == maxwords) {
					sciprintf("Out of regular words\n");
					return; /* return gracefully */
				}

				base_words[*base_words_nr] = word; /* register word */
				++(*base_words_nr);

			}
			if (major == WORD_TYPE_REF || refbranch) {
				if ((*ref_words_nr) == maxwords) {
					sciprintf("Out of reference words\n");
					return; /* return gracefully */
				}

				ref_words[*ref_words_nr] = word; /* register word */
				++(*ref_words_nr);

			}
			if (major != WORD_TYPE_SYNTACTIC_SUGAR && major != WORD_TYPE_BASE && major != WORD_TYPE_REF)
				sciprintf("aug_find_words_recursively(): Unknown word type %03x\n", major);
    
		} else /* Did NOT find a word group: Attempt to recurse */
			aug_find_words_recursively(tree, pos, base_words, base_words_nr,
						   ref_words, ref_words_nr, maxwords, refbranch || major == WORD_TYPE_REF);

		pos = aug_get_next_sibling(tree, pos, &major, &minor);
	}
}


static void
aug_find_words(parse_tree_node_t *tree, int startpos,
	       int *base_words, int *base_words_nr,
	       int *ref_words, int *ref_words_nr,
	       int maxwords)
     /* initializing wrapper for aug_find_words_recursively() */
{
	*base_words_nr = 0;
	*ref_words_nr = 0;

	aug_find_words_recursively(tree, startpos, base_words, base_words_nr, ref_words, ref_words_nr, maxwords, 0);
}


static inline int
aug_contains_word(int *list, int length, int word)
{
	int i;
	if (word == ANYWORD)
		return (length);

	for (i = 0; i < length; i++)
		if (list[i] == word)
			return 1;

	return 0;
}


static int
augment_sentence_expression(parse_tree_node_t *saidt, int augment_pos,
			    parse_tree_node_t *parset, int parse_branch,
			    int major, int minor,
			    int *base_words, int base_words_nr,
			    int *ref_words, int ref_words_nr);

static int
augment_match_expression_p(parse_tree_node_t *saidt, int augment_pos,
			   parse_tree_node_t *parset, int parse_basepos,
			   int major, int minor,
			   int *base_words, int base_words_nr,
			   int *ref_words, int ref_words_nr)
{
	int cmajor, cminor, cpos;
	cpos = aug_get_first_child(saidt, augment_pos, &cmajor, &cminor);
	if (!cpos) {
		sciprintf("augment_match_expression_p(): Empty condition\n");
		return 1;
	}

	scidprintf("Attempting to match (%03x %03x (%03x %03x\n", major, minor, cmajor, cminor);

	if ((major == WORD_TYPE_BASE) && (minor == AUGMENT_SENTENCE_MINOR_RECURSE))
		return augment_match_expression_p(saidt, cpos,
						  parset, parse_basepos,
						  cmajor, cminor,
						  base_words, base_words_nr,
						  ref_words, ref_words_nr);


	switch (major) {

	case WORD_TYPE_BASE:
		while (cpos) {
			if (cminor == AUGMENT_SENTENCE_MINOR_MATCH_WORD) {
				int word = aug_get_wgroup(saidt, cpos);
				scidprintf("Looking for word %03x\n", word);

				if (aug_contains_word(base_words, base_words_nr, word))
					return 1;
			} else if (cminor == AUGMENT_SENTENCE_MINOR_MATCH_PHRASE) {
				if (augment_sentence_expression(saidt, cpos,
								parset, parse_basepos,
								cmajor, cminor,
								base_words, base_words_nr,
								ref_words, ref_words_nr))
					return 1;
			} else if (cminor == AUGMENT_SENTENCE_MINOR_PARENTHESES) {
				int gc_major, gc_minor;
				int gchild = aug_get_first_child(saidt, cpos, &gc_major, &gc_minor);

				while (gchild) {
					if (augment_match_expression_p(saidt, cpos,
								       parset, parse_basepos,
								       major, minor,
								       base_words, base_words_nr,
								       ref_words, ref_words_nr))
						return 1;
					gchild = aug_get_next_sibling(saidt, gchild, &gc_major, &gc_minor);
				}
			} else
				sciprintf("augment_match_expression_p(): Unknown type 141 minor number %3x\n", cminor);

			cpos = aug_get_next_sibling(saidt, cpos, &cmajor, &cminor);

		}
		break;

	case WORD_TYPE_REF:
		while (cpos) {
			if (cminor == AUGMENT_SENTENCE_MINOR_MATCH_WORD) {
				int word = aug_get_wgroup(saidt, cpos);
				scidprintf("Looking for refword %03x\n", word);

				if (aug_contains_word(ref_words, ref_words_nr, word))
					return 1;
			} else if (cminor == AUGMENT_SENTENCE_MINOR_MATCH_PHRASE) {
				if (augment_match_expression_p(saidt, cpos,
							       parset, parse_basepos,
							       cmajor, cminor,
							       base_words, base_words_nr,
							       ref_words, ref_words_nr))
					return 1;
			} else if (cminor == AUGMENT_SENTENCE_MINOR_PARENTHESES) {
				int gc_major, gc_minor;
				int gchild = aug_get_first_child(saidt, cpos, &gc_major, &gc_minor);

				while (gchild) {
					if (augment_match_expression_p(saidt, cpos,
								       parset, parse_basepos,
								       major, minor,
								       base_words, base_words_nr,
								       ref_words, ref_words_nr))
						return 1;
					gchild = aug_get_next_sibling(saidt, gchild, &gc_major, &gc_minor);
				}
			} else
				sciprintf("augment_match_expression_p(): Unknown type 144 minor number %3x\n", cminor);

			cpos = aug_get_next_sibling(saidt, cpos, &cmajor, &cminor);

		}
		break;

	case AUGMENT_SENTENCE_PART_BRACKETS:
		if (augment_match_expression_p(saidt, cpos,
					       parset, parse_basepos,
					       cmajor, cminor,
					       base_words, base_words_nr,
					       ref_words, ref_words_nr))
			return 1;

		scidprintf("Didn't match subexpression; checking sub-bracked predicate %03x\n", cmajor);

		switch (cmajor) {
		case WORD_TYPE_BASE:
			if (!base_words_nr)
				return 1;
			break;

		case WORD_TYPE_REF:
			if (!ref_words_nr)
				return 1;
			break;

		default:
			sciprintf("augment_match_expression_p(): (subp1) Unkonwn sub-bracket predicate %03x\n", cmajor);
		}

		break;

	default:
		sciprintf("augment_match_expression_p(): Unknown predicate %03x\n", major);

	}

	scidprintf("Generic failure\n");
	return 0;
}

static int
augment_sentence_expression(parse_tree_node_t *saidt, int augment_pos,
			    parse_tree_node_t *parset, int parse_branch,
			    int major, int minor,
			    int *base_words, int base_words_nr,
			    int *ref_words, int ref_words_nr)
{
	int check_major, check_minor;
	int check_pos = aug_get_first_child(saidt, augment_pos, &check_major, &check_minor);
	do {
		if (!(augment_match_expression_p(saidt, check_pos, parset, parse_branch,
						 check_major, check_minor, base_words, base_words_nr,
						 ref_words, ref_words_nr)))
			return 0;
	} while ((check_pos = aug_get_next_sibling(saidt, check_pos, &check_major, &check_minor)));
	return 1;
}



static int
augment_sentence_part(parse_tree_node_t *saidt, int augment_pos,
		      parse_tree_node_t *parset, int parse_basepos,
		      int major, int minor)
{
	int pmajor, pminor;
	int parse_branch = parse_basepos;
	int optional = 0;
	int foundwords = 0;

	scidprintf("Augmenting (%03x %03x\n", major, minor);

	if (major == AUGMENT_SENTENCE_PART_BRACKETS) { /* '[/ foo]' is true if '/foo' or if there
						       ** exists no x for which '/x' is true
						       */
		if ((augment_pos = aug_get_first_child(saidt, augment_pos, &major, &minor))) {
			scidprintf("Optional part: Now augmenting (%03x %03x\n", major, minor);
			optional = 1;
		} else {
			scidprintf("Matched empty optional expression\n");
			return 1;
		}
	}

	if ((major < 0x141)
	    || (major > 0x143)) {
		scidprintf("augment_sentence_part(): Unexpected sentence part major number %03x\n", major);
		return 0;
	}

	while ((parse_branch = aug_get_next_sibling(parset, parse_branch, &pmajor, &pminor)))
		if (pmajor == major) { /* found matching sentence part */
			int success;
			int base_words_nr;
			int ref_words_nr;
			int base_words[AUGMENT_MAX_WORDS];
			int ref_words[AUGMENT_MAX_WORDS];
#ifdef SCI_DEBUG_PARSE_TREE_AUGMENTATION
			int i;
#endif

			scidprintf("Found match with pminor = %03x\n", pminor);
			aug_find_words(parset, parse_branch, base_words, &base_words_nr,
				       ref_words, &ref_words_nr, AUGMENT_MAX_WORDS);
			foundwords |= (ref_words_nr | base_words_nr);
#ifdef SCI_DEBUG_PARSE_TREE_AUGMENTATION
			sciprintf("%d base words:", base_words_nr);
			for (i = 0; i < base_words_nr; i++)
				sciprintf(" %03x", base_words[i]);
			sciprintf("\n%d reference words:", ref_words_nr);
			for (i = 0; i < ref_words_nr; i++)
				sciprintf(" %03x", ref_words[i]);
			sciprintf("\n");
#endif

			success = augment_sentence_expression(saidt, augment_pos,
							      parset, parse_basepos, major, minor,
							      base_words, base_words_nr,
							      ref_words, ref_words_nr);

			if (success) {
				scidprintf("SUCCESS on augmenting (%03x %03x\n", major, minor);
				return 1;
			}
		}

	if (optional && (foundwords == 0)) {
		scidprintf("Found no words and optional branch => SUCCESS on augmenting (%03x %03x\n", major, minor);
		return 1;
	}
	scidprintf("FAILURE on augmenting (%03x %03x\n", major, minor);
	return 0;
}

static int
augment_parse_nodes(parse_tree_node_t *parset, parse_tree_node_t *saidt)
{
	int augment_basepos = 0;
	int parse_basepos;
	int major, minor;
	int dontclaim = 0;

	parse_basepos = aug_get_base_node(parset);
	if (!parse_basepos) {
		sciprintf("augment_parse_nodes(): Parse tree is corrupt\n");
		return 0;
	}
  
	augment_basepos = aug_get_base_node(saidt);
	if (!augment_basepos) {
		sciprintf("augment_parse_nodes(): Said tree is corrupt\n");
		return 0;
	}
	while ((augment_basepos = aug_get_next_sibling(saidt, augment_basepos, &major, &minor))) {

		if ((major == 0x14b)
		    && (minor == SAID_LONG(SAID_GT)))
			dontclaim = 1; /* special case */
		else /* normal sentence part */
			if (!(augment_sentence_part(saidt, augment_basepos, parset, parse_basepos, major, minor))) {
				scidprintf("Returning failure\n");
				return 0; /* fail */
			}
	}

	scidprintf("Returning success with dontclaim=%d\n", dontclaim);

	if (dontclaim)
		return SAID_PARTIAL_MATCH;
	else return 1; /* full match */
}


/*******************/
/**** Main code ****/
/*******************/

int
said(state_t *s, byte *spec, int verbose)
{
	int retval;

	parse_tree_node_t *parse_tree_ptr = s->parser_nodes;

	if (s->parser_valid) {

		if (said_parse_spec(s, spec)) {
			sciprintf("Offending spec was: ");
			vocab_decypher_said_block(s, spec);
			return SAID_NO_MATCH;
		}

		if (verbose)
			vocab_dump_parse_tree("Said-tree", said_tree); /* Nothing better to do yet */
		retval = augment_parse_nodes(parse_tree_ptr, &(said_tree[0]));

		if (!retval)
			return SAID_NO_MATCH;
		else if (retval != SAID_PARTIAL_MATCH)
			return SAID_FULL_MATCH;
		else return SAID_PARTIAL_MATCH;
	}

	return SAID_NO_MATCH;
}



#ifdef SAID_DEBUG_PROGRAM
int
main (int argc, char *argv)
{
	byte block[] = {0x01, 0x00, 0xf8, 0xf5, 0x02, 0x01, 0xf6, 0xf2, 0x02, 0x01, 0xf2, 0x01, 0x03, 0xff};
	state_t s;
	con_passthrough = 1;

	s.parser_valid = 1;
	said(&s, block);
}
#endif


