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

#define DATA seq->sw_seq_data


typedef struct sfx_sw_buf_chain {
	sfx_audio_buf_t buf;
	struct sfx_sw_buf_chain *next;
} buf_chain_t;

/*-- Buffer implementation --*/

static void
bc_retire_buf(sfx_pcm_buffered_feed_t *bf)
{
	buf_chain_t *obsolete = bf->old_read;
	bf->old_read = bf->old_read->next;
	bf->readbuf = &(bf->old_read->buf);

	sfx_audbuf_free(&(obsolete->buf));
	sci_free(obsolete);
}

static void
bc_fresh_buf(sfx_pcm_buffered_feed_t *bf, GTimeVal ts)
{
	buf_chain_t *nc = sci_malloc(sizeof(buf_chain_t));
	nc->next = NULL;
	*(bf->old_append) = nc;
	bf->old_append = &(nc->next);
	bf->writebuf = &(nc->buf);

	sfx_audbuf_init(bf->writebuf, bf->pcm_conf);
	sfx_audbuf_write_timestamp(bf->writebuf, sfx_new_timestamp(ts.tv_sec, ts.tv_usec, bf->pcm_conf.rate));
}

static void
bc_init(sfx_pcm_buffered_feed_t *bf, sfx_pcm_config_t conf)
{
	GTimeVal zero_time = {0, 0};

	bf->pcm_conf = conf;
	bf->old_read = NULL;
	bf->old_append = &(bf->old_read);
	bc_fresh_buf(bf, zero_time);
	bf->readbuf = bf->writebuf = &(bf->old_read->buf);
}

static void
bc_delete(sfx_pcm_buffered_feed_t *bf)
{
	while (bf->old_read)
		bc_retire_buf(bf);
}

/*-- Feed implementation --*/

static inline void
retire_obsolete_feeds(sfx_pcm_buffered_feed_t *buffeed)
{
	int samples_nr = buffeed->readbuf->samples_nr;

	/* Discard empty feeds */
	while (!samples_nr
	       && buffeed->readbuf != buffeed->writebuf) {
		bc_retire_buf(buffeed);
		samples_nr = buffeed->readbuf->samples_nr;
	}
}

static int
_sfx_pcm_feed_poll(sfx_pcm_feed_t *feed, byte *dest, int size)
{
	sfx_pcm_buffered_feed_t *buffeed = (sfx_pcm_buffered_feed_t *)feed->internal;
	int samples_nr = buffeed->readbuf->samples_nr;
int oldsize = size;
	retire_obsolete_feeds(buffeed);

	if (size > samples_nr)
		size = samples_nr;

	sfx_audbuf_read(buffeed->readbuf, dest, size);

	retire_obsolete_feeds(buffeed);


	return size;
}

static int
_sfx_pcm_feed_get_timestamp(sfx_pcm_feed_t *feed, sfx_timestamp_t *ts)
{
	sfx_pcm_buffered_feed_t *buffeed = (sfx_pcm_buffered_feed_t *)feed->internal;

	if (!sfx_audbuf_read_timestamp(buffeed->readbuf, ts))
		return PCM_FEED_TIMESTAMP;

	return PCM_FEED_IDLE;
}

static void
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

	if (!mixer->dev) {
		fprintf(stderr, "[SFX:SW-SEQ] Sequencer initialised and mixer present, but mixer lacks PCM device-- inconsistency!\n");
		exit(1);
	}

	seq->min_write_ahead_ms =
		(1000 * mixer->dev->buf_size) / (mixer->dev->conf.rate);

	DATA = sci_malloc(sizeof(sfx_pcm_sw_seq_data_t));
	DATA->feeds_nr = feeds;
	DATA->feeds = sci_malloc(sizeof(sfx_pcm_buffered_feed_t) * feeds);

	for (i = 0; i < feeds; i++) {
		bc_init(&(DATA->feeds[i]), pcm_conf);
		DATA->feeds[i].feed = sci_malloc(sizeof(sfx_pcm_feed_t));
		DATA->feeds[i].feed->debug_name = seq->name;
		DATA->feeds[i].feed->debug_nr = i;
		DATA->feeds[i].feed->internal = &(DATA->feeds[i]);
		DATA->feeds[i].feed->poll = _sfx_pcm_feed_poll;
		DATA->feeds[i].feed->destroy = _sfx_pcm_feed_destroy;
		DATA->feeds[i].feed->conf = pcm_conf;
		DATA->feeds[i].feed->get_timestamp = _sfx_pcm_feed_get_timestamp;

		/* Finally, add our mixer */
		mixer->subscribe(mixer, DATA->feeds[i].feed);
	}
}

void
sfx_sw_sequencer_start_recording(sfx_sequencer_t *seq, GTimeVal ts)
{
	if (DATA->feeds_nr) {
		int i;
		for (i = 0; i < DATA->feeds_nr; i++) {
			bc_fresh_buf(&(DATA->feeds[i]), ts);
			retire_obsolete_feeds(&(DATA->feeds[i]));
		}
	}

}


void
sfx_exit_sw_sequencer(sfx_sequencer_t *seq)
{
	if (DATA->feeds_nr) {
		int i;

		for (i = 0; i < DATA->feeds_nr; i++) {
			bc_delete(&(DATA->feeds[i]));
			sci_free(DATA->feeds[i].feed);
		}
		sci_free(DATA->feeds);
	}
	sci_free(DATA);
	DATA = NULL;
}
