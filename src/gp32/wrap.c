/***************************************************************************
 wrap.c Copyright (C) 2004 Walter van Niftrik


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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include "wrap.h"

/* This file contains GP32 function wrappers.
**
** TODO:
**
** - More accurate implementations of open(), gp_ferror(), gp_clearerr().
** - Enable the use of relative pathnames in fopen() and family.
** - Implement several file routines, such as fprintf(), fputc() etc.
** - Stop using (GPFILE *) as file descriptors (?).
** - Implement errno handling (?).
** - Put wrappers in newlib's syscalls.c instead (?).
*/

int freesci_main(int argc, char** argv);

/* <unistd.h> */

/* Paths that indicate a directory should always end in '\\'. */
static char cur_dir[256] = "dev0:\\";

static char *
first_path_elem(char **src)
/* Returns first path element
** Parameters: (char **) src: The path to process, first char must be '\\'.
** Returns   : (char *) The first path element on success, NULL on error.
**                        Memory will be allocated for this string, the
**                        caller is responsible for freeing it. On success
**                        src is adjusted to point to the first char that
**                        follows the returned element in the path (this
**                        is either a '\\' or a '\0').
*/
{
	char *bs;
	char *retval;

	/* Look for second backslash. */
	bs = strchr(*src + 1, '\\');

	if (!bs)
		/* Not found, current element extends to end of path. */
		bs = *src + strlen(*src);

	/* Create string of path element (including leading backslash). */
	retval = malloc(bs - *src + 1);
	strncpy(retval, *src, bs - *src);
	retval[bs - *src] = '\0';

	/* Set src to first char that follows the element. */
	*src = bs;

	return retval;
}

static char *
cano_path(const char *src)
/* Canonicalizes an absolute pathname. Resolves '\\.', '\\..' and removes
** extra backslashes.
** Parameters: (char *) src: The absolute path to canonicalize.
** Returns   : (char *) The canonicalized pathname on success, NULL on
**                        error.
*/
{
	char *chunk, *retval;

	/* Look for colon. */
	char *str = strchr(src, ':');
	if (!str)
		/* src is not an absolute pathname. */
		return NULL;

	/* Copy device indicator to retval. */
	retval = strndup(src, str - src + 1);

	/* str iterates over path elements. */
	str++;

	while ((strlen(str) > 0) && (chunk = first_path_elem(&str))) {
		/* Examine next chunk. */
		if (!strcmp(chunk, "\\..")) {
			/* If "\\.." is encountered remove last path
			** element from retval.
			*/
			char *bs = strrchr(retval, '\\');
			if (!bs) {
				/* No elements found, error. */
				free(chunk);
				free(retval);
				return NULL;
			}
			*bs = '\0';
		}
		else if (strcmp(chunk, "\\.") && (strcmp(chunk, "\\") ||
		  !strlen(str))) {
			/* "\\." elements are ignored. A "\\" element is
			** only added when it's the last element in the
			** string. All other elements are added.
			*/
			retval = realloc(retval, strlen(retval) +
			  strlen(chunk) + 1);
			strcat(retval, chunk);
		}
		free(chunk);
	}

	return retval;
}

static char *
abs_path(const char *path, int dir)
/* Converts a pathname to a canonicalized absolute pathname.
** Parameters: (char *) src: The absolute path to process.
**             (int) dir: 0 -> path indicates a file, 1 -> path indicates
**                a directory.
** Returns   : (char *) The canonicalized absolute pathname on success,
**                        NULL on error.
*/
{
	int len = strlen(path);
	char *cano;

	/* If path ends with a backslash, don't add one. */
	if (path[len - 1] == '\\')
		dir = 0;

	if (strchr(path, ':')) {
		/* Absolute path. */
		char *new_dir = malloc(len + dir + 1);

		strcpy(new_dir, path);
		/* If path is a directory, add a backslash if needed. */
		if (dir)
			strcat(new_dir, "\\");

		cano = cano_path(new_dir);
		free(new_dir);
	}
	else {
		/* Relative path. */
		char *new_dir = malloc(len + dir + strlen(cur_dir) + 1);

		/* Concatenate relative path with current directory. */
		strcpy(new_dir, cur_dir);
		strcat(new_dir, path);
		/* If path is a directory, add a backslash if needed. */
		if (dir)
			strcat(new_dir, "\\");

		cano = cano_path(new_dir);
		free(new_dir);
	}

	return cano;
}

