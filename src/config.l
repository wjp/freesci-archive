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
#include <sci_conf.h>

#ifdef _MSC_VER
#  include <ctype.h>
#  include <direct.h>

#  define strcasecmp stricmp
#  define PATH_MAX 255

#endif

config_entry_t *conf;
int cur_section=0; /* Size-1 and current section in conf */
char *exported_conf_path; /* Path which the config file was found in */
int dospath; /* Use dos-style paths? */

typedef struct {
	char *name;
	int value;
} name_value_pair;

static int
_parse_name(char *name, name_value_pair* nvps, char *what, int oldval); /* Parses a string with a name value pair */

static gfx_driver_t *
_parse_gfx_driver(char *s); /* Parses a graphics driver */

static name_value_pair valid_modes[] = {
	{"default", GFXR_DITHER_MODE_D16},
	{"dither", GFXR_DITHER_MODE_D16},
	{"dither16", GFXR_DITHER_MODE_D16},
	{"dither_16", GFXR_DITHER_MODE_D16},
	{"dither-16", GFXR_DITHER_MODE_D16},
	{"d16", GFXR_DITHER_MODE_D16},
	{"flat", GFXR_DITHER_MODE_F256},
	{"interpol", GFXR_DITHER_MODE_F256},
	{"interpolate", GFXR_DITHER_MODE_F256},
	{"dither256", GFXR_DITHER_MODE_D256},
	{"dither_256", GFXR_DITHER_MODE_D256},
	{"d256", GFXR_DITHER_MODE_D256},
	{0, 0} /* Terminator */
};

static name_value_pair yesno[] = {
	{"yes", 1},
	{"no", 0},
	{"1", 1},
	{"0", 0},
	{"ok", 1},
	{"enable", 1},
	{"disable", 0},
	{"activate", 1},
	{"deactivate", 0},
	{"+", 1},
	{"-", 0},
	{"on", 1},
	{"off", 0},
	{0, 0}
};

static name_value_pair dither_pattern[] = {
	{"scaled", GFXR_DITHER_PATTERN_SCALED},
	{"unscaled", GFXR_DITHER_PATTERN_1},
	{"one", GFXR_DITHER_PATTERN_1},
	{"1", GFXR_DITHER_PATTERN_1},
	{0, 0}
};

static name_value_pair dirty_strategy[] = {
	{"1", GFXOP_DIRTY_FRAMES_ONE},
	{"one", GFXOP_DIRTY_FRAMES_ONE},
	{"cluster", GFXOP_DIRTY_FRAMES_CLUSTERS},
	{"clusters", GFXOP_DIRTY_FRAMES_CLUSTERS},
	{0, 0}
};

static name_value_pair brush_mode[] = {
	{"scaled", GFX_BRUSH_MODE_SCALED},
	{"ellipse", GFX_BRUSH_MODE_ELLIPSES},
	{"ellipses", GFX_BRUSH_MODE_ELLIPSES},
	{"rnd_ellipses", GFX_BRUSH_MODE_RANDOM_ELLIPSES},
	{"rnd-ellipses", GFX_BRUSH_MODE_RANDOM_ELLIPSES},
	{"random_ellipses", GFX_BRUSH_MODE_RANDOM_ELLIPSES},
	{"random-ellipses", GFX_BRUSH_MODE_RANDOM_ELLIPSES},
	{"morerandom", GFX_BRUSH_MODE_MORERANDOM},
	{"more-random", GFX_BRUSH_MODE_MORERANDOM},
	{"more_random", GFX_BRUSH_MODE_MORERANDOM},
	{0, 0}
};

static name_value_pair filter_mode[] = {
	{"none", GFX_XLATE_FILTER_NONE},
	{"linear", GFX_XLATE_FILTER_LINEAR},
	{"bilinear", GFX_XLATE_FILTER_LINEAR},
	{"bi-linear", GFX_XLATE_FILTER_LINEAR},
	{0, 0}
};

static name_value_pair line_mode[] = {
	{"correct", GFX_LINE_MODE_CORRECT},
	{"normal", GFX_LINE_MODE_CORRECT},
	{"fast", GFX_LINE_MODE_FAST},
	{"fine", GFX_LINE_MODE_FINE},
	{"thin", GFX_LINE_MODE_FINE},
	{0, 0}
};

%}

DIGIT [0-9]
PARAMTOKEN [[:alpha:]][[:alnum:]_]*
SCIVERSION {DIGIT}"."{DIGIT}{3}"."{DIGIT}{3}
NUMTOKEN {DIGIT}+

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

pic0_dither_mode[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].gfx_options.pic0_dither_mode = 
		_parse_name(yytext, valid_modes, "color mode", conf[cur_section].gfx_options.pic0_dither_mode);
}

