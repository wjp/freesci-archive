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

/* #define DEBUG_SOUND_SERVER 1 */

#define SOUND_SERVER_TIMEOUT 100000
/* microseconds until SOUND_COMMAND_TEST fails */
#define SOUND_TICK 1000000 / 60
/* Approximately 16666 microseconds */


/* INTERNAL SOUND COMMANDS */
#define SOUND_COMMAND_MAPPINGS 100
/* PARAMETER sound mappings are incoming. First comes an int describing their byte size,
** then PARAMETER blocks of this size, all of them valid General MIDI commands for channel 0.
*/

#define SOUND_STATUS_STOPPED   0
#define SOUND_STATUS_PLAYING   1
#define SOUND_STATUS_SUSPENDED 2
/* suspended: only if ordered from kernel space */
#define SOUND_STATUS_WAITING   3
/* "waiting" means "tagged for playing, but not active right now" */

#define MIDI_CHANNELS 16

#define MIDI_MUTE_OFF 0
#define MIDI_MUTE_ON  1

FILE *debug_stream;	/* Where error messages are output to */


#define SI_STATE_UNINITIALIZED -1
#define SI_STATE_DELTA_TIME 0 /* Now at a delta time */
#define SI_STATE_COMMAND 1 /* Now at a MIDI operation */
#define SI_STATE_FINISHED 2 /* End of song */


#define SI_FINISHED -1 /* Song finished playing */
#define SI_LOOP -2 /* Song just looped */
#define SI_CUE -3 /* Found a song cue */

typedef struct _song_iterator {

	int flags[MIDI_CHANNELS]; /* Flags for each channel */
	int polyphony[MIDI_CHANNELS]; /* # of simultaneous notes on each */
	int instruments[MIDI_CHANNELS]; /* Instrument number for each channel */
	int velocity[MIDI_CHANNELS]; /* Velocity for each channel (0 for "mute") */
	int pressure[MIDI_CHANNELS]; /* Channel pressure (MIDI Dx command) */
	int pitch[MIDI_CHANNELS]; /* Pitch wheel */
	int channel_map[MIDI_CHANNELS]; /* Number of HW channels to use */
	int reverb[MIDI_CHANNELS]; /* Reverb setting for the channel */

	byte *data;
	unsigned int size; /* Song size */
	int offset; /* Current read offset in data */
	int loop_offset; /* Loopback position */
	int loops; /* Number of loops */

	byte last_cmd; /* Last MIDI command, for 'running status' mode */

	int state; /* SI_STATE_* */
	int ccc; /* Cumulative cue counter, for those who need it */
	byte resetflag; /* for 0x4C -- on DoSound StopSound, do we return to start? */

	int (*read_next_command) (struct _song_iterator *self,
				  byte *buf, int *buf_size);
	/* Reads the next MIDI operation _or_ delta time
	** Parameters: (song_iterator_t *) self
	**             (byte *) buf: The buffer to write to (needs to be able to
	**                           store at least 4 bytes)
	**             (int *) buf_size: Number of bytes written to the buffer
	**                     (equals the number of bytes that need to be passed
	**                     to the lower layers)
	** Returns   : (int) zero if a MIDI operation was written, SI_FINISHED
	**                   if the song has finished playing, SI_LOOP if looping
	**                   (after updating the loop variable), SI_CUE if we found
	**                   a cue, or the number of ticks to wait before this
	**                   function should be called next.
	** If non-zero is returned, buf and buf_size are not written to, except
	** with SI_CUE, where buf[0] will be set to contain the cue value.
	*/

	byte * (*check_pcm) (struct _song_iterator *self,
			     int *size, int *sample_rate);
	/* Checks for the presence of a pcm sample
	** Parameters: (song_iterator_t *) self
	**             (int *) size: Return variable for the sample size
	**             (int *) sample_rate: Return variable for the sample rate
	**                                  in (Hz)
	** Returns   : (byte *) NULL if no sample was found, a pointer to the
	**                      PCM data otherwise
	** Only unsigned mono 8 bit PCMs (AFMT_U8) are supported ATM.
	*/

	void (*init) (struct _song_iterator *self);
	/* Resets/initializes the sound iterator
	** Parameters: (song_iterator_t *) self
	** Returns   : (void)
	*/

	/* See songit_* for the constructor and non-virtual member functions */

} song_iterator_t;


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
	int maxfade;  /* Total ticks in the fade (used to calculate volume */

	int reverb;   /* current reverb setting */

	byte *data;   /* dynamically allocated data */

	int priority; /* Song priority (more important if priority is higher) */
	int loops;    /* Loops left to do */
	int status;   /* See above */

	int resetflag; /* for 0x4C: does suspend reset song position? */
	unsigned int handle;  /* Handle for the game engine */

	int file_nr;	/* used for saving */
	song_iterator_t *it;

	int shared;   /* zero if *data is not shared with anyone else */

	struct _song *next; /* Next song or NULL if this is the last one */
} song_t;


