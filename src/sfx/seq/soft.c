/***************************************************************************
 soft.c  Copyright (C) 2002 Christoph Reichenbach


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
/* Software sequencer support */

#include <resource.h>
#include <sci_memory.h>
#include <sfx_mixer.h>
#include <sfx_sequencer.h>
#include <sfx_sw_sequencer.h>

#define DATA seq->sw_seq_data

int
_sfx_pcm_feed_poll(sfx_pcm_feed_t *feed, byte *dest, int size)
{
	sfx_audio_buf_t *buf = (sfx_audio_buf_t *)feed->internal;

	sfx_audbuf_read(buf, dest, feed->sample_size, size);
	return size;
}

void
_sfx_pcm_feed_destroy(sfx_pcm_feed_t *self)
{
	/* No-op */
}


void
sfx_init_sw_sequencer(sfx_sequencer_t *seq, int feeds, sfx_pcm_config_t pcm_conf)
{
	int i;

	if (!mixer) {
		fprintf(stderr, "[SFX:SW-SEQ] Sequencer initialised, but no mixer is present-- inconsitency!\n");
		exit(1);
	}

	DATA = sci_malloc(sizeof(sfx_pcm_sw_seq_data_t));
	DATA->feeds_nr = feeds;
	DATA->feeds = sci_malloc(sizeof(sfx_pcm_buffered_feed_t) * feeds);

	for (i = 0; i < feeds; i++) {
		sfx_audbuf_init(&DATA->feeds[i].buf);
		DATA->feeds[i].feed = sci_malloc(sizeof(sfx_pcm_feed_t));
		DATA->feeds[i].feed->debug_name = seq->name;
		DATA->feeds[i].feed->debug_nr = i;
		DATA->feeds[i].feed->internal = &(DATA->feeds[i].buf);
		DATA->feeds[i].feed->poll = _sfx_pcm_feed_poll;
		DATA->feeds[i].feed->destroy = _sfx_pcm_feed_destroy;
		DATA->feeds[i].feed->conf = pcm_conf;

		/* Finally, add our mixer */
		mixer->subscribe(mixer, DATA->feeds[i].feed);
	}
}

void
sfx_exit_sw_sequencer(sfx_sequencer_t *seq)
{
	if (DATA->feeds_nr) {
		int i;

		for (i = 0; i < DATA->feeds_nr; i++) {
			sfx_audbuf_free(&(DATA->feeds[i].buf));
			sci_free(DATA->feeds[i].feed);
		}
		sci_free(DATA->feeds);
	}
	sci_free(DATA);
	DATA = NULL;
}
