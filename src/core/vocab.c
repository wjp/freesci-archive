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


#include <engine.h>
#include <kernel.h>

#include <ctype.h>

char *class_names[] = 
  {"",               /* These strange names were taken from an SCI01 interpreter */
   "", 
   "conj", 
   "ass", 
   "pos", 
   "art", 
   "adj", 
   "pron", 
   "noun", 
   "auxv", 
   "adv", 
   "verb", 
   "", 
   "", 
   "", 
   ""}; 


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

  words = g_malloc(sizeof(word_t *));

  while (seeker < resource->length) {
    byte c;

    words = g_realloc(words, (counter + 1) * sizeof(word_t *));

    currentwordpos = resource->data[seeker++]; /* Parts of previous words may be re-used */

    do {
      c = resource->data[seeker++];
      currentword[currentwordpos++] = c & 0x7f; /* 0x80 is used to terminate the string */
    } while (c < 0x80);
    currentword[currentwordpos] = 0;

    words[counter] = g_malloc(sizeof(word_t) + currentwordpos);
    /* Allocate more memory, so that the word fits into the structure */

    strcpy(&(words[counter]->word[0]), &(currentword[0])); /* Copy the word */

    /* Now decode class and group: */
    c = resource->data[seeker + 1];
    words[counter]->class = ((resource->data[seeker]) << 4) | ((c & 0xf0) >> 4);
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
    g_free(words[i]);

  g_free(words);
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


static inline unsigned int
inverse_16(unsigned int foo)
{
  return (((foo & 0xff) << 8) | ((foo & 0xff00) >> 8));
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

  suffices = g_malloc(sizeof(suffix_t *));

  while ((seeker < resource->length-1) && (resource->data[seeker + 1] != 0xff)) {

    char *alt_suffix = resource->data + seeker + 1;
    int alt_len = strlen(alt_suffix);
    char *word_suffix;
    int word_len;

    suffices = g_realloc(suffices, sizeof(suffix_t *) * (counter + 1));

    seeker += alt_len + 2; /* Hit end of string */
    word_suffix = resource->data + seeker + 3; /* Beginning of next string +1 (ignore '*') */
    word_len = strlen(word_suffix);

    suffices[counter] = g_malloc(sizeof(suffix_t) + alt_len + word_len);
    /* allocate enough memory to store the strings */

    strcpy(&(suffices[counter]->word_suffix[0]), word_suffix);
    strcat(&(suffices[counter]->word_suffix[0]), alt_suffix);

    suffices[counter]->alt_suffix = &(suffices[counter]->word_suffix[word_len]);
    suffices[counter]->alt_suffix_length = alt_len;
    suffices[counter]->word_suffix_length = word_len;
    suffices[counter]->class_mask = inverse_16(getInt16(resource->data + seeker)); /* Inverse endianness */
    
    seeker += word_len + 2; /* Hit end of string */
    suffices[counter]->result_class = getInt16(resource->data + seeker);

    seeker += 4; /* Next entry */

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
    g_free(parser_branches);
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

  retval = g_malloc(sizeof(parse_tree_branch_t) * *branches_nr);

  for (i = 0; i < *branches_nr; i++) {
    int k;

    byte *base = resource->data + i*20;

    retval[i].id = getInt16(base);

    for (k = 0; k < 9; k++)
      retval[i].data[k] = getUInt16(base + 2 + 2*k);

    retval[i].data[9] = 0; /* Always terminate */
  }

  return retval;
}


result_word_t *
vocab_lookup_word(char *word, int word_len,
		  word_t **words, int words_nr,
		  suffix_t **suffices, int suffices_nr)
{
  word_t *tempword = g_malloc(sizeof(word_t) + word_len + 256);
  /* 256: For suffices. Should suffice. */
  word_t **dict_word;
  result_word_t *retval;
  char *tester;
  int i;

  strncpy(&(tempword->word[0]), word, word_len);
  tempword->word[word_len] = 0;

  retval = g_malloc(sizeof(result_word_t));

  dict_word = bsearch(&tempword, words, words_nr, sizeof(word_t *), _vocab_cmp_words);

  if (dict_word) {
    g_free(tempword);

    retval->class = (*dict_word)->class;
    retval->group = (*dict_word)->group;

    return retval;
  }

  /* Now try all suffices */
  for (i = 0; i < suffices_nr; i++)
    if (suffices[i]->alt_suffix_length <= word_len) {

      int suff_index = word_len - suffices[i]->alt_suffix_length;
      /* Offset of the start of the suffix */
      if (g_strncasecmp(suffices[i]->alt_suffix, word + suff_index,
			suffices[i]->alt_suffix_length) == 0) { /* Suffix matched! */
	strncpy(&(tempword->word[0]), word, word_len);
	tempword->word[suff_index] = 0; /* Terminate word at suffix start position... */
	strncat(&(tempword->word[0]), suffices[i]->word_suffix, suffices[i]->word_suffix_length); /* ...and append "correct" suffix */

	dict_word = bsearch(&tempword, words, words_nr, sizeof(word_t *), _vocab_cmp_words);

	if ((dict_word) && ((*dict_word)->class & suffices[i]->class_mask)) { /* Found it? */
	  g_free(tempword);

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
    g_free(tempword);

    retval->group = VOCAB_MAGIC_NUMBER_GROUP;
    retval->class = VOCAB_CLASS_NUMBER;

    return(retval);
  }

  g_free(tempword);
  g_free(retval);
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


#ifdef SCI_SIMPLE_SAID_CODE
int
vocab_match_simple(state_t *s, heap_ptr addr)
{
  int nextitem;
  int listpos = 0;

  if (!s->parser_valid)
    return SAID_NO_MATCH;

  if (s->parser_valid == 2) { /* debug mode: sim_said */
    do {
      sciprintf("DEBUGMATCH: ");
      nextitem = s->heap[addr++];

      if (nextitem < 0xf0) {
	nextitem = nextitem << 8 | s->heap[addr++];
	if (s->parser_nodes[listpos].type
	    || nextitem != s->parser_nodes[listpos++].content.value)
	  return SAID_NO_MATCH;
      } else {

	if (nextitem == 0xff)
	  return (s->parser_nodes[listpos++].type == -1)? SAID_FULL_MATCH : SAID_NO_MATCH; /* Finished? */

	if (s->parser_nodes[listpos].type != 1
	    || nextitem != s->parser_nodes[listpos++].content.value)
	  return SAID_NO_MATCH;

      }
    } while (42);
  } else { /* normal simple match mode */
    return vocab_simple_said_test(s, addr);
  }
}


static short _related_words[][2] = { /* 0 is backwards, 1 is forward */
  {0x800, 0x180}, /* preposition */
  {0x000, 0x180}, /* article */
  {0x000, 0x180}, /* adjective */
  {0x800, 0x000}, /* pronoun */
  {0x800, 0x180}, /* noun */
  {0x000, 0x800}, /* auxiliary verb */
  {0x800, 0x800}, /* adverb */
  {0x000, 0x180}, /* verb */
  {0x000, 0x180} /* number */
};

int
vocab_build_simple_parse_tree(parse_tree_node_t *nodes, result_word_t *words, int words_nr)
{
  int i, length, pos = 0;

  for (i = 0; i < words_nr; ++i) {
    if (words[i].class != VOCAB_CLASS_ANYWORD) {
      nodes[pos].type = words[i].class;
      nodes[pos].content.value = words[i].group;
      pos += 2; /* Link information is filled in below */
    }
  }
  nodes[pos].type = -1; /* terminate */
  length = pos >> 1;

  /* now find all referenced words */
#ifdef SCI_SIMPLE_SAID_DEBUG
  sciprintf("Semantic references:\n");
#endif

  for (i = 0; i < length; i++) {
    int j;
    int searchmask;
    int type;

    pos = (i << 1);
    type = sci_ffs(nodes[pos].type);

    if (type) {
      int found = -1;

      type -= 5; /* 1 because ffs starts counting at 1, 4 because nodes[pos].type is a nibble off */
      if (type < 0)
	type = 0;
#ifdef SCI_SIMPLE_SAID_DEBUG
      sciprintf("#%d: Word %04x: type %04x\n", i, nodes[pos].content.value, type);
#endif

      /* search backwards */
      searchmask = _related_words[type][0];
      if (searchmask) {
	for (j = i-1; j >= 0; j--)
	  if (nodes[j << 1].type & searchmask) {
	    found = j << 1;
	    break;
	  }
      }
      nodes[pos+1].content.branches[0] = found;
#ifdef SCI_SIMPLE_SAID_DEBUG
      if (found > -1)
	sciprintf("  %d <\n", found >> 1);
#endif

      /* search forward */
      found = -1;
      searchmask = _related_words[type][1];
      if (searchmask) {
	for (j = i+1; j < length; j++)
	  if (nodes[j << 1].type & searchmask) {
	    found = j << 1;
	    break;
	  }
      }
#ifdef SCI_SIMPLE_SAID_DEBUG
      if (found > -1)
	sciprintf("  > %d\n", found >> 1);
#endif

    } else {
#ifdef SCI_SIMPLE_SAID_DEBUG
      sciprintf("#%d: Untypified word\n", i); /* Weird, but not fatal */
#endif
      nodes[pos+1].content.branches[0] = -1;
      nodes[pos+1].content.branches[1] = -1;
    }
  }
#ifdef SCI_SIMPLE_SAID_DEBUG
  sciprintf("/Semantic references.\n");
#endif

  return 0;
}
#endif

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
  result_word_t *retval = g_malloc(sizeof(result_word_t));
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
	  *error = g_malloc0(wordlen + 1);
	  strncpy(*error, lastword, wordlen); /* Set the offending word */
	  g_free(retval);
	  return NULL; /* And return with error */
	}

	memcpy(retval + *result_nr, lookup_result, sizeof(result_word_t));
	/* Copy into list */
	++(*result_nr); /* Increase number of resulting words */

	retval = g_realloc(retval, sizeof(result_word_t) * (*result_nr + 1));

      }

      lastword = sentence + pos_in_sentence;
      wordlen = 0;
    }

  } while (c); /* Until terminator is hit */

  if (*result_nr == 0) {
    g_free(retval);
    return NULL;
  }

  return retval;
}


