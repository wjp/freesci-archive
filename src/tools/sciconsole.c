/***************************************************************************
 console.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

***************************************************************************/
/* Command console in text mode */


#include <resource.h>
#include <sound.h>
#include <uinput.h>
#include <console.h>
#include <readline/readline.h>
#include <readline/history.h>

static int quit = 0;
int sci_color_mode = 0; /* Required for linking */

int
c_quit(void)
{
  quit = 1;
  return 0;
}

int
main(int argc, char** argv)
{
  resource_t *resource;
  int i;

  printf("console.c Copyright (C) 1999 Christoph Reichenbach\n"
	 "This program is free software. You can copy and/or modify it freely\n"
	 "according to the terms of the GNU general public license, v2.0\n"
	 "or any later version, at your option.\n"
	 "It comes with ABSOLUTELY NO WARRANTY.\n");

  sci_color_mode = 0; /* Needed for correct linking */

  if (i = loadResources(SCI_VERSION_AUTODETECT, 1)) {
    fprintf(stderr,"SCI Error: %s!\n", SCI_Error_Types[i]);
    exit(-1);
  };
  printf("SCI resources loaded.\n");

  if (loadObjects()) {
    fprintf(stderr,"Could not load objects\n");
    sciprintf("Could not load objects\n");
  }

  cmdHook(&c_quit, "quit", "", "console: Quits");

  con_passthrough = 1; /* enables all sciprintf data to be sent to stdout */
  con_visible_rows = 1; /* Fool the functions into believing that we *have* a display */
  sciprintf("FreeSCI, version "VERSION"\n");

  using_history();

  while (!quit) {
    char *command;
    int oldlength;

    command = readline("$ ");

    if (strlen(command))
      add_history(command);

    cmdParse(command);

    free(command);
  }

  freeResources();
  return 0;
}
