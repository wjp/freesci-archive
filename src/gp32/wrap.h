/***************************************************************************
 wrap.h Copyright (C) 2004 Walter van Niftrik


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

   Walter van Niftrik <w.f.b.w.v.niftrik@stud.tue.nl>

***************************************************************************/

#ifndef WRAP_H
#define WRAP_H

#define DIR GPDIR
#define byte gp32_byte
#define word gp32_word
#include <gp32.h>
#undef byte
#undef word
#undef FILE
#undef DIR
#undef size_t
#undef fopen
#undef fread
#undef fwrite
#undef fclose
#undef ftell
#undef rewind
#undef fseek

typedef struct {
	char name[16];
} DIR;

/* Disables FAT for volume vol. */
int smNoFATUpdate(const char* vol);

/* Enables FAT for volume vol and updates FAT. */
int smFATUpdate(const char* vol);

/* Function declaration missing from mirkoSDK. */
short gp_initFramebufferBP(void *addr,u16 bitmode,u16 refreshrate);

/* The renamed main() in main.c. */
int freesci_main(int argc, char** argv);

#endif /* WRAP_H */
