/***************************************************************************
 listwords.c Copyright (C) 2000 Christoph Reichenbach, TU Darmstadt


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

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

 History:

   000413 - created (LS) from vocabdump.c

***************************************************************************/

#include <engine.h>
#include <kernel.h>

#define SORT_METHOD_ALPHA 0
#define SORT_METHOD_GROUP 1

#define DEFAULT_SORTING SORT_METHOD_ALPHA

int
_vocab_cmp_group(const void *word1, const void *word2)
{
#define fw (* ((word_t **) word1))
#define sw (* ((word_t **) word2))
  if (fw->group<sw->group)
    return -1;
  else if (fw->group==sw->group)
    return 0;
  else
    return 1;
}

int main(int argc, char** argv)
{
  int i, b, words_nr, counter;
  int sort = DEFAULT_SORTING;
  word_t **words, **tracker;

   if ((argc > 1) && ((argc == 2) && (strcmp("--version", argv[1])==0))) {
    printf("listwords for "PACKAGE", version "VERSION"\n");
    exit(-1);
  } else if (argc > 1) {
    if (strcmp("--sort-alpha", argv[1])==0) sort=SORT_METHOD_ALPHA; else
    if (strcmp("--sort-group", argv[1])==0) sort=SORT_METHOD_GROUP; else
    {
    printf("Usage:\nlistwords [<sort-method>] 	  dumps all words in the parser vocabulary\n"
           "\nsort-method is:\n\n"
	   "	--sort-alpha		sort in alphabetical order\n"
	   "	--sort-group		sort in group order\n");
    return 0; }
  }

  if ((i = loadResources(SCI_VERSION_AUTODETECT, 1))) {
    fprintf(stderr,"SCI Error: %s!\n", SCI_Error_Types[i]);
    return 0;
  }; /* i = 0 */

  tracker = words = vocab_get_words(&words_nr);

  counter=words_nr;

  if (sort==SORT_METHOD_GROUP)
    qsort(words, words_nr, sizeof(word_t *), _vocab_cmp_group); /* Sort entries */

  while (counter--)
    {
      printf("%s (class %03x, group %03x) ", &tracker[0]->word,
	     tracker[0]->class, tracker[0]->group);
      
        if ((tracker[0]->class>=0xf00)||
	    (tracker[0]->class==0)) 
        printf("anyword\n"); else
	while (tracker[0]->class)
	  {
	    b=sci_ffs(tracker[0]->class)-1;
	    tracker[0]->class&=~(1<<b);
            printf("%s", class_names[b]);
	    if (tracker[0]->class)
	      printf("|"); else
	      printf("\n");
          }    
      tracker++;
    }

  vocab_free_words(words, words_nr);

  return 0;
}






