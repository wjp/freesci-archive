/***************************************************************************
  Copyright (C) 2008 Christoph Reichenbach


 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantability,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Christoph Reichenbach (CR) <jameson@linuxgames.com>

***************************************************************************/

#include "sfx_iterator.h"
#include "sfx_iterator_internal.h"
#include <stdarg.h>

#define ASSERT_S(x) if (!(x)) { error("Failed assertion in L%d: " #x "\n", __LINE__); return; }
#define ASSERT(x) ASSERT_S(x)

/* Tests the song iterators */

int errors = 0;

void
error(char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "[ERROR] ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	++errors;
}


/* The simple iterator will finish after a fixed amount of time.  Before that,
** it emits (absolute) cues in ascending order.  */
struct simple_it_struct {
	INHERITS_SONG_ITERATOR;
	int lifetime_remaining;
	char *cues;
	int cue_counter;
	int cue_progress;
	int cues_nr;
}  simple_iterator;

int
simple_it_next(song_iterator_t *_self, unsigned char *buf, int *result)
{
	struct simple_it_struct *self = (struct simple_it_struct *) _self;

	if (self->lifetime_remaining == -1) {
		error("Song iterator called post mortem");
		return SI_FINISHED;
	}

	if (self->lifetime_remaining) {

		if (self->cue_counter < self->cues_nr) {
			int time_to_cue = self->cues[self->cue_counter];

			if (self->cue_progress == time_to_cue) {
				++self->cue_counter;
				self->cue_progress = 0;
				*result = self->cue_counter;
				return SI_ABSOLUTE_CUE;
			} else {
				int retval = time_to_cue - self->cue_progress;
				self->cue_progress = time_to_cue;

				if (retval > self->lifetime_remaining) {
					retval = self->lifetime_remaining;
					self->lifetime_remaining = 0;
					self->cue_progress = retval;
					return retval;
				}

				self->lifetime_remaining -= retval;
				return retval;
			}
		} else {
			int retval = self->lifetime_remaining;
			self->lifetime_remaining = 0;
			return retval;
		}

	} else {
		self->lifetime_remaining = -1;
		return SI_FINISHED;
	}
}

sfx_pcm_feed_t *
simple_it_pcm_feed(song_iterator_t *_self)
{
	error("No PCM feed!\n");
	return NULL;
}

void
simple_it_init(song_iterator_t *_self)
{
}

song_iterator_t *
simple_it_handle_message(song_iterator_t *_self, song_iterator_message_t msg)
{
	return NULL;
}

void
simple_it_cleanup(song_iterator_t *_self)
{
}

/* Initialises the simple iterator.
** Parameters: (int) delay: Number of ticks until the iterator finishes
**             (int *) cues: An array of cue delays (cue values are [1,2...])
**             (int) cues_nr:  Number of cues in ``cues''
** The first cue is emitted after cues[0] ticks, and it is 1.  After cues[1] additional ticks
** the next cue is emitted, and so on. */
song_iterator_t *
setup_simple_iterator(int delay, char *cues, int cues_nr)
{
	simple_iterator.lifetime_remaining = delay;
	simple_iterator.cues = cues;
	simple_iterator.cue_counter = 0;
	simple_iterator.cues_nr = cues_nr;
	simple_iterator.cue_progress = 0;

	simple_iterator.ID = 42;
	simple_iterator.channel_mask = 0x004f;
	simple_iterator.flags = 0;
	simple_iterator.priority = 1;

	simple_iterator.delegate = NULL;
	simple_iterator.death_listeners_nr = 0;

	simple_iterator.cleanup = simple_it_cleanup;
	simple_iterator.init = simple_it_init;
	simple_iterator.handle_message = simple_it_handle_message;
	simple_iterator.get_pcm_feed = simple_it_pcm_feed;
	simple_iterator.next = simple_it_next;

	return (song_iterator_t *) &simple_iterator;
}

#define ASSERT_SIT ASSERT(it == simple_it)
#define ASSERT_FFIT ASSERT(it == ff_it)
#define ASSERT_NEXT(n) ASSERT(songit_next(&it, data, &result, IT_READER_MASK_ALL) == n)
#define ASSERT_RESULT(n) ASSERT(result == n)
#define ASSERT_CUE(n) ASSERT_NEXT(SI_ABSOLUTE_CUE); ASSERT_RESULT(n)

