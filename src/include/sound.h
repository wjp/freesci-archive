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

/* Note 1: SysTicks are at 60 Hz, in case you didn't already know this. */
/* Note 2: Handles are actually set by the caller, so that they can directly
** map to heap addresses.
*/

#define SOUND_COMMAND_INIT_HANDLE         0
/* Loads a song, priority specified as PARAMETER, under the specified HANDLE.
** Followed by PARAMETER bytes containing the song.
*/
#define SOUND_COMMAND_PLAY_HANDLE         1
/* Plays the sound stored as the HANDLE at priority PARAMETER. */
#define SOUND_COMMAND_LOOP_HANDLE         2
/* Sets the amount of loops (PARAMETER) for the song at HANDLE to play. */
#define SOUND_COMMAND_DISPOSE_HANDLE      3
/* Disposes the song from the specified HANDLE. Stops playing if the song is active. */
#define SOUND_COMMAND_SET_MUTE            4
/* Mutes sound if PARAMETER is MUTE_ON, unmutes if PARAMETER != MUTE_ON. */
#define SOUND_COMMAND_GET_MUTE            5
/* Return the mute status of the system */
#define SOUND_COMMAND_STOP_HANDLE         6
/* Stops playing the song associated with the specified HANDLE. */
#define SOUND_COMMAND_SUSPEND_HANDLE      7
/* Suspends sound playing for sound with the specified HANDLE. */
#define SOUND_COMMAND_RESUME_HANDLE       8
/* Resumes sound playing for sound with the specified HANDLE. */
#define SOUND_COMMAND_SET_VOLUME          9
/* Sets the global sound volume to the specified level (0-15) */
#define SOUND_COMMAND_RENICE_HANDLE       10
/* Sets the priority of the sound playing under the HANDLE to PARAMETER */
#define SOUND_COMMAND_FADE_HANDLE         11
/* Fades the sound playing under HANDLE so that it will be finished in PARAMETER ticks */
#define SOUND_COMMAND_TEST                12
/* Returns 0 if sound playing works, 1 if it doesn't. */
#define SOUND_COMMAND_STOP_ALL            13
/* Stops all playing tracks and returns appropriate signals. */
#define SOUND_COMMAND_SHUTDOWN            14
/* Tells the sound server to die. Used by the default exit implementation.
** The server must not reply to this.
*/
#define SOUND_COMMAND_SAVE_STATE          15
/* Saves the current state of the sound engine. Followed by a zero-terminated
** directory name with a length as specified by PARAMETER (including the \0)
** where the information should be placed. Returns one int (0 for success,
** 1 for failure)
*/
#define SOUND_COMMAND_RESTORE_STATE       16
/* Inverse of SOUND_COMMAND_SAVE_STATE: Restore sound state from zero terminated
** directory following the command, size specified in PARAMETER (incl. trailing \0).
** Returns one int (0 for success, 1 for failure)
*/
#define SOUND_COMMAND_SUSPEND_ALL         17
/* Halt all sound execution (issued when the interpreter is stopped for debugging) */
#define SOUND_COMMAND_RESUME_ALL          18
/* Resume all sound execution (issued when the interpreter is re-enabled after debugging) */
#define SOUND_COMMAND_GET_VOLUME          19
/* retrn the global sound volume */
#define SOUND_COMMAND_PRINT_SONG_INFO     20
/* Prints the current song ID to the debug stream */
#define SOUND_COMMAND_PRINT_CHANNELS      21
/* Prints the current channel status to the debug stream */
#define SOUND_COMMAND_PRINT_MAPPING       22
/* Prints all current MT-32 -> GM instrument mappings to the debug stream */
#define SOUND_COMMAND_IMAP_SET_INSTRUMENT   23
/* Sets the 'gm_instr' part of an instrument mapping */
#define SOUND_COMMAND_IMAP_SET_KEYSHIFT     24
/* Sets the 'keyshift' part of an instrument mapping */
#define SOUND_COMMAND_IMAP_SET_FINETUNE     25
/* Sets the 'finetune' part of an instrument mapping */
#define SOUND_COMMAND_IMAP_SET_BENDER_RANGE 26
/* Sets the 'bender range' part of an instrument mapping */
#define SOUND_COMMAND_IMAP_SET_PERCUSSION   27
/* Sets the 'gm_rhythmkey' part of an instrument mapping */
#define SOUND_COMMAND_IMAP_SET_VOLUME       28
/* Sets the 'volume' part of an instrument mapping */
#define SOUND_COMMAND_MUTE_CHANNEL        29
/* Mutes one of the output channels */
#define SOUND_COMMAND_UNMUTE_CHANNEL      30
/* Un-mutes one of the output channels */
#define SOUND_COMMAND_REVERSE_STEREO      31
/* Turns on or off reverse stereo (only with event ss at present) */

