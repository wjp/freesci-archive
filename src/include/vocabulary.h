#ifndef VOCABULARY_H
#define VOCABULARY_H

/*#define VOCABULARY_DEBUG /**/

#define SCRIPT_UNKNOWN_FUNCTION_STRING "[Unknown]"
/* The string used to identify the "unknown" SCI0 function for each game */


typedef struct opcode_
{
  int type;
  int number;
  char* name;
} opcode;

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
#endif
