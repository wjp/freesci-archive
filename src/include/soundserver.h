/***************************************************************************
 soundserver.h (C) 1999, 2000 Christoph Reichenbach, TU Darmstadt


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

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

***************************************************************************/
/* Auxiliary functions for sound server implementation- this stuff is just there
** to help you, as opposed to the code from sound.h.
*/

#ifndef _SCI_SOUND_SERVER_H_
#define _SCI_SOUND_SERVER_H_

#include <sound.h>
#include <scitypes.h>

#ifdef PIPE_BUF
#  define SOUND_SERVER_XFER_SIZE PIPE_BUF
#else
#  define SOUND_SERVER_XFER_SIZE 256 /* be _very_ conservative */
#endif

#define SOUND_SERVER_XFER_ABORT -1
#define SOUND_SERVER_XFER_OK 0
#define SOUND_SERVER_XFER_WAITING 1
#define SOUND_SERVER_XFER_TIMEOUT -1000

#define SOUND_SERVER_TIMEOUT 100000
/* microseconds until SOUND_COMMAND_TEST fails */
#define SOUND_TICK 1000000 / 60
/* Approximately 16666 microseconds */


#define SCI_MIDI_EOT 0xfc
/* End of track command */
#define SCI_MIDI_CONTROLLER(status) ((status & 0xf0) == 0xb0)

#define SCI_MIDI_SET_SIGNAL 0xcf
#define SCI_MIDI_SET_POLYPHONY 0x4b
#define SCI_MIDI_RESET_ON_STOP 0x4c
#define SCI_MIDI_SET_VELOCITY 0x4e
#define SCI_MIDI_SET_REVERB 0x50
#define SCI_MIDI_CUMULATIVE_CUE 0x60

#define SCI_MIDI_SET_SIGNAL_LOOP 0x7f
/* If this is the parameter of 0xcf, the loop point is set here */

#define SCI_MIDI_TIME_EXPANSION_PREFIX 0xf8
#define SCI_MIDI_TIME_EXPANSION_LENGHT 240

/* [RS] tsk tsk tsk */
/* [CR] WTF? */
#define SCI_MIDI_TIME_EXPANSION_LENGTH SCI_MIDI_TIME_EXPANSION_LENGHT

/* INTERNAL SOUND COMMANDS */
#define SOUND_COMMAND_MAPPINGS 100
/* PARAMETER sound mappings are incoming. First comes an int describing their byte size,
** then PARAMETER blocks of this size, all of them valid General MIDI commands for channel 0.
*/
#define SOUND_COMMAND_SHUTDOWN -1
/* Tells the sound server to die. Used by the default exit implementation.
** The server must not reply to this.
*/


#define SOUND_STATUS_STOPPED   0
#define SOUND_STATUS_PLAYING   1
#define SOUND_STATUS_SUSPENDED 2
/* suspended: only if ordered from kernel space */
#define SOUND_STATUS_WAITING   3
/* "waiting" means "tagged for playing, but not active right now" */

#define MIDI_CHANNELS 16

typedef struct _song {

  int flags[MIDI_CHANNELS]; /* Flags for each channel */
  int polyphony[MIDI_CHANNELS]; /* # of simultaneous notes on each */
  int instruments[MIDI_CHANNELS]; /* Instrument number for each channel */
  int velocity[MIDI_CHANNELS]; /* Velocity for each channel (0 for "mute") */
  int pressure[MIDI_CHANNELS]; /* Channel pressure (MIDI Dx command) */
  int pitch[MIDI_CHANNELS]; /* Pitch wheel */
  int channel_map[MIDI_CHANNELS]; /* Number of HW channels to use */

  int size; /* Song size */
  int pos;  /* Current position in song */
  int loopmark; /* loop position */
  int fading;   /* Ticks left until faded out, or -1 if not fading */

  byte *data;   /* dynamically allocated data */
  int file_nr;  /* Temporarily used to save and restore song data */

  int priority; /* Song priority */
  int loops;    /* Loops left to do */
  int status;   /* See above */

  int resetflag; /* for 0x4C -- on DoSound StopSound, do we return to start? */
  word handle;  /* Handle for the game engine */

  struct _song *next; /* Next song or NULL if this is the last one */

} song_t;


