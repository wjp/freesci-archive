/***************************************************************************
 config.h Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
#include <graphics.h>

typedef struct {

  sci_version_t version; /* The version to emulate */
  int color_mode; /* The mode to use for pic drawing */
  gfx_driver_t *gfx_driver; /* The graphics driver to use */

} config_entry_t;


void
config_init(config_entry_t *conf, char *name, char *conffile);
/* Initializes the config entry based on information found in the config file.
** Parameters: (config_entry_t *) conf: Pointer to the configuration entry to initialize
**             (char *) name: The name of the game to initialize it for
**             (char *) conffile: Filename of the config file, or NULL to use the default name
** This function reads the ~/.freesci/config file, parses it, and inserts the appropriate
** data into conf.
** Not threadsafe. Uses flex.
*/

#endif /* !_SCI_CONFIG_H */