int
chdir(const char *path)
{
	char *cano = abs_path(path, 1);

	if (cano) {
		if (strlen(cano) < 256) {
			strcpy(cur_dir, cano);
			free(cano);
			return 0;
		}

		free(cano);
		return -1;
	}

	return -1;
}

char *
getcwd(char *buf, size_t size)
{
	if (strlen(cur_dir) + 1 > size)
		return NULL;

	strcpy(buf, cur_dir);
	return buf;
}

ssize_t
read(int fd, void *buf, size_t count)
{
	return smc_fread(buf, 1, count, (GPFILE *) fd);
}

off_t
lseek(int fildes, off_t offset, int whence)
{
	if (fseek((GPFILE *) fildes, offset, whence))
		return -1;

	return ftell((GPFILE *) fildes);
}

int
close(int fd)
{
	if (!smc_fclose((GPFILE *) fd))
		return 0;

	return -1;
}

void
usleep(unsigned long usec) {
	int start = gp_getRTC();
	int add = usec / 15625;
	if (!add)
		add = 1;
	while (gp_getRTC() < start + add);
}

unsigned int
sleep(unsigned int seconds) {
	usleep(seconds * 1000000);
	return 0;
}

/* <sys/stat.h> */

int
mkdir(const char *pathname, mode_t mode)
{
	char *cano = abs_path(pathname, 1);

	if (cano) {
		if (!smc_createdir(cano)) {
			free(cano);
			return -1;
		}
		free(cano);
		return 0;
	}

	return -1;
}

/* <fcntl.h> */

int
open(const char *pathname, int flags, ...)
{
	GPFILE *file;
	char *mode;
	char *cano = abs_path(pathname, 0);

	if (flags & O_RDONLY)
		mode = "r";
	else if (flags & O_WRONLY)
		mode = "w";
	else
		/* O_RDWR not supported, open read-only. */
		mode = "r";

	file = smc_fopen(cano, mode);

	free(cano);

	if (file)
		return (int) file;

	return -1;
}

int
creat(const char *pathname, mode_t mode) {
	return open(pathname, O_CREAT|O_WRONLY|O_TRUNC, mode);
}

/* <stdio.h> */

int
gp_fgetc(GPFILE *stream)
{
	/* Not implemented in SMC library. Read a single character with
	** smc_fread() instead.
	*/
	unsigned char retval;

	if (!smc_fread(&retval, 1, 1, stream))
		return EOF;

	return retval;
}

int
gp_ferror(GPFILE *stream)
{
	/* Not implemented in SMC library. Return 'no error'. */
	return 0;
}

int
getchar()
{
	/* Not supported. */
	return EOF;
}

void
gp_clearerr(GPFILE *stream)
{
	/* Not implemented in SMC library. Currently ignored. */
}

static void
gp_print(char *msg)
/* Prints a string to the framebuffer and waits for a key. Formatting
** characters are ignored.
** Parameters: (char *) msg: The string to print.
** Returns   : (void)
*/
{
	int len = 0;

	gp_clearFramebuffer16((unsigned short *) FRAMEBUFFER, 0xFFFF);

	while ((len * 40 < strlen(msg)) && (len < 30)) {
		int left = strlen(msg) - len * 40;
		if (left > 40)
			left = 40;
		gp_drawString(0, len * 8, (left > 40 ? 40 : left), msg +
		  (len * 40), 0xF800, (unsigned short *) FRAMEBUFFER);
		len++;
	}

	while (!gp_getButton());
	while (gp_getButton());
}

int
printf(const char *format, ...)
{
	char *str;
	int len;

	va_list argp;
	va_start(argp, format);

	/* Get needed buffer size. */
	len = vsnprintf(NULL, 0, format, argp) + 1;
	str = malloc(len);
	vsnprintf(str, len, format, argp);

	va_end(argp);

#ifdef GP32_DEBUG
	gp_print(str);
#endif
	return len;
}

void
gp32_main()
{
	char *argv[] = { "freesci.fxe", NULL };

	/* Set CPU speed 133Mhz. */
	gp_setCpuspeed(133);

	/* Start realtime clock. */
	gp_initRTC();

	/* Call freesci's main(). */
	exit(freesci_main(1, argv));
}
