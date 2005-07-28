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

#if defined(HAVE_ALSA) && defined(HAVE_PTHREADS)
#  ifdef HAVE_ALSA_ASOUNDLIB_H
#    include <alsa/asoundlib.h>
#  else
#    include <sys/asoundlib.h>
#  endif
#  if (SND_LIB_MAJOR > 0) || (SND_LIB_MINOR > 5)
#    define SUPPORT_ALSA_PCM
#  endif
#endif


typedef struct _pcmout_driver {
	char *name;
	char *version;
	int (*set_parameter)(struct _pcmout_driver *drv, char *attribute, char *value);
	int (*pcmout_open)(gint16 *b, guint16 buffer_size, guint16 rate, guint8 stereo);
	int (*pcmout_close)(void);
} pcmout_driver_t;

extern DLLEXTERN pcmout_driver_t *pcmout_driver;

extern DLLEXTERN pcmout_driver_t pcmout_driver_null;

extern DLLEXTERN guint16 pcmout_sample_rate;
extern DLLEXTERN guint8 pcmout_stereo;
extern DLLEXTERN guint16 pcmout_buffer_size;

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
#ifdef _DREAMCAST
extern pcmout_driver_t pcmout_driver_dc;
#endif
#ifdef _GP32
extern pcmout_driver_t pcmout_driver_gp32;
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

typedef int (*synth_mixer_func_t)(gint16 *tmp_bk, int samples);
void pcmout_set_mixer(synth_mixer_func_t func);

struct _pcmout_driver *pcmout_find_driver(char *name);

#endif /* _PCMOUT_H_ */

