/***************************************************************************
 vocabdump.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

   990504 - created (CJR)

***************************************************************************/

#include <engine.h>

int main(int argc, char** argv)
{
  char **names;
  opcode *opcodes;
  int i, count;
  int *classes;

  if ((argc > 1) && ((argc == 2) && (strcmp("--version", argv[1])==0))) {
    printf("vocabdump for "PACKAGE", version "VERSION"\n");
    exit(-1);
  } else if (argc > 1) {
    printf("Usage:\nvocabdump   dumps all selector, kernel, and opcode"
	   " names.\n");
    return 0;
  }

  if ((i = loadResources(SCI_VERSION_AUTODETECT, 1))) {
    fprintf(stderr,"SCI Error: %s!\n", SCI_Error_Types[i]);
    return 0;
  }; /* i = 0 */

  printf("Selectors:\n");
  names = vocabulary_get_snames(NULL, 0);
  while (names[i]) printf("0x%02X: %s\n", i, names[i++]);
  vocabulary_free_snames(names);

  i = 0;
  printf("\nOpcodes:\n");
  opcodes = vocabulary_get_opcodes();
  while ((i < 256) && (opcodes[i].name))
  {
    printf("%s: Type %i, Number %i\n", opcodes[i].name,
           opcodes[i].type, opcodes[i].number);
    i++;
  }

  names = vocabulary_get_knames(&count);
  printf("\nKernel names:\n");
  if (names == 0) printf("Error loading kernel names\n");
  else
  {
    for (i=0; i<count; i++) printf("0x%02X: %s\n", i, names[i]);
    vocabulary_free_knames(names);
  }

  classes = vocabulary_get_classes(&count);
  printf ("\nClasses:\n");
  if (classes == 0) printf("Error loading classes\n");
  else
  {
    for (i=0; i<count; i++) printf("0x%02X: script %i\n", i, classes [i]);
    free(classes);
  }
  
  freeResources();
  return 0;
}