typedef song_t **songlib_t;

typedef struct _sound_event_queue_node {

  sound_event_t *event;
  struct _sound_event_queue_node *prev, *next;

} sound_eq_node_t;


typedef struct {

  sound_eq_node_t *first, *last;

} sound_eq_t;


extern sound_event_t sound_eq_eoq_event; /* An "end of queue" event */


int
sound_init_pipes(struct _state *s);
/* Initializes the data pipes
** Parameters: (state_t *) s: Our state
** Returns   : (int) 0 on success, 1 otherwise
** This function initializes sound_pipe_in, sound_pipe_out and sound_pipe_debug,
** which are used by the sfx default implementations for data transmission.
** Also installs signal handles for SIGCHLD and SIGPIPE.
*/

int
sound_map_instruments(struct _state *s);
/* Maps instruments and transmits corresponding MIDI sequences to the sound server
** Parameters: (state_t *) s: The state
** Returns   : (int) 0 on success, 1 otherwise
*/

sound_event_t*
sound_get_event(struct _state *s);
/* Default implementation for get_event from the sfx_driver_t structure. */

int
sound_command(struct _state *s, int command, int handle, int parameter);
/* Default implementation for command from the sfx_driver_t structure. */

void
sound_exit(struct _state *s);
/* Default implementation for exit from the sfx_driver_t structure. */

int
sound_save(struct _state *s, char *dir);
/* Default implementation for saving the sound status */

int
sound_restore(struct _state *s, char *dir);
/* Default implementation for restoring the sound state */

void
sound_suspend(struct _state *s);
/* Default implementation for suspending sound output */

void
sound_resume(struct _state *s);
/* Default implementation for resuming sound ouput after it was suspended */

/* Song library commands: */


song_t *
song_new(word handle, byte *data, int size, int priority);
/* Initializes a new song
** Parameters: (word) handle: The sound handle
**             (byte *) data: The sound data
**             (int) size: The song size
**             (int) priority: The song's priority
** Returns   : (song_t *) A freshly allocated song
** Other values are set to predefined defaults.
*/


void
song_lib_free(songlib_t songlib);
/* Frees a song library
** Parameters: (songlib_t) songlib: The library to free
** Returns   : (void)
*/


void
song_lib_add(songlib_t songlib, song_t *song);
/* Adds a song to a song library.
** Parameters: (songlib_t) songlib: An existing sound library, or NULL
**             (song_t *) song: The song to add
** Returns   : (void)
*/

song_t *
song_lib_find(songlib_t songlib, word handle);
/* Looks up the song with the specified handle
** Parameters: (songlib_t) songlib: An existing sound library, may point to NULL
**             (word) handle: The sound handle to look for
** Returns   : (song_t *) The song or NULL if it wasn't found
*/

song_t *
song_lib_find_active(songlib_t songlib, song_t *last_played_song);
/* Finds the song playing with the highest priority
** Parameters: (songlib_t) songlib: An existing sound library
**             (song_t *) last_played_song: The song that was played last
** Returns   : (song_t *) The song that should be played next, or NULL if there is none
*/

int
song_lib_remove(songlib_t songlib, word handle);
/* Removes a song from the library
** Parameters: (songlib_t) songlib: An existing sound library
**             (word) handle: Handle of the song to remove
** Returns   : (int) The status of the song that was removed
*/

void
song_lib_resort(songlib_t songlib, song_t *song);
/* Removes a song from the library and sorts it in again; for use after renicing
** Parameters: (songlib_t) songlib: An existing sound library
**             (song_t *) song: The song to work on
** Returns   : (void)
*/

