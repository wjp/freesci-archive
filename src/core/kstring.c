/***************************************************************************
 kstring.c Copyright (C) 1999 Christoph Reichenbach


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
/* String and parser handling */

#include <kernel.h>



char *
kernel_lookup_text(state_t *s, int address, int index)
     /* Returns the string the script intended to address */
{
  char *seeker;
  resource_t *textres;

  if (address < 1000) {
    int textlen;
    int _index = index;
    textres = findResource(sci_text, address);

    if (!textres) {
      SCIkwarn(SCIkERROR, "text.%03d not found\n", address);
      return NULL; /* Will probably segfault */
    }

    textlen = textres->length;
    seeker = textres->data;

    while (index--)
      while ((textlen--) && (*seeker++));

    if (textlen)
      return seeker;
    else {
      SCIkwarn(SCIkERROR, "Index %d out of bounds in text.%03d\n", _index, address);
      return 0;
    }

  } else return s->heap + address;
}


/*************************************************************/
/* Parser */
/**********/

void
kSaid(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr said_block = UPARAM(0);

  CHECK_THIS_KERNEL_FUNCTION;

  if (s->debug_mode & (1 << SCIkPARSER_NR)) {
    SCIkdebug(SCIkPARSER, "Said block:", 0);
    vocab_decypher_said_block(s, said_block);
  }

  SCIkdebug(SCIkSTUB,"stub\n");
  
  s->acc = 0; /* Never true */
}

void
kParse(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int stringpos = UPARAM(0);
  char *string = s->heap + stringpos;
  int words_nr;
  char *error;
  result_word_t *words;
  heap_ptr event = UPARAM(1);
  CHECK_THIS_KERNEL_FUNCTION;
  words = vocab_tokenize_string(string, &words_nr,
				s->parser_words, s->parser_words_nr,
				s->parser_suffices, s->parser_suffices_nr,
				&error);

  if (words) {

    int syntax_fail = 0;

    s->acc = 1;

    if (s->debug_mode & (1 << SCIkPARSER_NR)) {
      int i;

      SCIkdebug(SCIkPARSER, "Parsed to the following blocks:\n", 0);

      for (i = 0; i < words_nr; i++)
	SCIkdebug(SCIkPARSER, "   Type[%04x] Group[%04x]\n", words[i].class, words[i].group);
    }

    if (vocab_build_parse_tree(&(s->parser_nodes[0]), words, words_nr, s->parser_branches,
			       s->parser_branches_nr))
      syntax_fail = 1; /* Building a tree failed */

    g_free(words);

    if (syntax_fail) {

      s->acc = 1;
      PUT_SELECTOR(event, claimed, 1);
      invoke_selector(INV_SEL(s->game_obj, syntaxFail, 0), 2, s->parser_base, stringpos);
      /* Issue warning */

      SCIkdebug(SCIkPARSER, "Tree building failed\n");

    } else {
      if (s->debug_mode & (1 << SCIkPARSER_NR))
	vocab_dump_parse_tree(s->parser_nodes);
    }

  } else {

    s->acc = 0;
    PUT_SELECTOR(event, claimed, 1);
    if (error) {

      strcpy(s->heap + s->parser_base, error);
      SCIkdebug(SCIkPARSER,"Word unknown: %s\n", error);
      /* Issue warning: */

      invoke_selector(INV_SEL(s->game_obj, wordFail, 0), 2, s->parser_base, stringpos);
      g_free(error);
      s->acc = 1; /* Tell them that it dind't work */
    }
  }
}


void
kStrEnd(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr address = UPARAM(0);
  char *seeker = s->heap + address;

  while (*seeker++)
    ++address;

  s->acc = address + 1; /* End of string */
}

void
kStrCat(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;

  strcat(s->heap + UPARAM(0), s->heap + UPARAM(1));
}

void
kStrCmp(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int length;

  if (argc > 2)
    s->acc = strncmp(s->heap + UPARAM(0), s->heap + UPARAM(1), UPARAM(2));
  else
    s->acc = strcmp(s->heap + UPARAM(0), s->heap + UPARAM(1));
}


void
kStrCpy(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int length;

  CHECK_THIS_KERNEL_FUNCTION;

  if (argc > 2)
    strncpy(s->heap + UPARAM(0), s->heap + UPARAM(1), UPARAM(2));
  else
    strcpy(s->heap + UPARAM(0), s->heap + UPARAM(1));

  s->acc = PARAM(0);

}


