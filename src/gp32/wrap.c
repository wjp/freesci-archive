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

int freesci_main(int argc, char** argv);

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

	while (gp_getButton());
	while (!gp_getButton());
}

int stdout_write(char *buf, int len)
{
	return len;
}

int stderr_write(char *buf, int len)
{
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
