/***************************************************************************
 game_select.c Copyright (C) 2004 Hugues Valois, Ismail Khatib


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

***************************************************************************/

#include <game_select.h>
#include <vm.h>

int
find_config(char *game_name, config_entry_t *conf, int conf_entries,
	    sci_version_t *version)
{
	int i, conf_nr = 0;

	for (i = 1; i < conf_entries; i++)
		if (!strcasecmp(conf[i].name, game_name)) {
			conf_nr = i;
			*version = conf[i].version;
		}

	return conf_nr;
}

int
game_select_resource_found(void)
{
	int fd;

	fd = sci_open("resource.map", O_RDONLY | O_BINARY);
	if (IS_VALID_FD(fd))
	{
		close(fd);
		return 1;
	}

	return 0;
}

int
game_select_init_gfx(config_entry_t *conf,
		     gfx_state_t *gfx_state,
		     gfx_options_t *gfx_options,
		     cl_options_t *cl_options,
		     gfx_driver_t *driver,
		     sci_version_t sci_version)
{
	int scale_x = 0, scale_y = 0, color_depth = 0;

	if (conf) {
		if (conf->scale)
			scale_x = scale_y = conf->scale;

		if (conf->x_scale)
			scale_x = conf->x_scale;

		if (conf->y_scale)
			scale_y = conf->y_scale;

		if (conf->color_depth)
			color_depth = conf->color_depth; /* In there it's bpp */
	}

	gfx_state->driver = driver;
	gfx_state->version = sci_version;

	if (cl_options->scale_x > 0) {
		scale_x = cl_options->scale_x;

		if (!scale_y)
			scale_y = cl_options->scale_x;
	}

	if (cl_options->scale_y > 0) {
		scale_y = cl_options->scale_y;

		if (!scale_x)
			scale_x = cl_options->scale_y;
	}

	if (cl_options->color_depth > 0){
		color_depth = cl_options->color_depth;
	}

	if (color_depth > 0 && scale_x == 0)
		scale_x = scale_y = 2; /* Some default setting */

	/* Convert to bytespp */
	color_depth = (color_depth + 7) >> 3;
	fprintf(stderr, "Checking byte depth %d\n", color_depth);

	if (scale_x > 0) {

		if (color_depth > 0) {
			if (game_select_gfxop_init(gfx_state, scale_x,
				       scale_y, color_depth,
				       gfx_options, 0)) {
				fprintf(stderr,"Graphics initialization failed. Aborting...\n");
				return 1;
			}
		} else {
			color_depth = 4;
			while (game_select_gfxop_init(gfx_state, scale_x,
					  scale_y, color_depth,
					  gfx_options, 0) && --color_depth);

			if (!color_depth) {
				fprintf(stderr,"Could not find a matching color depth. Aborting...\n");
				return 1;
			}
		}

	} else if (game_select_gfxop_init_default(gfx_state, gfx_options, 0)) {
		fprintf(stderr,"Graphics initialization failed. Aborting...\n");
		return 1;
	}
	return 0;
}

static int
compare_config_entry(const void* arg1, const void* arg2)
{
	config_entry_t* config1 = (config_entry_t*)arg1;
	config_entry_t* config2 = (config_entry_t*)arg2;

	return strcmp(config1->name, config2->name);
}