void
dump_song_pos(int i, song_t *song);
/* Dumps data at a song position to stderr.
** Parameters: (int) i: Song position
**             (song_t *) song: Song to dump
*/

void
dump_song(song_t *song);
/* Dumps song to stderr.
** Parameters: (song_t *) song: Song to dump
*/

typedef song_t **songlib_t;


typedef struct _sound_event_queue_node {

	sound_event_t *event;
	struct _sound_event_queue_node *prev, *next;

} sound_eq_node_t;


typedef struct {

	sound_eq_node_t *first, *last;

} sound_eq_t;


extern sound_event_t sound_eq_eoq_event; /* An "end of queue" event */

extern sound_eq_t queue; /* The event queue */


int
sound_command_default(struct _state *s, unsigned int command, unsigned int handle, long value);
/* Interface for other parts of FreeSCI to command the sound server
** Parameters: (struct _state *) s: Game state
**             (unsigned int) command: Command to process and queue for sound server
**             (unsigned int) handle: Song handle (usually)
**             (long) value: Some parameter
** Returns   : (int) A value dependent on what is relevant for the command
*/

int
sound_save_default(struct _state *s, char *dir);
/* Default implementation for saving the sound status
** Parameters: (struct _state *) s: Game state
**             (char *) dir: Where to save
** Returns   : (int) FIXME
*/

int
sound_restore_default(struct _state *s, char *dir);
/* Default implementation for restoring the sound state
** Parameters: (struct _state *) s: Game state
**             (char *) dir: Where to restore from
** Returns   : (int) FIXME
*/

void
sound_suspend_default(struct _state *s);
/* Default implementation for suspending sound output
** Parameters: (struct _state *) s: Game state
** Returns   : (void)
*/

void
sound_resume_default(struct _state *s);
/* Default implementation for resuming sound output after it was suspended
** Parameters: (struct _state *) s: Game state
** Returns   : (void)
*/


/********************************/
/*-- Song iterator operations --*/
/********************************/

#define SCI_SONG_ITERATOR_TYPE_SCI0 0
#define SCI_SONG_ITERATOR_TYPE_SCI1 1

song_iterator_t *
songit_new(byte *data, unsigned int size, int type);
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
song_lib_init(songlib_t *songlib);
/* Initializes a static song library
** Parameters: (songlib_t *) songlib: Pointer to the library
**             to initialize
** Returns   : (void)
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
song_lib_remove(songlib_t songlib, word handle, resource_mgr_t *resmgr);
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

int
song_lib_count(songlib_t songlib);
/* Counts the number of songs in a song library
** Parameters: (songlib_t) songlib: The library to count
** Returns   : (int) The number of songs
*/


GTimeVal
song_sleep_time(GTimeVal *lastslept, long ticks);
/* Caluculates the amount of seconds and microseconds to sleep.
** Parameters: (GTimeVal *) lastslept: The time to start counting on
**             (long) ticks: Number of ticks to sleep
** Returns   : (GTimeVal) The amount of time to sleep
*/

GTimeVal
song_next_wakeup_time(GTimeVal *lastslept, long ticks);
/* Calculates the time at which "ticks" have passed, counting from "lastslept".
** Parameters: (GTimeVal *) lastslept: The base to start counting on
**             (long) ticks: Number of ticks to count
** Returns   : (GTimeVal) A structure describing the time at which the
**                              specified number of ticks has passed
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
sound_eq_queue_event(sound_eq_t *queue, unsigned int handle, unsigned int signal, long value);
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

sound_event_t *
sound_eq_peek_event(sound_eq_t *queue);
/* FIXME: Verify this is correct */
/* Retreives the oldest event on the event queue without removing it from the queue.
** Parameters: (sound_eq_t *) queue: The queue to retreive the event from
** Returns   : (sound_event_t *): A pointer to the oldest event on the queue, or NULL of there is none
** The return value must be free()d manually, if appropriate.
*/

void sci_midi_command(FILE *debugstream, song_t *song, guint8 command, guint8 param,
		      guint8 param2, guint32 other_data, int *ccc, int master_volume);
