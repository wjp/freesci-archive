/***************************************************************************
 mixer.c Copyright (C) 2003 Christoph Reichenbach


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

#include <sfx_mixer.h>


/*#define DEBUG 3*/
/* Set DEBUG to one of the following:
** anything -- high-level debugging (feed subscriptions/deletions etc.)
**     >= 1 -- rough input and output analysis (once per call)
**     >= 2 -- more detailed input analysis (once per call and feed)
**     >= 3 -- fully detailed input and output analysis (once per sample and feed)
*/

#define DEBUG 0

#define MIN_DELTA_OBSERVATIONS 100 /* Number of times the mixer is called before it starts trying to improve latency */
#define MAX_DELTA_OBSERVATIONS 1000000 /* Number of times the mixer is called before we assume we truly understand timing */

static int diagnosed_too_slow = 0;

struct mixer_private {
	byte *outbuf; /* Output buffer to write to the PCM device next time */
	byte *writebuf; /* Buffer we're supposed to write to */
	gint32 *compbuf_l, *compbuf_r; /* Intermediate buffers for computation */
	int lastbuf_len; /* Number of samples stored in the last buffer */

	long skew; /* Millisecond relative to which we compute time. This is the millisecond
		   ** part of the first time we emitted sound, to simplify some computations.  */
	long lsec; /* Last point in time we updated buffers, if any (seconds since the epoch) */
	int played_this_second; /* Number of samples emitted so far in second lsec */

	int max_delta; /* maximum observed time delta (using 'samples' as a metric unit) */
	int delta_observations; /* Number of times we played; confidence measure for max_delta */

	/* Pause data */
	int paused;
};

#define P ((struct mixer_private *)(self->private_bits))


static int
mix_init(sfx_pcm_mixer_t *self, sfx_pcm_device_t *device)
{
	self->dev = device;
	P = malloc(sizeof(struct mixer_private));
	P->outbuf = P->writebuf = NULL;
	P->lastbuf_len = 0;
	P->compbuf_l = malloc(sizeof(gint32) * device->buf_size);
	P->compbuf_r = malloc(sizeof(gint32) * device->buf_size);
fprintf(stderr, "-- ALLOCD size %d\n", device->buf_size);
	P->played_this_second = 0;
	P->paused = 0;
#ifdef DEBUG
	sciprintf("[soft-mixer] Initialised device %s v%s (%d Hz, %d/%x)\n",
		  device->name, device->version,
		  device->conf.rate, device->conf.stereo, device->conf.format);
#endif
	return SFX_OK;
}

static inline unsigned int
gcd(unsigned int a, unsigned int b)
{
	if (a == b)
		return a;

	if (a < b) {
		unsigned int c = b % a;

		if (!c)
			return a;

		return gcd(c, a);
	} else
		return gcd(b, a);
}

static sfx_pcm_urat_t
urat(unsigned int nom, unsigned int denom)
{
	sfx_pcm_urat_t rv;
	unsigned int g;

	rv.val = nom / denom;
	nom -= rv.val * denom;
	if (nom == 0)
		g = 1;
	else
		g = gcd(nom, denom);

	rv.nom = nom / g;
	rv.den = denom / g;

	return rv;
}

