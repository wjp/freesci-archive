/***************************************************************************
 vocab.c (C) 1999 Christoph Reichenbach


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
/* Main vocabulary support functions and word lookup */


#include <resource.h>
#include <engine.h>



int
_vocab_cmp_words(const void *word1, const void *word2)
{
  return g_strcasecmp((*((word_t **) word1))->word, (*((word_t **) word2))->word);
}


word_t **
vocab_get_words(int *word_counter)
{
  int counter = 0;
  int seeker;
  word_t **words;

  char currentword[256] = ""; /* They're not going to use words longer than 255 ;-) */
  int currentwordpos = 0;

  resource_t *resource = findResource(sci_vocab, VOCAB_RESOURCE_MAIN_VOCAB);

  if (!resource) {
    fprintf(stderr,"Could not find a main vocabulary!\n");
    return NULL; /* NOT critical: SCI1 games and some demos don't have one! */
  }

  seeker = 52; /* vocab.000 starts with 26 16-bit pointers which we don't use */
 
  if (resource->length < 52) {
    fprintf(stderr, "Invalid main vocabulary encountered: Too small\n");
    return NULL;
    /* Now this ought to be critical, but it'll just cause parse() and said() not to work */
  }

  words = malloc(sizeof(word_t *));

  while (seeker < resource->length) {
    byte c;

    words = realloc(words, (counter + 1) * sizeof(word_t *));

    currentwordpos = resource->data[seeker++]; /* Parts of previous words may be re-used */

    do {
      c = resource->data[seeker++];
      currentword[currentwordpos++] = c & 0x7f; /* 0x80 is used to terminate the string */
    } while (c < 0x80);
    currentword[currentwordpos] = 0;

    words[counter] = malloc(sizeof(word_t) + currentwordpos);
    /* Allocate more memory, so that the word fits into the structure */

    strcpy(&(words[counter]->word[0]), &(currentword[0])); /* Copy the word */

    /* Now decode class and group: */
    c = resource->data[seeker + 1];
    words[counter]->class = resource->data[seeker] | ((c & 0xf0) << 4);
    words[counter]->group = (resource->data[seeker + 2]) | ((c & 0x0f) << 8);

    ++counter;

    seeker += 3;

  }

  *word_counter = counter;

  qsort(words, counter, sizeof(word_t *), _vocab_cmp_words); /* Sort entries */

  return words;
}


void
vocab_free_words(word_t **words, int words_nr)
{
  int i;

  for (i = 0; i < words_nr; i++)
    free(words[i]);

  free(words);
}


char *
vocab_get_any_group_word(int group, word_t **words, int words_nr)
{
  int i;

  if (group == VOCAB_MAGIC_NUMBER_GROUP)
    return "{number}";

  for (i = 0; i < words_nr; i++)
    if (words[i]->group == group)
      return words[i]->word;

  return "{invalid}";
}


suffix_t **
vocab_get_suffices(int *suffices_nr)
{
  int counter = 0;
  suffix_t **suffices;
  resource_t *resource = findResource(sci_vocab, VOCAB_RESOURCE_SUFFIX_VOCAB);
  int seeker = 0;

  if (!resource) {
    fprintf(stderr,"Could not find suffix vocabulary!\n");
    return NULL; /* Not critical */
  }

  suffices = malloc(sizeof(suffix_t *));

  while ((seeker < resource->length) && (resource->data[seeker] = '*')) {

    char *alt_suffix = resource->data + seeker + 1;
    int alt_len = strlen(alt_suffix);
    char *word_suffix;
    int word_len;

    suffices = realloc(suffices, sizeof(suffix_t *) * (counter + 1));

    seeker += alt_len + 2; /* Hit end of string */
    word_suffix = resource->data + seeker + 3; /* Beginning of next string +1 (ignore '*') */
    word_len = strlen(word_suffix);

    suffices[counter] = malloc(sizeof(suffix_t) + alt_len + word_len);
    /* allocate enough memory to store the strings */

    strcpy(&(suffices[counter]->word_suffix[0]), word_suffix);
    strcat(&(suffices[counter]->word_suffix[0]), alt_suffix);

    suffices[counter]->alt_suffix_length = alt_len;
    suffices[counter]->word_suffix_length = word_len;
    suffices[counter]->class_mask = getInt16(resource->data + seeker);

    seeker += word_len + 2; /* Hit end of string */
    suffices[counter]->result_class = getInt16(resource->data + seeker);

    seeker += 2; /* Next entry */

    ++counter;

  }

  *suffices_nr = counter;
  return suffices;
}



void
vocab_free_suffices(suffix_t **suffices, int suffices_nr)
{
  vocab_free_words((word_t **) suffices, suffices_nr);
  /* They were allocated the same way, so they can be freed in the same way */
}


void
vocab_free_branches(parse_tree_branch_t *parser_branches)
{
  if (parser_branches)
    free(parser_branches);
}


parse_tree_branch_t *
vocab_get_branches(int *branches_nr)
{
  resource_t *resource = findResource(sci_vocab, VOCAB_RESOURCE_PARSE_TREE_BRANCHES);
  parse_tree_branch_t *retval;
  int i;

  if (!resource) {
    fprintf(stderr,"No parser tree data found!\n");
    return NULL;
  }

  *branches_nr = resource->length / 20;

  if (*branches_nr == 0) {
    fprintf(stderr,"Parser tree data is empty!\n");
    return NULL;
  }

  retval = malloc(sizeof(parse_tree_branch_t) * *branches_nr);

  for (i = 0; i < *branches_nr; i++) {
    int k;

    byte *base = resource->data + i*20;

    retval[i].id = getInt16(base);

    for (k = 0; k < 9; k++)
      retval[i].data[k] = getInt16(base + 2 + 2*k);

    retval[i].data[9] = 0; /* Always terminate */
  }

  return retval;
}


