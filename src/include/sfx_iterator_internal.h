/***************************************************************************
 sfx_iterator_internal.h Copyright (C) 2003 Christoph Reichenbach


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

#ifndef _SFX_ITERATOR_INTERNAL_
#define _SFX_ITERATOR_INTERNAL_

#include <sfx_iterator.h>

#define MIDI_RHYTHM_CHANNEL 9

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

#define SI_STATE_UNINITIALISED -1
#define SI_STATE_DELTA_TIME 0 /* Now at a delta time */
#define SI_STATE_COMMAND 1 /* Now at a MIDI operation */
#define SI_STATE_PENDING 2 /* Pending for loop */
#define SI_STATE_FINISHED 3 /* End of song */
#define SI_STATE_PCM 4 /* Should report a PCM next (-> DELTA_TIME) */


/* Iterator types */

#define SCI_SONG_ITERATOR_TYPE_SCI0 0
#define SCI_SONG_ITERATOR_TYPE_SCI1 1

#define SIPFX __FILE__" : "


typedef struct {
	int state;	/* SI_STATE_* */
	int offset;     /* Offset into the data chunk */
	int end;	/* Last allowed byte in track */
	int id;		/* Some channel ID */
	int loop_offset;
	int delay;	/* Number of ticks before the
			** specified channel is next
			** used, or
			** CHANNEL_DELAY_MISSING to
			** indicate that the delay has
			** not yet been read  */

	/* Two additional offsets for recovering: */
	int initial_offset;
	int playmask; /* Active playmask (MIDI channels to play in here) */
	int notes_played; /* #of notes played since the last loop start */
	int loop_timepos; /* Total delay for this channel's loop marker */
	int total_timepos; /* Number of ticks since the beginning, ignoring loops */
	int timepos_increment; /* Number of ticks until the next command (to add) */

	byte last_cmd;	/* Last operation executed, for running status */
} song_iterator_channel_t;

#define INHERITS_BASE_SONG_ITERATOR								\
	INHERITS_SONG_ITERATOR; /* aka "extends song iterator" */				\
												\
	int polyphony[MIDI_CHANNELS]; /* # of simultaneous notes on each */			\
												\
												\
	int ccc; /* Cumulative cue counter, for those who need it */				\
	unsigned char resetflag; /* for 0x4C -- on DoSound StopSound, do we return to start? */	\
	int device_id; /* ID of the device we generating events for */				\
	int active_channels; /* Number of active channels */					\
	unsigned int size; /* Song size */							\
	unsigned char *data;									\
												\
	int loops; /* Number of loops remaining */						\
	int recover_delay;									\

typedef struct _base_song_iterator {
	INHERITS_BASE_SONG_ITERATOR;
} base_song_iterator_t;

/********************************/
/*--------- SCI 0 --------------*/
/********************************/

typedef struct {
	INHERITS_BASE_SONG_ITERATOR;
	song_iterator_channel_t channel;
	int delay_remaining; /* Number of ticks that haven't been polled yet */
} sci0_song_iterator_t;


/********************************/
/*--------- SCI 1 --------------*/
/********************************/


typedef struct _sci1_sample {
	int delta; /* Time left-- initially, this is 'Sample point 1'.
		   ** After initialisation, it is 'sample point 1 minus the sample point of the previous sample'  */
	int size;
	int announced; /* Announced for download (SI_PCM) */
	sfx_pcm_config_t format;
	byte *data;
	struct _sci1_sample *next;
} sci1_sample_t;

typedef struct {
	INHERITS_BASE_SONG_ITERATOR;
	song_iterator_channel_t channels[MIDI_CHANNELS];

	/* Invariant: Whenever channels[i].delay == CHANNEL_DELAY_MISSING,
	** channel_offset[i] points to a delta time object. */

	int initialised; /* Whether the MIDI channel setup has been initialised */
	int channels_nr; /* Number of channels actually used */
	sci1_sample_t *next_sample;
	int channels_looped; /* Number of channels that are ready to loop */

	int delay_remaining; /* Number of ticks that haven't been polled yet */
} sci1_song_iterator_t;

#define PLAYMASK_NONE 0x0


/**********************************/
/*--------- Fast Forward ---------*/
/**********************************/

typedef struct {
	INHERITS_SONG_ITERATOR;
	int delta; /* Remaining time */
} fast_forward_song_iterator_t;


song_iterator_t *
new_fast_forward_iterator(song_iterator_t *it, int delta);
/* Creates a new song iterator which fast-forwards
** Parameters: (song_iterator_t *) it: The iterator to wrap
**             (int) delta: The number of ticks to skip
** Returns   : (song_iterator_t) A newly created song iterator
**                               which skips all note commands and
**                               delta times until 'delta' has been
**                               used up
*/

/**********************************/
/*--------- Fast Forward ---------*/
/**********************************/

#define MAX_BUF_SIZE 4

#define TEE_NONE 0
/* Otherwise, we detail where the 'next' is from: */
#define TEE_LEFT 1
#define TEE_RIGHT 2
#define TEE_IT(selector) (((selector) == TEE_LEFT)? it->left : it->right)
#define TEE_NOTIT(selector) (((selector) == TEE_RIGHT)? it->left : it->right)

typedef struct {
	INHERITS_SONG_ITERATOR;
	song_iterator_t *left;
	song_iterator_t *right;


	int active_left, active_right;
	int has_next; /* If a 'next' result is cached */
	int reported_pcm; /* Did someone report a PCM? */

	byte next_buf[MAX_BUF_SIZE];
	int next_result;
	int next_retval;
} tee_song_iterator_t;


sfx_pcm_feed_t *
sfx_iterator_make_feed(byte *base_data, int offset,
		       int size,
		       sfx_pcm_config_t conf);
/* Generates a feed for a song iterator
** Parameters: (byte *) base_data: A refcounted memory chunk containing
**                                 (among other things) PCM data
**             (int) offset; Offset into base_data
**             (int) size: Number of bytes to consider
**             (pcm_data_internal_t) conf: PCM encoding
*/

#endif /* !defined(_SFX_ITERATOR_INTERNAL_ */
