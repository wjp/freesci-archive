#ifndef VOCABULARY_H
#define VOCABULARY_H

/*#define VOCABULARY_DEBUG /**/

#define SCRIPT_UNKNOWN_FUNCTION_STRING "[Unknown]"
/* The string used to identify the "unknown" SCI0 function for each game */

#define PARSE_HEAP_SIZE 64
/* Number of bytes allocated on the heap to store bad words if parsing fails */


typedef struct opcode_
{
  int type;
  int number;
  char* name;
} opcode;


#define VOCAB_RESOURCE_MAIN_VOCAB 0
#define VOCAB_RESOURCE_PARSE_TREE_BRANCHES 900
#define VOCAB_RESOURCE_SUFFIX_VOCAB 901


#define VOCAB_CLASS_PREPOSITION 0x01
#define VOCAB_CLASS_ARTICLE 0x02
#define VOCAB_CLASS_ADJECTIVE 0x04
#define VOCAB_CLASS_PRONOUN 0x08
#define VOCAB_CLASS_NOUN 0x10
#define VOCAB_CLASS_INDICATIVE_VERB 0x20
#define VOCAB_CLASS_ADVERB 0x40
#define VOCAB_CLASS_IMPERATIVE_VERB 0x80
#define VOCAB_CLASS_NUMBER 0x100

#define VOCAB_CLASS_ANYWORD 0xff
/* Anywords are ignored by the parser */

#define VOCAB_MAGIC_NUMBER_GROUP 0xffe
/* This word class is used for numbers */

typedef struct {

  int class; /* Word class */
  int group; /* Word group */
  char word[1]; /* The actual word */

} word_t;


typedef struct {

  int class_mask; /* the word class this suffix applies to */
  int result_class; /* the word class a word is morphed to if it doesn't fail this check */

  int alt_suffix_length; /* String length of the suffix */
  int word_suffix_length; /* String length of the other suffix */

  char *alt_suffix; /* The alternative suffix */
  char word_suffix[2]; /* The suffix as used in the word vocabulary */

} suffix_t;


typedef struct {

  int class; /* Word class */
  int group; /* Word group */

} result_word_t;


typedef struct {

  int id;

  int data[10];

} parse_tree_branch_t;



/*FIXME: These need freeing functions...*/

int* vocabulary_get_classes();

/**
 * Returns a null terminated array of selector names.
 */
char** vocabulary_get_snames();
/**
 * Frees the aforementioned array
 */
void vocabulary_free_snames(char **snames_list);

/**
 * Returns a null terminated array of opcodes.
 */
opcode* vocabulary_get_opcodes();

/**
 * Returns a null terminated array of kernel function names.
 *
 * This function reads the kernel function name table from resource_map,
 * and returns a null terminated array of deep copies of them.
 * The returned array has the same format regardless of the format of the
 * name table of the resource (the format changed between version 0 and 1).
 */
char** vocabulary_get_knames(int* count);
void vocabulary_free_knames(char** names);



word_t **
vocab_get_words(int *word_counter);
/* Gets all words from the main vocabulary
** Parameters: (int *) word_counter: The int which the number of words is stored in
** Returns   : (word_t **): A list of all words, dynamically allocated
*/


void
vocab_free_words(word_t **words, int words_nr);
/* Frees memory allocated by vocab_get_words
** Parameters: (word_t **) words: The words to free
**             (int) words_nr: Number of words in the structure
** Returns   : (void)
*/


suffix_t **
vocab_get_suffices(int *suffices_nr);
/* Gets all suffixes from the suffix vocabulary
** Parameters: (int *) suffices_nr: The variable to store the number of suffices in
** Returns   : (suffix_t **): A list of suffixes
*/

void
vocab_free_suffices(suffix_t **suffices, int suffices_nr);
/* Frees suffices_nr suffices
** Parameters: (suffix_t **) suffices: The suffixes to free
**             (int) suffices_nr: Number of entrie sin suffices
*/

parse_tree_branch_t *
vocab_get_branches(int *branches_nr);
/* Retrieves all parse tree branches from the resource data
** Parameters: (int *) branches_nr: Pointer to the variable which the number of entries is to be
**                     stored in
** Returns   : (parse_tree_branch_t *): The branches, or NULL on error
*/

void
vocab_free_branches(parse_tree_branch_t *parser_branches);
/* Frees all branches
** Parameters: (parse_tree_branch_t *) parser_branches: The branches to free
** Returns   : (null)
*/

result_word_t *
vocab_tokenize_string(char *sentence, int *result_nr,
		      word_t **words, int words_nr,
		      suffix_t **suffices, int suffices_nr,
		      char **error);
/* Tokenizes a string and compiles it into word_ts.
** Parameters: (char *) sentence: The sentence to examine
**             (int *) result_nr: The variable to store the resulting number of words in
**             (word_t **) words: The words to scan for
**             (int) words_nr: Number of words to scan for
**             (suffix_t **) suffices: suffixes to scan for
**             (int) suffices_nr: Number of suffices to scan for
**             (char **) error: Points to a malloc'd copy of the offending text or to NULL on error
** Returns   : (word_t *): A list of word_ts containing the result, or NULL.
** On error, NULL is returned. If *error is NULL, the sentence did not contain any useful words;
** if not, *error points to a malloc'd copy of the offending word.
** The returned list may contain anywords.
*/

#endif
