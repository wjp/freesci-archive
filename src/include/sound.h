/***************************************************************************
 sound.h (C) 1999,2000,01 Christoph Reichenbach, TU Darmstadt


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
/* Everything concerning the handling of sound.
** As usual, don't rely on the values of the #defines to be final; those
** may very well change if support for OSS or ALSA (or whatever) is added.
*/

#ifndef _SCI_SOUND_H_
#define _SCI_SOUND_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sci_memory.h>

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

#include <resource.h>
#include <sciresource.h>

#ifdef HAVE_OBSTACK_H
#include <obstack.h>
#endif /* HAVE_OBSTACK_H */


typedef struct {

	int handle; /* The handle which the data is for; 0 if it's system data */

	unsigned int signal; /* Description of value */
	int value;

} sound_event_t;

struct _state;

extern int soundserver_dead;
/* Non-zero IFF the sound server died- set by sound.c, must also be
** set by non-fork()ed sound server implementations */

/* A word on priorities: A song is more important if its priority is higher.  */
/* Another note: SysTicks are at 60 Hz, in case you didn't already know this. */
/* Third note: Handles are actually set by the caller, so that they can directly
** map to heap addresses.
*/


unsigned int SOUND_DATA;
/* Used for transferring data.
*/

unsigned int SOUND_COMMAND_INIT_HANDLE;
/* Loads a song, priority specified as PARAMETER, under the specified HANDLE.
** Followed by PARAMETER bytes containing the song.
*/
unsigned int SOUND_COMMAND_PLAY_HANDLE;
/* Plays the sound stored as the HANDLE at priority PARAMETER. */
unsigned int SOUND_COMMAND_LOOP_HANDLE;
/* Sets the amount of loops (PARAMETER) for the song at HANDLE to play. */
unsigned int SOUND_COMMAND_DISPOSE_HANDLE;
/* Disposes the song from the specified HANDLE. Stops playing if the song is active. */
unsigned int SOUND_COMMAND_SET_MUTE;
/* Mutes sound if PARAMETER is 0, unmutes if PARAMETER != 0. */
unsigned int SOUND_COMMAND_GET_MUTE;
/* Return the mute status of the system */
unsigned int SOUND_COMMAND_STOP_HANDLE;
/* Stops playing the song associated with the specified HANDLE. */
unsigned int SOUND_COMMAND_SUSPEND_HANDLE;
/* Suspends sound playing for sound with the specified HANDLE. */
unsigned int SOUND_COMMAND_RESUME_HANDLE;
/* Resumes sound playing for sound with the specified HANDLE. */
unsigned int SOUND_COMMAND_SET_VOLUME;
/* Sets the global sound volume to the specified level (0-15) */
unsigned int SOUND_COMMAND_RENICE_HANDLE;
/* Sets the priority of the sound playing under the HANDLE to PARAMETER */
unsigned int SOUND_COMMAND_FADE_HANDLE;
/* Fades the sound playing under HANDLE so that it will be finished in PARAMETER ticks */
unsigned int SOUND_COMMAND_TEST;
/* Returns 0 if sound playing works, 1 if it doesn't. */
unsigned int SOUND_COMMAND_STOP_ALL;
/* Stops all playing tracks and returns appropriate signals. */
unsigned int SOUND_COMMAND_SHUTDOWN;
/* Tells the sound server to die. Used by the default exit implementation.
** The server must not reply to this.
*/
unsigned int SOUND_COMMAND_SAVE_STATE;
/* Saves the current state of the sound engine. Followed by a zero-terminated
** directory name with a length as specified by PARAMETER (including the \0)
** where the information should be placed. Returns one int (0 for success,
** 1 for failure)
*/
unsigned int SOUND_COMMAND_RESTORE_STATE;
/* Inverse of SOUND_COMMAND_SAVE_STATE: Restore sound state from zero terminated
** directory following the command, size specified in PARAMETER (incl. trailing \0).
** Returns one int (0 for success, 1 for failure)
*/
unsigned int SOUND_COMMAND_SUSPEND_ALL;
/* Halt all sound execution (issued when the interpreter is stopped for debugging) */
unsigned int SOUND_COMMAND_RESUME_ALL;
/* Resume all sound execution (issued when the interpreter is re-enabled after debugging) */
/*unsigned int SOUND_COMMAND_GET_NEXT_EVENT;*/
/* Request that the next event on the sound server should be transmitted */
unsigned int SOUND_COMMAND_GET_VOLUME;
/* retrn the global sound volume */
unsigned int SOUND_COMMAND_PRINT_SONG_INFO;
/* Prints the current song ID to the debug stream */
unsigned int SOUND_COMMAND_PRINT_CHANNELS;
/* Prints the current channel status to the debug stream */
unsigned int SOUND_COMMAND_PRINT_MAPPING;
/* Prints all current MT-32 -> GM instrument mappings to the debug stream */
unsigned int SOUND_COMMAND_IMAP_SET_INSTRUMENT;
/* Sets the 'gm_instr' part of an instrument mapping */
unsigned int SOUND_COMMAND_IMAP_SET_KEYSHIFT;
/* Sets the 'keyshift' part of an instrument mapping */
unsigned int SOUND_COMMAND_IMAP_SET_FINETUNE;
/* Sets the 'finetune' part of an instrument mapping */
unsigned int SOUND_COMMAND_IMAP_SET_BENDER_RANGE;
/* Sets the 'bender range' part of an instrument mapping */
unsigned int SOUND_COMMAND_IMAP_SET_PERCUSSION;
/* Sets the 'gm_rhythmkey' part of an instrument mapping */
unsigned int SOUND_COMMAND_IMAP_SET_VOLUME;
/* Sets the 'volume' part of an instrument mapping */
unsigned int SOUND_COMMAND_MUTE_CHANNEL;
/* Mutes one of the output channels */
unsigned int SOUND_COMMAND_UNMUTE_CHANNEL;
/* Un-mutes one of the output channels */

