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
  sci_obj_synonyms,
  sci_obj_said,
  sci_obj_strings,
  sci_obj_class,
  sci_obj_exports,
  sci_obj_pointers,
  sci_obj_preload_text, /* This is really just a flag. */
  sci_obj_localvars
} script_object_types;

void script_dissect(int res_no);

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
  Script_SRelative,
  Script_Property,
  Script_Global,
  Script_Param,
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
  op_selfID,
  op_lofsa = 0x39,
  op_lofss
} sci_opcodes;
 
extern opcode_format formats[128][4];

#endif
