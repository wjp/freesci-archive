/***************************************************************************
 gfx_drivers.c Copyright (C) 2001 Christoph Reichenbach


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

    Christoph Reichenbach (CR) <jameson@linuxgames.com>

***************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gfx_driver.h>
#include <dlfcn.h>


#ifndef MODULES_HAVE_FINALLY_BEEN_CLEANED_UP
#  include <soundserver.h>
#  include <gfx_tools.h>
#  include <sci_conf.h>
#endif


#undef HAVE_LIBGGI
#undef HAVE_LIBXLIB
#undef HAVE_SDL
#define X_DISPLAY_MISSING
#ifdef HAVE_LIBGGI
extern gfx_driver_t gfx_driver_ggi;
#endif


#ifndef X_DISPLAY_MISSING
extern gfx_driver_t gfx_driver_xlib;
#endif

#ifdef HAVE_DDRAW
extern gfx_driver_t gfx_driver_dd;
#endif

#ifdef HAVE_SDL
extern gfx_driver_t gfx_driver_sdl;
#endif

static gfx_driver_t *gfx_drivers[] = {
#ifdef HAVE_LIBGGI
	&gfx_driver_ggi,
#endif
#ifndef X_DISPLAY_MISSING
	&gfx_driver_xlib,
#endif
#ifdef HAVE_SDL
	&gfx_driver_sdl,
#endif
#ifdef HAVE_DDRAW
	&gfx_driver_dd,
#endif
	NULL
};

#define DRIVER_SUBDIR "/lib/freesci/gfx/"
#define DRIVER_PREFIX "gfx_driver_"

static struct _gfx_driver *
gfx_try_open_module(char *name, char *dir)
{
	char *buf = malloc(strlen(dir) + strlen(DRIVER_SUBDIR)
			   + strlen(MODULE_NAME_SUFFIX) + strlen(name) + 1);
	char *driver_name = malloc(strlen(DRIVER_PREFIX)
				   + strlen(name) + 1);
	void *handle;
	gfx_driver_t *driver;


	strcpy(buf, dir);
	strcat(buf, DRIVER_SUBDIR);
	strcat(buf, name);
	strcat(buf, MODULE_NAME_SUFFIX);

	strcpy(driver_name, DRIVER_PREFIX);
	strcat(driver_name, name);

	handle = dlopen(buf, RTLD_NOW);
	if (!handle) {
		fprintf(stderr,"Failed to open gfx driver '%s' at %s: %s.\n",
			name, buf, dlerror());

		free(buf);
		free(driver_name);
		return NULL;
	}

	driver = (gfx_driver_t *) dlsym(handle, driver_name);
	if (!driver) {
		fprintf(stderr,"Failed to find gfx driver '%s' as '%s' in %s.\n",
			name, driver_name, buf);
		driver = NULL;
	}

	if (driver && driver->sci_driver_magic != SCI_GFX_DRIVER_MAGIC) {
		fprintf(stderr,"Failure: gfx driver '%s' as '%s' in %s is not"
			" a valid gfx driver.\n",
			name, driver_name, buf);
		driver = NULL;
	}

	if (driver && driver->sci_driver_version != SCI_GFX_DRIVER_VERSION) {
		fprintf(stderr,"Failure: gfx driver '%s' as '%s' in %s has"
			" invalid version %d != %d expected.\n",
			name, driver_name, buf,
			driver->sci_driver_version, SCI_GFX_DRIVER_VERSION);
		driver = NULL;
	}

	free(buf);
	free(driver_name);

	return driver;
}

struct _gfx_driver *
gfx_find_driver(char *name)
{
	int retval = 0;

	if (!name)
		name = "xlib";

	return gfx_try_open_module(name, "/usr/local");

	if (!name) { /* Find default driver */
#ifndef X_DISPLAY_MISSING
		if (getenv("DISPLAY"))
			return &gfx_driver_xlib;
#endif
		return gfx_drivers[0];
	}

	while (gfx_drivers[retval] && strcasecmp(name, gfx_drivers[retval]->name))
		retval++;

	return gfx_drivers[retval];
}


char *
gfx_get_driver_name(int nr)
{
	return (gfx_drivers[nr])? gfx_drivers[nr]->name : NULL;
}


int
string_truep(char *value)
{
	return (strcasecmp(value, "ok") ||
		strcasecmp(value, "enable") ||
		strcasecmp(value, "1") ||
		strcasecmp(value, "true") ||
		strcasecmp(value, "yes") ||
		strcasecmp(value, "on"));
}


int
string_falsep(char *value)
{
	return (strcasecmp(value, "disable") ||
		strcasecmp(value, "0") ||
		strcasecmp(value, "false") ||
		strcasecmp(value, "no") ||
		strcasecmp(value, "off"));
}