void
kStrAt(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr address = UPARAM(0) + UPARAM(1);

  s->acc = s->heap[address];

  if (argc > 2)
    s->heap[address]=UPARAM(2); /* Request to modify this char */
}


void
kReadNumber(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *source = s->heap + UPARAM(0);

  while (isspace(*source))
    source++; /* Skip whitespace */

  if (*source == '$') /* SCI uses this for hex numbers */
    s->acc = strtol(source + 1, NULL, 16); /* Hex */
  else
    s->acc = strtol(source, NULL, 10); /* Force decimal */
}


/*  Format(targ_address, textresnr, index_inside_res, ...)
** or
**  Format(targ_address, heap_text_addr, ...)
** Formats the text from text.textresnr (offset index_inside_res) or heap_text_addr according to
** the supplied parameters and writes it to the targ_address.
*/
void
kFormat(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int *arguments;
  int dest = UPARAM(0);
  char *target = ((char *) s->heap) + dest;
  int position = UPARAM(1);
  int index = UPARAM(2);
  resource_t *resource;
  char *source;
  int mode = 0;
  int paramindex = 0; /* Next parameter to evaluate */
  char xfer;
  int i;
  int startarg;
  int str_leng; /* Used for stuff like "%13s" */
  char *abstarget = target;

  CHECK_THIS_KERNEL_FUNCTION;

  source = kernel_lookup_text(s, position, index);

  SCIkdebug(SCIkSTRINGS, "Formatting \"%s\"\n", source);

  if (position < 1000) {
    startarg = 3; /* First parameter to use for formatting */
  } else  { /* position >= 1000 */
    startarg = 2;
  }

  arguments = g_malloc(sizeof(int) * argc);
  for (i = startarg; i < argc; i++)
    arguments[i-startarg] = UPARAM(i); /* Parameters are copied to prevent overwriting */

  while (xfer = *source++) {
    if (xfer == '%') {
      if (mode == 1)
	*target++ = '%'; /* Literal % by using "%%" */
      else {
	mode = 1;
	str_leng = 0;
      }
    }
    else { /* xfer != '%' */
      if (mode == 1) {
	switch (xfer) {
	case 's': { /* Copy string */
	  char *tempsource = kernel_lookup_text(s, arguments[paramindex], arguments[paramindex + 1]);
	  int extralen = str_leng - strlen(tempsource);

	  if (arguments[paramindex] > 1000) /* Heap address? */
	    paramindex++;
	  else
	    paramindex += 2; /* No, text resource address */

	  while (extralen-- > 0)
	    *target++ = ' '; /* Format into the text */

	  while (*target++ = *tempsource++);
	  target--; /* Step back on terminator */
	  mode = 0;
	}
	break;

	case 'c': { /* insert character */
	  while (str_leng-- > 1)
	    *target++ = ' '; /* Format into the text */

	  *target++ = arguments[paramindex++];
	  mode = 0;
	}
	break;

	case 'd': { /* Copy decimal */
	  int templen = sprintf(target, "%d", arguments[paramindex++]);

	  if (templen < str_leng) {
	    int diff = str_leng - templen;
	    char *dest = target + str_leng - 1;

	    while (templen--) {
	      *dest = *(dest + diff);
	      dest--;
	    } /* Copy the number */

	    while (diff--)
	      *dest-- = ' '; /* And fill up with blanks */

	    templen = str_leng;
	  }

	  target += templen;
	  mode = 0;
	}
	break;
	default:
	  if (isdigit(xfer))
	    str_leng = (str_leng * 10) + (xfer - '0');
	  else {
	    *target = '%';
	    *target++;
	    *target = xfer;
	    *target++;
	    mode = 0;
	  } /* if (!isdigit(xfer)) */
	}
      } else { /* mode != 1 */
	*target = xfer;
	*target++;
      }
    }
  }

  g_free(arguments);

  *target = 0; /* Terminate string */
  s->acc = dest; /* Return target addr */
}


void
kStrLen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = strlen(s->heap + UPARAM(0));
}


void
kGetFarText(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  resource_t *textres = findResource(sci_text, UPARAM(0));
  char *seeker;
  int counter = PARAM(1);

  CHECK_THIS_KERNEL_FUNCTION;

  if (!textres) {
    SCIkwarn(SCIkERROR, "text.%d does not exist\n", PARAM(0));
    return;
  }

  seeker = textres->data;

  while (counter--)
    while (*seeker++);
  /* The second parameter (counter) determines the number of the string inside the text
  ** resource.
  */

  s->acc = UPARAM(2);
  strcpy(s->heap + UPARAM(2), seeker); /* Copy the string and get return value */
}

