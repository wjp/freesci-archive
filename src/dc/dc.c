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
#include <resource.h>
#include <dc.h>

#define FPRINTF_BUFSIZE 1024

struct tm *localtime(const time_t *time_p)
{
	struct tm *retval;
	retval = malloc(sizeof(struct tm));
	localtime_r(time_p, retval);
	return retval;
}

int remove(const char *pathname)
{
	/* Only works for files, but KOS doesn't support creating directories
	** yet.
	*/
	return fs_unlink(pathname);
}

/* vfprintf implementation is only correct for an output length of
** FPRINTF_BUFSIZE chars or less.
*/
int vfprintf(FILE *stream, const char *format, va_list ap)
{
	char *s = malloc(FPRINTF_BUFSIZE);

	vsnprintf(s, FPRINTF_BUFSIZE, format, ap);
	return fprintf(stream, "%s", s);
}

/* fprintf implementation is only correct for an output length of
** FPRINTF_BUFSIZE chars or less.
*/
int fprintf(FILE *stream, const char *format, ...)
{
	va_list argp;
	char *s = malloc(FPRINTF_BUFSIZE);
	
	int cnt;

	if ((stream == stdout) || (stream == stderr)) {
		va_start(argp, format);
		vsnprintf(s, FPRINTF_BUFSIZE, format, argp);
		va_end(argp);
		
		cnt = printf("%s", s);
	}
	else {
		va_start(argp, format);
		cnt = vsnprintf(s, FPRINTF_BUFSIZE, format, argp);
		if (cnt > FPRINTF_BUFSIZE) {
			sciprintf("Warning: fprintf() output truncated!\n");
			cnt = FPRINTF_BUFSIZE;
		}
		va_end(argp);
		
		cnt = fwrite(s, 1, cnt, stream);
	}

	free(s);
	return(cnt);
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