/* Performs a regular midi event in the song.
** Parameters: (FILE *) debugstream: The stream to write all debug information to
**             (song_t *) song: The song to play the event from
**             (guint8) command: MIDI command
**             (guint8) param: MIDI command parameter 1
**             (guint8) param2: MIDI command parameter 2
**             (guint32) other_data: Driver-specific data
**             (int *) ccc: Cumulative cue counter
**             (int) master_volume: (duh)
*/


/* Sound server has its own cwd (relative to the main program) */
#define SOUNDSERVER_FLAG_SEPARATE_CWD (1 << 0)

/* Soundserver operates within the same memory context as themain program.
   Meaning we access the song data off the heap directly */
#define SOUNDSERVER_FLAG_SHAREDMEM (1 << 1)

#define SOUNDSERVER_INIT_FLAG_REVERSE_STEREO (1 << 0) /* Reverse pan control changes */

#define SOUNDSERVER_FLAG_PIPED (1 << 0)	/* driver is piped (UNIX) */

typedef struct {

	char *name; /* Name of this particular driver */
	char *version; /* Driver's version number */

	int flags;

	int (*init)(struct _state *s, int flags);
	/* Initializes the sound driver
	** Parameters: (state_t *) s: The state that we're going to play on
	**             (int) flags: Any SOUNDSERVER_INIT_FLAG combination
	** Returns   : (int) 0 if successful, 1 if failed
	*/

	int (*configure)(struct _state *s, char *option, char *value);
	/* Set a particular configuration option
	** Parameters: (state_t *) s: The state_t to operate on
	**             (char *) option: The option to set
	**             (char *) value: The value to set it to
	** Returns   : (int) 0 if "option" could be interpreted by the driver, regardless
	**                   of whether value was correct, or 1 if the option does not apply
	**                   to this particular driver.
	*/

	void (*exit)(struct _state *s);
	/* Stops playing sound, uninitializes the sound driver, and frees all associated memory
	** Parameters: (state_t *) s: The state_t to operate on
	** Returns   : (void)
	*/

	sound_event_t* (*get_event)();
	/* Synchronizes the sound driver with the rest of the world
	** Parameters: none
	** Returns   : (sound_event_t *) Pointer to a dynamically allocated sound_event_t structure
	**                               containing the next event, or NULL if none is available
	** This function should be called at least 60 times per second. It will return finish, loop,
	** and cue events, which can be written directly to the sound objects.
	*/

	void (*queue_event)(unsigned int handle, unsigned int signal, long value);
	/* Queue a signal for the main thread
	** Parameters: (unsigned int) handle: Song handle for signal (usually)
	**             (unsigned int) signal: The signal to pass to the main thread
	**             (long) value: Parameter for signal
	** Returns   : (void)
	*/

	sound_event_t* (*get_command)(GTimeVal *wait_tvp);
	/* Gets a sound command from the main thread
	** Parameters: (GTimeVal *) wait_tvp: How long to wait before giving up
	**                                    (not always used)
	** Returns   : (sound_event_t *) The command received
	*/

	void (*queue_command)(unsigned int handle, unsigned int signal, long value);
	/* Called by (*command) (see below) to queue a command for the sound
	** server. The server should be the only part of the program that uses this
	** function directly.
	** Parameters: (unsigned int) handle: Song handle for command (usually)
	**             (unsigned int) signal: The command to pass to the main thread
	**             (long) value: Parameter for command
	** Returns   : (void)
	*/

	int (*get_data)(byte **data_ptr, int *size);
	/* Gets data from the main thread
	** Parameters: (byte **) data_ptr: Pointer to where data will be received
	**             (int *) size: Size of data received
	** Returns   : (int) Size of data received
	*/

	int (*send_data)(byte *data_ptr, int maxsend);
	/* Sends data to the sound server
	** Parameters: (byte *) data_ptr: Data to send
	**             (int) maxsend: Size of the data to expect
	** Returns   : (int) Size of data received
	*/

	int (*save)(struct _state *s, char *name);
	/* Saves the sound system state to the directory /name/.
	** Parameters: (state_t *) s: The current state
	**             (char *) name: The directory name of the directory to write to (must exist)
	** Returns   : (int) 0 on success, 1 otherwise
	*/

	int (*restore)(struct _state *s, char *name);
	/* Restores the sound system state from the directory with the specified name.
	** Parameters: (state_t *) s: The current state
	**             (char *) name: The name of the directory to read from
	** Returns   : (int) 0 on success, 1 otherwise
	*/

	int (*command)(struct _state *s, unsigned int command, unsigned int param1, long param2);
	/* Executes a sound command (one of SOUND_COMMAND_xxx). Used by other
	** parts of FreeSCI such as the kernel to control the sound server.
	** Parameters: (int) command: The command to execute
	**             (unsigned int) param1: First parameter (traditionally the song handle)
	**             (long) param2: Second parameter
	*/

	void (*suspend)(struct _state *s);
	/* Suspends the sound subsystem
	** Parameters: (state_t *) s: The current state
	** Only resume, shutdown, save and restore commands need to be handled in suspended mode.
	*/

	void (*resume)(struct _state *s);
	/* Resumes the sound subsystem
	** Parameters: (state_t *) s: The current state
	*/

	void (*poll)();
	/* This will poll the current sound driver */

} sound_server_t;