inline int
_vocab_ptree_next_branch(parse_tree_branch_t *branches, int branches_nr, int current, int id)
     /* Looks for the next branch with a specific ID */
{
  current++;

  while ((current < branches_nr) && (branches[current].id != id))
    current++;

  return (current >= branches_nr) ? -1 : current;
}


inline int
_vocab_ptree_add_final(parse_tree_node_t *nodes, int pos, int storage_code, int value)
{
  /*  sciprintf("adding FINAL leafbranch (%04x, %04x) at %03x\n", storage_code, value, pos);*/
  nodes[pos].type = PARSE_TREE_NODE_BRANCH;
  nodes[pos].content.branches[0] = pos+1;
  nodes[pos].content.branches[1] = pos+2;

  nodes[pos+1].type = PARSE_TREE_NODE_LEAF;
  nodes[pos+1].content.value = storage_code;

  nodes[pos+2].type = PARSE_TREE_NODE_LEAF;
  nodes[pos+2].content.value = value;

  return pos+2;
}

inline int
_vocab_ptree_add_storage_list(parse_tree_node_t *nodes, int pos, int storage_code, int list_nr)
     /* Adds storage nodes for new branches:
     **       ___pos__
     **      /        \
     **  storage_code  []
     **               /  \
     **        list_nr    pos+4 (return value)
     */
{
  /*  sciprintf("Adding BRANCH (%04x, %04x) from %03x to %03x\n", storage_code, list_nr, pos, pos+4);*/
  nodes[pos].type = PARSE_TREE_NODE_BRANCH;
  nodes[pos].content.branches[0] = pos + 1;
  nodes[pos].content.branches[1] = pos + 2;

  nodes[pos + 1].type = PARSE_TREE_NODE_LEAF;
  nodes[pos + 1].content.value = storage_code;

  nodes[pos + 2].type = PARSE_TREE_NODE_BRANCH;
  nodes[pos + 2].content.branches[0] = pos + 3;
  nodes[pos + 2].content.branches[1] = pos + 4;

  nodes[pos + 3].type = PARSE_TREE_NODE_LEAF;
  nodes[pos + 3].content.value = list_nr;

  nodes[pos + 4].type = PARSE_TREE_NODE_BRANCH;
  nodes[pos + 4].content.branches[0] = 0;
  nodes[pos + 4].content.branches[1] = 0;

  return pos + 4;
}
#define VOCAB_TREE_NODE_LAST_WORD_STORAGE 0x140
#define VOCAB_TREE_NODE_COMPARE_TYPE 0x146
#define VOCAB_TREE_NODE_COMPARE_GROUP 0x14d
#define VOCAB_TREE_NODE_FORCE_STORAGE 0x154

