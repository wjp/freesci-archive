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
#include <string.h>
#include <fcntl.h>
#include "wrap.h"

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

int
creat(const char *pathname, mode_t mode)
{
	return open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

void
gp_print(char *msg, int mlen)
/* Prints a string to the framebuffer and waits for a key. Formatting
** characters are ignored.
** Parameters: (char *) msg: The string to print.
**             (int) mlen: The length of msg
** Returns   : (void)
*/
{
	int len = 0;

	gp_clearFramebuffer16((unsigned short *) FRAMEBUFFER, 0xFFFF);

	while ((len * 40 < mlen) && (len < 30)) {
		int left = mlen - len * 40;
		if (left > 40)
			left = 40;
		gp_drawString(0, len * 8, left, msg + (len * 40), 0xF800,
		              (unsigned short *) FRAMEBUFFER);
		len++;
	}

	while (gp_getButton());
	while (!gp_getButton());
}

int write_stdout(char *buf, int len)
{
	return len;
}

int write_stderr(char *buf, int len)
{
	return len;
}

int read_stdin(char *buf, int len)
{
	return 0;
}

void
gp32_main()
{
	char *argv[] = { "freesci.fxe", NULL };

	/* Set CPU speed to 133Mhz. */
	gp_setCpuspeed(133);

	/* Call freesci's main(). */
	exit(freesci_main(1, argv));
}
