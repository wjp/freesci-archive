/***************************************************************************
 dc.c Copyright (C) 2002 Walter van Niftrik


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

/* Implements needed functions that are missing from KOS */

#include <stdlib.h>

struct tm *localtime(const time_t *time_p)
{
	struct tm *retval;
	retval = malloc(sizeof(struct tm));
	localtime_r(time_p, retval);
	return retval;
}

int remove(const char *pathname)
{
	file_t d;
	d = fs_open(pathname, O_RDONLY | O_DIR);
	if (d) {
		/* pathname is a directory */

		fs_close(d);
		return fs_rmdir(pathname);
	}

	/* pathname is a file or does not exist */

	return fs_unlink(pathname);
}

/* vfprintf implementation is only correct for an output length of 10000 chars
** or less.
*/
int vfprintf(FILE *stream, const char *format, va_list ap)
{
	char s[10000];
	vsnprintf((char *) &s, 10000, format, ap);
	return fprintf(stream, "%s", s);
}

char *getcwd(char *buf, size_t size)
{
	const char *wd;
	if (buf) {
		wd = fs_getwd();
		if (strlen(wd)+1 > size) return NULL;
		strcpy(buf, wd);
		return buf;
	}
	return NULL;
}

char *strerror(int errn)
{
	return "Unknown error";
}
