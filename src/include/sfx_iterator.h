/***************************************************************************
 sfx_iterator.h (C) 2002 Christoph Reichenbach

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

		Christoph Reichenbach (CJR) [reichenb@colorado.edu]

***************************************************************************/
/* Song iterator declarations */

#ifndef _SCI_SFX_ITERATOR_H_
#define _SCI_SFX_ITERATOR_H_

/* Special SCI sound stuff */

#define SCI_MIDI_TIME_EXPANSION_PREFIX 0xF8
#define SCI_MIDI_TIME_EXPANSION_LENGTH 240

#define SCI_MIDI_EOT 0xFC
#define SCI_MIDI_SET_SIGNAL 0xCF
#define SCI_MIDI_SET_POLYPHONY 0x4B
#define SCI_MIDI_RESET_ON_SUSPEND 0x4C
#define SCI_MIDI_SET_VELOCITY 0x4E
#define SCI_MIDI_SET_REVERB 0x50
#define SCI_MIDI_CUMULATIVE_CUE 0x60

#define SCI_MIDI_SET_SIGNAL_LOOP 0x7F
/* If this is the parameter of 0xCF, the loop point is set here */

#define SCI_MIDI_CONTROLLER(status) ((status & 0xF0) == 0xB0)

/* States */

#define SI_STATE_UNINITIALIZED -1
#define SI_STATE_DELTA_TIME 0 /* Now at a delta time */
#define SI_STATE_COMMAND 1 /* Now at a MIDI operation */
#define SI_STATE_FINISHED 2 /* End of song */


#define SI_FINISHED -1 /* Song finished playing */
#define SI_LOOP -2 /* Song just looped */
#define SI_CUE -3 /* Found a song cue */
#define SI_PCM -4 /* Found a PCM */

#define SONG_ITERATOR_MESSAGE_ARGUMENTS_NR 2

/* Helper defs for messages */
/* Base messages */
#define _SIMSG_BASE 0 /* Any base decoder */
#define _SIMSG_BASEMSG_SET_LOOPS 0 /* Set loops */
#define _SIMSG_BASEMSG_CLONE 1 /* Clone object and data */
#define _SIMSG_BASEMSG_SET_PLAYMASK 2 /* Set the current playmask for filtering */

/* Messages */
#define SIMSG_SET_LOOPS(x) _SIMSG_BASE,_SIMSG_BASEMSG_SET_LOOPS,(x),0
#define SIMSG_SET_PLAYMASK(x) _SIMSG_BASE,_SIMSG_BASEMSG_SET_PLAYMASK,(x),0
#define SIMSG_CLONE _SIMSG_BASE,_SIMSG_BASEMSG_CLONE,0,0

/* Message transmission macro: Takes song reference, message reference */
#define SIMSG_SEND(o, m) songit_handle_message(&(o), songit_make_message(m))

typedef struct {
	unsigned int recipient; /* Type of iterator supposed to receive this */
	unsigned int type;
	unsigned int args[SONG_ITERATOR_MESSAGE_ARGUMENTS_NR];
} song_iterator_message_t;


#define INHERITS_SONG_ITERATOR \
	int (*next) (song_iterator_t *self, unsigned char *buf, int *buf_size);			  \
	unsigned char * (*get_pcm) (song_iterator_t *s, int *size, int *sample_rate, int *type);  \
	song_iterator_t * (* handle_message)(song_iterator_t *self, song_iterator_message_t msg); \
	void (*init) (struct _song_iterator *self);						  \
	void (*cleanup) (struct _song_iterator *self);						  \
	struct _song_iterator *delegate