extern DLLEXTERN sound_server_t *sound_servers[]; /* All available sound fx drivers, NULL-terminated */

extern sound_server_t *global_sound_server; /* current soundserver */

#define SOUND_SERVER_STATE_SAVED_MEMBERS \
	songlib_t songlib;	/* sound server's song library */ \
	song_t *current_song;	/* currently playing song */ \
	unsigned int master_volume;	/* (stored as percentage) */ \
	int sound_cue;	/* cumulative cue counter */

typedef struct {
	int suspended;	/* if sound server is suspended */
	sound_server_t *ss_driver;	/* driver currently being used for sound server */
	resource_mgr_t *resmgr;
	int reverse_stereo;	/* reverse stereo setting */
	byte mute_channel[MIDI_CHANNELS];	/* which channels are muted */

	SOUND_SERVER_STATE_SAVED_MEMBERS

} sound_server_state_t;


void
sci0_event_ss(sound_server_state_t *ss_state);

void
sci0_polled_ss(int reverse_stereo, sound_server_state_t *ss_state);


int
soundsrv_save_state(FILE *debugstream, char *dir, sound_server_state_t *sss);
/* Stores the sound server state to a file
** Parameters: (FILE *) debugstream: The stream which all errors are sent to
**             (char *) dir: The name of the directory to enter and write to
**             (ss_state*) sss: Sound server state to save
** Returns   : (int) 0 on success, 1 otherwise
*/


int
soundsrv_restore_state(FILE *debugstream, char *dir, sound_server_state_t *sss);
/* Restores the sound state written to a directory
** Parameters: (FILE *) debugstream: The stream to write all debug information to
**             (char *) dir: The directory to enter and read from
**             (ss_state*) sss: Sound server state to restore into
** Returns   : (int) 0 on success, 1 otherwise
** If restoring failed, an error message will be written to debugstream, and the
** variables pointed to in the parameter list will be left untouched.
*/


void
_restore_midi_state(sound_server_state_t *ss_state);
/* Restores some parts of the MIDI state
** Parameters: (struct sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/


sound_server_t *
sound_server_find_driver(char *name);
/* Searches for a sound server
** Parameters: (char *) name: Name of the sound server to look for, or NULL for the default
** Returns   : (sound_server_t *): A pointer to the first matching driver
*/


/***********************************/
/*-- Sound server core functions --*/
/***********************************/

/* The following functions should be used directly by sound server
** implementors, if possible- they implement processing commands
** and doing 'regular' maintenance, i.e. playing notes at regular
** intervals.
*/

sound_server_state_t *
snd_new_server_state(sound_server_t *srv);
/* Creates a new sound server state structure
** Parameters: (sound_server_t *) srv: The sound server to set
**                                as the state's server
** Returns   : (sound_server_state_t *)
*/

void
snd_free_server_state(sound_server_state_t *state);
/* Destroys a sound server state structure
** Parameters: (sound_server_state_t *) state: The state to destroy
** Returns   : (void)
** Typically only called during sound server shutdown.
*/

#define SND_SRV_PLAY_INDEF -1

gint32
snd_srv_play(sound_server_state_t *s);
/* Determines whether a MIDI command should be processed and, if positive, does so
** Parameters:(sound_server_state_t *) s: Pointer to a structure storing
**                                     internal state information
** Returns   : (int32) The number of microseconds the server should wait before
**                     calling this function again, or SND_SRV_PLAY_INDEF to wait
**                     indefinitely.
** snd_srv_play() MAY be called before the time has passed; it will then decide whether
** to return without doing anything or to ignore the timing inaccuracy.
** When SND_SRV_PLAY_INDEF is returned, the caller only needs to wait for incoming
** commands and should call snd_srv_process() before even bothering with snd_srv_play().
** s->queue_event() is called from here on occasion, to send back events.
*/

