/***************************************************************************
 core.c Copyright (C) 2002 Christoph Reichenbach


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
/* Sound subsystem core: Event handler, sound player dispatching */

#include <stdio.h>
#include <sfx_engine.h>

#define DEBUG_CUES
#ifdef DEBUG_CUES
int sciprintf(char *msg, ...);
#endif

#define MILLION 1000000

static long
time_minus(GTimeVal t1, GTimeVal t2)
{
	return (t1.tv_sec - t2.tv_sec) * MILLION
		+ (t1.tv_usec - t2.tv_usec);
}

static GTimeVal
time_plus(GTimeVal t1, long delta)
{
	if (delta > 0)
		t1.tv_usec += delta % MILLION;
	else
		t1.tv_usec -= (-delta) % MILLION;

	t1.tv_sec += delta / MILLION;

	if (t1.tv_usec > MILLION) {
		t1.tv_sec++;
		t1.tv_usec -= MILLION;
	}

	return t1;
}


static void
_freeze_time(sfx_state_t *self)
{
	/* Freezes the top song delay time */
	GTimeVal ctime;
	long delta;

	if (!self->song)
		return;

	sci_get_current_time(&ctime);
	delta = time_minus(self->wakeup_time, ctime);
	if (delta < 0)
		delta = 0;

	self->song->delay = delta;
}

static void
_thaw_time(sfx_state_t *self)
{
	/* inverse of _freeze_time() */
	GTimeVal ctime;

	if (!self->song)
		return;

	sci_get_current_time(&ctime);
	self->wakeup_time = time_plus(ctime, self->song->delay);
}

static void
_update(sfx_state_t *self)
{
	song_t *newsong = song_lib_find_active(self->songlib, self->song);
	if (newsong != self->song) {
		_freeze_time(self); /* Store song delay time */
		if (newsong) {
			/* Change song */
			if (newsong->status == SOUND_STATUS_WAITING)
				newsong->status = SOUND_STATUS_PLAYING;

			/* Stop waiting */
			sci_get_current_time(&self->wakeup_time);

			/* Change instrument mappings */
		} else {
			/* Turn off sound */
		}
		if (self->song) {
			if (self->song->status == SOUND_STATUS_PLAYING)
				self->song->status = SOUND_STATUS_WAITING;
		}

#ifdef DEBUG_CUES
		sciprintf("[SFX:CUE] Changing active song:");
		if (!self->song)
			sciprintf(" New song:");
		else
			sciprintf(" pausing %08lx, now playing", self->song->handle);

		if (newsong)
			sciprintf(" %08lx\n", newsong->handle);
		else
			sciprintf(" none\n");
#endif

		self->song = newsong;
		_thaw_time(self); /* Recover song delay time */
	}
}

void
sfx_init(sfx_state_t *self)
{
	song_lib_init(&self->songlib);
	self->song = NULL;
	sci_get_current_time(&self->wakeup_time); /* No need to delay */
}

void
sfx_exit(sfx_state_t *self)
{
	song_lib_free(self->songlib);
}

static inline int
time_le(GTimeVal a, GTimeVal b)
{
	return time_minus(a, b) <= 0;
}

void
sfx_suspend(sfx_state_t *self, int suspend)
{
	if (suspend && (!self->suspended)) {
		/* suspend */

		_freeze_time(self);

		/* Suspend song player */

	} else if (!suspend && (self->suspended)) {
		/* unsuspend */

		_thaw_time(self);

		/* Unsuspend song player */
	}

	self->suspended = suspend;
}

int
sfx_poll(sfx_state_t *self, song_handle_t *handle, int *cue)
/* Polls the sound server for cues etc.
** Returns   : (int) 0 if the cue queue is empty, SI_LOOP, SI_CUE, or SI_FINISHED otherwise
**             (song_handle_t) *handle: The affected handle
**             (int) *cue: The sound cue number (if SI_CUE)
*/
{
	GTimeVal ctime;

	if (!self->song)
		return 0; /* No milk today */

	sci_get_current_time(&ctime);
	*handle = self->song->handle;

	while (1) {
		unsigned char buf[8];
		int result;
		int buf_size;

		if (!time_le(self->wakeup_time, ctime))
			return 0; /* Patience, young hacker! */
		result = self->song->it->next(self->song->it, buf, cue);

		switch (result) {

		case SI_FINISHED:
			self->song->status = SOUND_STATUS_STOPPED;
			_update(self);
			/* ...fall through... */
		case SI_LOOP:
		case SI_CUE:
#ifdef DEBUG_CUES
			sciprintf("[SFX:CUE] %lx: <= ", *handle);
			if (result == SI_FINISHED)
				sciprintf("Finished\n");
			else {
				if (result == SI_LOOP)
					sciprintf("Loop: ");
				else
					sciprintf("Cue: ");

				sciprintf("%d (0x%x)\n", *cue, *cue);
			}
#endif
			return result;

		default:
			if (result > 0)
				self->wakeup_time =
					time_plus(self->wakeup_time, result * SOUND_TICK);
			/* Delay */
			break;
		}
	}
}


/*****************/
/*  Song basics  */
/*****************/

void
sfx_add_song(sfx_state_t *self, song_iterator_t *it, int priority, song_handle_t handle)
{
	song_t *song;

	if (!it) {
		fprintf(stderr, "Attempt to add song with handle %08lx\n", handle);
		return;
	}

	it->init(it);
	song = song_new(handle, it, priority);
	song_lib_remove(self->songlib, handle); /* No duplicates */
	self->song = NULL; /* Make sure we don't keep stale references */
	song_lib_add(self->songlib, song);
	_update(self);
}


void
sfx_remove_song(sfx_state_t *self, song_handle_t handle)
{
	if (self->song && self->song->handle == handle)
		self->song = NULL;

	song_lib_remove(self->songlib, handle);
	_update(self);
}



/**********************/
/* Song modifications */
/**********************/

#define ASSERT_SONG(s) if (!(s)) { fprintf(stderr, "Looking up song handle %08lx failed in %s, L%d\n", handle, __FILE__, __LINE__); return; }

void
sfx_song_set_status(sfx_state_t *self, song_handle_t handle, int status)
{
	song_t *song = song_lib_find(self->songlib, handle);
	ASSERT_SONG(song);

	switch (status) {

	case SOUND_STATUS_STOPPED:
		song->it->init(song->it); /* Reset */
		break;

	case SOUND_STATUS_SUSPENDED:
		break;

	case SOUND_STATUS_WAITING:
	case SOUND_STATUS_PLAYING:
		if (song == self->song)
			status = SOUND_STATUS_PLAYING;
		else
			status = SOUND_STATUS_WAITING;
		break;

	default:
		fprintf(stderr, "%s L%d: Attempt to set invalid song state %d!\n", __FILE__, __LINE__, status);
		return;

	}
	song->status = status;
	_update(self);
}


void
sfx_song_renice(sfx_state_t *self, song_handle_t handle, int priority)
{
	song_t *song = song_lib_find(self->songlib, handle);
	ASSERT_SONG(song);

	song->priority = priority;

	_update(self);
}

void
sfx_song_set_loops(sfx_state_t *self, song_handle_t handle, int loops)
{
	song_t *song = song_lib_find(self->songlib, handle);
	ASSERT_SONG(song);

	SIMSG_SEND(song->it, SIMSG_SET_LOOPS(loops));
}