static void
mix_subscribe(sfx_pcm_mixer_t *self, sfx_pcm_feed_t *feed)
{
	sfx_pcm_feed_state_t *fs;

	if (!self->feeds) {
		self->feeds_allocd = 2;
		self->feeds = malloc(sizeof(sfx_pcm_feed_state_t) * self->feeds_allocd);
	} else if (self->feeds_allocd == self->feeds_nr) {
		self->feeds_allocd += 2;
		self->feeds = realloc(self->feeds, sizeof(sfx_pcm_feed_state_t) * self->feeds_allocd);
	}

	fs = self->feeds + self->feeds_nr++;
	fs->feed = feed;

	feed->sample_size = SFX_PCM_SAMPLE_SIZE(feed->conf);

	fs->buf_size = (self->dev->buf_size
			  * (feed->conf.rate
			     + self->dev->conf.rate - 1))
		/ self->dev->conf.rate;

	fs->buf = malloc(fs->buf_size * feed->sample_size);
	fs->scount = urat(0, 1);
	fs->spd = urat(feed->conf.rate, self->dev->conf.rate);
	fs->scount.den = fs->spd.den;
	fs->mode = SFX_PCM_FEED_MODE_ALIVE;

	fs->sample_bufstart = 0;

#ifdef DEBUG
	sciprintf("[soft-mixer] Subscribed %s-%x (%d Hz, %d/%x) at %d+%d/%d, buffer size %d\n",
		  feed->debug_name, feed->debug_nr, feed->conf.rate, feed->conf.stereo, feed->conf.format,
		  fs->spd.val, fs->spd.nom, fs->spd.den, fs->buf_size);
#endif
}

static void
mix_unsubscribe(sfx_pcm_mixer_t *self, sfx_pcm_feed_t *feed)
{
	int i;
#ifdef DEBUG
	sciprintf("[soft-mixer] Unsubscribing %s-%x\n", feed->debug_name, feed->debug_nr);
#endif
	for (i = 0; i < self->feeds_nr; i++) {
		sfx_pcm_feed_state_t *fs = self->feeds + i;

		if (fs->feed == feed) {
			feed->destroy(feed);

			if (fs->buf)
				free(fs->buf);

			feed->debug_name = "DESTROYED";
			
			self->feeds_nr--;

			/* Copy topmost into deleted so that we don't have any holes */
			self->feeds[i] = self->feeds[self->feeds_nr];

			if (self->feeds_allocd > 8 && self->feeds_allocd > (self->feeds_nr << 1)) {
				/* Limit memory waste */
				self->feeds_allocd >>= 1;
				self->feeds = realloc(self->feeds, sizeof(sfx_pcm_feed_t *) * self->feeds_allocd);
			}

			return;
		}
	}

	fprintf(stderr, "[sfx-mixer] Assertion failed: Deleting invalid feed %p out of %d\n", 
		feed, self->feeds_nr);
	BREAKPOINT();
}


static void
mix_exit(sfx_pcm_mixer_t *self)
{
	while (self->feeds_nr)
		mix_unsubscribe(self, self->feeds[0].feed);

	if (P->outbuf)
		free(P->outbuf);
	if (P->writebuf)
		free(P->writebuf);

	if (P->compbuf_l)
		free(P->compbuf_l);
	if (P->compbuf_l)
		free(P->compbuf_r);

	free(P);
	P = NULL;

#ifdef DEBUG
	sciprintf("[soft-mixer] Uninitialising mixer\n");
#endif
}	



#define LIMIT_16_BITS(v) 			\
		if (v < -32767)			\
			v = -32768;		\
		else if (v > 32766)		\
			v = 32767