void
test_simple_it()
{
	song_iterator_t *it;
	song_iterator_t *simple_it = (song_iterator_t *) &simple_iterator;
	unsigned char data[4];
	int result;
	puts("[TEST] simple iterator (test artifact)");

	it = setup_simple_iterator(42, NULL, 0);
	
	ASSERT_SIT;
	ASSERT_NEXT(42);
	ASSERT_SIT;
	ASSERT_NEXT(SI_FINISHED);
	ASSERT_SIT;

	it = setup_simple_iterator(42, "\003\004", 2);
	ASSERT_SIT;
	ASSERT_NEXT(3);
	ASSERT_CUE(1);
	ASSERT_SIT;
	ASSERT_NEXT(4);
	ASSERT_CUE(2);
	ASSERT_SIT;
//	fprintf(stderr, "XXX => %d\n", songit_next(&it, data, &result, IT_READER_MASK_ALL));
	ASSERT_NEXT(35);
	ASSERT_NEXT(SI_FINISHED);
	ASSERT_SIT;

	puts("[TEST] Test OK.");
}

void
test_fastforward()
{
	song_iterator_t *it;
	song_iterator_t *simple_it = (song_iterator_t *) &simple_iterator;
	song_iterator_t *ff_it;
	unsigned char data[4];
	int result;
	puts("[TEST] fast-forward iterator");

	it = setup_simple_iterator(42, NULL, 0);
	ff_it = it = new_fast_forward_iterator(it, 0);
	ASSERT_FFIT;
	ASSERT_NEXT(42);
	ASSERT_SIT; /* Must have morphed back */
	ASSERT_NEXT(SI_FINISHED);
	ASSERT_SIT;

	it = setup_simple_iterator(42, NULL, 0);
	ff_it = it = new_fast_forward_iterator(it, 1);
	ASSERT_FFIT;
	ASSERT_NEXT(41);
	/* May or may not have morphed back here */
	ASSERT_NEXT(SI_FINISHED);
	ASSERT_SIT;

	it = setup_simple_iterator(42, NULL, 0);
	ff_it = it = new_fast_forward_iterator(it, 41);
	ASSERT_FFIT;
	ASSERT_NEXT(1);
	/* May or may not have morphed back here */
	ASSERT_NEXT(SI_FINISHED);
	ASSERT_SIT;

	it = setup_simple_iterator(42, NULL, 0);
	ff_it = it = new_fast_forward_iterator(it, 42);
	ASSERT_NEXT(SI_FINISHED);
	/* May or may not have morphed back here */

	it = setup_simple_iterator(42, NULL, 0);
	ff_it = it = new_fast_forward_iterator(it, 10000);
	ASSERT_NEXT(SI_FINISHED);
	/* May or may not have morphed back here */

	it = setup_simple_iterator(42, "\003\004", 2);
	ff_it = it = new_fast_forward_iterator(it, 2);
	ASSERT_FFIT;
	ASSERT_NEXT(1);
	ASSERT_CUE(1);
	ASSERT_SIT;
	ASSERT_NEXT(4);
	ASSERT_CUE(2);
	ASSERT_SIT;
	ASSERT_NEXT(35);
	ASSERT_NEXT(SI_FINISHED);
	ASSERT_SIT;

	it = setup_simple_iterator(42, "\003\004", 2);
	ff_it = it = new_fast_forward_iterator(it, 5);
	ASSERT_FFIT;
	ASSERT_CUE(1);
	ASSERT_FFIT;
	ASSERT_NEXT(2);
	ASSERT_CUE(2);
	ASSERT_SIT;
	ASSERT_NEXT(35);
	ASSERT_NEXT(SI_FINISHED);
	ASSERT_SIT;

	it = setup_simple_iterator(42, "\003\004", 2);
	ff_it = it = new_fast_forward_iterator(it, 41);
	ASSERT_FFIT;
	ASSERT_CUE(1);
	ASSERT_FFIT;
	ASSERT_CUE(2);
	ASSERT_FFIT;
	ASSERT_NEXT(1);
	ASSERT_NEXT(SI_FINISHED);
	ASSERT_SIT;

	puts("[TEST] Test OK.");
}


int
main(int argc, char **argv)
{
	test_simple_it();
	test_fastforward();
	if (errors != 0)
		fprintf(stderr, "[ERROR] %d errors total.\n", errors);
	return (errors != 0);
}
