/***************************************************************************
 config.l (C) 1999 Christoph Reichenbach, TU Darmstadt


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

%{
#include <engine.h>
#include <gfx_system.h>
#include <gfx_tools.h>
#include <gfx_resource.h>
#include <gfx_drivers_list.h>
#include <sci_conf.h>
#ifndef _DOS
# include <glib.h>
#endif

#ifdef _MSC_VER
#include <ctype.h>
#include <direct.h>

#define strcasecmp stricmp
#define PATH_MAX 255

#endif

config_entry_t *conf;
int cur_section=0; /* Size-1 and current section in conf */
char *exported_conf_path; /* Path which the config file was found in */
int dospath; /* Use dos-style paths? */

static int
_parse_color_mode(char *s); /* Parses the color mode string */

static gfx_driver_t *
_parse_gfx_driver(char *s); /* Parses a graphics driver */


%}

DIGIT [0-9]
PARAMTOKEN [[:alpha:]][[:alnum:]_]*
SCIVERSION {DIGIT}"."{DIGIT}{3}"."{DIGIT}{3}

%%

"["[_[:alnum:]]+"]" {
  ++yytext; /* Get over opening bracket */

  ++cur_section; /* Start new section */

  /* Create new entry... */
  conf = realloc(conf, sizeof(config_entry_t) * (cur_section + 1));

  /* ...and initialize it */
  memcpy(&(conf[cur_section]), &(conf[0]), sizeof(config_entry_t));
  if (conf[0].console_log)
    conf[cur_section].console_log = strdup (conf[0].console_log);
  /* Copy the gfx system init strings */
  if (conf[cur_section].gfx_config) {
    conf[cur_section].gfx_config = malloc(sizeof(gfx_option_t *) * conf[0].gfx_config_nr);
    memcpy(&(conf[cur_section].gfx_config), &(conf[0].gfx_config),
	   sizeof(gfx_option_t *) * conf[0].gfx_config_nr);
  }

  while (isspace(*yytext))
    yytext++;

  conf[cur_section].name = strdup(yytext);
  conf[cur_section].name[strlen(yytext)-1] = 0; /* Terminate string at "]" */
  conf[cur_section].resource_dir = strdup(".");

  if (dospath)
    conf[cur_section].work_dir = strdup(exported_conf_path);
  else {
    char *tmp = malloc(strlen(exported_conf_path) + 1 + strlen(yytext));
    strcpy(tmp, exported_conf_path);
    strcat(tmp, "/");
    strcat(tmp, conf[cur_section].name);

    conf[cur_section].work_dir = tmp;
    scimkdir(tmp, 0700); /* Make sure that the directory exists. */
    /* This will be checked later, for the current game. */
  }
}

color_mode[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
  yytext = strchr(yytext, '=') + 1;

  while (isspace(*yytext))
    yytext++;
  conf[cur_section].color_mode = _parse_color_mode(yytext);
}

version[[:blank:]]*"="[[:blank:]]*{SCIVERSION} {
  yytext = strchr(yytext, '=') + 1;

  while (isspace(*yytext))
    yytext++;

  conf[cur_section].version = version_parse(yytext);
}

console_log[[:blank:]]*"="[[:blank:]]*.+  {
  yytext = strchr (yytext, '=') + 1;

  while (isspace(*yytext))
    yytext++;

  if (conf[cur_section].console_log)
    free (conf[cur_section].console_log);
  conf[cur_section].console_log = (char *) malloc (strlen (yytext)+1);
  strcpy (conf[cur_section].console_log, yytext);
}

resource_dir[[:blank:]]*"="[[:blank:]]*.+  if (cur_section) {
  yytext = strchr(yytext, '=') + 1;
  while (isspace(*yytext))
    yytext++;

  free(conf[cur_section].resource_dir);

  conf[cur_section].resource_dir = strdup(yytext);
}


gfx_driver[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
  yytext = strchr(yytext, '=') + 1;

  while (isspace(*yytext))
    yytext++;

  conf[cur_section].gfx_driver = _parse_gfx_driver(yytext);
}

debug_mode[[:blank:]]*"="[::blank:]]*.+  {
  yytext = strchr(yytext, '=') + 1;

  while (isspace(*yytext))
    yytext++;

  strcpy (conf[cur_section].debug_mode, yytext);
}

[[:alnum:]]+"."[[:alnum:]]+[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
  char *p=yytext;
  char *p2;

  yytext = strchr(yytext, '.');
  *yytext++ = 0;
  if (!strcasecmp (p, conf->gfx_driver->name))
  {
    int newcfg;

    p = yytext;
    yytext = strchr(yytext, '=');
    *yytext++ = 0;

    p2 = yytext-2;                    /* trim right spaces */
    while (p2 > p && isspace (*p2))
      *p2-- = 0;

    while (isspace(*yytext))
      yytext++;

    p2=yytext+strlen (yytext);        /* trim right spaces */
    while (p2 > p && isspace (*p2))
      *p2-- = 0;

    newcfg = (conf[cur_section].gfx_config_nr)++;

    if (newcfg)
      conf[cur_section].gfx_config = realloc(conf[cur_section].gfx_config,
					     (newcfg + 1) * sizeof(gfx_option_t *));
    else
      conf[cur_section].gfx_config = malloc(conf[cur_section].gfx_config_nr * sizeof(gfx_option_t *));

    conf[cur_section].gfx_config[newcfg].option = strdup(p);
    conf[cur_section].gfx_config[newcfg].value = strdup(yytext);
  }
}




