/***************************************************************************
 sfx_pcm.h Copyright (C) 2003 Christoph Reichenbach


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

#ifndef _SFX_PCM_H_
#define _SFX_PCM_H_

#include <sfx_engine.h>
#include <sfx_timer.h>


/* A number of standard options most devices will support */
#define SFX_PCM_OPTION_RATE "rate"            /* Sampling rate: Number of samples per second */
#define SFX_PCM_OPTION_BITS "bits"            /* Sample size in bits */
#define SFX_PCM_OPTION_STEREO "stereo"        /* Whether to support stereo output */
#define SFX_PCM_OPTION_BUF_SIZE "buffer-size" /* Requested buffer size */
/* Device implementors are advised to use these constants whenever possible. */

#define SFX_PCM_MONO 0
#define SFX_PCM_STEREO_LR 1 /* left sample, then right sample */
#define SFX_PCM_STEREO_RL 2 /* right sample, then left sample */

/* The following are used internally by the mixer */
#define SFX_PCM_FORMAT_LMASK 0x7
#define SFX_PCM_FORMAT_BE 0
#define SFX_PCM_FORMAT_LE 1
#define SFX_PCM_FORMAT_ENDIANNESS 1
#define SFX_PCM_FORMAT_8  0
#define SFX_PCM_FORMAT_16 2


/* Pick one of these formats (including the _NATIVE) ones for your PCM feed */
#define SFX_PCM_FORMAT_U8     (0x0080 | SFX_PCM_FORMAT_8)			/* Unsigned (bias 128) 8 bit format */
#define SFX_PCM_FORMAT_S8     (0x0000 | SFX_PCM_FORMAT_8)			/* Signed 8 bit format */
#define SFX_PCM_FORMAT_U16_LE (0x8000 | SFX_PCM_FORMAT_16 | SFX_PCM_FORMAT_LE)	/* Unsigned (bias 32768) 16 bit LE format */
#define SFX_PCM_FORMAT_S16_LE (0x0000 | SFX_PCM_FORMAT_16 | SFX_PCM_FORMAT_LE)	/* Signed 16 bit format, little endian */
#define SFX_PCM_FORMAT_U16_BE (0x8000 | SFX_PCM_FORMAT_16 | SFX_PCM_FORMAT_BE)	/* Unsigned (bias 32768) 16 bit BE format */
#define SFX_PCM_FORMAT_S16_BE (0x0000 | SFX_PCM_FORMAT_16 | SFX_PCM_FORMAT_BE)	/* Signed 16 bit format, big endian */

#ifdef WORDS_BIGENDIAN
#  define SFX_PCM_FORMAT_U16_NATIVE SFX_PCM_FORMAT_U16_BE
#  define SFX_PCM_FORMAT_S16_NATIVE SFX_PCM_FORMAT_S16_BE
#else
#  define SFX_PCM_FORMAT_U16_NATIVE SFX_PCM_FORMAT_U16_LE
#  define SFX_PCM_FORMAT_S16_NATIVE SFX_PCM_FORMAT_S16_LE
#endif

#define SFX_PCM_SAMPLE_SIZE(conf) ((conf).stereo? 2 : 1) * (((conf).format & SFX_PCM_FORMAT_16)? 2 : 1)


typedef struct {
	int nom, den;
	int val;

	/* Total value: val + nom/den, where (nom < den) guaranteed. */
} sfx_pcm_urat_t; /* Finitary unsigned rational numbers */

typedef struct {
	int rate;   /* Sampling rate */
	int stereo; /* The stereo mode used (SFX_PCM_MONO or SFX_PCM_STEREO_*) */
	unsigned int format; /* Sample format (SFX_PCM_FORMAT_*) */
} sfx_pcm_config_t;

