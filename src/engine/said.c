
/*  A Bison parser, made from said.y
 by  GNU Bison version 1.25
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	WGROUP	258
#define	YY_COMMA	259
#define	YY_AMP	260
#define	YY_SLASH	261
#define	YY_PARENO	262
#define	YY_PARENC	263
#define	YY_BRACKETSO	264
#define	YY_BRACKETSC	265
#define	YY_HASH	266
#define	YY_LT	267
#define	YY_GT	268
#define	YY_BRACKETSO_LT	269
#define	YY_BRACKETSO_SLASH	270

#line 28 "said.y"


#include <engine.h>

#define SAID_BRANCH_NULL 0

#define MAX_SAID_TOKENS 128

/* Maximum number of words to be expected in a parsed sentence */
#define AUGMENT_MAX_WORDS 64


#define WORD_TYPE_BASE 0x141
#define WORD_TYPE_REF 0x144
#define WORD_TYPE_SYNTACTIC_SUGAR 0x145

#define AUGMENT_SENTENCE_PART_BRACKETS 0x152

  /* Minor numbers */
#define AUGMENT_SENTENCE_MINOR_MATCH_PHRASE 0x14c
#define AUGMENT_SENTENCE_MINOR_MATCH_WORD 0x153
#define AUGMENT_SENTENCE_MINOR_RECURSE 0x144


  /*#define SCI_DEBUG_PARSE_TREE_AUGMENTATION *//* uncomment to debug parse tree augmentation */


#ifdef SCI_DEBUG_PARSE_TREE_AUGMENTATION
#define scidprintf sciprintf
#else
#define scidprintf if (0) sciprintf
#endif

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

#ifndef YYSTYPE
#define YYSTYPE int
#endif
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		51
#define	YYFLAG		-32768
#define	YYNTBASE	16

#define YYTRANSLATE(x) ((unsigned)(x) <= 270 ? yytranslate[x] : 28)

static const char yytranslate[] = {     0,
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
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     3,     7,    12,    13,    15,    16,    18,    21,    26,
    28,    31,    36,    38,    40,    42,    46,    48,    52,    56,
    62,    65,    67,    69,    71,    75,    79,    82,    86
};

static const short yyrhs[] = {    18,
    17,     0,    18,    19,    17,     0,    18,    19,    20,    17,
     0,     0,    13,     0,     0,    24,     0,     6,    24,     0,
    15,     6,    24,    10,     0,     6,     0,     6,    24,     0,
    15,     6,    24,    10,     0,     6,     0,     3,     0,    23,
     0,     9,    23,    10,     0,    21,     0,     7,    24,     8,
     0,    23,     4,    23,     0,    23,     4,     9,    23,    10,
     0,    22,    25,     0,    22,     0,    25,     0,    26,     0,
    14,    26,    10,     0,    12,    21,    27,     0,    12,    23,
     0,    12,    23,    27,     0,    12,    23,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   133,   135,   137,   142,   144,   150,   152,   158,   160,   162,
   168,   170,   172,   178,   183,   185,   190,   192,   194,   196,
   202,   204,   206,   212,   214,   220,   222,   228,   230
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","WGROUP",
"YY_COMMA","YY_AMP","YY_SLASH","YY_PARENO","YY_PARENC","YY_BRACKETSO","YY_BRACKETSC",
"YY_HASH","YY_LT","YY_GT","YY_BRACKETSO_LT","YY_BRACKETSO_SLASH","saidspec",
"optcont","leftspec","midspec","rightspec","word","cwordset","wordset","expr",
"cwordrefset","wordrefset","recref", NULL
};
#endif

static const short yyr1[] = {     0,
    16,    16,    16,    17,    17,    18,    18,    19,    19,    19,
    20,    20,    20,    21,    22,    22,    23,    23,    23,    23,
    24,    24,    24,    25,    25,    26,    26,    27,    27
};

static const short yyr2[] = {     0,
     2,     3,     4,     0,     1,     0,     1,     2,     4,     1,
     2,     4,     1,     1,     1,     3,     1,     3,     3,     5,
     2,     1,     1,     1,     3,     3,     2,     3,     2
};

static const short yydefact[] = {     6,
    14,     0,     0,     0,     0,     4,    17,    22,    15,     7,
    23,    24,     0,     0,    17,    27,     0,    10,     5,     0,
     1,     4,    21,     0,    18,    16,     0,    26,    25,     8,
     0,    13,     0,     2,     4,     0,    19,    29,     0,    11,
     0,     3,     0,    28,     9,     0,    20,    12,     0,     0,
     0
};

static const short yydefgoto[] = {    49,
    21,     6,    22,    35,     7,     8,     9,    10,    11,    12,
    28
};

