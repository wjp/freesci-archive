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
#include <kdebug.h>
#include <vm.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif /* HAVE_READLINE_HISTORY_H */
#endif /* HAVE_READLINE_READLINE_H */

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */


#ifdef _WIN32
#include <direct.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define sleep Sleep
#endif

static int quit = 0;
static state_t gamestate; /* The main game state */


extern int _debugstate_valid;
extern int _debug_seeking;
extern int _debug_step_running;

int
c_quit(state_t *s)
{
  script_abort_flag = 1; /* Terminate VM */
  _debugstate_valid = 0;
  _debug_seeking = 0;
  _debug_step_running = 0;
  return 0;
}

int
c_die(state_t *s)
{
  exit(0); /* Die */
}


char *old_input = NULL;

#ifdef HAVE_READLINE_READLINE_H
char *
get_readline_input(void)
{
  char *input = readline("> ");
  if (strlen(input) == 0) {
    free (input);
  } else {

#ifdef HAVE_READLINE_HISTORY_H
    add_history(input);
#endif /* HAVE_READLINE_HISTORY_H */

    if (old_input) {
      free(old_input);
    }
    old_input = input;
  }

  return old_input? old_input : "";
}
#endif /* HAVE_READLINE_READLINE_H */


char *
get_gets_input(void)
{
  static char input[1024];

  puts("> ");
  fgets(input, 1024, stdin);

  if (strlen(input) == 0) {
    return old_input? old_input : "";
  }

  if (old_input)
    free(old_input);

  old_input = malloc(1024);
  strcpy(old_input, input);
  return input;
}

#ifdef HAVE_GETOPT_H
static struct option options[] = {
  {"gamedir", required_argument, 0, 'd'},
  {"run", no_argument, &script_debug_flag, 0 },
  {"debug", no_argument, &script_debug_flag, 1 },
  {"sci-version", required_argument, 0, 'V'},
  {"version", no_argument, 0, 'v'},
  {"help", no_argument, 0, 'h'},
  {0,0,0,0}
};
#endif /* HAVE_GETOPT_H */

