/***************************************************************************
 pcmout.h Copyright (C) 2002 Solomon Peachy

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

***************************************************************************/

#ifndef _PCMOUT_H_
#define _PCMOUT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sci_memory.h>
#include <resource.h>

typedef struct _pcmout_driver {
	char *name;
	char *version;
	int (*set_parameter)(struct _pcmout_driver *drv, char *attribute, char *value);
	int (*pcmout_open)(gint16 *b, guint16 rate);
	int (*pcmout_close)(void);
} pcmout_driver_t;

#define BUFFER_SIZE 1024

extern DLLEXTERN pcmout_driver_t *pcmout_driver;

extern DLLEXTERN pcmout_driver_t pcmout_driver_null;

extern DLLEXTERN guint16 pcmout_sample_rate;

#ifdef HAVE_SYS_SOUNDCARD_H
extern pcmout_driver_t pcmout_driver_oss;
#endif

#ifdef HAVE_SDL
extern pcmout_driver_t pcmout_driver_sdl;
#endif
#ifdef HAVE_ALSA
extern pcmout_driver_t pcmout_driver_alsa;
#endif
#ifdef HAVE_DMEDIA_AUDIO_H
extern pcmout_driver_t pcmout_driver_al;
#endif

 
extern DLLEXTERN pcmout_driver_t *pcmout_drivers[];

int pcmout_open();
int pcmout_close();
int mix_sound(int count);
void pcmout_disable(void);
/* Disables PCM output by setting the 'null' driver
** Returns   : (void)
** Modifies  : Active PCM driver
*/

struct _pcmout_driver *pcmout_find_driver(char *name);

#endif /* _PCMOUT_H_ */

