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

#define EXPLAIN_OPTION(longopt, shortopt, description) "  " longopt "\t" shortopt "\t" description "\n"

#else /* !HAVE_GETOPT_H */

#define EXPLAIN_OPTION(longopt, shortopt, description) "  " shortopt "\t" description "\n"

#endif /* !HAVE_GETOPT_H */


#ifdef _WIN32
#include <direct.h>
#define PATH_MAX 255
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define sleep Sleep
#define strcasecmp stricmp
#endif

#ifndef HAVE_SCHED_YIELD
#define sched_yield() sleep(1)
/* Neither NetBSD nor Win32 have this function, although it's in POSIX 1b */
#endif /* !HAVE_SCHED_YIELD */

static int quit = 0;
static state_t *gamestate; /* The main game state */


extern int _debugstate_valid;
extern int _debug_seeking;
extern int _debug_step_running;

char *requested_gfx_driver = NULL;

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
  static char input[1024] = "";
  int seeker;

  putchar('>');

  while (!strchr(input, '\n'))
    fgets(input, 1024, stdin);

  if (strlen(input))
    if (input[strlen(input)-1] == '\n');
    input[strlen(input)-1] = 0; /* Remove trailing '\n' */

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
  {"graphics", required_argument, 0, 'g'},
  {"version", no_argument, 0, 'v'},
  {"help", no_argument, 0, 'h'},
  {0,0,0,0}
};
#endif /* HAVE_GETOPT_H */