result_word_t *
vocab_lookup_word(char *word, int word_len,
		  word_t **words, int words_nr,
		  suffix_t **suffices, int suffices_nr)
{
  word_t *tempword = malloc(sizeof(word_t) + word_len + 256); /* 256: For suffices. Should suffice. */
  word_t **dict_word;
  result_word_t *retval;
  char *tester;
  int i;

  strncpy(&(tempword->word[0]), word, word_len);
  tempword->word[word_len] = 0;

  fprintf(stderr,"Looking up '%s'\n", tempword->word);

  retval = malloc(sizeof(result_word_t));

  dict_word = bsearch(&tempword, words, words_nr, sizeof(word_t *), _vocab_cmp_words);

  if (dict_word) {
    free(tempword);

    retval->class = (*dict_word)->class;
    retval->group = (*dict_word)->group;

    return retval;
  }

  /* Now try all suffices */

  for (i = 0; i < suffices_nr; i++)
    if (suffices[i]->alt_suffix_length > word_len) {

      int suff_index = suffices[i]->alt_suffix_length - word_len;
      /* Offset of the start of the suffix */

      if (strncasecmp(suffices[i]->alt_suffix, word + suff_index,
		      suffices[i]->alt_suffix_length) == 0) { /* Suffix matched! */

	strncpy(&(tempword->word[0]), word, word_len);
	tempword->word[suff_index] = 0; /* Terminate word at suffix start position... */
	strcat(&(tempword->word[0]), suffices[i]->word_suffix); /* ...and append "correct" suffix */

	dict_word = bsearch(tempword, words, words_nr, sizeof(word_t *), _vocab_cmp_words);

	if ((dict_word) && ((*dict_word)->class & suffices[i]->class_mask)) { /* Found it? */
	  free(tempword);

	  retval->class = suffices[i]->result_class; /* Use suffix class */
	  retval->group = (*dict_word)->group;

	  return retval;
	}

      }
    }

  /* No match so far? Check if it's a number. */

  strncpy(&(tempword->word[0]), word, word_len);
  tempword->word[word_len] = 0;

  if ((strtol(&(tempword->word[0]), &tester, 10) >= 0)
      && (*tester == '\0')) { /* Do we have a complete number here? */
    free(tempword);

    retval->group = VOCAB_MAGIC_NUMBER_GROUP;
    retval->class = VOCAB_CLASS_NUMBER;

    return(retval);
  }

  free(tempword);
  free(retval);
  return NULL;
}


void
vocab_decypher_said_block(state_t *s, heap_ptr addr)
{
  int nextitem;

  do {
    nextitem = s->heap[addr++];

    if (nextitem < 0xf0) {
      nextitem = nextitem << 8 | s->heap[addr++];
      sciprintf(" %s[%03x]", vocab_get_any_group_word(nextitem, s->parser_words, s->parser_words_nr),
		nextitem);

      nextitem = 42; /* Make sure that group 0xff doesn't abort */
    } else switch(nextitem) {
    case 0xf0: sciprintf(" ,"); break;
    case 0xf1: sciprintf(" &"); break;
    case 0xf2: sciprintf(" /"); break;
    case 0xf3: sciprintf(" ("); break;
    case 0xf4: sciprintf(" )"); break;
    case 0xf5: sciprintf(" ["); break;
    case 0xf6: sciprintf(" ]"); break;
    case 0xf7: sciprintf(" #"); break;
    case 0xf8: sciprintf(" <"); break;
    case 0xf9: sciprintf(" >"); break;
    case 0xff: break;
    }
  } while (nextitem != 0xff);

  sciprintf("\n");
}


result_word_t *
vocab_tokenize_string(char *sentence, int *result_nr,
		      word_t **words, int words_nr,
		      suffix_t **suffices, int suffices_nr,
		      char **error)
{
  char *lastword = sentence;
  int pos_in_sentence = 0;
  char c;
  int wordlen = 0;
  result_word_t *retval = malloc(sizeof(result_word_t));
  /* malloc'd size is always one result_word_t too big */

  result_word_t *lookup_result;


  *result_nr = 0;
  *error = NULL;

  do {

    c = sentence[pos_in_sentence++];

    if (isalnum(c))
      ++wordlen; /* Continue on this word */

    else {

      if (wordlen) { /* Finished a word? */

	lookup_result = vocab_lookup_word(lastword, wordlen,
					  words, words_nr,
					  suffices, suffices_nr);
	/* Look it up */

	if (!lookup_result) { /* Not found? */
	  *error = calloc(wordlen + 1, 1);
	  strncpy(*error, lastword, wordlen); /* Set the offending word */
	  free(retval);
	  return NULL; /* And return with error */
	}

	memcpy(retval + *result_nr, lookup_result, sizeof(result_word_t));
	/* Copy into list */
	++(*result_nr); /* Increase number of resulting words */

	retval = realloc(retval, sizeof(result_word_t) * (*result_nr + 1));

      }

      lastword = sentence + pos_in_sentence;
      wordlen = 0;
    }

  } while (c); /* Until terminator is hit */

  if (*result_nr == 0) {
    free(retval);
    return NULL;
  }

  return retval;
}
