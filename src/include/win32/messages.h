/***************************************************************************
 messages.h Copyright (C) 2001 Alexander R Angas

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

		Alexander R Angas (Alex) <wgd@internode.on.net>

 History:

   20010922 - largely assembled from sound.h
                -- Alex Angas

***************************************************************************/


/* UP == Unsigned int Parameter */
/* LP == Long Parameter */


/*** SOUND COMMANDS (sent to the sound server) ***/

#define UM_SOUND_COMMAND_DO_SOUND_MSG                    "UM_SOUND_COMMAND_DO_SOUND-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Used to keep time (i.e. should get posted every 16.6667 milliseconds)
** UP: unused
** LP: unused
*/

#define UM_SOUND_COMMAND_INIT_HANDLE_MSG                 "UM_SOUND_COMMAND_INIT_HANDLE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Loads a song
** UP: song handle (32 bits)
** LP: priority (? bits)
*/

#define UM_SOUND_COMMAND_PLAY_HANDLE_MSG                 "UM_SOUND_COMMAND_PLAY_HANDLE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Plays a sound handle
** UP: song handle (32 bits)
** LP: priority (<= 32 bits)
*/

#define UM_SOUND_COMMAND_LOOP_HANDLE_MSG                 "UM_SOUND_COMMAND_LOOP_HANDLE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Sets the amount of loops for the song handle to play
** UP: song handle (32 bits)
** LP: number of loops (<= 32 bits)
*/

#define UM_SOUND_COMMAND_DISPOSE_HANDLE_MSG              "UM_SOUND_COMMAND_DISPOSE_HANDLE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Disposes a song handle. Stops playing if the song is active
** UP: song handle (32 bits)
** LP: unused
*/

#define UM_SOUND_COMMAND_SET_MUTE_MSG                    "UM_SOUND_COMMAND_SET_MUTE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Mutes/unmutes sound (returns mute setting if SendMessage() is used)
** UP: mutes sound if MUTE_ON, unmutes otherwise (<= 32 bits)
** LP: unused
*/

#define UM_SOUND_COMMAND_GET_MUTE_MSG                    "UM_SOUND_COMMAND_GET_MUTE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Returns mute setting if SendMessage() is used
** UP: unused
** LP: unused
*/

#define UM_SOUND_COMMAND_STOP_HANDLE_MSG                 "UM_SOUND_COMMAND_STOP_HANDLE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Stops playing a song
** UP: song handle (32 bits)
** LP: unused
*/

#define UM_SOUND_COMMAND_SUSPEND_HANDLE_MSG              "UM_SOUND_COMMAND_SUSPEND_HANDLE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Suspends song playing
** UP: song handle (32 bits)
** LP: unused
*/

#define UM_SOUND_COMMAND_RESUME_HANDLE_MSG               "UM_SOUND_COMMAND_RESUME_HANDLE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Resumes song playing
** UP: song handle (32 bits)
** LP: unused
*/

#define UM_SOUND_COMMAND_SET_VOLUME_MSG                  "UM_SOUND_COMMAND_SET_VOLUME-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Sets the global sound volume (returns the new volume when SendMessage() is used)
** UP: unused
** LP: level (0-15) (<= 32 bits)
*/

#define UM_SOUND_COMMAND_RENICE_HANDLE_MSG               "UM_SOUND_COMMAND_RENICE_HANDLE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Sets a song's priority
** UP: song handle (32 bits)
** LP: priority (<= 32 bits)
*/

#define UM_SOUND_COMMAND_FADE_HANDLE_MSG                 "UM_SOUND_COMMAND_FADE_HANDLE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Fades a playing song to finish in a certain number of ticks
** UP: song handle (32 bits)
** LP: number of ticks to finish in (<= 32 bits)
*/

#define UM_SOUND_COMMAND_TEST_MSG                        "UM_SOUND_COMMAND_TEST-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Tests sound driver
** UP: unused
** LP: MIDI polyphony
*/

#define UM_SOUND_COMMAND_STOP_ALL_MSG                    "UM_SOUND_COMMAND_STOP_ALL-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Stops all playing tracks (returns 0 when SendMessage() is used)
** UP: unused
** LP: unused
*/

#define UM_SOUND_COMMAND_SHUTDOWN_MSG                    "UM_SOUND_COMMAND_SHUTDOWN-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Shuts down server
** UP: unused
** LP: unused
*/

#define UM_SOUND_COMMAND_SAVE_STATE_MSG                  "UM_SOUND_COMMAND_SAVE_STATE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Retreives target directory from command stream and saves state to that directory
** UP: unused
** LP: maximum length for directory
*/

#define UM_SOUND_COMMAND_RESTORE_STATE_MSG               "UM_SOUND_COMMAND_RESTORE_STATE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Retreives target directory from command stream and restores state from that directory
** UP: unused
** LP: maximum length for directory
*/

#define UM_SOUND_COMMAND_SUSPEND_ALL_MSG                 "UM_SOUND_COMMAND_SUSPEND_ALL-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Halt all sound execution (issued when the interpreter is stopped for debugging) */

#define UM_SOUND_COMMAND_RESUME_ALL_MSG                  "UM_SOUND_COMMAND_RESUME_ALL-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Resume all sound execution (issued when the interpreter is re-enabled after debugging) */

