/***************************************************************************
 sfx_player.h Copyright (C) 2002 Christoph Reichenbach


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
/* song player structure */

#include <sfx_engine.h>
#include <sfx_iterator.h>
#include <sciresource.h>

#ifndef _SFX_PLAYER_H
#define _SFX_PLAYER_H

typedef struct {
	char *name;
	char *version;

	int
	(*set_option)(char *name, char *value);
	/* Sets an option for player timing mechanism
	** Parameters: (char *) name: The name describing what to set
	**             (char *) value: The value to set
	** Returns   : (int) SFX_OK, or SFX_ERROR if the name wasn't understood
	*/

	int
	(*init)(resource_mgr_t *resmgr, int expected_latency);
	/* Initializes the player
	** Parameters: (resource_mgr_t *) resmgr: A resource manager for driver initialization
	**             (int) expected_latency: Expected delay in between calls to 'maintenance'
	**                   (in microseconds)
	** Returns   : (int) SFX_OK on success, SFX_ERROR on failure
	*/

	int
	(*set_iterator)(song_iterator_t *it, GTimeVal start_time);
	/* Sets an iterator for immediate playing
	** Parameters: (songx_iterator_t *) it: The iterator to play
	**             (GTimeVal) start_time: The time to assume as the
	**                        time the first MIDI command executes at
	** Returns   : (int) SFX_OK on success, SFX_ERROR on failure
	** The iterator is for immediate use; it may be modified appropriately
	** for player but need not be cloned
	*/

	int
	(*fade_out)();
	/* Fades out the currently playing song (within two seconds
	** Returns   : (int) SFX_OK on success, SFX_ERROR on failure
	*/

	int
	(*stop)();
	/* Stops the currently playing song and deletes the associated iterator
	** Returns   : (int) SFX_OK on success, SFX_ERROR on failure
	*/

	int
	(*iterator_message)(song_iterator_message_t msg);
	/* Transmits a song iterator message to the active song
	** Parameters: (song_iterator_message_t) msg: The message to transmit
	** Returns   : (int) SFX_OK on success, SFX_ERROR on failure
	** OPTIONAL -- may be NULL
	** If this method is not present, sending messages will stop
	** and re-start playing, so it is preferred that it is present
	*/

	int
	(*pause)(); /* OPTIONAL -- may be NULL */
	/* Pauses song playing
	** Returns   : (int) SFX_OK on success, SFX_ERROR on failure
	*/

	int
	(*resume)(); /* OPTIONAL -- may be NULL */
	/* Resumes song playing after a pause
	** Returns   : (int) SFX_OK on success, SFX_ERROR on failure
	*/

	int
	(*exit)();
	/* Stops the player
	** Returns   : (int) SFX_OK on success, SFX_ERROR on failure
	*/

	void
	(*maintenance)(); /* OPTIONAL -- may be NULL */
	/* Regularly called maintenance function
	** This function is called frequently and regularly (if present), it can be
	** used to emit sound.
	*/

	int polyphony; /* Number of voices that can play simultaneously */

} sfx_player_t;

sfx_player_t *
sfx_find_player(char *name);
/* Looks up a player by name or finds the default player
** Parameters: (char *) name: Name of the player to look up, or NULL for dedault
** Returns   : (sfx_player_t *) The player requested, or NULL if none was found
*/

int
sfx_get_player_polyphony();
/* Determines the polyphony of the player in use
** Returns   : (int) Number of voices the active player can emit
*/

#endif /* !_SFX_PLAYER_H */