GTimeVal
song_sleep_time(GTimeVal *lastslept, int ticks);
/* Caluculates the amount of seconds and microseconds to sleep.
** Parameters: (GTimeVal *) lastslept: The time to start counting on
**             (int) ticks: Number of ticks to sleep
** Returns   : (GTimeVal) The amount of time to sleep
*/

GTimeVal
song_next_wakeup_time(GTimeVal *lastslept, int ticks);
/* Calculates the time at which "ticks" have passed, counting from "lastslept".
** Parameters: (GTimeVal *) lastslept: The base to start counting on
**             (int) ticks: Number of ticks to count
** Returns   : (GTimeVal) A structure describing the time at which the
**                              specified number of ticks has passed
*/


int
soundsrv_save_state(FILE *debugstream, char *dir, songlib_t songlib, song_t *curr_song,
		    int soundcue, int usecs_to_sleep, int ticks_to_wait, int ticks_to_fade);
/*·Stores the sound server state to a file
** Parameters: (FILE *) debugstream: The stream which all errors are sent to
**             (char *) dir: The name of the directory to enter and write to
**             (songlib_t) songlib: The song library to write
**             (song_t *) curr_song: Pointer to the currently active song
**             (int) soundcute: Status of the sound cue variable
**             (int) usecs_to_sleep: Milliseconds the sound server has to sleep before the next tick
**             (int) ticks_to_wait: Ticks the sound server has to wait before the next event
**             (int) ticks_to_fade: Number of ticks left for fading (if fading is attempted)
** Returns   : (int) 0 on success, 1 otherwise
*/


int
soundsrv_restore_state(FILE *debugstream, char *dir, songlib_t songlib, song_t **curr_song,
		       int *soundcue, int *usecs_to_sleep, int *ticks_to_wait, int *ticks_to_fade);
/* Restores the sound state written to a directory
** Parameters: (FILE *) debugstream: The stream to write all debug information to
**             (char *) dir: The directory to enter and read from
**             (songlib_t) songlib: The song library to overwrite (if successful)
**             (song_t **) curr_song: The "currently active song" pointer to overwrite
**             (int *) soundcue: The sound cue variable to set
**             (int *) usecs_to_sleep: The "milliseconds left to sleep" variable to set
**             (int *) ticks_to_wait: The "ticks left to wait" variable to set
**             (int *) ticks_to_fade: THe "number of ticks to fade" variable to set
** Returns   : (int) 0 on success, 1 otherwise
** If restoring failed, an error message will be written to debugstream, and the
** variables pointed to in the parameter list will be left untouched.
*/



/**********************/
/* SOUND EVENT QUEUES */
/**********************/

void
sound_eq_init(sound_eq_t *queue);
/* Initializes an event queue.
** Parameters: (sound_eq_t *) queue: Pointer to the queue to initialize
** Returns   : (void)
** This function must be called to the other event queue functions to operate
** normally, unless the queue was allocated with calloc().
*/

void
sound_eq_queue_event(sound_eq_t *queue, int handle, int signal, int value);
/* Queues an event into an event queue.
** Parameters: (sound_eq_t *) queue: The queue to add the event to
**             (int x int x int) (handle, signal, value): The event data to queue
** Returns   : (void)
*/

sound_event_t *
sound_eq_retreive_event(sound_eq_t *queue);
/* Retreives the oldest event on the event queue and removes it from the queue.
** Parameters: (sound_eq_t *) queue: The queue to retreive the event from
** Returns   : (sound_event_t *): A pointer to the oldest event on the queue, or NULL of there is none
** The return value must be free()d manually, if appropriate.
*/

void sci_midi_command(song_t *song, guint8 command, guint8 param,
                      guint8 param2, 
		      int *ccc, FILE *ds);
/* performs a regular midi event in the song. */

#endif /* !_SCI_SOUND_SERVER_H_ */
