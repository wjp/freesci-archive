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
#include <sci_conf.h>
#include <graphics.h>
#include <engine.h>
#include <glib.h>

#ifdef _MSC_VER
#include <ctype.h>
#include <direct.h>
#endif

config_entry_t *conf;
char *name;
int our_section=1;

#ifdef _WIN32
#define scimkdir(arg1,arg2) mkdir(arg1)
#else
#define scimkdir(arg1,arg2) mkdir(arg1,arg2)
#endif

int
_parse_color_mode(char *s); /* Parses the color mode string */

sci_version_t
_parse_version_nr(char *s); /* Parses a version number */

gfx_driver_t *
_parse_gfx_driver(char *s); /* Parses a graphics driver */

%}

DIGIT [0-9]
PARAMTOKEN [[:alpha:]][[:alnum:]_]*
SCIVERSION {DIGIT}"."{DIGIT}{3}"."{DIGIT}{3}

%%

"["[[:alnum:]]+"]" {
  if (strncmp(yytext + 1, name, yyleng -2) == 0) /* Are we looking for this entry? */
    our_section = 1;
  else our_section = 0;
}

color_mode[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} if (our_section) {
  yytext = strchr(yytext, '=') + 1;

  while (isspace(*yytext))
    yytext++;

  conf->color_mode = _parse_color_mode(yytext);
}

version[[:blank:]]*"="[[:blank:]]*{SCIVERSION}  if (our_section) {
  yytext = strchr(yytext, '=') + 1;

  while (isspace(*yytext))
    yytext++;

  conf->version = _parse_version_nr(yytext);
}

console_log[[:blank:]]*"="[[:blank:]]*.+  if (our_section) {
  yytext = strchr (yytext, '=') + 1;

  while (isspace(*yytext))
    yytext++;

  if (conf->console_log) free (conf->console_log);
  conf->console_log = (char *) malloc (strlen (yytext)+1);
  strcpy (conf->console_log, yytext);
}


gfx_driver[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} if (our_section) {
  yytext = strchr(yytext, '=') + 1;

  while (isspace(*yytext))
    yytext++;

  conf->gfx_driver = _parse_gfx_driver(yytext);
}

[[:alnum:]]+"."[[:alnum:]]+[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} if (our_section) {
  char *p=yytext;
  char *p2;

  yytext = strchr(yytext, '.');
  *yytext++ = 0;
  if (!stricmp (p, conf->gfx_driver->Name))
  {
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

    if (conf->gfx_driver->Configure)
      (*conf->gfx_driver->Configure)(p, yytext);
  }
}




"#".+$ /* Ignore comments */

[[:blank:]\n]+ /* Eat whitespace */

.* printf("Unrecognized option: %s\n", yytext);

%%

void
config_init(config_entry_t *_conf, char *_name, char *conffile)
{
  char *path;
  char *homedir = getenv("HOME");
  char *conf_path;

  conf = _conf;
  name = _name;

/**** Default config: */
  conf->color_mode = SCI_COLOR_DITHER;

  if (sci_version == SCI_VERSION_0)
    conf->version = SCI_VERSION_DEFAULT_SCI0;
  else if (sci_version == SCI_VERSION_01)
    conf->version = SCI_VERSION_DEFAULT_SCI01;

  conf->gfx_driver = graph_get_default_driver();

  conf->console_log = NULL;
/**** Default config ends */

  if (conffile)
    conf_path = strdup(conffile); /* Use config file if supplied */
  else {
    if (!homedir) { /* We're probably not under UNIX if this happens */

      conf_path = strdup(FREESCI_CONFFILE_DOS); /* Use DOS config style */

    } else {

      /* So we've got a home directory */
      if (chdir(homedir)) {
	fprintf(stderr,"Warning: Could not enter home directory!\n");
	return;
      }

      if (chdir(FREESCI_GAMEDIR))
	if (scimkdir(FREESCI_GAMEDIR, 0700)) {

	  fprintf(stderr,"Warning: Could not enter/create ~/"FREESCI_GAMEDIR"\n");
	  return;
	}

      conf_path = malloc(strlen(homedir) + 3 + strlen(FREESCI_GAMEDIR) + strlen(FREESCI_CONFFILE));
      strcpy(conf_path, homedir);
      strcat(conf_path, "/");
      strcat(conf_path, FREESCI_GAMEDIR);
      strcat(conf_path, "/");
      strcat(conf_path, FREESCI_CONFFILE);
    }
  } /* !conffile */

  our_section = 1;

  if (!(yyin = fopen(conf_path, "r"))) {
    printf("No configuration file found; using defaults.\n");
    return;
  }

  printf("Parsing config file...\n");

  free(conf_path);

  yylex(); /* Parse the file */

}

int
_parse_color_mode(char *mode) /* Parses the color mode string */
{
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
    if (g_strcasecmp(mode, valid_modes[i].name))
      return valid_modes[i].value;
    i++;
  }

  printf("Invalid color mode %s\n", mode);

  return 0;
}

sci_version_t
_parse_version_nr(char *vn) /* Parses a version number */
{
  int major = *vn - '0'; /* One version digit */
  int minor = atoi(vn + 2);
  int patchlevel = atoi(vn + 6);

  return SCI_VERSION(major, minor, patchlevel);
}

gfx_driver_t *
_parse_gfx_driver(char *driver_name)
{
  int i;

  for (i=0; gfx_drivers [i]; i++)
    if (!strcmp (gfx_drivers [i]->Name, driver_name))
      return gfx_drivers [i];

  /* not found - return default */

  printf ("Unknown graphics driver %s\n", driver_name);
  return conf->gfx_driver;
}
