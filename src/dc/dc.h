/***************************************************************************
 dc.h Copyright (C) 2002 Walter van Niftrik


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

#ifndef __DC_H
#define __DC_H

#include <stdio.h>

/* Constants missing in KOS's errno.h */

#ifndef EINTR
#define EINTR 4
#endif

/* Functions implemented in dc_save.c */

char *dc_get_first_vmu();
char *dc_get_cat_name();
void dc_delete_save_files(char *);
int dc_delete_temp_file();
int dc_retrieve_savegame(char *, int);
int dc_store_savegame(char *, char *, int);
int dc_retrieve_mirrored(char *);
int dc_store_mirrored(char *);

/* Functions missing from KOS */

/* Implemented in dc.c */
struct tm *localtime(const time_t *);
int remove(const char *);
int vfprintf(FILE *, const char *, va_list);
#undef fprintf
int fprintf(FILE *, const char *, ...);
char *getcwd(char *, size_t);
char *strerror(int);

/* Implemented in bsearch.c */
void *bsearch(const void *, const void *, size_t, size_t,
	int (*)(const void *, const void *));

/* Implemented in snprintf.c */
int snprintf(char *, size_t, const char *, ...);
int vsnprintf(char *, size_t, const char *, va_list);

#endif  /* __DC_H */
