/***************************************************************************
 kemu_old.c Copyright (C) 2002 Christoph Reichenbach


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
/* Emulation code for old kernel functions */

#include <engine.h>
#include <kernel_compat.h>

#define EMU_HEAP_START_ADDR 1000

reg_t
kFsciEmu(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	if (!s->kfunct_emu_table[funct_nr]) {
		SCIkwarn(SCIkERROR, "Attempt to emulate unknown kernel function %x\n",
			 funct_nr);
		return NULL_REG;
	} else {
		heap_ptr argp = EMU_HEAP_START_ADDR; /* arguments go here */
		heap_ptr datap = argp + argc * 2; /* copied stuff goes here */
		int i;

		for (i = 0; i < argc; i++) {
			int emu_value = datap; /* Value we'll pass to the function */
			int data_occupied = 0;
			if (argv[i].segment) {
				SCIkwarn(SCIkERROR, "Argument %d for kcall %x was non-value,"
					 " cannot emulate\n", i, funct_nr);
				return NULL_REG;
			} else /* numeric value */
				emu_value = argv[i].offset;

			PUT_HEAP(argp, argv[i].offset);
			argp += 2;

			datap += data_occupied; /* Step over last block we wrote */
		}

		s->kfunct_emu_table[funct_nr](s, funct_nr, argc, EMU_HEAP_START_ADDR);
		return make_reg(0, s->acc);
	}
}