typedef struct _song_iterator {
	int (*next) (struct _song_iterator *self,
		     unsigned char *buf, int *result);
	/* Reads the next MIDI operation _or_ delta time
	** Parameters: (song_iterator_t *) self
	**             (byte *) buf: The buffer to write to (needs to be able to
	**                           store at least 4 bytes)
	** Returns   : (int) zero if a MIDI operation was written, SI_FINISHED
	**                   if the song has finished playing, SI_LOOP if looping
	**                   (after updating the loop variable), SI_CUE if we found
	**                   a cue, SI_PCM if a PCM was found, or the number of ticks
	**                   to wait before this function should be called next.
	**             (int) *result: Number of bytes written to the buffer
	**                   (equals the number of bytes that need to be passed
	**                   to the lower layers) for 0, the cue value for SI_CUE,
	**                   or the number of loops remaining for SI_LOOP.
	**   If SI_PCM is returned, get_pcm() may be used to retrieve the associated
	** PCM, but this must be done before any subsequent calls to next().
	*/

	unsigned char * (*get_pcm) (struct _song_iterator *self,
				    int *size, int *sample_rate, int *type);
	/* Checks for the presence of a pcm sample
	** Parameters: (song_iterator_t *) self
	** Returns   : (byte *) NULL if no sample was found, a pointer to the
	**                      PCM data otherwise
	**             (int) *size: Return variable for the sample size
	**             (int) *sample_rate: Return variable for the sample rate
	**                                 in (Hz)
	**             (int) *PCM data type
	** Only unsigned mono 8 bit PCMs (AFMT_U8) are supported ATM.
	*/


	struct _song_iterator *
	(* handle_message)(struct _song_iterator *self, song_iterator_message_t msg);
	/* Handles a message to the song iterator
	** Parameters: (song_iterator_t *) self
	**             (song_iterator_messag_t) msg: The message to handle
	** Returns   : (song_iterator_t *) NULL if the message was not understood,
	**             self if the message could be handled, or a new song iterator
	**             if the current iterator had to be morphed (but the message could
	**             still be handled)
	** This function is not supposed to be called directly; use
	** songit_handle_message() instead. It should not recurse, since songit_handle_message()
	** takes care of that and makes sure that its delegate received the message (and
	** was morphed) before self.
	*/


	void (*init) (struct _song_iterator *self);
	/* Resets/initializes the sound iterator
	** Parameters: (song_iterator_t *) self
	** Returns   : (void)
	*/

	void (*cleanup) (struct _song_iterator *self);
	/* Frees any content of the iterator structure
	** Parameters: (song_iterator_t *) self
	** Does not physically free(self) yet. May be NULL if nothing needs to be done.
	** Must not recurse on its delegate.
	*/

	struct _song_iterator *delegate;
	/* A delegate, for stacking song iterators */

	/* See songit_* for the constructor and non-virtual member functions */

} song_iterator_t;

#define MIDI_CHANNELS 16

typedef struct _base_song_iterator {
	INHERITS_SONG_ITERATOR; /* aka "extends song iterator" */

	int flags[MIDI_CHANNELS]; /* Flags for each channel */
	int polyphony[MIDI_CHANNELS]; /* # of simultaneous notes on each */
	int instruments[MIDI_CHANNELS]; /* Instrument number for each channel */
	int velocity[MIDI_CHANNELS]; /* Velocity for each channel (0 for "mute") */
	int pressure[MIDI_CHANNELS]; /* Channel pressure (MIDI Dx command) */
	int pitch[MIDI_CHANNELS]; /* Pitch wheel */
	int channel_map[MIDI_CHANNELS]; /* Number of HW channels to use */
	int reverb[MIDI_CHANNELS]; /* Reverb setting for the channel */

	unsigned int size; /* Song size */
	int offset; /* Current read offset in data */
	int loop_offset; /* Loopback position */
	int loops; /* Number of loops */

	unsigned char last_cmd; /* Last MIDI command, for 'running status' mode */

	int state; /* SI_STATE_* */
	int ccc; /* Cumulative cue counter, for those who need it */
	unsigned char resetflag; /* for 0x4C -- on DoSound StopSound, do we return to start? */
	int playmask; /* Active playmask, default 0 */

	unsigned char *data;

} base_song_iterator_t;



/********************************/
/*-- Song iterator operations --*/
/********************************/

#define SCI_SONG_ITERATOR_TYPE_SCI0 0
#define SCI_SONG_ITERATOR_TYPE_SCI1 1

song_iterator_t *
songit_new(unsigned char *data, unsigned int size, int type);
/* Constructs a new song iterator object
** Parameters: (byte *) data: The song data to iterate over
**             (unsigned int) size: Number of bytes in the song
**             (int) type: One of the SCI_SONG_ITERATOR_TYPEs
** Returns   : (song_iterator_t *) A newly allocated but uninitialized song
**             iterator, or NULL if 'type' was invalid or unsupported
*/

void
songit_free(song_iterator_t *it);
/* Frees a song iterator and the song it wraps
** Parameters: (song_iterator_t *) it: The song iterator to free
** Returns   : (void)
*/

song_iterator_message_t
songit_make_message(int recipient, int type, int a1, int a2);
/* Create a song iterator message
** Parameters: (int) recipient: Message recipient
**             (int) type: Message type
**             (int x int) a1, a2: Arguments
** You should only use this with the SIMSG_* macros
*/

int
songit_handle_message(song_iterator_t **it_reg, song_iterator_message_t msg);
/* Handles a message to the song iterator
** Parameters: (song_iterator_t **): A reference to the variable storing the song iterator
** Returns   : (int) Non-zero if the message was understood
** The song iterator may polymorph as result of msg, so a writeable reference is required.
*/


song_iterator_t *
songit_clone(song_iterator_t *it);
/* Clones a song iterator
** Parameters: (song_iterator_t *) it: The iterator to clone
** Returns   : (song_iterator_t *) A shallow clone of 'it'.
** This performs a clone on the bottom-most part (containing the actual song data) _only_. 
*/

#endif
