/***************************************************************************
 sfx_sw_sequencer.h  Copyright (C) 2002 Christoph Reichenbach


 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public Licence as
 published by the Free Software Foundaton; either version 2 of the
 Licence, or (at your option) any later version.

 It is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 merchantibility or fitness for a particular purpose. See the
 GNU General Public Licence for more details.

 You should have received a copy of the GNU General Public Licence
 along with this program; see the file COPYING. If not, write to
 the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.


 Please contact the maintainer for any program-related bug reports or
 inquiries.

 Current Maintainer:

    Christoph Reichenbach (CR) <jameson@linuxgames.com>

***************************************************************************/

#ifndef _SFX_SW_SEQUENCER_H_
#define _SFX_SW_SEQUENCER_H_

#include <sfx_audiobuf.h>
#include <sfx_pcm.h>

typedef struct {
	sfx_audio_buf_t buf;
	sfx_pcm_feed_t *feed;
} sfx_pcm_buffered_feed_t;

typedef struct {
	int feeds_nr;
	sfx_pcm_buffered_feed_t* feeds;
} sfx_pcm_sw_seq_data_t;


/* Accesses the sfx_audio_buf_t associated with a channel */
#define PCM_SW_CHANNEL(self, index) 	\
	((self)->sw_seq_data->feeds[index].buf)

struct _sfx_sequencer;

void
sfx_init_sw_sequencer(struct _sfx_sequencer *seq, int feeds, sfx_pcm_config_t conf);
/* Initialises a sequencer as a software sequencer
** Parameters: (sfx_sequencer_t *) seq: The sequencer to initialise
**             (int) feeds: Number of feeds to allocate, initialise, and
**                          register with the global mixer
**             (sfx_pcm_config_t) conf: PCM configuration to use for the feeds
** Modifiers : seq->sw_seq_data
** Returns   : (void)
*/

void
sfx_exit_sw_sequencer(struct _sfx_sequencer *seq);
/* De-initialises a software sequencer
** Parameters: (sfx_sequencer_t *) seq: The sequencer to uninitialise
** Modifiers : seq->sw_seq_data
** Returns   : (void)
*/

#endif /* !defined(_SFX_SW_SEQUENCER_H_) */
