/***************************************************************************
 game_select.h Copyright (C) 2004 Hugues Valois


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

#ifndef GAME_SELECT_H_
#define GAME_SELECT_H_

#include <resource.h>
#include <sci_conf.h>
#include <gfx_operations.h>
#include <gfx_system.h>
#include <gfx_tools.h>
#include <engine.h>


typedef void *lookup_funct_t(char *path, char *name);

/* DOCUMENTATION MISSING */

int
game_select_gfxop_init_default(gfx_state_t *state,
			       gfx_options_t *options,
			       void *misc_info);

int
game_select_gfxop_init(gfx_state_t *state,
		       int xfact, int yfact,
		       gfx_color_mode_t bpp,
		       gfx_options_t *options,
		       void *misc_info);

int
game_select_display(gfx_driver_t *gfx_driver,
		    char* freesci_version,
		    char** game_list,
		    int game_count,
		    gfx_bitmap_font_t* font_default,
		    gfx_bitmap_font_t* font_small);

int
game_select(state_t *gamestate,
	    cl_options_t cl_options,
	    gfx_state_t *gfx_state,
	    gfx_options_t *gfx_options,
	    config_entry_t *confs,
	    int conf_entries,
	    char* freesci_dir);

int
game_select_resource_found(void);

int
find_config(char *game_name, config_entry_t *conf, int conf_entries,
	    sci_version_t *version);


void *
lookup_driver(lookup_funct_t lookup_func, void explain_func(void),
	      char *driver_class, char *driver_name, char *path);

void
list_graphics_drivers();

#if 0
/* this can be used to generate code that creates a particular font at runtime */
/* this is meant to be used as a development tool */
void save_font(int id,
	       gfx_bitmap_font_t* font)
#endif

#endif /* !defined(GAME_SELECT_H_) */