static inline void
mix_compute_output(sfx_pcm_mixer_t *self, int outplen)
{
	int sample_i;
	sfx_pcm_config_t conf = self->dev->conf;
	int use_16 = conf.format & SFX_PCM_FORMAT_16;
	int bias = conf.format & ~SFX_PCM_FORMAT_LMASK;
	byte *lchan, *rchan;
	gint32 *lsrc = P->compbuf_l;
	gint32 *rsrc = P->compbuf_r;
	int sample_size = SFX_PCM_SAMPLE_SIZE(conf);


	if (!P->writebuf)
		P->writebuf = malloc(self->dev->buf_size * sample_size);

	if (conf.stereo) {
		if (conf.stereo == SFX_PCM_STEREO_RL) {
			lchan = P->writebuf + ((use_16)? 2 : 1);
			rchan = P->writebuf;
		} else {
			lchan = P->writebuf;
			rchan = P->writebuf + ((use_16)? 2 : 1);
		}
	} else
		lchan = P->writebuf;


	for (sample_i = 0; sample_i < outplen; sample_i++) {
		int left = *lsrc++;
		int right = *rsrc++;

		if (conf.stereo) {
			LIMIT_16_BITS(left);
			LIMIT_16_BITS(right);

			if (!use_16) {
				left >>= 8;
				right >>= 8;
			}

			left += bias;
			right += bias;

			if (use_16) {
				if (SFX_PCM_FORMAT_LE == (conf.format & SFX_PCM_FORMAT_ENDIANNESS)) {
					lchan[0] = left & 0xff;
					lchan[1] = (left >> 8) & 0xff;
					rchan[0] = right & 0xff;
					rchan[1] = (right >> 8) & 0xff;
				} else {
					lchan[1] = left & 0xff;
					lchan[0] = (left >> 8) & 0xff;
					rchan[1] = right & 0xff;
					rchan[0] = (right >> 8) & 0xff;
				}

				lchan += 4;
				rchan += 4;
			} else {
				*lchan = left & 0xff;
				*rchan = right & 0xff;

				lchan += 2;
				rchan += 2;
			}

		} else {
			left += right;
			left >>= 1;
			LIMIT_16_BITS(left);
			if (!use_16)
				left >>= 8;

			left += bias;

			if (use_16) {
				if (SFX_PCM_FORMAT_LE == (conf.format & SFX_PCM_FORMAT_ENDIANNESS)) {
					lchan[0] = left & 0xff;
					lchan[1] = (left >> 8) & 0xff;
				} else {
					lchan[1] = left & 0xff;
					lchan[0] = (left >> 8) & 0xff;
				}

				lchan += 2;
			} else {
				*lchan = left & 0xff;
				lchan += 1;
			}
		}
	}
}

static inline void
mix_swap_buffers(sfx_pcm_mixer_t *self)
{ /* Swap buffers */
	byte *tmp = P->outbuf;
	P->outbuf = P->writebuf;
	P->writebuf = tmp;
}


#define SAMPLE_OFFSET(usecs) \
			((usecs >> 7) /* approximate, since uint32 is too small */ \
			 * ((long) self->dev->conf.rate)) \
			/ (1000000L >> 7)

static inline int
mix_compute_buf_len(sfx_pcm_mixer_t *self, int *skip_samples)
     /* Computes the number of samples we ought to write. It tries to minimise the number,
     ** in order to reduce latency. */
     /* It sets 'skip_samples' to the number of samples to assume lost by latency, effectively
     ** skipping them.  */
{
	int free_samples;
	int recommended_samples;
	int played_samples = 0; /* since the last call */
	long secs, usecs;
	int sample_pos;
	int result_samples;

	sci_gettime(&secs, &usecs);

	if (!P->outbuf) {
		/* Called for the first time ever? */
		P->skew = usecs;
		P->lsec = secs;
		P->max_delta = 0;
		P->delta_observations = 0;
		P->played_this_second = self->dev->buf_size;
		return self->dev->buf_size;
	}

	/*	fprintf(stderr, "[%d:%d]S%d ", secs, usecs, P->skew);*/

	if (P->skew > usecs) {
		secs--;
		usecs += (1000000 - P->skew);
	}
	else
		usecs -= P->skew;
	
	sample_pos = SAMPLE_OFFSET(usecs);

	played_samples = P->played_this_second - sample_pos
		- ((secs - P->lsec) * self->dev->conf.rate);
	/*
	fprintf(stderr, "Between %d:? offset=%d and %d:%d offset=%d: Played %d at %d\n", P->lsec, P->played_this_second,
		secs, usecs, sample_pos, played_samples, self->dev->conf.rate);
	*/

	if (played_samples > P->max_delta)
		P->max_delta = played_samples;

	free_samples = self->dev->buf_size - played_samples;

	if (free_samples > self->dev->buf_size) {
		if (!diagnosed_too_slow) {
			sciprintf("[sfx-mixer] Your timer is too slow for your PCM output device (%d/%d).\n"
				  "[sfx-mixer] You might want to try changing the device, timer, or mixer, if possible.\n",
				  played_samples, self->dev->buf_size);
		}
		diagnosed_too_slow = 1;

		*skip_samples = free_samples - self->dev->buf_size;
		free_samples = self->dev->buf_size;
	} else
		*skip_samples = 0;

	++P->delta_observations;
	if (P->delta_observations > MAX_DELTA_OBSERVATIONS)
		P->delta_observations = MAX_DELTA_OBSERVATIONS;

	if (P->delta_observations > MIN_DELTA_OBSERVATIONS) { /* Start improving after a while */
		int d = P->delta_observations - MIN_DELTA_OBSERVATIONS;
		int diff = self->dev->conf.rate - P->max_delta;

		/* log-approximate P->max_delta over time */
		recommended_samples = P->max_delta +
			((diff * MIN_DELTA_OBSERVATIONS) / P->delta_observations);
/* WTF? */
	} else
		recommended_samples = self->dev->buf_size; /* Initially, keep the buffer full */

#if (DEBUG >= 1)
	sciprintf("[soft-mixer] played since last time: %d, recommended: %d, free: %d\n",
		  played_samples, recommended_samples, free_samples);
#endif

	if (recommended_samples > free_samples)
		result_samples = free_samples;
	else
		result_samples = recommended_samples;

	if (result_samples < 0)
		result_samples = 0;

	P->played_this_second += result_samples;
	while (P->played_this_second > self->dev->conf.rate) {
		/* Won't normally happen more than once */
		P->played_this_second -= self->dev->conf.rate;
		P->lsec++;
	}

	if (result_samples > self->dev->buf_size) {
		fprintf(stderr, "[soft-mixer] Internal assertion failed: samples-to-write %d > %d\n",
			result_samples, self->dev->buf_size);
	}

	return result_samples;
}



