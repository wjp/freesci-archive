/***************************************************************************
 Copyright (C) 2005 Christoph Reichenbach <reichenb@colorado.edu>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public Licence as
 published by the Free Software Foundaton; either version 2 of the
 Licence, or (at your option) any later version.

 It is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 merchantibility or fitness for a particular purpose. See the
 GNU General Public Licence for more details.

 You should have received a copy of the GNU General Public Licence
 along with this program; see the file COPYING. If not, write to
 the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.

***************************************************************************/

#define NEED_SCI_VERSIONS
#include <versions.h>
#include <engine.h>
#include <resource.h>
#include <ctype.h>
#include <games.h>
#include <exe.h>

#define VERSION_DETECT_BUF_SIZE 4096

static int
scan_file(char *filename, sci_version_t *version)
{
	char buf[VERSION_DETECT_BUF_SIZE];
	char result_string[10]; /* string-encoded result, copied from buf */
	int characters_left;
	int state = 0;
	/* 'state' encodes how far we have matched the version pattern
	**   "n.nnn.nnn"
	**
	**   n.nnn.nnn
	**  0123456789
	**
	** Since we cannot be certain that the pattern does not begin with an
	** alphanumeric character, some states are ambiguous.
	** The pattern is expected to be terminated with a non-alphanumeric
	** character.
	*/

	exe_file_t *f = exe_open(filename);

	if (!f)
		return 1;

	do {
		int i;
		int accept;

		characters_left = exe_read(f, buf, VERSION_DETECT_BUF_SIZE);

		for (i = 0; i < characters_left; i++) {
			const char ch = buf[i];
			accept = 0; /* By default, we don't like this character */

			if (isalnum((unsigned char) ch)) {
				accept = (state != 1
					  && state != 5
					  && state != 9);
			} else if (ch == '.') {
				accept = (state == 1
					  || state == 5);
			} else if (state == 9) {
				result_string[9] = 0; /* terminate string */

				if (!version_parse(result_string, version))
				{
					exe_close(f);
					return 0; /* success! */
				}

				/* Continue searching. */
			}

			if (accept)
				result_string[state++] = ch;
			else
				state = 0;

		}

	} while (characters_left == VERSION_DETECT_BUF_SIZE);

	exe_close(f);
	return 1; /* failure */
}

void
version_require_earlier_than(state_t *s, sci_version_t version)
{
	if (s->version_lock_flag)
		return;

	if (version <= s->min_version) {
		sciprintf("Version autodetect conflict: Less than %d.%03d.%03d was requested, but "
			  "%d.%03d.%03d is the current minimum\n",
			  SCI_VERSION_MAJOR(version), SCI_VERSION_MINOR(version), SCI_VERSION_PATCHLEVEL(version),
			  SCI_VERSION_MAJOR(s->min_version), SCI_VERSION_MINOR(s->min_version),
			  SCI_VERSION_PATCHLEVEL(s->min_version));
		return;
	}
	else if (version < s->max_version) {
		s->max_version = version -1;
		if (s->max_version < s->version)
			s->version = s->max_version;
	}
}


void
version_require_later_than(state_t *s, sci_version_t version)
{
	if (s->version_lock_flag)
		return;

	if (version > s->max_version) {
		sciprintf("Version autodetect conflict: More than %d.%03d.%03d was requested, but less than"
			  "%d.%03d.%03d is required ATM\n",
			  SCI_VERSION_MAJOR(version), SCI_VERSION_MINOR(version), SCI_VERSION_PATCHLEVEL(version),
			  SCI_VERSION_MAJOR(s->max_version), SCI_VERSION_MINOR(s->max_version),
			  SCI_VERSION_PATCHLEVEL(s->max_version));
		return;
	}
	else if (version > s->min_version) {
		s->min_version = version;
		if (s->min_version > s->version)
			s->version = s->min_version;
	}
}

int
version_parse(char *vn, sci_version_t *result)
{
	char *endptr[3];
	int major = strtol(vn, &endptr[0], 10);
	int minor = strtol(vn + 2, &endptr[1], 10);
	int patchlevel = strtol(vn + 6, &endptr[2], 10);

	if (endptr[0] != vn + 1 || endptr[1] != vn + 5
	    || *endptr[2] != '\0') {
		sciprintf("Warning: Failed to parse version string '%s'\n", vn);
		return 1;
	}

	*result = SCI_VERSION(major, minor, patchlevel);
	return 0;
}

int
version_detect_from_executable(sci_version_t *result)
{
	sci_dir_t dir;
	char *filename;
	char *masks[] = {"*.exe", "*.EXE", NULL};
	int masknr = 0;

	while (masks[masknr] != NULL) {
		sci_init_dir(&dir);

		filename = sci_find_first(&dir, masks[masknr]);

		while (filename) {
			if (!scan_file(filename, result)) {
				sci_finish_find(&dir);
				return 0;
			}

			filename = sci_find_next(&dir);
		}

		masknr++;
		sci_finish_find(&dir);
	}

	return 1;
}

#define HASHCODE_MAGIC_RESOURCE_000 0x55555555
#define HASHCODE_MAGIC_RESOURCE_001 0x00000001

char *  /* Original version by Solomon Peachy */
version_guess_from_hashcode(sci_version_t *result, guint32 *code)
{
	int i, len = 0;
	int fd = -1;
	int hash_code;
	guint8 buf[VERSION_DETECT_BUF_SIZE];

	if (IS_VALID_FD(fd = sci_open("resource.001", O_RDONLY|O_BINARY))) {
		hash_code = HASHCODE_MAGIC_RESOURCE_001;
	} else if (IS_VALID_FD(fd = sci_open("resource.000", O_RDONLY|O_BINARY))) {
		hash_code = HASHCODE_MAGIC_RESOURCE_000;
	} else {
		sciprintf("Warning: Could not find RESOURCE.000 or RESOURCE.001, cannot determine hash code\n");
		*code = 0;
		/* complete and utter failure */
		return NULL;
	}

	for (len = 1; len > 0; ) {
		len = read(fd, buf, VERSION_DETECT_BUF_SIZE);
		for (i = 0; i < len; i++)
			hash_code = (hash_code * 19) + *(buf + i);

		/* This is the string hashing algorithm used by Objective Caml 3.08; the general idea
		** of multiplying the previous hash code with a prime number between 5 and 23 appears
		** to be generally considered to be a "good" approach to exhausting the entire 32 bit
		** number space in a somewhat equal distribution. For large chunks of data, such as
		** SCI resource files, this should both perform well and yield a good distribution,
		** or at least that's what standard library designers have been assuming for quite a
		** while. */
	}

	close(fd);

	*code = hash_code;

	for (i = 0 ; sci_games[i].name ; i++) {
		if (sci_games[i].id == hash_code) {
			*result = sci_games[i].version;
			return sci_games[i].name;
		}
	}

	return NULL; /* Failed to find matching game */
}


#undef VERSION_DETECT_BUF_SIZE
