/***************************************************************************
 devices.c  Copyright (C) 2002 Christoph Reichenbach


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
#include <sfx_device.h>
#include <stdio.h>

#ifdef HAVE_ALSA
extern struct _midi_device sfx_device_midi_alsa;
#endif
#if !defined(_DOS) && !defined(_WIN32) && !defined(_DREAMCAST) && !defined(__MORPHOS__) && !defined(ARM_WINCE) && !defined(_GP32)
extern struct _midi_device sfx_device_midi_unixraw;
#endif

#include <resource.h>

static struct _midi_device *devices_midi[] = {
#ifdef HAVE_ALSA
		&sfx_device_midi_alsa,
#endif
#if !defined(_DOS) && !defined(_WIN32) && !defined(_DREAMCAST) && !defined(__MORPHOS__) && !defined(ARM_WINCE) && !defined(_GP32)
		&sfx_device_midi_unixraw,
#endif
		NULL
};

static struct _midi_device *devices_opl2[] = {
	NULL
};


/** -- **/

struct _midi_device **devices[] = {
	NULL, /* No device */
	devices_midi,
	devices_opl2,
};


static struct _midi_device *
find_dev(int type, char *name)
{
	int i = 0;

	if (!type)
		return NULL;

	if (!name)
		return devices[type][0];

	while (devices[type][i] && !strcmp(name, devices[type][i]->name))
		++i;

	return devices[type][i];
}


void *
sfx_find_device(int type, char *name)
{
	struct _midi_device *dev = find_dev(type, name);

	if (dev) {
		if (dev->init(dev)) {
			fprintf(stderr, "[SFX] Opening device '%s' failed\n",
				dev->name);
			return NULL;
		}

		return dev;
	};
	
	return NULL;
}