#define READ_NEW_VALUES() \
		if (samples_left > 0) {						\
			if (bias) { /* unsigned data */					\
				if (!use_16) {						\
					new_left = (*lsrc) << 8;			\
					new_right = (*rsrc) << 8;			\
				} else {						\
					if (conf.format & SFX_PCM_FORMAT_LE) {		\
						new_left = lsrc[0] | lsrc[1] << 8;	\
						new_right = rsrc[0] | rsrc[1] << 8;	\
					} else {					\
						new_left = lsrc[1] | lsrc[0] << 8;	\
						new_right = rsrc[1] | rsrc[0] << 8;	\
					}						\
				}							\
			} else { /* signed data */							\
				if (!use_16) {								\
					new_left = (*((signed char *)lsrc)) << 8;			\
					new_right = (*((signed char *)rsrc)) << 8;			\
				} else {								\
					if (conf.format & SFX_PCM_FORMAT_LE) {				\
						new_left = lsrc[0] | ((signed char *)lsrc)[1] << 8;	\
						new_right = rsrc[0] | ((signed char *)rsrc)[1] << 8;	\
					} else {							\
						new_left = lsrc[1] | ((signed char *)lsrc)[0] << 8;	\
						new_right = rsrc[1] | ((signed char *)rsrc)[0] << 8;	\
					}								\
				}									\
			}										\
										\
			new_left -= bias;					\
			new_right -= bias;					\
										\
			lsrc += sample_size;					\
			rsrc += sample_size;					\
		} else {							\
			new_left = new_right = 0;				\
			break;							\
		}


