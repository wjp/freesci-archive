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


enum {
  sci_obj_terminator,
  sci_obj_object,
  sci_obj_code,
  sci_obj_strings = 5,
  sci_obj_class,
  sci_obj_exports,
  sci_obj_pointers,
  sci_obj_localvars = 10
} script_object_types;

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

/* Opcode formats as used by script.c */
typedef enum {
  Script_Invalid=-1,
  Script_None=0, 
  Script_Byte,
  Script_SByte,
  Script_Word, 
  Script_SWord,
  Script_Variable,
  Script_SVariable,
  Script_End
} opcode_format;

typedef enum { /* FIXME */
  op_bnot = 0,
  op_add,
  op_sub,
  op_mul,
  op_div,
  op_mod,
  op_shr,
  op_shl,
  op_xor,
  op_and,
  op_or,
  op_neg,
  op_not,
  op_eq,
  op_ne_,
  op_gt_,
  op_ge_,
  op_lt_,
  op_le_,
  op_ugt_,
  op_uge_,
  op_ult_,
  op_ule_,
  op_bt,
  op_bnt,
  op_jmp,
  op_ldi,
  op_push,
  op_pushi,
  op_toss,
  op_dup,
  op_link,
  op_call = 0x20,
  op_callk,
  op_callb,
  op_calle,
  op_ret,
  op_send,
  op_class = 0x28,
  op_self = 0x2a,
  op_super,
  op_rest,
  op_lea,
  op_selfID
} sci_opcodes;
 
extern char* globals[];
extern opcode_format formats[128][4];

#endif