color_mode[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} { /* Deprecated: For backward compatibility */
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].gfx_options.pic0_dither_mode =
		_parse_name(yytext, valid_modes, "color mode", conf[cur_section].gfx_options.pic0_dither_mode);
}

pic0_dither_pattern[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].gfx_options.pic0_dither_pattern =
		_parse_name(yytext, dither_pattern, "SCI0 pic dither pattern", conf[cur_section].gfx_options.pic0_dither_pattern);
}

pic0_brush_mode[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].gfx_options.pic0_brush_mode =
		_parse_name(yytext, brush_mode, "SCI0 pic brush mode", conf[cur_section].gfx_options.pic0_brush_mode);
}

pic0_line_mode[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].gfx_options.pic0_line_mode =
		_parse_name(yytext, line_mode, "SCI0 pic line mode", conf[cur_section].gfx_options.pic0_line_mode);
}

dirty_strategy[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].gfx_options.dirty_frames =
		_parse_name(yytext, dirty_strategy, "dirty strategy", conf[cur_section].gfx_options.dirty_frames);
}

pic0_scaled[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].gfx_options.pic0_unscaled =
		!_parse_name(yytext, yesno, "pic0 scaling", !conf[cur_section].gfx_options.pic0_unscaled);
}

pic_filter[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].gfx_options.pic_xlate_filter =
		_parse_name(yytext, filter_mode, "pic filter", conf[cur_section].gfx_options.pic_xlate_filter);
}

text_filter[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].gfx_options.text_xlate_filter =
		_parse_name(yytext, filter_mode, "text filter", conf[cur_section].gfx_options.text_xlate_filter);
}

view_filter[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].gfx_options.view_xlate_filter =
		_parse_name(yytext, filter_mode, "view filter", conf[cur_section].gfx_options.view_xlate_filter);
}

cursor_filter[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].gfx_options.cursor_xlate_filter =
		_parse_name(yytext, filter_mode, "cursor filter", conf[cur_section].gfx_options.cursor_xlate_filter);
}

mouse[[:blank:]]*"="[[:blank:]]*{PARAMTOKEN} {
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;
	conf[cur_section].mouse =
		!_parse_name(yytext, yesno, "mouse", !conf[cur_section].mouse);
}

pic_buffer_size[[:blank:]]*"="[[:blank:]]*{NUMTOKEN} {
	int val;
	yytext = strchr(yytext, '=') + 1;

	while (isspace(*yytext))
		yytext++;

	val = atoi(yytext);
	if (val < 0) {
		printf("No negative values allowed for pic buffer size\n");
	} else conf[cur_section].gfx_options.buffer_pics_nr = val;
}

version[[:blank:]]*"="[[:blank:]]*{SCIVERSION} { /***** End of graphics *****/

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
        if (!strcasecmp (p, conf->gfx_driver->name)) {
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

<<EOF>> {
        yy_delete_buffer( YY_CURRENT_BUFFER );
        yyterminate();
}

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
	conf->gfx_options.buffer_pics_nr = 0;
	conf->gfx_options.correct_rendering = 1;
	conf->gfx_options.pic0_unscaled = 0;
	conf->gfx_options.pic0_dither_mode = GFXR_DITHER_MODE_D16;
	conf->gfx_options.pic0_dither_pattern = GFXR_DITHER_PATTERN_SCALED;
	conf->gfx_options.pic0_brush_mode = GFX_BRUSH_MODE_RANDOM_ELLIPSES;
	conf->gfx_options.pic0_line_mode = GFX_LINE_MODE_CORRECT;
	conf->gfx_options.cursor_xlate_filter = GFX_XLATE_FILTER_NONE;
	conf->gfx_options.view_xlate_filter = GFX_XLATE_FILTER_NONE;
	conf->gfx_options.pic_xlate_filter = GFX_XLATE_FILTER_NONE;
	conf->gfx_options.text_xlate_filter = GFX_XLATE_FILTER_NONE;
	conf->gfx_options.dirty_frames = GFXOP_DIRTY_FRAMES_CLUSTERS;

	conf->gfx_driver = gfx_find_driver(NULL);

	conf->mouse = 1;

	conf->version = 0;

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

	if ((*conf)->console_log)
		free((*conf)->console_log);

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
_parse_name(char *name, name_value_pair *nvps, char *what, int oldval)
{
	int i = 0;

	while (nvps[i].name) {
		if (0 == strcasecmp(name, nvps[i].name))
			return nvps[i].value;

		i++;
	}

	printf("Invalid %s mode %s\n", what, name);

	return oldval;
}


static gfx_driver_t *
_parse_gfx_driver(char *driver_name)
{
	gfx_driver_t *retval = gfx_find_driver(driver_name);

	if (retval)
		return retval;
	/* not found - return default */

	printf ("Unknown graphics driver %s\n", driver_name);
	return conf->gfx_driver;
}