int
game_select(state_t *gamestate,
	    cl_options_t cl_options,
	    gfx_state_t *gfx_state,
	    gfx_options_t *gfx_options,
	    config_entry_t *confs,
	    int conf_entries,
	    char* freesci_dir)
{
	char start_dir[PATH_MAX+1] = "";
	char work_dir[PATH_MAX+1] = "";
	char *gfx_driver_name			= NULL;
	sci_version_t version			= 0;
	gfx_driver_t *gfx_driver		= NULL;
	char *module_path			= SCI_DEFAULT_MODULE_PATH;
	int current_config;
	int selected_game = -1;
	char** game_list = NULL;
	char* game_name = NULL;
	config_entry_t* sorted_confs;
	gfx_bitmap_font_t* font_default;
	gfx_bitmap_font_t* font_small;
	int font_default_allocated = 0;
	int font_small_allocated = 0;


	getcwd(start_dir, PATH_MAX);
	script_debug_flag = cl_options.script_debug_flag;

	chdir(start_dir);


	/* gcc doesn't warn about (void *)s being typecast. If your compiler doesn't like these
	** implicit casts, don't hesitate to typecast appropriately.  */
	/* CR: IIRC, any C compiler should do that. It's usually C++ compilers
	** that warn/issue errors (appropriate to the C++ ANSI standard, IIRC.)
	** As far as I'm concerned, this isn't supposed to be compiled with
	** a C++ compiler, though.  */
	if (cl_options.gfx_driver_name) {
		gfx_driver_name = sci_strdup(cl_options.gfx_driver_name);
		/* free(cl_options.gfx_driver_name); */
	} /* else it's still NULL */

	if (confs) {
		memcpy(gfx_options, &(confs->gfx_options), sizeof(gfx_options_t)); /* memcpy so that console works */
		if (!gfx_driver_name)
			gfx_driver_name = confs->gfx_driver_name;
	}

	if (confs) {
		module_path = confs->module_path;

		if (!gfx_driver_name)
			gfx_driver_name = confs->gfx_driver_name;
	}

	gfx_driver = (gfx_driver_t *)
		lookup_driver((lookup_funct_t *)gfx_find_driver,
#if defined(WIN32_) || defined(ARM_WINCE)
				MSVC_FUNCTYPECAST_KLUDGE
#endif
			      list_graphics_drivers,
				"graphics driver", gfx_driver_name, module_path);

	if (!gfx_driver) {
		if (gfx_driver_name)
			fprintf(stderr,"Failed to find graphics driver \"%s\"\n"
				"Please run 'sciv -v' to get a list of all "
				"available drivers.\n", gfx_driver_name);
		else
			fprintf(stderr,"No default gfx driver available.\n");

		return 1;
	}

	/* Now configure the graphics driver with the specified options */
	{
		driver_option_t *option = get_driver_options(confs, FREESCI_DRIVER_SUBSYSTEM_GFX, gfx_driver->name);
		while (option) {
			if ((gfx_driver->set_parameter)(gfx_driver, option->option, option->value)) {
				fprintf(stderr, "Fatal error occured in graphics driver while processing \"%s = %s\"\n",
					option->option, option->value);
				exit(1);
			}

			option = option->next;
		}
	}

	if (game_select_init_gfx(confs, gfx_state, gfx_options,
				 &cl_options, gfx_driver, 0))
		return 1;
	/* Sort all the games in alphabetical order, do this on a copy of the original config structure */
	sorted_confs = malloc(sizeof(config_entry_t) * conf_entries);
	memcpy(sorted_confs, confs, sizeof(config_entry_t) * conf_entries);
	qsort(&(sorted_confs[1]), conf_entries - 1, sizeof(config_entry_t), compare_config_entry);
	/* Create the array of strings to pass to game selection display routine */
	game_list = malloc(sizeof(char*) * conf_entries);
	for (current_config = 0; current_config < conf_entries; current_config++)
	{
		game_list[current_config] = sorted_confs[current_config].name;

		/* Replace all '_'with ' ' */


		game_name = game_list[current_config];
		if (game_name != NULL)
		{
			while (*game_name != 0)
			{
				if (*game_name == '_')
					*game_name = ' ';

				game_name++;
			}
		}
	}
	/* load user supplied font from disk, if not found then use built-in font */
	//font_default = load_font(freesci_dir, FONT_DEFAULT);
	//font_default = 0;
	//if (!font_default)
		font_default = gfxr_get_font(NULL, GFX_FONT_BUILTIN_6x10, 0);
	//else
	//	font_default_allocated = 1;

	/* load user supplied font from disk, if not found then use built-in font */
	//font_small = load_font(freesci_dir, FONT_SMALL);
	//font_small = 0;
	//if (!font_small)
		font_small = gfxr_get_font(NULL, GFX_FONT_BUILTIN_5x8, 0);
	//else
	//	font_small_allocated = 1;
	/* Index of game selected is returned - 0 means no selection (quit) */
	selected_game = game_select_display(gfx_driver, VERSION, game_list, conf_entries, font_default, font_small);
	if (selected_game > 0)
	{
		chdir(sorted_confs[selected_game].resource_dir);

	}
	if (font_default_allocated == 1)
		gfxr_free_font(font_default);

	if (font_small_allocated == 1)
		gfxr_free_font(font_small);

	free(sorted_confs);
	free(game_list);

	gfx_driver->exit(gfx_driver);

	return 0;
}