static const short yypact[] = {     1,
-32768,     1,     2,     2,    -6,     5,-32768,    22,     3,-32768,
-32768,-32768,    16,    18,    25,     3,    30,     1,-32768,    35,
-32768,     6,-32768,    24,-32768,-32768,     2,-32768,-32768,-32768,
     1,     1,    36,-32768,    31,     2,     3,    13,    33,-32768,
     1,-32768,    28,-32768,-32768,    37,-32768,-32768,    45,    46,
-32768
};

static const short yypgoto[] = {-32768,
   -21,-32768,-32768,-32768,    44,-32768,    -1,    -2,    41,    47,
    12
};


#define	YYLAST		52


static const short yytable[] = {    13,
    34,    14,    16,     1,     1,     4,    24,     2,     2,     3,
    18,    32,     4,    42,     5,    30,    24,    19,    19,    20,
    33,    24,    37,    25,    27,    38,     1,    26,    39,    40,
     2,    24,    36,     4,    43,     5,    27,    47,    46,    29,
    31,    41,    45,    19,    50,    51,    48,    15,    23,    44,
     0,    17
};

static const short yycheck[] = {     2,
    22,     3,     4,     3,     3,    12,     4,     7,     7,     9,
     6,     6,    12,    35,    14,    18,     4,    13,    13,    15,
    15,     4,    24,     8,    12,    27,     3,    10,    31,    32,
     7,     4,     9,    12,    36,    14,    12,    10,    41,    10,
     6,     6,    10,    13,     0,     0,    10,     4,     8,    38,
    -1,     5
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

#ifndef YYPARSE_RETURN_TYPE
#define YYPARSE_RETURN_TYPE int
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
YYPARSE_RETURN_TYPE yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 196 "/usr/share/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

YYPARSE_RETURN_TYPE
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 134 "said.y"
{ yyval = said_top_branch(said_attach_branch(yyvsp[-1], yyvsp[0])) ;
    break;}
case 2:
#line 136 "said.y"
{ yyval = said_top_branch(said_attach_branch(yyvsp[-2], said_attach_branch(yyvsp[-1], yyvsp[0]))) ;
    break;}
case 3:
#line 138 "said.y"
{ yyval = said_top_branch(said_attach_branch(yyvsp[-3], said_attach_branch(yyvsp[-2], said_attach_branch(yyvsp[-1], yyvsp[0])))) ;
    break;}
case 4:
#line 143 "said.y"
{ yyval = SAID_BRANCH_NULL ;
    break;}
case 5:
#line 145 "said.y"
{ yyval = said_paren(said_value(0x14b, said_value(0xf900, said_terminal(0xf900))), SAID_BRANCH_NULL) ;
    break;}
case 6:
#line 151 "said.y"
{ yyval = SAID_BRANCH_NULL ;
    break;}
case 7:
#line 153 "said.y"
{ yyval = said_paren(said_value(0x141, said_value(0x149, yyvsp[0])), SAID_BRANCH_NULL) ;
    break;}
case 8:
#line 159 "said.y"
{ yyval = said_aug_branch(0x142, 0x14a, yyvsp[0], SAID_BRANCH_NULL) ;
    break;}
case 9:
#line 161 "said.y"
{ yyval = said_aug_branch(0x152, 0x142, said_aug_branch(0x142, 0x14a, yyvsp[-1], SAID_BRANCH_NULL), SAID_BRANCH_NULL) ;
    break;}
case 10:
#line 163 "said.y"
{ yyval = SAID_BRANCH_NULL ;
    break;}
case 11:
#line 169 "said.y"
{ yyval = said_aug_branch(0x143, 0x14a, yyvsp[0], SAID_BRANCH_NULL) ;
    break;}
case 12:
#line 171 "said.y"
{ yyval = said_aug_branch(0x152, 0x143, said_aug_branch(0x143, 0x14a, yyvsp[-1], SAID_BRANCH_NULL), SAID_BRANCH_NULL) ;
    break;}
case 13:
#line 173 "said.y"
{ yyval = SAID_BRANCH_NULL ;
    break;}
case 14:
#line 179 "said.y"
{ yyval = said_paren(said_value(0x141, said_value(0x153, said_terminal(yyvsp[0]))), SAID_BRANCH_NULL) ;
    break;}
case 15:
#line 184 "said.y"
{ yyval = said_aug_branch(0x141, 0x14f, yyvsp[0], SAID_BRANCH_NULL) ;
    break;}
case 16:
#line 186 "said.y"
{ yyval = said_aug_branch(0x141, 0x14f, said_aug_branch(0x152, 0x14c, said_aug_branch(0x141, 0x14f, yyvsp[-1], SAID_BRANCH_NULL), SAID_BRANCH_NULL), SAID_BRANCH_NULL) ;
    break;}
case 17:
#line 191 "said.y"
{ yyval = yyvsp[0] ;
    break;}
case 18:
#line 193 "said.y"
{ yyval = said_aug_branch(0x141, 0x14c, yyvsp[-1], SAID_BRANCH_NULL) ;
    break;}
case 19:
#line 195 "said.y"
{ yyval = said_attach_branch(yyvsp[-2], yyvsp[0]) ;
    break;}
case 20:
#line 197 "said.y"
{ yyval = said_attach_branch(yyvsp[-4], yyvsp[-2]) ;
    break;}
case 21:
#line 203 "said.y"
{ yyval = said_attach_branch(yyvsp[-1], yyvsp[0]) ;
    break;}
case 22:
#line 205 "said.y"
{ yyval = yyvsp[0] ;
    break;}
case 23:
#line 207 "said.y"
{ yyval = yyvsp[0] ;
    break;}
case 24:
#line 213 "said.y"
{ yyval = yyvsp[0] ;
    break;}
case 25:
#line 215 "said.y"
{ yyval = said_aug_branch(0x152, 0x144, yyvsp[-1], SAID_BRANCH_NULL) ;
    break;}
case 26:
#line 221 "said.y"
{ yyval = said_aug_branch(0x144, 0x14f, yyvsp[-1], yyvsp[0]) ;
    break;}
case 27:
#line 223 "said.y"
{ yyval = said_aug_branch(0x144, 0x14f, yyvsp[0], SAID_BRANCH_NULL) ;
    break;}
case 28:
#line 229 "said.y"
{ yyval = said_aug_branch(0x141, 0x144, said_aug_branch(0x144, 0x14f, yyvsp[-1], SAID_BRANCH_NULL), yyvsp[0]) ;
    break;}
case 29:
#line 231 "said.y"
{ yyval = said_aug_branch(0x141, 0x144, said_aug_branch(0x144, 0x14f, yyvsp[0], SAID_BRANCH_NULL), SAID_BRANCH_NULL) ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 498 "/usr/share/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 236 "said.y"



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
			   int maxwords)
     /* Finds and lists all base (141) and reference (144) words */
{
  int major, minor;
  int word;
  int pos = aug_get_first_child(tree, startpos, &major, &minor);

  while (pos) {
    if ((word = aug_get_wgroup(tree, pos))) { /* found a word */

      if (major == WORD_TYPE_BASE) {	
	if ((*base_words_nr) == maxwords) {
	  sciprintf("Out of regular words\n");
	  return; /* return gracefully */
	}

	base_words[*base_words_nr] = word; /* register word */
	++(*base_words_nr);

      } else if (major == WORD_TYPE_REF) {
	if ((*ref_words_nr) == maxwords) {
	  sciprintf("Out of reference words\n");
	  return; /* return gracefully */
	}

	ref_words[*ref_words_nr] = word; /* register word */
	++(*ref_words_nr);

      } else if (major != WORD_TYPE_SYNTACTIC_SUGAR)
	sciprintf("aug_find_words_recursively(): Unknown word type %03\n", major);
    
    } else /* Did NOT find a word group: Attempt to recurse */
      aug_find_words_recursively(tree, pos, base_words, base_words_nr,
				 ref_words, ref_words_nr, maxwords);

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

  aug_find_words_recursively(tree, startpos, base_words, base_words_nr, ref_words, ref_words_nr, maxwords);
}


static inline int
aug_contains_word(int *list, int length, int word)
{
  int i;
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
      } else sciprintf("augment_match_expression_p(): Unknown type 141 minor number %3x\n", cminor);

      cpos = aug_get_next_sibling(saidt, cpos, &cmajor, &cminor);
    }
    break;

  case WORD_TYPE_REF:
    while (cpos) {
      if (cminor == AUGMENT_SENTENCE_MINOR_MATCH_WORD) {
	int word = aug_get_wgroup(saidt, cpos);
	scidprintf("Looking for word %03x\n", word);

	if (aug_contains_word(ref_words, ref_words_nr, word))
	  return 1;
      } else if (cminor == AUGMENT_SENTENCE_MINOR_MATCH_PHRASE) {
	if (augment_match_expression_p(saidt, cpos,
				       parset, parse_basepos,
				       cmajor, cminor,
				       base_words, base_words_nr,
				       ref_words, ref_words_nr))
	  return 1;
      } else sciprintf("augment_match_expression_p(): Unknown type 144 minor number %3x\n", cminor);

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
/**** Baic code ****/
/*******************/

int
said(state_t *s, byte *spec, int verbose)
{
  int retval;

  parse_tree_node_t *parse_tree_ptr = s->parser_nodes;

  if (s->parser_valid) {

    if (said_parse_spec(s, spec)) {
      sciprintf("Offending spec was: ");
      vocab_decypher_said_block(s, spec - s->heap);
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
  byte block[] = {0x01, 0x00, 0xf0, 0xf3, 0x01, 0x01, 0xf8, 0x01, 0x02, 0xf4, 0xf2, 0x01, 0x03, 0xff};
  state_t s;
  con_passthrough = 1;

  s.parser_valid = 1;
  said(&s, block);
}
#endif
