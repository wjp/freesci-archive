/***************************************************************************
 sound.h (C) 1999 Christoph Reichenbach, TU Darmstadt


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

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

#include <resource.h>
#include <glib.h>

#ifdef HAVE_OBSTACK_H
#include <obstack.h>
#endif /* HAVE_OBSTACK_H */


typedef struct {

  int handle; /* The handle which the data is for; 0 if it's system data */

  int signal; /* Description of value */
  int value;

} sound_event_t;


struct _state;


typedef struct {

  char *name; /* Name of this particular driver */

  int (*init)(struct _state *s);
  /* Initializes the sounnd driver
  ** Parameters: (state_t *) s: The state that we're going to play on
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
  /* Stops playing sound, uninitializes the sound driver, and frees all associated memory.
  ** Parameters: (state_t *) s: The state_t to operate on
  ** Returns   : (void)
  */

  sound_event_t* (*get_event)(struct _state *s);
  /* Synchronizes the sound driver with the rest of the world.
  ** Parameters: (state_t *) s: The state_t to operate on (it's getting boring)
  ** Returns   : (sound_event_t *) Pointer to a dynamically allocated sound_event_t structure
  **                               containing the next event, or NULL if none is available
  ** This function should be called at least 60 times per second. It will return finish, loop,
  ** and cue events, which can be written directly to the sound objects.
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

  int (*command)(struct _state *s, int command, int handle, int parameter);
  /* Executes a sound command (one of SOUND_COMMAND_xxx).
  ** Parameters: (state_t *) s: The current state
  **             (int) command: The command to execute
  **             (int) handle: The handle to execute it on, if available
  **             (int) parameter: The function parameter
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

} sfx_driver_t;


extern sfx_driver_t *sfx_drivers[]; /* All available sound fx drivers, NULL-terminated */

/* A word on priorities: A song is more important if its priority is higher.  */
/* Another note: SysTicks are at 60 Hz, in case you didn't already know this. */
/* Third note: Handles are actually set by the caller, so that they can directly
** map to heap addresses.
*/

#define SOUND_COMMAND_INIT_SONG 0
/* Loads a song, priority specified as PARAMETER, under the specified HANDLE.
** Followed by PARAMETER bytes containing the song.
*/
#define SOUND_COMMAND_PLAY_HANDLE 1
/* Plays the sound stored as the HANDLE at priority PARAMETER. */
#define SOUND_COMMAND_SET_LOOPS 2
/* Sets the amount of loops (PARAMETER) for the song at HANDLE to play. */
#define SOUND_COMMAND_DISPOSE_HANDLE 3
/* Disposes the song from the specified HANDLE. Stops playing if the song is active. */
#define SOUND_COMMAND_SET_MUTE 4
/* Mutes sound if PARAMETER is 0, unmutes if PARAMETER != 0. */
#define SOUND_COMMAND_STOP_HANDLE 5
/* Stops playing the song associated with the specified HANDLE. */
#define SOUND_COMMAND_SUSPEND_HANDLE 6
/* Suspends sound playing for sound with the specified HANDLE. */
#define SOUND_COMMAND_RESUME_HANDLE 7
/* Resumes sound playing for sound with the specified HANDLE. */
#define SOUND_COMMAND_SET_VOLUME 8
/* Sets the global sound volume to the specified level (0-255) */
#define SOUND_COMMAND_RENICE_HANDLE 9
/* Sets the priority of the sound playing under the HANDLE to PARAMETER */
#define SOUND_COMMAND_FADE_HANDLE 10
/* Fades the sound playing under HANDLE so that it will be finished in PARAMETER ticks */
#define SOUND_COMMAND_TEST 11
/* Returns 0 if sound playing works, 1 if it doesn't. */
#define SOUND_COMMAND_STOP_ALL 12
/* Stops all playing tracks and returns appropriate signals. */
#define SOUND_COMMAND_SAVE_STATE 13
/* Saves the current state of the sound engine. Followed by a zero-terminated
** directory name with a length as specified by PARAMETER (including the \0)
** where the information should be placed. Returns one int (0 for success,
** 1 for failure)
*/
#define SOUND_COMMAND_RESTORE_STATE 14
/* Inverse of SOUND_COMMAND_SAVE_STATE (13): Restore sound state from zero terminated
** directory following the command, size specified in PARAMETER (incl. trailing \0).
** Returns one int (0 for success, 1 for failure)
*/
#define SOUND_COMMAND_SUSPEND_SOUND 15
/* Halt all sound execution (issued when the interpreter is stopped for debugging) */
#define SOUND_COMMAND_RESUME_SOUND 16
/* Resume all sound execution (issued when the interpreter is re-enabled after debugging) */


#define SOUND_SIGNAL_CUMULATIVE_CUE 0
/* Request for the specified HANDLE's signal to be increased */
#define SOUND_SIGNAL_LOOP 1
/* Finished a loop: VALUE is the number of loops remaining for the HANDLEd sound */
#define SOUND_SIGNAL_FINISHED 2
/* HANDLE has finished playing and was removed from the sound list */
#define SOUND_SIGNAL_PLAYING 3
/* Playing HANDLE */
#define SOUND_SIGNAL_PAUSED 4
/* Pausing HANDLE */
#define SOUND_SIGNAL_RESUMED 5
/* Resuming HANDLE after it was paused */
#define SOUND_SIGNAL_INITIALIZED 6
/* HANDLE has been successfully initialized */
#define SOUND_SIGNAL_ABSOLUTE_CUE 7
/* Set the HANDLE's signal to a fixed VALUE */


#ifdef HAVE_OBSTACK_H
guint8 *
makeMIDI0(const guint8 *src, int *size);
/* Turns a sound resource into a MIDI block.
** Parameters: src: points to the resource.data information
**             size: points to an int which is set to contain the MIDI
**                   block length.
** Returns   : (guint8 *) The malloc()ed MIDI block, or NULL if conversion
**             failed.
** *FIXME*: Aborts in some cases if out of memory. This is OK while testing,
** but should be adjusted for the public release.
*/
#endif /* HAVE_OBSTACK_H */


int
mapMIDIInstruments(void);
/* Automagically detects MIDI instrument mappings
** Parameters: (void)
** Returns   : (int) 0 on success, 1 on failure
** This function requires patch.002 to be present, or it will fail. However,
** for SCI1 and later, it will expect MIDI information to be stored as GM,
** and return with success.
** If detection fails, the MIDI maps will default to 1:1 General MIDI mappings.
*/

#endif /* _SCI_SOUND_H_ */
