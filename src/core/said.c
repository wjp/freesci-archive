
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

#line 28 "said.y"


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

#ifndef YYSTYPE
#define YYSTYPE int
#endif
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		42
#define	YYFLAG		-32768
#define	YYNTBASE	14

#define YYTRANSLATE(x) ((unsigned)(x) <= 268 ? yytranslate[x] : 24)

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
     6,     7,     8,     9,    10,    11,    12,    13
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     3,     7,    12,    13,    15,    19,    25,    31,    33,
    35,    39,    43,    47,    49,    50,    53,    56,    58,    62,
    64,    68,    69
};

static const short yyrhs[] = {    16,
    23,     0,    16,    21,    23,     0,    16,    21,    22,    23,
     0,     0,    16,     0,    17,    12,    15,     0,    17,    12,
    15,    12,    15,     0,    17,     9,    12,    15,    10,     0,
    17,     0,    18,     0,    18,     4,    17,     0,     9,    15,
    10,     0,     7,    15,     8,     0,     3,     0,     0,     6,
    15,     0,     6,    15,     0,    20,     0,     9,    19,    10,
     0,    20,     0,     9,    19,    10,     0,     0,    13,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    98,   100,   102,   107,   109,   114,   117,   119,   121,   125,
   127,   131,   133,   135,   139,   141,   145,   149,   151,   155,
   157,   161,   163
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","WGROUP",
"YY_COMMA","YY_AMP","YY_SLASH","YY_PARENO","YY_PARENC","YY_BRACKETSO","YY_BRACKETSC",
"YY_HASH","YY_LT","YY_GT","said_spec","subexpression","ne_subexpression","expression",
"main_expression","before_exp","ne_before_exp","nested_before_first","nested_before_second",
"more_after", NULL
};
#endif

static const short yyr1[] = {     0,
    14,    14,    14,    15,    15,    16,    16,    16,    16,    17,
    17,    18,    18,    18,    19,    19,    20,    21,    21,    22,
    22,    23,    23
};

static const short yyr2[] = {     0,
     2,     3,     4,     0,     1,     3,     5,     5,     1,     1,
     3,     3,     3,     1,     0,     2,     2,     1,     3,     1,
     3,     0,     1
};

static const short yydefact[] = {     0,
    14,     4,     4,    22,     9,    10,     0,     5,     0,     4,
    15,    23,    18,    22,     1,     0,     4,     0,    13,    12,
    17,     4,     0,    15,    20,    22,     2,     4,     6,    11,
    16,    19,     0,     3,     0,     4,    21,     8,     7,     0,
     0,     0
};

static const short yydefgoto[] = {    40,
     7,     8,     5,     6,    23,    13,    14,    26,    15
};

static const short yypact[] = {     3,
-32768,     3,     3,    -4,     8,    -1,    10,-32768,    -6,     3,
    15,-32768,-32768,     2,-32768,     4,     3,     3,-32768,-32768,
-32768,     3,    12,    15,-32768,    11,-32768,     3,-32768,-32768,
-32768,-32768,    13,-32768,    16,     3,-32768,-32768,-32768,    27,
    28,-32768
};

static const short yypgoto[] = {-32768,
    -3,    29,    14,-32768,     6,    17,-32768,-32768,   -13
};


#define	YYLAST		33


static const short yytable[] = {     9,
    27,    10,    18,    20,    11,     1,    21,    10,    12,     2,
    24,     3,    34,    29,    12,    28,    16,    19,    31,    17,
    22,    32,    37,    12,    35,    38,    41,    42,     4,    33,
    25,    30,    39
};

static const short yycheck[] = {     3,
    14,     6,     4,    10,     9,     3,    10,     6,    13,     7,
     9,     9,    26,    17,    13,    12,     9,     8,    22,    12,
     6,    10,    10,    13,    28,    10,     0,     0,     0,    24,
    14,    18,    36
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
#line 99 "said.y"
{ yyval = said_top_branch(said_attach_branch(yyvsp[-1], said_aug_branch(0x142, 0x14a, SAID_BRANCH_NULL, yyvsp[0]))) ;
    break;}
case 2:
#line 101 "said.y"
{ yyval = said_top_branch(said_attach_branch(yyvsp[-2], said_attach_branch(yyvsp[-1], yyvsp[0]))) ;
    break;}
case 3:
#line 103 "said.y"
{ yyval = said_top_branch(said_attach_branch(yyvsp[-3], said_attach_branch(yyvsp[-2], said_attach_branch(yyvsp[-1], yyvsp[0])))) ;
    break;}
case 4:
#line 108 "said.y"
{ yyval = SAID_BRANCH_NULL ;
    break;}
case 5:
#line 110 "said.y"
{ yyval = yyvsp[0] ;
    break;}
case 6:
#line 116 "said.y"
{ yyval = said_aug_branch(0x144, 0x14f, yyvsp[-2], yyvsp[0]) ;
    break;}
case 7:
#line 118 "said.y"
{ yyval = said_aug_branch(0x141, 0x144, yyvsp[-4], said_aug_branch(0x144, 0x14f, yyvsp[-2], yyvsp[0])) ;
    break;}
case 8:
#line 120 "said.y"
{ yyval = said_aug_branch(0x152, 0x144, yyvsp[-4], yyvsp[-1]) ;
    break;}
case 9:
#line 122 "said.y"
{ yyval = yyvsp[0] ;
    break;}
case 10:
#line 126 "said.y"
{ yyval = yyvsp[0] ;
    break;}
case 11:
#line 128 "said.y"
{ yyval = said_attach_branch(yyvsp[-2], yyvsp[0]) ;
    break;}
case 12:
#line 132 "said.y"
{ yyval = said_aug_branch(0x152, 0x14c, yyvsp[-1], SAID_BRANCH_NULL) ;
    break;}
case 13:
#line 134 "said.y"
{ yyval = said_aug_branch(0x141, 0x14c, yyvsp[-1], SAID_BRANCH_NULL) ;
    break;}
case 14:
#line 136 "said.y"
{ yyval = said_wgroup_branch(yyvsp[0]) ;
    break;}
case 15:
#line 140 "said.y"
{ yyval = SAID_BRANCH_NULL ;
    break;}
case 16:
#line 142 "said.y"
{ yyval = yyvsp[0] ;
    break;}
case 17:
#line 146 "said.y"
{ yyval = yyvsp[0] ;
    break;}
case 18:
#line 150 "said.y"
{ yyval = said_aug_branch(0x142, 0x14a, yyvsp[0], SAID_BRANCH_NULL) ;
    break;}
case 19:
#line 152 "said.y"
{ yyval = said_aug_branch(0x152, 0x142, yyvsp[-1], SAID_BRANCH_NULL) ;
    break;}
case 20:
#line 156 "said.y"
{ yyval = said_aug_branch(0x143, 0x14a, yyvsp[0], SAID_BRANCH_NULL) ;
    break;}
case 21:
#line 158 "said.y"
{ yyval = said_aug_branch(0x152, 0x143, yyvsp[-1], SAID_BRANCH_NULL) ;
    break;}
case 22:
#line 162 "said.y"
{ yyval = SAID_BRANCH_NULL ;
    break;}
case 23:
#line 164 "said.y"
{ yyval = said_aug_branch(0x14b, SAID_LONG(SAID_GT), SAID_BRANCH_NULL, SAID_BRANCH_NULL) ;
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
#line 169 "said.y"


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
