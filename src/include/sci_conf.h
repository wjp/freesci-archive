/***************************************************************************
 sci_conf.h Copyright (C) 1999,2000,01 Christoph Reichenbach


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
/* Configuration and setup stuff for FreeSCI */

#ifndef _SCI_CONFIG_H_
#define _SCI_CONFIG_H_

#include <versions.h>
#include <gfx_options.h>

typedef struct {
	char *option;
	char *value;
} gfx_option_t;

typedef struct {

	char *name; /* Game identifier */
	sci_version_t version; /* The version to emulate */

	gfx_options_t gfx_options;

	int animation_delay; /* Number of microseconds to wait between each pic transition animation cycle */
	int alpha_threshold; /* Crossblitting alpha threshold */
	int unknown_count; /* The number of "unknown" kernel functions */ 
	char *resource_dir; /* Resource directory */
	char *work_dir;     /* Working directory (save games, additional graphics) */
	gfx_driver_t *gfx_driver; /* The graphics driver to use */
	gfx_option_t *gfx_config; /* Graphics subsystem configuration options */
	int gfx_config_nr; /* Number of options */
	char *console_log; /* The file to which console output should be echoed */
	char debug_mode [80]; /* Characters specifying areas for which debug output should be enabled */

	int mouse; /* Whether the mouse should be active */

} config_entry_t;


int
config_init(config_entry_t **conf, char *conffil);
/* Initializes the config entry structurre based on information found in the config file.
** Parameters: (config_entry_t **) conf: See below
**             (char *) conffile: Filename of the config file, or NULL to use the default name
** Returns   : (int) The number of config file entries found
** This function reads the ~/.freesci/config file, parses it, and inserts the appropriate
** data into *conf. *conf will be malloc'd to be an array containing default information in [0]
** and game-specific data in each of the subsequent record entries.
** Not threadsafe. Uses flex-generated code.
*/

void
config_free(config_entry_t **conf, int entries);
/* Frees a config entry structure
** Parameters: (config_entry_t **) conf: Pointer to the pointer to the first entry of the list
**             (int) entries: Number of entries to free
** Returns   : (void)
*/

#endif /* !_SCI_CONFIG_H */
