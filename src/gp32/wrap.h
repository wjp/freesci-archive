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
#undef fopen

#undef stdin
#define stdin ((GPFILE *) 1)
#undef stdout
#define stdout ((GPFILE *) 2)
#undef stderr
#define stderr ((GPFILE *) 3)

#define fopen gp_fopen
#define fprintf gp_fprintf
#define vfprintf gp_vfprintf
#define fputs gp_fputs
#define perror printf
#undef getc
#define getc gp_fgetc
#define fgetc gp_fgetc
#define ungetc gp_ungetc
#define fgets gp_fgets
#undef ferror
#define ferror gp_ferror
#undef clearerr
#define clearerr gp_clearerr
#undef getchar
#undef putchar

int gp_fgetc(GPFILE *stream);
int gp_ferror(GPFILE *stream);
void gp_clearerr(GPFILE *stream);
int gp_fprintf(GPFILE *stream, const char *format, ...);
char *gp_fgets(char *s, int size, GPFILE *stream);

void usleep(unsigned long usec);

#endif /* WRAP_H */
