/***************************************************************************
 sciconsole.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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


#include <sciresource.h>
#include <engine.h>
#include <sound.h>
#include <uinput.h>
#include <console.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif /* HAVE_READLINE_HISTORY_H */
#endif /* HAVE_READLINE_READLINE_H */

static int quit = 0;

int
c_quit(state_t *s)
{
  quit = 1;
  return 0;
}

int
main(int argc, char** argv)
{
  int i;

  printf("console.c Copyright (C) 1999 Christoph Reichenbach\n"
	 "This program is free software. You can copy and/or modify it freely\n"
	 "according to the terms of the GNU general public license, v2.0\n"
	 "or any later version, at your option.\n"
	 "It comes with ABSOLUTELY NO WARRANTY.\n");

  if ((i = loadResources(SCI_VERSION_AUTODETECT, 1))) {
    fprintf(stderr,"SCI Error: %s!\n", SCI_Error_Types[i]);
    exit(-1);
  };
  printf("SCI resources loaded.\n");

  /*  if (loadObjects()) {
    fprintf(stderr,"Could not load objects\n");
    sciprintf("Could not load objects\n");
    }*/

  con_hook_command(&c_quit, "quit", "", "console: Quits");

  /*con_set_passthrough (1);*/ con_passthrough = 1;  /* enables all sciprintf data to be sent to stdout */
  sciprintf("FreeSCI, version "VERSION"\n");

#ifdef HAVE_READLINE_HISTORY_H
  using_history();
#endif /* HAVE_READLINE_HISTORY_H */

  while (!quit) {
    char *command;

#ifdef HAVE_READLINE_READLINE_H
    command = readline("$ ");
#else /* !HAVE_READLINE_READLINE_H */
    command = sci_malloc(1024);
    fgets(command, 1023, stdin);
    if (command [strlen (command)-1] == '\n')
      command [strlen (command)-1] = 0;
#endif /* !HAVE_READLINE_READLINE_H */

#ifdef HAVE_READLINE_HISTORY_H
    if (strlen(command))
      add_history(command);
#endif /* HAVE_READLINE_HISTORY_H */

    con_parse(NULL, command);

    free(command);
  }

  freeResources();
  return 0;
}