#define OUT_OF_NODES 2

int
_vocab_ptree_try_branches(parse_tree_node_t *nodes, result_word_t *words, int words_nr,
			  parse_tree_branch_t *branches, int branches_nr,
			  int branch_id, int *curr_word, int *last_node, int storage_code,
			  int parent_node, int last_branch, int linkside, int ourfirstnode)
     /* Returns 0 on success, 1 otherwise */
{
  int branch_nr = _vocab_ptree_next_branch(branches, branches_nr, last_branch, branch_id);
  int prev_lastnode = *last_node;

  /* Node tree overflow is imminent */
  if (*last_node + 5 > VOCAB_TREE_NODES) {
    sciprintf("Out of nodes while building parse tree (%d)\n", *last_node);
    return OUT_OF_NODES;
  }

  if (parent_node >= 0) { /* Must be a branch node */
    nodes[parent_node].content.branches[linkside] = ourfirstnode;
  }

  while (branch_nr >= 0) {

    int branchpos = 0;
    int command, data;
    int this_branch_is_bad = 0;
    int descends = 0; /* This branch tries to descend => don't finish it */

    parse_tree_branch_t *branch = &(branches[branch_nr]);
/** sciprintf("--Checking branch %d at node %03x\n", branch_nr, *last_node);*/
    do {

      command = branch->data[branchpos];
      data = branch->data[branchpos+1];
      branchpos += 2;
/** sciprintf("id=%04x cmd=%04x dat=%04x\n", branch_id, command, data);*/
      if (command >= 0)

	if (((command == 0) && (!descends)) || (command <= VOCAB_TREE_NODE_LAST_WORD_STORAGE)) {
	  /* Store current word here */

	  *last_node = _vocab_ptree_add_final(nodes, *last_node + 1,
					      command, words[(*curr_word)++].group);
	  return 0;

	} else if (command < VOCAB_TREE_NODE_COMPARE_TYPE) { /* Continue on branch */
	  int result;
	  int old_last_node = *last_node;

	  descends = 1; /* We're descending */

	  /* Add branches for next subtree */
	  *last_node = _vocab_ptree_add_storage_list(nodes, *last_node + 1, command, data);
/** sciprintf("descending\n"); */
	  if ((result = _vocab_ptree_try_branches(nodes, words, words_nr,
						 branches, branches_nr,
						 data, curr_word, last_node, command,
						 old_last_node, branch_nr, 0, old_last_node + 1)) == 0)
	    return 0; /* Return if succeeded */

	  *last_node = old_last_node; /* No success, so remove branch */
/** sciprintf("returned without success\n");*/

	  if (result == OUT_OF_NODES) {
	    fprintf(stderr,"Returning Out Of Nodes\n");
	    return OUT_OF_NODES;
	  }

	} else if (command == VOCAB_TREE_NODE_COMPARE_TYPE) {

	  if (!(data & words[*curr_word].class));
	    this_branch_is_bad = 1;

	} else if (command == VOCAB_TREE_NODE_COMPARE_GROUP) {

	  if (!(data & words[*curr_word].group));
	    this_branch_is_bad = 1;

	} else if (command == VOCAB_TREE_NODE_FORCE_STORAGE) {

	  *last_node = _vocab_ptree_add_final(nodes, *last_node, command, data);
	  return 0;

	} else {
	  sciprintf("Unknown parse tree command: %04x, param %04x, in branch %d\n",
		    command, data, branch_nr);
	  return 1;
	}

    } while (command && !this_branch_is_bad);



    branch_nr = _vocab_ptree_next_branch(branches, branches_nr, branch_nr, branch_id);

  }

  return 1; /* Found no matching branches */
}


