#ifndef SCRIPT_H
#define SCRIPT_H

#include <resource.h>
#include <util.h>

/*#define SCRIPT_DEBUG /**/

typedef struct script_opcode_
{
  unsigned opcode;
  int arg1, arg2, arg3;
  int pos, size;
} script_opcode;

typedef FLEXARRAY(script_opcode,int number;) script_method;

typedef struct object_
{
	/*These are based on cached selector values, and set to the values
	 *the selectors had at load time. If the selectors are changed in
	 *instances, inconsistency will follow*/
	struct object_* parent;
	char* name;

	FLEXARRAY(struct object_*,) children;

	/*No flexarray, size the size is known from the start*/
	script_method** methods;
	int method_count;

	int selector_count;
	int* selector_numbers;
} object;

object **object_map, *object_root;
int max_object;

#define SCRIPT_PRINT_METHODS	1
#define SCRIPT_PRINT_CHILDREN	2
void printObject(object* obj, int flags);
int loadObjects();
void freeObject(object*);

typedef enum {
  Script_Invalid=-1,
  Script_None=0, 
  Script_Byte, 
  Script_Word, 
  Script_SWord,
  Script_Variable,
  Script_End
} opcode_format;
 
extern char* globals[];
opcode_format formats[128][4];

#endif