"#".+$ /* Ignore comments */

[[:blank:]\n]+ /* Eat whitespace */

.* printf("Unrecognized option: %s\n", yytext);

%%

int
yywrap()
{
  return 1; /* no further input */
}


int
config_init(config_entry_t **_conf, char *conffile)
{
  char *homedir = getenv("HOME");
  char *conf_path;

  conf = malloc(sizeof(config_entry_t));

/**** Default config: */
#warning fixme
  //  conf->color_mode = SCI_COLOR_DITHER;

  conf->version = 0;

#warning fixme
  conf->gfx_driver = &gfx_driver_ggi;//graph_get_default_driver();

  conf->console_log = NULL;
  conf->debug_mode [0] = '\0';
  conf->name = NULL;
  conf->resource_dir = NULL;
  conf->work_dir = NULL;
  conf->gfx_config = NULL;
  conf->gfx_config_nr = 0;
/**** Default config ends */


  if (conffile) {
    exported_conf_path = (char *) malloc(PATH_MAX + 1);
    getcwd(exported_conf_path, PATH_MAX+1);

    conf_path = strdup(conffile); /* Use config file if supplied */
  } else {
    if (!homedir) { /* We're probably not under UNIX if this happens */

      conf_path = strdup(FREESCI_CONFFILE_DOS); /* Use DOS config style */

      exported_conf_path = (char *) malloc(PATH_MAX + 1);
      getcwd(exported_conf_path, PATH_MAX+1);

      dospath = 1; /* Use DOS-style paths */

    } else {

      /* So we've got a home directory */
      if (chdir(homedir)) {
	fprintf(stderr,"Warning: Could not enter home directory!\n");
	return 1;
      }

      if (chdir(FREESCI_GAMEDIR))
	if (scimkdir(FREESCI_GAMEDIR, 0700)) {

	  fprintf(stderr,"Warning: Could not enter/create ~/"FREESCI_GAMEDIR"\n");
	  return 1;
	}

      conf_path = malloc(strlen(homedir) + 3 + strlen(FREESCI_GAMEDIR) + strlen(FREESCI_CONFFILE));
      strcpy(conf_path, homedir);
      strcat(conf_path, "/");
      strcat(conf_path, FREESCI_GAMEDIR);

      exported_conf_path = strdup(conf_path);

      strcat(conf_path, "/");
      strcat(conf_path, FREESCI_CONFFILE);

      dospath = 0; /* Use UN*X-style paths */
    }
  } /* !conffile */


  if (!(yyin = fopen(conf_path, "r"))) {
    printf("No configuration file found; using defaults.\n");
    *_conf = conf; /* Set the result variable */
    free(conf_path);
    free(exported_conf_path);
    return 1;
  }

  printf("Parsing config file...\n");

  yylex(); /* Parse the file */

  free(conf_path);
  free(exported_conf_path);

  *_conf = conf; /* Store the result */
  return cur_section + 1;
}


void
config_free(config_entry_t **conf, int entries)
{
  int i;

  for (i = 1; i < entries; i++) {
    free ((*conf)[i].name);
    free ((*conf)[i].work_dir);
    if ((*conf)[i].resource_dir)
      free ((*conf)[i].resource_dir);
    if ((*conf)[i].console_log)
      free((*conf)[i].console_log);
    if ((*conf)[i].gfx_config) {
      int j;

      for (j = 0; j < (*conf)[i].gfx_config_nr; j++) {
	free ((*conf)[i].gfx_config[j].option);
	free ((*conf)[i].gfx_config[j].value);
      }

      free((*conf)[i].gfx_config);
    }
  }

  free (*conf);
}


static int
_parse_color_mode(char *mode) /* Parses the color mode string */
{
#warning fixme!
#if 0
  struct {
    char* name;
    int value;
  } valid_modes[] = {
    {"default", SCI_COLOR_DITHER},
    {"dither", SCI_COLOR_DITHER},
    {"dither16", SCI_COLOR_DITHER},
    {"dither_16", SCI_COLOR_DITHER},
    {"flat", SCI_COLOR_INTERPOLATE},
    {"interpol", SCI_COLOR_INTERPOLATE},
    {"interpolate", SCI_COLOR_INTERPOLATE},
    {"dither256", SCI_COLOR_DITHER256},
    {"dither_256", SCI_COLOR_DITHER256},
    {0, 0} /* Terminator */
  };
  int i = 0;

  while (valid_modes[i].name) {
    if (0 == g_strcasecmp(mode, valid_modes[i].name))
      return valid_modes[i].value;

    i++;
  }
#endif
  printf("Invalid color mode %s\n", mode);

  return 0;
}


static gfx_driver_t *
_parse_gfx_driver(char *driver_name)
{
  int i;

  for (i=0; gfx_drivers [i]; i++)
    if (!strcasecmp (gfx_drivers[i]->name, driver_name))
      return gfx_drivers [i];

  /* not found - return default */

  printf ("Unknown graphics driver %s\n", driver_name);
  return conf->gfx_driver;
}
