/***************************************************************************
 audiobuf.h Copyright (C) 2003 Christoph Reichenbach


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

/* Auxiliary audio buffer for PCM devices
** Polled PCM devices must store data written to them until it is explicitly
** requiested. This is facilitated by the structures and functions defined
** here.
**   This is generic for all PCM devices; it implies no specific requirements.
**
** Usage: Introduce an sfx_audio_buf_t into your state and make sure to use
** each of the functions provided here at least once in the appropriate
** places.
*/


#ifndef _AUDIOBUF_H_
#define _AUDIOBUF_H_

#include <resource.h>
#include <sci_memory.h>

#define SFX_AUDIO_BUF_SIZE 8192	/* Must be multiple of framesize */
#define SFX_AUDIO_MAX_FRAME 8	/* Max. individual sample size */

typedef struct _sfx_audio_buf_chunk {
	unsigned char data[SFX_AUDIO_BUF_SIZE];
	int used;
	struct _sfx_audio_buf_chunk *next;
} sfx_audio_buf_chunk_t;

typedef struct {
	int read_offset;
	sfx_audio_buf_chunk_t *first; /* Next to read-- can be = last */
	sfx_audio_buf_chunk_t *last; /* Next to write-- can be = first */
	sfx_audio_buf_chunk_t *unused; /* Unused chunk list, can be NULL */
	unsigned char last_frame[SFX_AUDIO_MAX_FRAME];
	/* Contains the last frame successfully read; used for buffer
	** underruns to avoid crack before silance  */
} sfx_audio_buf_t;


void
sfx_audbuf_init(sfx_audio_buf_t *buf);
/* Initialises an audio buffer
** Parameters: (sfx_audio_buf_t *) buf: The buffer to initialise
** Modifies  : (sfx_audio_buf_t) *buf
*/

void
sfx_audbuf_free(sfx_audio_buf_t *buf);
/* Frees all memory associated with an audio buffer
** Parameters: (sfx_audio_buf_t *) buf: The buffer whose associated memory
**					should be freed
** Modifies  : (sfx_audio_buf_t) *buf
*/

void
sfx_audbuf_write(sfx_audio_buf_t *buf, unsigned char *src, int framesize,
		 int frames);
/* Store data in an audion buffer
** Parameters: (sfx_audio_buf_t *) buf: The buffer to write to
**             (unsigned char *) src: Pointer to the data that should be
**                                    written
**             (int) framesize: Size of a single sample in bytes
**             (int) frames: Number of samples to write
** Modifies  : (sfx_audio_buf_t) *buf
*/


int
sfx_audbuf_read(sfx_audio_buf_t *buf, unsigned char *dest, int framesize,
		int frames);
/* Read data from audio buffer
** Parameters: (sfx_audio_buf_t *) buf: The buffer to write to
**             (unsigned char *) dest: Pointer to the place the read data
**                                     should be written to
**             (int) framesize: Size of a single sample in bytes
**             (int) frames: Number of samples to write
** Returns   : (int) Number of samples actually read
** Affects   : (sfx_audio_buf_t) *buf
**             (unsigned char ) *dest
**             global error stream
** If the returned number of samples is smaller than the number of samples
** requested to be written, this function will issue a buffer underrun
** warning and fill up the remaining space with the last sample it en--
** countered, or a block of '0' if no such sample is known.
*/




#endif /* !_AUDIOBUF_H_ */
