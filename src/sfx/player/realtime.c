/***************************************************************************
 realtime.c Copyright (C) 2002 Christoph Reichenbach


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
/* OK... 'realtime' may be a little too euphemistic, as this one just
** prays for some reasonable amount of soft real-time, but it's close
** enough, I guess.  */

#include <sfx_player.h>
#include <sfx_timer.h>
#include <sfx_sequencer.h>

static sfx_timer_t *timer;
static sfx_sequencer_t *seq;

/* Playing mechanism */

static inline long
delta_time(GTimeVal comp_tv)
{
	GTimeVal tv;
	sci_get_current_time(&tv);
	return (comp_tv.tv_sec - tv.tv_sec) * 1000000
		+ (comp_tv.tv_usec - tv.tv_usec);
}

static song_iterator_t *play_it = NULL;
static GTimeVal play_last_time;
static int play_paused = 0;
static int play_it_done = 0;
static int play_writeahead_initial = 0;
static int play_writeahead = 0;
static int play_moredelay = 0;

static void
play_song(song_iterator_t *it, GTimeVal *wakeup_time, int writeahead_time)
{
	unsigned char buf[8];
	int result;

	while (play_it && delta_time(*wakeup_time) < writeahead_time) {
		int delay;

		switch ((delay = play_it->next(it, &(buf[0]), &result))) {

		case SI_FINISHED:
			play_it_done = 1;
			return;

		case SI_LOOP:
		case SI_CUE:
		case SI_PCM:
			break;

		case 0:
			seq->event(buf[0], result-1, buf+1);

			break;

		default:
			play_moredelay = delay - 1;
			*wakeup_time = song_next_wakeup_time(wakeup_time, delay);
			seq->delay(delay);
		}
	}
}

static void
_rt_timer_callback(void *data)
{
	if (play_it && !play_it_done) {
		if (!play_moredelay) {
			long delta = delta_time(play_last_time);

			if (delta < 0) {
				play_writeahead -= (int)((double)delta * 1.2); /* Adjust upwards */
			} else if (delta > 15000) {
				play_writeahead -= 2500; /* Adjust downwards */
			}
		} else --play_moredelay;

		if (play_writeahead < seq->min_write_ahead_ms)
			play_writeahead = seq->min_write_ahead_ms;

		play_song(play_it, &play_last_time, play_writeahead);
	}
}

/* API implementation */

static int
rt_set_option(char *name, char *value)
{
	return SFX_ERROR;
}

static int
rt_init(resource_mgr_t *resmgr)
{
	resource_t *res = NULL;
	void *seq_res = NULL;
	void *seq_dev = NULL;

	timer = sfx_find_timer(NULL);
	seq = sfx_find_sequencer(NULL);

	if (!timer) {
		fprintf(stderr, "[SFX] " __FILE__": Could not find timing mechanism\n");
		return SFX_ERROR;
	}

	if (!seq) {
		fprintf(stderr, "[SFX] " __FILE__": Could not find sequencer\n");
		return SFX_ERROR;
	}

	if (seq->patchfile) {
		res = scir_find_resource(resmgr, sci_patch, seq->patchfile, 0);
		if (!res) {
			fprintf(stderr, "[SFX] " __FILE__": patch.%03d requested by sequencer (%s), but not found\n",
				seq->patchfile, seq->name);
		}
	}

	if (seq->device)
		seq_dev = sfx_find_device(seq->device, NULL);

	if (seq->open(res? res->size : 0,
		      res? res->data : NULL,
		      seq_dev)) {
		fprintf(stderr, "[SFX] " __FILE__": Sequencer failed to initialize\n");
		return SFX_ERROR;
	}


	if (timer->init(_rt_timer_callback, seq_res) || timer->start()) {
		fprintf(stderr, __FILE__": Timer failed to initialize\n");
		seq->close();
		return SFX_ERROR;
	}

	play_writeahead = timer->delay_ms;
	if (play_writeahead < seq->min_write_ahead_ms)
		play_writeahead = seq->min_write_ahead_ms;

	play_writeahead *= 1000; /* microseconds */

	return SFX_OK;
}

static int
rt_set_iterator(song_iterator_t *it, GTimeVal start_time)
{
	if (play_it) {
		fprintf(stderr, __FILE__": set_iterator: Attempted while iterator was set!\n");
		return SFX_ERROR;
	}

	SIMSG_SEND(it, SIMSG_SET_PLAYMASK(seq->playmask));

	play_last_time = start_time;
	play_it = it;
	play_it_done = 0;
	play_moredelay = 0;

	return SFX_OK;
}

static int
rt_fade_out()
{
	fprintf(stderr, __FILE__": Attempt to fade out- not implemented yet\n");
	return SFX_ERROR;
}

static int
rt_stop()
{
	song_iterator_t *it = play_it;

	play_it = NULL;

	if (it)
		songit_free(it);
	if (seq && seq->allstop)
		seq->allstop();

	return SFX_OK;
}

static int
rt_send_iterator_message(song_iterator_message_t msg)
{
	if (!play_it)
		return SFX_ERROR;

	songit_handle_message(&play_it, msg);
	return SFX_OK;
}

static int
rt_pause()
{
	play_paused = 1;
	return (seq->allstop() || timer->stop());
}

static int
rt_resume()
{
	play_paused = 0;
	return timer->start();
}

static int
rt_exit()
{
	int retval = SFX_OK;

	if(seq->close()) {
		fprintf(stderr, "[SFX] Sequencer reported error on close\n");
		retval = SFX_ERROR;
	}

	if(timer->stop()) {
		fprintf(stderr, "[SFX] Timer reported error on close\n");
		retval = SFX_ERROR;
	}

	return retval;
}

sfx_player_t sfx_player_realtime = {
	"realtime",
	"0.1",
	&rt_set_option,
	&rt_init,
	&rt_set_iterator,
	&rt_fade_out,
	&rt_stop,
	&rt_send_iterator_message,
	&rt_pause,
	&rt_resume,
	&rt_exit
};
