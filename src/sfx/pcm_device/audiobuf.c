/***************************************************************************
 audiobuf.c Copyright (C) 2003 Christoph Reichenbach


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

#include <sfx_audiobuf.h>

static sfx_audio_buf_chunk_t *
sfx_audbuf_alloc_chunk(void)
{
	sfx_audio_buf_chunk_t *ch = sci_malloc(sizeof(sfx_audio_buf_chunk_t));
	ch->used = 0;
	ch->next = NULL;

	return ch;
}

void
sfx_audbuf_init(sfx_audio_buf_t *buf, sfx_pcm_config_t pcm_conf)
{
	int framesize = SFX_PCM_SAMPLE_SIZE(pcm_conf);
	byte silence[16];
	int silencew = pcm_conf.format & ~SFX_PCM_FORMAT_LMASK;

	/* Determine the correct 'silence' for the channel and install it */
	/* Conservatively assume stereo */
	if (pcm_conf.format & SFX_PCM_FORMAT_16) {
		if (pcm_conf.format & SFX_PCM_FORMAT_LE) {
			silence[0] = silencew & 0xff;
			silence[1] = (silencew >> 8) & 0xff;
		} else {
			silence[0] = (silencew >> 8) & 0xff;
			silence[1] = silencew & 0xff;
		}
		memcpy(silence + 2, silence, 2);
	} else {
		silence[0] = silencew;
		silence[1] = silencew;
	}

	buf->last = buf->first = sfx_audbuf_alloc_chunk();
	buf->unused = NULL;
	memcpy(buf->last_frame, silence, framesize);	 /* Initialise, in case we
							 ** underrun before the
							 ** first write  */
	buf->read_offset = 0;
	buf->framesize = framesize;
	buf->read_timestamp.secs = -1; /* Mark as inactive */
}

static void
sfx_audbuf_free_chain(sfx_audio_buf_chunk_t *b)
{
	while (b) {
		sfx_audio_buf_chunk_t *n = b->next;
		sci_free(b);
		b = n;
	}
}

void
sfx_audbuf_free(sfx_audio_buf_t *buf)
{
	sfx_audbuf_free_chain(buf->first);
	sfx_audbuf_free_chain(buf->unused);
	buf->first = buf->last = buf->unused = NULL;
	buf->read_offset = (int) 0xdeadbeef;
}

void
sfx_audbuf_write(sfx_audio_buf_t *buf, unsigned char *src, int frames)
{
	/* In here, we compute PER BYTE */
	int data_left = buf->framesize * frames;


	if (!buf->last) {
		fprintf(stderr, "FATAL: Violation of audiobuf.h usage protocol: Must use 'init' before 'write'\n");
		exit(1);
	}


	while (data_left) {
		int cpsize;
		int buf_free;
		buf_free = SFX_AUDIO_BUF_SIZE - buf->last->used;


		if (buf_free >= data_left)
			cpsize = data_left;
		else
			cpsize = buf_free;

		/* Copy and advance pointers */
		memcpy(buf->last->data + buf->last->used, src, cpsize);
		data_left -= cpsize;
		buf->last->used += cpsize;
		src += cpsize;

		if (buf->last->used == SFX_AUDIO_BUF_SIZE) {
			if (!buf->last->next) {
				if (buf->unused) { /* Re-use old chunks */
					buf->last->next = buf->unused;
					buf->unused = buf->unused->next;
					buf->last->next->next = NULL;
					buf->last->next->used = 0;
				} else /* Allocate */
					buf->last->next =
						sfx_audbuf_alloc_chunk();
			}

			buf->last = buf->last->next;
		}
	}

#ifdef TRACE_BUFFER
	{
		sfx_audio_buf_chunk_t *c = buf->first;
		int t = buf->read_offset;

		while (c) {
			fprintf(stderr, "-> [");
			for (; t < c->used; t++)
				fprintf(stderr, " %02x", c->data[t]);
			t = 0;
			fprintf(stderr, " ] ");
			c = c->next;
		}
		fprintf(stderr, "\n");
	}
#endif

	if (frames)
	/* Backup last frame */
		memcpy(buf->last_frame, src - buf->framesize, buf->framesize);
}

static int
reported_buffer_underrun = 0;

int
sfx_audbuf_read(sfx_audio_buf_t *buf, unsigned char *dest, int frames)
{
	int written = 0;
	if (frames <= 0)
		return 0;

#ifdef TRACE_BUFFER
	{
		sfx_audio_buf_chunk_t *c = buf->first;
		int t = buf->read_offset;

		while (c) {
			fprintf(stderr, "-> [");
			for (; t < c->used; t++)
				fprintf(stderr, " %02x", c->data[t]);
			t = 0;
			fprintf(stderr, " ] ");
			c = c->next;
		}
		fprintf(stderr, "\n");
	}
#endif

	while (frames) {
		int data_needed = frames * buf->framesize;
		int rdbytes = data_needed;
		int rdframes;

		if (rdbytes > buf->first->used - buf->read_offset)
			rdbytes = buf->first->used - buf->read_offset;

		memcpy(dest, buf->first->data + buf->read_offset, rdbytes);
		buf->read_offset += rdbytes;
		dest += rdbytes;

		if (buf->read_offset == SFX_AUDIO_BUF_SIZE) {
			/* Continue to next, enqueue the current chunk as
			** being unused */
			sfx_audio_buf_chunk_t *lastfirst = buf->first;

			buf->first = buf->first->next;
			lastfirst->next = buf->unused;
			buf->unused = lastfirst;

			buf->read_offset = 0;
		}

		rdframes = (rdbytes / buf->framesize);
		frames -= rdframes;
		written += rdframes;

		if (frames &&
		    (!buf->first || buf->read_offset == buf->first->used)) {
			/* Buffer underrun! */
			if (!reported_buffer_underrun) {
				sciprintf("[audiobuf] Buffer underrun\n");
				reported_buffer_underrun = 1;
			}
			do {
				memcpy(dest, buf->last_frame, buf->framesize);
				dest += buf->framesize;
			} while (--frames);
		}

	}

	return written;
}

void
sfx_audbuf_write_timestamp(sfx_audio_buf_t *buf, sfx_timestamp_t ts);

int
sfx_audbuf_read_timestamp(sfx_audio_buf_t *buf, sfx_timestamp_t *ts);