#define SOUND_SIGNAL_CUMULATIVE_CUE  32
/* Request for the specified HANDLE's signal to be increased */
#define SOUND_SIGNAL_LOOP            33
/* Finished a loop: VALUE is the number of loops remaining for the HANDLEd sound */
#define SOUND_SIGNAL_FINISHED        34
/* HANDLE has finished playing and was removed from the sound list */
#define SOUND_SIGNAL_PLAYING         35
/* Playing HANDLE */
#define SOUND_SIGNAL_PAUSED          36
/* Pausing HANDLE */
#define SOUND_SIGNAL_RESUMED         37
/* Resuming HANDLE after it was paused */
#define SOUND_SIGNAL_INITIALIZED     38
/* HANDLE has been successfully initialized */
#define SOUND_SIGNAL_ABSOLUTE_CUE    39
/* Set the HANDLE's signal to a fixed VALUE */

#define SOUND_COMMAND_DO_SOUND    40

#define NUMBER_SOUND_EVENTS 41

#define UNRECOGNISED_SOUND_SIGNAL -1


/* MIDI defines */
#define MIDI_MSG_TYPE_NOTE_OFF          '\x8'
#define MIDI_MSG_TYPE_NOTE_ON           '\x9'
#define MIDI_MSG_TYPE_AFTERTOUCH        '\xA'
#define MIDI_MSG_TYPE_CONTROLLER_CHANGE '\xB'
#define MIDI_MSG_TYPE_INSTRUMENT_CHANGE '\xC'
#define MIDI_MSG_TYPE_CHANNEL_PRESSURE  '\xD'
#define MIDI_MSG_TYPE_PITCH_WHEEL       '\xE'
#define MIDI_MSG_TYPE_SYSTEM            '\xF'

#define MIDI_CONTROLLER_PAN_COARSE 10

#define MIDI_NOTE_ON           0x90
#define MIDI_CONTROL_CHANGE    0xB0
#define MIDI_INSTRUMENT_CHANGE 0xC0
#define MIDI_SYSTEM_SYSEX      0xF0
#define MIDI_SYSTEM_SYSEX_END  0xF7

#define MIDI_PARAMETERS_TWO(typ)  ( ((typ >= '\x8') && (typ <= '\xB'))  || (typ == '\xE') )
#define MIDI_PARAMETERS_ONE(typ)  ( (typ >= '\xC') && (typ <= '\xD') )

#define SCI_MIDI_EOT 0xFC
#define SCI_MIDI_END_OF_TRACK SCI_MIDI_EOT

#define SCI_MIDI_CONTROLLER(status) ((status & 0xF0) == 0xB0)

#define SCI_MIDI_SET_SIGNAL 0xCF
#define SCI_MIDI_SET_POLYPHONY 0x4B
#define SCI_MIDI_RESET_ON_SUSPEND 0x4C
#define SCI_MIDI_SET_VELOCITY 0x4E
#define SCI_MIDI_SET_REVERB 0x50
#define SCI_MIDI_CUMULATIVE_CUE 0x60

#define SCI_MIDI_SET_SIGNAL_LOOP 0x7F
/* If this is the parameter of 0xCF, the loop point is set here */

#define SCI_MIDI_TIME_EXPANSION_PREFIX 0xF8
#define SCI_MIDI_TIME_EXPANSION_LENGHT 240
/* [RS] tsk tsk tsk */
/* [CR] WTF? */
#define SCI_MIDI_TIME_EXPANSION_LENGTH SCI_MIDI_TIME_EXPANSION_LENGHT


typedef struct {
	unsigned int pos_in_song;	/* where this command appears in the song */
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