static void
mix_compute_input_linear(sfx_pcm_mixer_t *self, int add_result, sfx_pcm_feed_state_t *fs, int len)
     /* if add_result is non-zero, P->outbuf should be added to rather than overwritten. */
{
	sfx_pcm_feed_t *f = fs->feed;
	sfx_pcm_config_t conf = f->conf;
	int use_16 = conf.format & SFX_PCM_FORMAT_16;
	gint32 *lchan = P->compbuf_l;
	gint32 *rchan = P->compbuf_r;
	int sample_size = f->sample_size;
	byte *wr_dest = fs->buf + (sample_size * fs->sample_bufstart);
	byte *lsrc = fs->buf;
	byte *rsrc = fs->buf;
	/* Location to write to */
	int samples_nr;
	int bias = (conf.format & ~SFX_PCM_FORMAT_LMASK)? 0x8000 : 0;
	/* We use this only on a 16 bit level here */
	int i;
	sfx_pcm_urat_t counter = urat(0, 1);

	/* The two most extreme source samples we consider for a
	** destination sample  */
	int old_left, old_right;
	int new_right, new_left;

	int samples_read;
	int samples_left;

	/* First, compute the number of samples we want to retreive */
	samples_nr = fs->spd.val * len;
	/* A little complicated since we must consider partial samples */
	samples_nr += (fs->spd.nom * len
		       - fs->scount.nom /* remember that we may have leftovers */
		       + (fs->scount.den - 1 /* round up */))
		/ fs->spd.den;

	/* Make sure we have sufficient information */
	samples_left = samples_read = f->poll(f, wr_dest, samples_nr);

#if (DEBUG >= 2)
	sciprintf("[soft-mixer] Examining %s-%x (sample size %d); read %d/%d/%d, re-using %d samples\n",
		  f->debug_name, f->debug_nr, sample_size, samples_read, samples_nr,
		  fs->buf_size, fs->sample_bufstart);
#endif


	if (conf.stereo == SFX_PCM_STEREO_LR)
		rsrc += (use_16)? 2 : 1;
	else if (conf.stereo == SFX_PCM_STEREO_RL)
		lsrc += (use_16)? 2 : 1;
	/* Otherwise, we let both point to the same place */

#if (DEBUG >= 2)
	sciprintf("[soft-mixer] Stretching theoretical %d (physical %d) results to %d\n", samples_nr, samples_left, len);
#endif
	for (i = 0; i < len; i++) {
		int leftsum = 0; /* Sum of any complete samples we used */
		int rightsum = 0;

		int left; /* Sum of the two most extreme source samples
			  ** we considered, i.e. the oldest and newest
			  ** one corresponding to the output sample we are
			  ** computing  */
		int right;

		int sample_steps = fs->spd.val;
		int j;

		if (fs->scount.nom >= fs->scount.den) {
			fs->scount.nom -= fs->scount.den; /* Ensure fractional part < 1 */
			++sample_steps;
		}

		if (sample_steps) {
			old_left = new_left;
			old_right = new_right;
		}

		if (i == 0) {
			READ_NEW_VALUES();
			--samples_left;
#if (DEBUG >= 3)
			sciprintf("[soft-mix] Initial read %d:%d\n", new_left, new_right);
#endif
			old_left = new_left;
			old_right = new_right;
		}

		for (j = 0; j < sample_steps; j++) {
			READ_NEW_VALUES();
			--samples_left;
#if (DEBUG >= 3)
			sciprintf("[soft-mix] Step %d/%d made %d:%d\n", j, sample_steps, new_left, new_right);
#endif

			/* The last sample will be subject to the fractional
			** part analysis, so we add it to 'left' and 'right'
			** later-- all others are added to (leftsum, rightsum).
			*/
			if (j+1 < sample_steps) {
				leftsum += new_left;
				rightsum += new_right;
			}
		}

		left = new_left * fs->scount.nom
			+ old_left * (fs->scount.den - fs->scount.nom);
		right = new_right * fs->scount.nom
			+ old_right * (fs->scount.den - fs->scount.nom);

		/* Normalise */
		left  /= fs->spd.den;
		right /= fs->spd.den;

		leftsum += left;
		rightsum += right;


		/* Make sure to divide by the number of samples we added here */
		if (sample_steps > 1) {
			leftsum  /= (sample_steps);
			rightsum /= (sample_steps);
		}

#if (DEBUG >= 3)
		sciprintf("[soft-mix] Ultimate result: %d:%d (frac %d:%d)\n", leftsum, rightsum, left, right);
#endif

		if (add_result) {
			*(lchan++) += leftsum;
			*(rchan++) += rightsum;
		} else {
			*(lchan++) = leftsum;
			*(rchan++) = rightsum;
		}

		fs->scount.nom += fs->spd.nom; /* Count up fractional part */
	}

	/* Save whether we have a partial sample still stored */
	fs->sample_bufstart = samples_left + ((fs->scount.nom != 0)? 1 : 0);
#if (DEBUF >= 2)
	sciprintf("[soft-mix] Leaving %d over", fs->sample_bufstart);
#endif

	if (samples_read < samples_nr)
		fs->mode = SFX_PCM_FEED_MODE_DEAD;
}

