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

#include <versions.h>
#include <engine.h>
#include <resource.h>
#include <ctype.h>
#include <games.h>

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

sci_version_t
version_parse(char *vn)
{
  int major = *vn - '0'; /* One version digit */
  int minor = atoi(vn + 2);
  int patchlevel = atoi(vn + 6);

  return SCI_VERSION(major, minor, patchlevel);
}


#define VERSION_DETECT_BUF_SIZE 4096
int
version_detect_from_executable(sci_version_t *result)
{
	char buf[VERSION_DETECT_BUF_SIZE];
	char result_string[10]; /* string-encoded result, copied from buf */
	int characters_left;
	int finished = 0;

	int state = 0;
	/* 'state' encodes how far we have matched the version pattern
	**   "n.nnn.nnn"
	** so far:
	**
	**   n.nnn.nnn
	**  0123456789
	**  456789
	**  
	**
	** Since we cannot be certain that the pattern does not begin with an
	** alphanumeric character, some states are ambiguous.
	** The pattern is expected to be terminated with a '\0' character.
	*/

	/* First try the new filename */
	int fh = sci_open("sciv.exe", O_RDONLY|O_BINARY);

	if (!IS_VALID_FD(fh))
		fh = sci_open("sierra.exe", O_RDONLY|O_BINARY);

	if (!IS_VALID_FD(fh))
		return 0;

	do {
		int i;
		int accept;

		characters_left = read(fh, buf, VERSION_DETECT_BUF_SIZE);

		for (i = 0; i < characters_left; i++) {
			const char ch = buf[i];
			accept = 0; /* By default, we don't like this character */

			/* Warning: This doesn't accomodate for "strange" SCI1+ version
			** numbers. My perception is that we can't handle those anyway, since
			** the corresponding files appear to be compressed.  */
			if (isdigit((unsigned char) ch)) {
				accept = (state != 1
					  && state != 5
					  && state != 9);
			} else if (ch == '.') {
				accept = (state == 1
					  || state == 5);
			} else if (state == 9
				   && ch == '\0') {

				result_string[9] = 0; /* terminate string */
				*result = version_parse(result_string);
				close (fh);

				return 1; /* success! */
			}

			if (accept)
				result_string[state++] = ch;
			else
				state = 0;

		}

	} while (characters_left == VERSION_DETECT_BUF_SIZE);

	close(fh);
	return 0; /* failure */
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
	sci_version_t version = 0;

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
