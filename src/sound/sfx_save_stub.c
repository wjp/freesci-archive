/***************************************************************************
 sfx_save_stub.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/

/* -- THIS IS JUST A STUB -- */
/* Fix sfx_save.c for re-inclusion to make it work again */

#include <stdio.h>
#include <sound.h>
#include <soundserver.h>
#include <sci_memory.h>

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#define getcwd _getcwd
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif  /* !O_BINARY */

/* Sound state saving reference implementation */
int
soundsrv_save_state(FILE *debugstream, char *dir, sound_server_state_t *sss)
{
	return 1;
}


/* Sound state restore complement for the saving reference implementation */
int
soundsrv_restore_state(FILE *debugstream, char *dir, sound_server_state_t *sss)
{
	return 1;
}