long xlastsec = 0;
long xlastusec = 0;
long xsec = 0;
long xusec = 0;

static int
mix_process_linear(sfx_pcm_mixer_t *self)
{
	int src_i; /* source feed index counter */
	int sample_size = SFX_PCM_SAMPLE_SIZE(self->dev->conf);
	int samples_skip; /* Number of samples to discard, rather than to emit */
	int buflen = mix_compute_buf_len(self, &samples_skip); /* Compute # of samples we must compute and write */
	int fake_buflen;
	byte *left, *right;

	if (P->outbuf && P->lastbuf_len) {
		int rv = self->dev->output(self->dev, P->outbuf, P->lastbuf_len);

		if (rv == SFX_ERROR)
			return rv; /* error */
	}

#if (DEBUG >= 1)
	if (self->feeds_nr)
		sciprintf("[soft-mixer] Mixing %d output samples on %d input feeds\n", buflen, self->feeds_nr);
#endif
	if (self->feeds_nr && !P->paused) {
		/* Below, we read out all feeds in case we have to skip samples first, then get the
		** most current sound. 'fake_buflen' is either the actual buflen (for the last iteration)
		** or a fraction of the buf length to discard.  */
		do {
			if (samples_skip) {
				if (samples_skip > self->dev->buf_size)
					fake_buflen = self->dev->buf_size;
				else
					fake_buflen = samples_skip;

				samples_skip -= fake_buflen;
			} else {
				fake_buflen = buflen;
				samples_skip = -1; /* Mark us as being completely done */
			}

			for (src_i = 0; src_i < self->feeds_nr; src_i++) {
				mix_compute_input_linear(self, src_i, self->feeds + src_i, fake_buflen);
			}

			/* Destroy all feeds we finished */
			for (src_i = 0; src_i < self->feeds_nr; src_i++)
				if (self->feeds[src_i].mode == SFX_PCM_FEED_MODE_DEAD)
					mix_unsubscribe(self, self->feeds[src_i].feed);
		} while (samples_skip >= 0);

	} else { /* Zero it out */
		memset(P->compbuf_l, 0, sizeof(gint32) * buflen);
		memset(P->compbuf_r, 0, sizeof(gint32) * buflen);
	}

#if (DEBUG >= 1)
	if (self->feeds_nr)
		sciprintf("[soft-mixer] Done mixing for this session, the result will be our next output buffer\n");
#endif

#if (DEBUG >= 3)
	if (self->feeds_nr) {
		int i;
		sciprintf("[soft-mixer] Intermediate representation:\n");
		for (i = 0; i < buflen; i++)
			sciprintf("[soft-mixer] Offset %d:\t[%04x:%04x]\t%d:%d\n", i,
				  P->compbuf_l[i] & 0xffff, P->compbuf_r[i] & 0xffff,
				  P->compbuf_l[i], P->compbuf_r[i]);
	}
#endif

	mix_compute_output(self, buflen);
	P->lastbuf_len = buflen;

	/* Finalize */
	mix_swap_buffers(self);

	return SFX_OK;
}

static void
mix_pause(sfx_pcm_mixer_t *self)
{
	P->paused = 1;
}

static void
mix_resume(sfx_pcm_mixer_t *self)
{
	P->paused = 0;
}

sfx_pcm_mixer_t sfx_pcm_mixer_soft_linear = {
	"soft-linear",
	"0.1",

	mix_init,
	mix_exit,
	mix_subscribe,
	mix_pause,
	mix_resume,
	mix_process_linear,

	0,
	0,
	NULL,
	NULL,
	NULL
};