#define UM_SOUND_COMMAND_GET_VOLUME_MSG                  "UM_SOUND_COMMAND_GET_VOLUME-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Returns the global sound volume (SendMessage() must be used) */

#define UM_SOUND_COMMAND_PRINT_SONG_INFO_MSG             "UM_SOUND_COMMAND_PRINT_SONG_INFO-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Prints song info to the debug stream (if handle == 0 then prints current song)
** UP: handle
** LP: unused
*/

#define UM_SOUND_COMMAND_PRINT_CHANNELS_MSG              "UM_SOUND_COMMAND_PRINT_CHANNELS-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Prints the current channel status to the debug stream */

#define UM_SOUND_COMMAND_PRINT_MAPPING_MSG               "UM_SOUND_COMMAND_PRINT_MAPPING-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Prints all current MT-32 -> GM instrument mappings to the debug stream */

#define UM_SOUND_COMMAND_IMAP_SET_INSTRUMENT_MSG     "UM_SOUND_COMMAND_IMAP_SET_INSTRUMENT-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Sets the 'gm_instr' part of an instrument mapping
** UP: instrument
** LP: value
*/

#define UM_SOUND_COMMAND_IMAP_SET_KEYSHIFT_MSG       "UM_SOUND_COMMAND_IMAP_SET_KEYSHIFT-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Sets the 'keyshift' part of an instrument mapping
** UP: ??? (<= 32 bits)
** LP: ??? (<= 32 bits)
*/

#define UM_SOUND_COMMAND_IMAP_SET_FINETUNE_MSG       "UM_SOUND_COMMAND_IMAP_SET_FINETUNE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Sets the 'finetune' part of an instrument mapping
** UP: ??? (<= 32 bits)
** LP: ??? (<= 32 bits)
*/

#define UM_SOUND_COMMAND_IMAP_SET_BENDER_RANGE_MSG   "UM_SOUND_COMMAND_IMAP_SET_BENDER_RANGE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Sets the 'bender range' part of an instrument mapping
** UP: ??? (<= 32 bits)
** LP: ??? (<= 32 bits)
*/

#define UM_SOUND_COMMAND_IMAP_SET_PERCUSSION_MSG     "UM_SOUND_COMMAND_IMAP_SET_PERCUSSION-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Sets the 'gm_rhythmkey' part of an instrument mapping
** UP: ??? (<= 32 bits)
** LP: ??? (<= 32 bits)
*/

#define UM_SOUND_COMMAND_IMAP_SET_VOLUME_MSG         "UM_SOUND_COMMAND_IMAP_SET_VOLUME-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Sets the 'volume' part of an instrument mapping
** UP: ??? (<= 32 bits)
** LP: ??? (<= 32 bits)
*/

#define UM_SOUND_COMMAND_MUTE_CHANNEL_MSG                "UM_SOUND_COMMAND_MUTE_CHANNEL-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Mutes one of the output channels
** UP: channel to mute (<= 32 bits)
** LP: unused
*/

#define UM_SOUND_COMMAND_UNMUTE_CHANNEL_MSG              "UM_SOUND_COMMAND_UNMUTE_CHANNEL-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Unmutes one of the output channels
** UP: channel to mute (<= 32 bits)
** LP: unused
*/

#define UM_SOUND_COMMAND_REVERSE_STEREO_MSG              "UM_SOUND_COMMAND_REVERSE_STEREO-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Changes the reverse stereo setting
** UP: new setting
** LP: unused
*/


/*** SOUND SIGNALS (sent to the main thread) ***/

#define UM_SOUND_SIGNAL_CUMULATIVE_CUE_MSG               "UM_SOUND_SIGNAL_CUMULATIVE_CUE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Request a song's signal to be increased
** UP: song handle (32 bits)
** LP: new cumulative cue value (<= 32 bits)
*/

#define UM_SOUND_SIGNAL_LOOP_MSG                         "UM_SOUND_SIGNAL_LOOP-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Finished a loop
** UP: song handle (32 bits)
** LP: number of loops remaining (<= 32 bits)
*/

#define UM_SOUND_SIGNAL_FINISHED_MSG                     "UM_SOUND_SIGNAL_FINISHED-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Finished playing a song
** UP: song handle (32 bits)
** LP: unused
*/

#define UM_SOUND_SIGNAL_PLAYING_MSG                      "UM_SOUND_SIGNAL_PLAYING-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Playing a song
** UP: song handle (32 bits)
** LP: unused
*/

#define UM_SOUND_SIGNAL_PAUSED_MSG                       "UM_SOUND_SIGNAL_PAUSED-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Pausing a song
** UP: song handle (32 bits)
** LP: unused
*/

#define UM_SOUND_SIGNAL_RESUMED_MSG                      "UM_SOUND_SIGNAL_RESUMED-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Resuming a song after pause
** UP: song handle (32 bits)
** LP: unused
*/

#define UM_SOUND_SIGNAL_INITIALIZED_MSG                  "UM_SOUND_SIGNAL_INITIALIZED-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Song initialized
** UP: song handle (32 bits)
** LP: unused
*/

#define UM_SOUND_SIGNAL_ABSOLUTE_CUE_MSG                 "UM_SOUND_SIGNAL_ABSOLUTE_CUE-{4529B2CC-990C-4d24-9207-406EF6C8C3AA}"
/* Set a song's signal
** UP: song handle (32 bits)
** LP: value to set signal to (<= 32 bits)
*/
