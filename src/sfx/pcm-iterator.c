/***************************************************************************
 pcm-iterator.c  Copyright (C) 2002 Christoph Reichenbach


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

#include <sfx_iterator.h>
#include <resource.h> /* for BREAKPOINT */
#include <sci_memory.h>

#define D ((pcm_data_internal_t *)self->internal)

static int
pi_poll(sfx_pcm_feed_t *self, byte *dest, int size);
static void
pi_destroy(sfx_pcm_feed_t *self);

typedef struct {
	byte *data;
	int samples_left;
	song_iterator_t *it;
} pcm_data_internal_t;


static sfx_pcm_feed_t pcm_it_prototype = {
	pi_poll,
	pi_destroy,
	NULL, /* No timestamp getter */
	NULL, /* Internal data goes here */
	{0,0,0}, /* Must fill in configuration */
	"song-iterator",
	0, /* Ideally the resource number should go here */
	0  /* The mixer computes this for us */
};


sfx_pcm_feed_t *
sfx_iterator_feed(song_iterator_t *it)
{
	sfx_pcm_feed_t *feed;
	sfx_pcm_config_t conf;
	pcm_data_internal_t *idat;
	byte *data;
	int size;

	if (!it) { BREAKPOINT(); }

	data = it->get_pcm(it, &size, &conf);

	if (!data) {
		/* Now this is silly; why'd you call this function in the first place? */
		return NULL;
	}

	idat = sci_malloc(sizeof(pcm_data_internal_t));
	idat->data = data;
	idat->samples_left = size;
	idat->it = it;
	feed = sci_malloc(sizeof(sfx_pcm_feed_t));
	*feed = pcm_it_prototype;
	feed->internal = idat;
	feed->conf = conf;

	return feed;
}

static int
pi_poll(sfx_pcm_feed_t *self, byte *dest, int size)
{
	int data_len;

	if (size > D->samples_left)
		size = D->samples_left;
	D->samples_left -= size;

	data_len = size * self->sample_size;

	memcpy(dest, D->data, data_len);
#if 0
memset(dest, 0xff, data_len);
#endif

	D->data += data_len;

	return size;
}

static void
pi_destroy(sfx_pcm_feed_t *self)
{
	songit_free(D->it);
	sci_free(D);
	sci_free(self);
}
