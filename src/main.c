/***************************************************************************
 main.c Copyright (C) 1999 Christoph Reichenbach


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

#include <resource.h>
#include <sound.h>
#include <uinput.h>
#include <console.h>
#include <graphics.h>
#include <sci_conf.h>
#include <readline/readline.h>
#include <readline/history.h>

static int quit = 0;
static state_t gamestate; /* The main game state */



int
c_quit(void)
{
  exit(0); /* Force exit */
}


char *old_input = NULL;

char *
get_readline_input(void)
{
  char *input = readline("> ");
  if (strlen(input) == 0) {
    free (input);
  } else {
    add_history(input);
    if (old_input) {
      free(old_input);
    }
    old_input = input;
  }

  return old_input? old_input : "";
}


int
main(int argc, char** argv)
{
  resource_t *resource;
  config_entry_t conf;
  int i;

  printf("FreeSCI "VERSION" Copyright (C) 1999 Christopher T. Lansdown, Sergey Lapin,\n"
	 "Carl Muckenhoupt, Christoph Reichenbach, Magnus Reftel\n"
	 "This program is free software. You can copy and/or modify it freely\n"
	 "according to the terms of the GNU general public license, v2.0\n"
	 "or any later version, at your option.\n"
	 "It comes with ABSOLUTELY NO WARRANTY.\n");

  graphInit();

  sci_color_mode = SCI_COLOR_DITHER256;

  if (i = loadResources(SCI_VERSION_AUTODETECT, 1)) {
    fprintf(stderr,"SCI Error: %s!\n", SCI_Error_Types[i]);
    exit(-1);
  };
  printf("SCI resources loaded.\n");

  printf("Sound output interface: %s\n",
	 SCI_sound_interfaces[initSound(SCI_SOUND_INTERFACE_AUTODETECT)]);

  printf("Mapping instruments to General Midi\n");
  mapMIDIInstruments();

  cmdHook(&c_quit, "quit", "", "console: Quits");

  con_passthrough = 1; /* enables all sciprintf data to be sent to stdout */
  con_visible_rows = 1; /* Fool the functions into believing that we *have* a display */
  sciprintf("FreeSCI, version "VERSION"\n");

  using_history(); /* Activate history for readline */

  _debug_get_input = get_readline_input; /* Use readline for debugging input */


  if (script_init_state(&gamestate)) { /* Initialize game state */
    fprintf(stderr,"Initialization failed. Aborting...\n");
    return 1;
  }
  gamestate.have_mouse_flag = 0; /* Assume that no pointing device is present */

  if (graphOpen(&gamestate)) { /* initialize graphics */
    fprintf(stderr,"Graphics initialization failed. Aborting...\n");
    exit(1);
  };

  game_init(&gamestate); /* Initialize */

  config_init(&conf, gamestate.game_name, NULL);
  gamestate.version = conf.version;
  sci_color_mode = conf.color_mode;

  printf("Emulating SCI version %d.%03d.%03d\n",
	 gamestate.version >> 20,
	 (gamestate.version >> 10) & 0x3ff,
	 gamestate.version & 0x3ff);

  game_run(&gamestate); /* Run the game */
  game_exit(&gamestate);

  graphClose(&gamestate); /* Close graphics */

  script_free_state(&gamestate); /* Uninitialize game state */

  freeResources();

  graphExit();

  return 0;
}