unsigned int SOUND_SIGNAL_CUMULATIVE_CUE;
/* Request for the specified HANDLE's signal to be increased */
unsigned int SOUND_SIGNAL_LOOP;
/* Finished a loop: VALUE is the number of loops remaining for the HANDLEd sound */
unsigned int SOUND_SIGNAL_FINISHED;
/* HANDLE has finished playing and was removed from the sound list */
unsigned int SOUND_SIGNAL_PLAYING;
/* Playing HANDLE */
unsigned int SOUND_SIGNAL_PAUSED;
/* Pausing HANDLE */
unsigned int SOUND_SIGNAL_RESUMED;
/* Resuming HANDLE after it was paused */
unsigned int SOUND_SIGNAL_INITIALIZED;
/* HANDLE has been successfully initialized */
unsigned int SOUND_SIGNAL_ABSOLUTE_CUE;
/* Set the HANDLE's signal to a fixed VALUE */


/* MIDI defines */
#define MIDI_CONTROL_CHANGE 0xb0
#define MIDI_CC_PAN 10

typedef struct {
	unsigned int delta_time;	/* number of ticks before send this command */
	unsigned char midi_cmd;		/* command to send */
	unsigned int param1;		/* first command parameter */
	unsigned int param2;		/* second command parameter */
} midi_op_t;

#ifdef HAVE_OBSTACK_H
guint8 *
makeMIDI0(const guint8 *src, int *size, guint8 flag);
/* Turns a sound resource into a MIDI block.
** Parameters: src: points to the resource.data information
**             size: points to an int which is set to contain the MIDI
**                   block length.
**             flag: The midi instrument's play flag.
** Returns   : (guint8 *) The sci_malloc()ed MIDI block, or NULL if conversion
**             failed.
** *FIXME*: Aborts in some cases if out of memory. This is OK while testing,
** but should be adjusted for the public release.
*/
#endif /* HAVE_OBSTACK_H */


int
map_MIDI_instruments(resource_mgr_t *resmgr);
/* Automagically detects MIDI instrument mappings
** Parameters: (resource_mgr_t *) resmgr: The resource manager to read from
** Returns   : (int) 0 on success, 1 on failure
** This function requires patch.002 to be present, or it will fail. However,
** for SCI1 and later, it will expect MIDI information to be stored as GM,
** and return with success.
** If detection fails, the MIDI maps will default to 1:1 General MIDI mappings.
*/

int
init_midi_device(struct _state *s);
/* sets up the midi device; loads the patch, etc. */

void
register_sound_messages();
/* Initialises the various SOUND_COMMAND and SOUND_SIGNAL variables. */

unsigned int
get_msg_value(char *msg);
/* Returns the value for the given sound message. Required to get the correct
** value from the main procedure.
** Parameters: (char *) msg: Sound message in words (not case sensitive)
** Returns   : (unsigned int) The message value
*/


#define MAP_NOT_FOUND -1
#define NOMAP -2
#define RHYTHM -3

extern char *GM_Instrument_Names[];
extern char *GM_Percussion_Names[];

typedef struct {
	gint8 gm_instr;
	int keyshift;
	int finetune;
	int bender_range;
	gint8 gm_rhythmkey;
	int volume;
} MIDI_map_t;

extern MIDI_map_t MIDI_mapping[128];
extern int MIDI_mappings_nr; /* Number of MIDI mappings */


extern int MIDI_cmdlen[16]; /* Number of parameters for each MIDI operation */

#endif /* _SCI_SOUND_H_ */