int
main(int argc, char** argv)
{
  resource_t *resource;
  config_entry_t conf;
  int i, c;
  FILE *console_logfile = NULL;
  int optindex = 0;
  char gamedir [256], startdir [256];
  sci_version_t cmd_version = 0;


  strcpy (gamedir, ".");

#ifdef HAVE_GETOPT_H
  while ((c = getopt_long(argc, argv, "rd:v:", options, &optindex)) > -1)
#else /* !HAVE_GETOPT_H */
  while ((c = getopt(argc, argv, "rd:v:")) > -1)
#endif /* !HAVE_GETOPT_H */
  {
    switch (c)
    {
    case 'r':
      script_debug_flag=0;
      break;

    case 'd':
      strcpy (gamedir, optarg);
      break;

    case 'V':
      {
        int major = *optarg - '0'; /* One version digit */
        int minor = atoi(optarg + 2);
        int patchlevel = atoi(optarg + 6);

        cmd_version = SCI_VERSION(major, minor, patchlevel);
      }
      break;

    case 0: /* getopt_long already did this for us */
    case '?':
      /* getopt_long already printed an error message. */
      break;

    case 'v':
      printf("%s\n", VERSION);
      return 0;
    
    case 'h':
      printf("Usage: sciv [OPTION]...\n"
             "Run a sierra SCI game.\n"
             "\n"
             "  --gamedir dir	-ddir  read game resources from dir\n"
             "  --run		-r     do not start the debugger\n"
             "  --sci-version	-Vver  set the version of sciv to emulate\n"
             "  --version	-v     display version information and exit\n"
	     "  --debug                Start up with the debugger enabled\n"
             "  --help	        -h     display this help text and exit\n"
             );
      return 0;

    default:
      return 1;
    }
  }

  printf("FreeSCI "VERSION" Copyright (C) 1999 Dmitry Jemerov, Christopher T. Lansdown,\n"
	 "Sergey Lapin, Carl Muckenhoupt, Christoph Reichenbach, Magnus Reftel\n"
	 "This program is free software. You can copy and/or modify it freely\n"
	 "according to the terms of the GNU general public license, v2.0\n"
	 "or any later version, at your option.\n"
	 "It comes with ABSOLUTELY NO WARRANTY.\n");

  
  sci_color_mode = SCI_COLOR_DITHER256;

  getcwd (startdir, sizeof (startdir)-1);
  if (chdir (gamedir))
  {
    printf ("Error changing to game directory %s\n", gamedir);
    exit(1);
  }

  if (i = loadResources(SCI_VERSION_AUTODETECT, 1)) {
    fprintf(stderr,"SCI Error: %s!\n", SCI_Error_Types[i]);
    exit(1);
  };
  printf("SCI resources loaded.\n");

  printf("Mapping instruments to General Midi\n");
  mapMIDIInstruments();

  cmdHook(&c_quit, "quit", "", "console: Quits gracefully");
  cmdHook(&c_die, "die", "", "console: Quits ungracefully");

  con_passthrough = 1; /* enables all sciprintf data to be sent to stdout */
  con_visible_rows = 1; /* Fool the functions into believing that we *have* a display */
  sciprintf("FreeSCI, version "VERSION"\n");

#ifdef HAVE_READLINE_HISTORY_H
  using_history(); /* Activate history for readline */
#endif /* HAVE_READLINE_HISTORY_H */

#ifdef HAVE_READLINE_READLINE_H
  _debug_get_input = get_readline_input; /* Use readline for debugging input */
#else /* !HAVE_READLINE_READLINE_H */
  _debug_get_input = get_gets_input; /* Use gets for debug input */
#endif /* !HAVE_READLINE_READLINE_H */


  if (script_init_state(&gamestate, cmd_version)) { /* Initialize game state */
    fprintf(stderr,"Script initialization failed. Aborting...\n");
    return 1;
  }
  gamestate.have_mouse_flag = 1; /* Assume that a pointing device is present */

  if (game_init(&gamestate)) { /* Initialize */
    fprintf(stderr,"Game initialization failed: Aborting...\n");
    return 1;
  }

  chdir (startdir);
  config_init(&conf, gamestate.game_name, NULL);
  chdir (gamedir);

  if (conf.version!=SCI_VERSION_LAST_SCI0) gamestate.version = conf.version;
  sci_color_mode = conf.color_mode;
  gamestate.gfx_driver = conf.gfx_driver;
  if (strlen (conf.debug_mode))
    set_debug_mode (&gamestate, 1, conf.debug_mode);

  gamestate.sfx_driver = sfx_drivers[0];
  
  if (gamestate.sfx_driver)
  {
    gamestate.sfx_driver->init(&gamestate);
    sleep(1);
    gamestate.sfx_driver->get_event(&gamestate); /* Get init message */
  }

  if (conf.console_log)
  {
    console_logfile = fopen (conf.console_log, "w");
    con_file = console_logfile;
  }

  /* initialize graphics */
  if (gamestate.gfx_driver->Initialize(&gamestate, gamestate.pic)) { 
    fprintf(stderr,"Graphics initialization failed. Aborting...\n");
    exit(1);
  };

  printf("Emulating SCI version %d.%03d.%03d\n",
	 gamestate.version >> 20,
	 (gamestate.version >> 10) & 0x3ff,
	 gamestate.version & 0x3ff);

  game_run(&gamestate); /* Run the game */

  if (gamestate.sfx_driver)
    gamestate.sfx_driver->exit(&gamestate); /* Shutdown sound daemon first */

  game_exit(&gamestate);

  gamestate.gfx_driver->Shutdown(&gamestate); /* Close graphics */

  script_free_state(&gamestate); /* Uninitialize game state */

  freeResources();


  if (console_logfile)
    fclose (console_logfile);

  chdir (startdir);

  return 0;
}