typedef struct _sfx_pcm_device {
	/* SFX devices are PCM players, i.e. output drivers for digitalised audio (sequences of audio samples).
	** Implementors are (in general) allowed to export specifics of these devices and let the mixer handle
	** endianness/signedness/bit size/mono-vs-stereo conversions.
	*/

	char *name;
	char *version;

	int (*init)(struct _sfx_pcm_device *self);
	/* Initializes the device
	** Parameters: (sfx_pcm_device_t *) self: Self reference
	** Returns   : (int) SFX_OK on success, SFX_ERROR if the device could not be
	**                   opened
	** This should attempt to open the highest quality output allowed by any options
	** specified beforehand.
	*/

	void (*exit)(struct _sfx_pcm_device *self);
	/* Uninitialises the device
	** Parameters: (sfx_pcm_device_t *) self: Self reference
	*/

	int (*set_option)(struct _sfx_pcm_device *self, char *name, char *value);
	/* Sets an option for the device
	** Parameters: (sfx_pcm_device_t *) self: Self reference
	**             (char *) name: Name of the option to set
	**             (char *) value: Value of the option to set
	** Returns   : (int) SFX_OK on success, SFX_ERROR otherwise (unsupported option)
	** May be NULL
	*/

	int (*output)(struct _sfx_pcm_device *self, byte *buf, int count);
	/* Writes output to the device
	** Parameters: (sfx_pcm_device_t *) self: Self reference
	**             (byte *) buf: The buffer to write
	**             (int) count: Number of /samples/ that should be written
	** Returns   : (int) SFX_OK on success, SFX_ERROR on error
	** The size of the buffer allocated as 'buf' equals buf_size.
	** 'buf' is guaranteed not to be modified in between calls to 'output()'.
	*/

	/* The following must be set after initialisation */
	sfx_pcm_config_t conf;
	int buf_size; /* Output buffer size, i.e. the number of samples (!)
		      ** that can be queued by this driver before calling
		      ** output() will block or fail, drained according
		      ** to conf.rate  */

	/* The following are optional */
	sfx_timer_t *timer;
	/* Many PCM drivers use a callback mechanism, which can be
	** exploited as a timer. Such a timer may be exported here and
	** will be preferred over other timers.  */
	/* This is an _optional_ timer provided by the PCM
	** subsystem (may be NULL). It is checked for afer
	** initialisation, and used in preference to any
	** other timers available.
	*/
	void *internal; /* The private bits */

} sfx_pcm_device_t;


typedef struct _sfx_pcm_feed_t {
	/* PCM feeds are sources of input for the PCM mixer. Their member functions
	** are invoked as callbacks on demand, to provide the mixer with input it
	** (in turn) passes on to PCM output devices.
	**   PCM feeds must explicitly register themselves with the mixer in order
	** to be considered.
	*/

	int (*poll)(struct _sfx_pcm_feed_t *self, byte *dest, int size);
	/* Asks the PCM feed to write out the next stuff it would like to have written
	** Parameters: (sfx_pcm_feed_t *) self: Self reference
	**             (byte *) dest: The destination buffer to write to
	**             (int) size: The maximum number of _samples_ (not neccessarily bytes)
	**                         to write
	** Returns   : (int) The number of samples written
	** If the number of samples written is smaller than 'size', the PCM feed will
	** be destroyed subsequently.
	*/

	void (*destroy)(struct _sfx_pcm_feed_t *self);
	/* Asks the PCM feed to free all resources it occupies
	** Parameters: (sfx_pcm_feed_t *) self: Self reference
	** free(self) should be part of this function, if applicable.
	*/

	void *internal; /* The private bits of a PCM feed. */

	sfx_pcm_config_t conf; /* The channel's setup */

	char *debug_name; /* The channel name, for debugging */
	int debug_nr; /* A channel number relative to the channel name, for debugging
		      ** (print in hex)  */
	int sample_size; /* Sample size, computed by the mixer for the feed */

} sfx_pcm_feed_t;


sfx_pcm_device_t *
sfx_pcm_find_device(char *name);
/* Finds a PCM device by name
** Parameters: (char *) name: Name of the PCM device to look for, or NULL to
**                            use the system default
** Returns   : (sfx_pcm_device_t *) The requested device, or NULL if no matching
**                                  device could be found
*/

#endif /* !defined(_SFX_PCM_H_) */