void
snd_srv_process(sound_server_state_t *s, sound_event_t cmd);
/* Processes a sound server command
** Parameters: (sound_server_state_t *) s: Sound server state
**             (sound_event_t) cmd: The sound command to process
** Returns   : (void)
** This function must be invoked with 'cmd' filled out whenever a command was
** received.
** s->queue_event() is called from here on occasion, to send back events.
** snd_srv_play() should be called immediately after snd_srv_process().
*/

/***********************************************/
/*-- Operations utilized by the sound server --*/
/***********************************************/

void
init_handle(int priority, word song_handle, sound_server_state_t *ss_state);
/* Commands server to initialise a song handle
** Parameters: (int) priority: Priority for new handle
**             (word) song_handle: New song handle
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
play_handle(int priority, word song_handle, sound_server_state_t *ss_state);
/* Commands server to play a song handle
** Parameters: (int) priority: Priority to play at
**             (word) song_handle: Song handle to play
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
stop_handle(word song_handle, sound_server_state_t *ss_state);
/* Commands server to stop a song
** Parameters: (word) song_handle: Song handle of song to stop
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
suspend_handle(word song_handle, sound_server_state_t *ss_state);
/* Commands server to suspend (pause) a song
** Parameters: (word) song_handle: Song handle of song to suspend
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
resume_handle(word song_handle, sound_server_state_t *ss_state);
/* Commands server to resume a suspended song
** Parameters: (word) song_handle: Song handle of song to resume
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
renice_handle(int priority, word song_handle, sound_server_state_t *ss_state);
/* Commands server to change the priority of a song
** Parameters: (int) priority: New priority of song
**             (word) song_handle: Song handle of song to renice
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
fade_handle(int ticks, word song_handle, sound_server_state_t *ss_state);
/* Commands server to fade a song
** Parameters: (int) ticks: Number of ticks to fade song within
**             (word) song_handle: Song handle of song to fade
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
loop_handle(int loops, word song_handle, sound_server_state_t *ss_state);
/* Commands server to loop a song
** Parameters: (int) loops: FIXME
**             (word) song_handle: Song handle of song to loop
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
dispose_handle(word song_handle, sound_server_state_t *ss_state);
/* Commands server to dispose (destroy) a song handle
** Parameters: (word) song_handle: Song handle of song to dispose
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
set_channel_mute(int channel, unsigned char mute_setting, sound_server_state_t *ss_state);
/* Commands server to mute a MIDI channel
** Parameters: (int) channel: MIDI channel to change the mute setting for
**             (unsigned char) mute_setting: MUTE_ON or MUTE_OFF
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
set_master_volume(guint8 new_volume, sound_server_state_t *ss_state);
/* Commands server to set the master (global) volume
** Parameters: (guint8) new_volume: New master volume
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
sound_check(int mid_polyphony, sound_server_state_t *ss_state);
/* Commands server to test driver (exchanges MIDI polyphony setting between
** main thread and sound server.
** Parameters: (int) mid_polyphony: MIDI polyphony value to transfer
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
stop_all(sound_server_state_t *ss_state);
/* Commands server to stop all sound handles
** Parameters: (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
suspend_all(sound_server_state_t *ss_state);
/* Commands server to pause all sound handles
** Parameters: (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
resume_all(sound_server_state_t *ss_state);
/* Commands server to resume all suspended sound handles
** Parameters: (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
set_reverse_stereo(int rs, sound_server_state_t *ss_state);
/* Commands server to change the reverse stereo value
** Parameters: (int) rs: New reverse stereo value
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
imap_set(unsigned int action, int instr, int value);
/* Alters the instrument mappings
** Parameters: (int) action: What to do to the instrument mapping
**             (int) instr: Instrument to change
**             (int) value: Value to change to
*/

void
change_song(song_t *new_song, sound_server_state_t *ss_state);
/* Commands server to change a song handle
** Parameters: (song_t *) new_song: The new song to change to
**             (sound_server_state_t *) ss_state: Sound server state
** Returns   : (void)
*/

void
save_sound_state(sound_server_state_t *ss_state);
/* Saves current sound state
** Parameters: (sound_server_state_t *) ss_state: State to save
** Returns   : (void)
*/

void
restore_sound_state(sound_server_state_t *ss_state);
/* Restores current sound state
** Parameters: (sound_server_state_t *) ss_state: State to restore into
** Returns   : (void)
*/

#ifdef DEBUG_SOUND_SERVER
extern int channel_instrument[16];
extern void print_channels_any(int mapped, sound_server_state_t *ss_state);
void print_song_info(word song_handle, sound_server_state_t *ss_state);
#endif

#endif /* !_SCI_SOUND_SERVER_H_ */