int
vocab_build_parse_tree(parse_tree_node_t *nodes, result_word_t *words, int words_nr,
		       parse_tree_branch_t *branches, int branches_nr)
{
  int lastnode = -1;
  int nextword_forknode = -1; /* The node which the next word will be linked to */

  int first_branchstore = 0x141;
  int first_branchid = branches[0].id;

  int curr_word = 0;


  if (words_nr <= 0)
    return 1;
  while (curr_word < words_nr) {

    /* Add initial branch */
    int overnext_word_forknode = 
      lastnode = _vocab_ptree_add_storage_list(nodes, lastnode + 1, first_branchstore, first_branchid);
/** sciprintf("======== Parsing word %d of %d to %03x at node %03x\n", curr_word, words_nr-1, nextword_forknode, lastnode);*/

    /* Try to interpret branches */
    if (_vocab_ptree_try_branches(nodes, words, words_nr,
				  branches, branches_nr,
				  first_branchid, &curr_word, &lastnode, first_branchstore,
				  nextword_forknode, -1, 1, lastnode + 1))
      return 1; /* Appending failed for one => parsing failed for all */

    nextword_forknode = overnext_word_forknode;

  }

  return 0;

}


void
_vocab_recursive_ptree_dump(parse_tree_node_t *nodes, int nr, int prevnr)
{
  if ((nr > VOCAB_TREE_NODES) || (nr < prevnr)) {
    sciprintf("Error(%04x)", nr);
    return;
  }

  if (nodes[nr].type == PARSE_TREE_NODE_LEAF)
    /*    sciprintf("[%03x]%04x", nr, nodes[nr].content.value); */
    sciprintf("%04x", nodes[nr].content.value);
  else {
    int lbranch = nodes[nr].content.branches[0];
    int rbranch = nodes[nr].content.branches[1];
    /*    sciprintf("<[%03x]",nr); */
    sciprintf("<");

    if (lbranch)
      _vocab_recursive_ptree_dump(nodes, lbranch, nr);
    else sciprintf("NULL");

    sciprintf(",");

    if (rbranch)
      _vocab_recursive_ptree_dump(nodes, rbranch, nr);
    else sciprintf("NULL");

    sciprintf(">");
  }
}

void
vocab_dump_parse_tree(parse_tree_node_t *nodes)
{
  _vocab_recursive_ptree_dump(nodes, 0, 0);
  sciprintf("\n");
}

void
vocab_synonymize_tokens(result_word_t *words, int words_nr, synonym_t *synonyms, int synonyms_nr)
{
  int i, sync;

  if (!synonyms || !synonyms_nr)
    return; /* No synonyms: Nothing to check */

  for (i = 0; i < words_nr; i++)
    for(sync = 0; sync < synonyms_nr; sync++)
      if (words[i].class = synonyms[sync].replaceant)
	words[i].class = synonyms[sync].replacement;
}
