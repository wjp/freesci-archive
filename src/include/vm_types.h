/***************************************************************************
 vm_types.h Copyright (C) 2002 Christoph Reichenbach


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

    Christoph Reichenbach (CR) <jameson@linuxgames.com>

***************************************************************************/


#ifndef _SCI_VM_TYPES_H_
#define _SCI_VM_TYPES_H_

#define SCI_REG_SIZE 16;
#define SCI_SEG_SIZE 16;

typedef int seg_id_t; /* Segment ID type */

struct _state; /* engine.h */

#ifdef S_SPLINT_S
/* Splint can't deal with packed notation */
typedef struct {
	seg_id_t segment;
	unsigned int offset;
} reg_t;
#else
typedef struct {
	seg_id_t segment	: SCI_SEG_SIZE;
	unsigned int offset	: SCI_REG_SIZE;
} reg_t;
#endif

#define PREG "%04x:%04x"
#define PRINT_REG(r) (0xffff) & (unsigned) (r).segment, (unsigned) (r).offset

typedef reg_t *stack_ptr_t; /* Stack pointer type */
typedef int selector_t; /* Selector ID */
#define NULL_SELECTOR -1

#define PSTK "ST:%04x"
#define PRINT_STK(v) (unsigned) (v - s->stack_base)

static inline reg_t
make_reg(int segment, int offset)
{
	reg_t r;
	r.offset = offset;
	r.segment = segment;
	return r;
}

#define IS_NULL_REG(r) (!((r).offset || (r).segment))
#define REG_EQ(a, b) (((a).offset = (b).offset) && ((a).segment == (b).segment))
#define NULL_REG_INITIALIZER {0, 0}
static reg_t NULL_REG = NULL_REG_INITIALIZER;


#endif /* !_SCI_VM_TYPES_H_ */