int
main(int argc, char** argv)
{
  resource_t *resource;
  config_entry_t *conf = NULL;
  int conf_entries; /* Number of config entries */
  int conf_nr; /* Element of conf to use */
  int i, c;
  FILE *console_logfile = NULL;
  int optindex = 0;
  char *gamedir = NULL;
  char startdir[PATH_MAX+1];
  char *game_name;
  sci_version_t cmd_version = 0;

  getcwd(startdir, PATH_MAX);

#ifdef HAVE_GETOPT_H
  while ((c = getopt_long(argc, argv, "vrhDd:V:g:", options, &optindex)) > -1)
#else /* !HAVE_GETOPT_H */
  while ((c = getopt(argc, argv, "vrhDd:V:g:")) > -1)
#endif /* !HAVE_GETOPT_H */
  {
    switch (c)
    {
    case 'r':
      script_debug_flag = 0;
      break;

    case 'D':
      script_debug_flag = 1;
      break;

    case 'd':
      if (gamedir)
	free(gamedir);

      gamedir = strdup (optarg);
      break;

    case 'V':
      {
        int major = *optarg - '0'; /* One version digit */
        int minor = atoi(optarg + 2);
        int patchlevel = atoi(optarg + 6);

        cmd_version = SCI_VERSION(major, minor, patchlevel);
      }
      break;

    case 'g':
      if (requested_gfx_driver)
	free(requested_gfx_driver);
      requested_gfx_driver = strdup(optarg);
      break;

    case '?':
      /* getopt_long already printed an error message. */
      return 0;

    case 0: /* getopt_long already did this for us */
      break;

    case 'v':
      printf("This is FreeSCI, version %s\n", VERSION);
      printf("Supported graphics drivers: ");

      i = 0;
      while (gfx_drivers[i]) {
	if (i != 0)
	  printf(", ");

	printf(gfx_drivers[i]->Name);

	i++;
      }
      printf("\n");
      printf("Supported sound drivers: ");
      i = 0;
      while (sfx_drivers[i]) {
	if (i != 0)
	  printf(", ");

	printf(sfx_drivers[i]->name);

	i++;
      }
      printf("\n");

      return 0;
    
    case 'h':
      printf("Usage: sciv [options] [game name]\n"
             "Run a Sierra SCI game.\n"
             "\n"
	     EXPLAIN_OPTION("--gamedir dir\t", "-ddir", "read game resources from dir")
	     EXPLAIN_OPTION("--run\t\t", "-r", "do not start the debugger")
	     EXPLAIN_OPTION("--sci-version\t", "-Vver", "set the version for sciv to emulate")
	     EXPLAIN_OPTION("--version\t", "ver", "display version number and exit")
	     EXPLAIN_OPTION("--debug\t", "-D", "start up in debug mode")
	     EXPLAIN_OPTION("--help\t", "-h", "display this help text and exit")
	     EXPLAIN_OPTION("--graphics gfx", "-ggfx", "use the 'gfx' graphics driver")
	     "\n"
	     "The game name, if provided, must be equal to a game name as specified in the\n"
	     "FreeSCI config file.\n"
	     "It is overridden by --gamedir.\n"
	     "\n"
             );
      return 0;

    default:
      return 1;
    }
  }

  printf("FreeSCI "VERSION" Copyright (C) 1999 Dmitry Jemerov, Christopher T. Lansdown,\n"
     "Sergey Lapin, Carl Muckenhoupt, Christoph Reichenbach, Magnus Reftel,\n"
     "Rink Springer\n"
	 "This program is free software. You can copy and/or modify it freely\n"
	 "according to the terms of the GNU general public license, v2.0\n"
	 "or any later version, at your option.\n"
	 "It comes with ABSOLUTELY NO WARRANTY.\n");


  if (optind == argc)
    game_name = NULL;
  else {
    game_name = argv[optind];

    conf_entries = config_init(&conf, NULL);

    conf_nr = 0;

    for (i = 1; i < conf_entries; i++)
      if (!strcasecmp(conf[i].name, game_name)) {
	conf_nr = i;
      }

    if (!gamedir)
      if (chdir(conf[conf_nr].resource_dir)) {
	if (conf_nr)
	  fprintf(stderr,"Error entering '%s' to load resource data\n", conf[conf_nr].resource_dir);
	else
	  fprintf(stderr,"Game '%s' isn't registered in your config file.\n", game_name);
	exit(1);
      }
  }

  if (gamedir)
    if (chdir (gamedir))
      {
	printf ("Error changing to game directory '%s'\n", gamedir);
	exit(1);
      }

  printf ("Loading resources...\n");
  if (i = loadResources(SCI_VERSION_AUTODETECT, 1)) {
    fprintf(stderr,"Error while loading resources: %s!\n", SCI_Error_Types[i]);
    exit(1);
  };
  printf("SCI resources loaded.\n");

  chdir (startdir);

  printf("Mapping instruments to General Midi\n");
  mapMIDIInstruments();

  con_init();
  con_init_gfx();

  con_hook_command(&c_quit, "quit", "", "console: Quits gracefully");
  con_hook_command(&c_die, "die", "", "console: Quits ungracefully");

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

  gamestate = malloc(sizeof(state_t));

  if (script_init_state(gamestate, cmd_version)) { /* Initialize game state */
    fprintf(stderr,"Script initialization failed. Aborting...\n");
    return 1;
  }

  gamestate->have_mouse_flag = 1; /* Assume that a pointing device is present */

  if (game_init(gamestate)) { /* Initialize */
    fprintf(stderr,"Game initialization failed: Aborting...\n");
    return 1;
  }

  if (!game_name)
    game_name = gamestate->game_name;

  if (!conf) { /* Unless the configuration has been read... */
    conf_entries = config_init(&conf, NULL);

    conf_nr = 0;

    for (i = 1; i < conf_entries; i++)
      if (!strcasecmp(conf[i].name, game_name)) {
	conf_nr = i;
      }
  }

  if (conf && conf[conf_nr].work_dir)
    if (chdir(conf[conf_nr].work_dir)) {
      fprintf(stderr,"Error entering working directory '%s'\n", conf[conf_nr].work_dir);
      exit(1);
    }

  if (conf[conf_nr].version)
    gamestate->version = conf[conf_nr].version;

  sci_color_mode = conf[conf_nr].color_mode;
  gamestate->gfx_driver = conf[conf_nr].gfx_driver;

  if (requested_gfx_driver) {
    int i = 0, j = -1;

    while ((j == -1) && gfx_drivers[i]) {
      if (g_strcasecmp(gfx_drivers[i]->Name, requested_gfx_driver) == 0)
	j = i;
      i++;
    }

    if (j == -1) {
      fprintf(stderr,"Unsupported graphics subsystem: %s\n", requested_gfx_driver);
      return 1;
    } 
    else gamestate->gfx_driver = gfx_drivers[j];
  }

  if (strlen (conf[conf_nr].debug_mode))
    set_debug_mode (gamestate, 1, conf[conf_nr].debug_mode);

  /* Now configure the graphics driver with the specified options */
  for (i = 0; i < conf[conf_nr].gfx_config_nr; i++)
    (conf[conf_nr].gfx_driver->Configure)(conf[conf_nr].gfx_config[i].option,
					  conf[conf_nr].gfx_config[i].value);

  gamestate->sfx_driver = sfx_drivers[0];

  if (gamestate->sfx_driver)
  {
    gamestate->sfx_driver->init(gamestate);
    sched_yield(); /* Specified by POSIX 1b. If it doesn't work on your
		   ** system, make up an #ifdef'd version of it above.
		   */
    gamestate->sfx_driver->get_event(gamestate); /* Get init message */
  }

  if (conf[conf_nr].console_log)
  {
    console_logfile = fopen (conf[conf_nr].console_log, "w");
    con_file = console_logfile;
  }

  /* initialize graphics */
  if (gamestate->gfx_driver->Initialize(gamestate, gamestate->pic)) { 
    fprintf(stderr,"Graphics initialization failed. Aborting...\n");
    exit(1);
  };

  printf("Emulating SCI version %d.%03d.%03d\n",
	 SCI_VERSION_MAJOR(gamestate->version),
	 SCI_VERSION_MINOR(gamestate->version),
	 SCI_VERSION_PATCHLEVEL(gamestate->version));

  game_run(&gamestate); /* Run the game */
  

  if (gamestate->sfx_driver)
    gamestate->sfx_driver->exit(gamestate); /* Shutdown sound daemon first */

  game_exit(gamestate);

  script_free_state(gamestate); /* Uninitialize game state */

  freeResources();

  config_free(&conf, conf_entries);

  if (console_logfile)
    fclose (console_logfile);

  chdir (startdir); /* ? */

#ifdef HAVE_FORK
  printf("Waiting for sound server to die...");
  wait(NULL); /* Wait for sound server process to die, if neccessary */
  printf(" OK.\n");
#endif

  gamestate->gfx_driver->Shutdown(gamestate); /* Shutdown graphics */

  free(gamestate);

  return 0;
}
