/***************************************************************************
 iterator.c Copyright (C) 2001..04 Christoph Reichenbach


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
/* Song iterators */

#include <stdio.h>
#include <sfx_iterator_internal.h>
#include <resource.h>
#include <sci_memory.h>

static const int MIDI_cmdlen[16] = {0, 0, 0, 0, 0, 0, 0, 0,
				    2, 2, 2, 2, 1, 1, 2, 0};

#define DEBUG_DECODING

#ifndef HAVE_MEMCHR
static void *
memchr(void *_data, int c, int n)
{
	unsigned char *data = (unsigned char *) _data;

	while (n && !(*data == c)) {
		++data;
		--n;
	}

	if (n)
		return data;
	else
		return NULL;
}
#endif

static void
_common_init(base_song_iterator_t *self)
{
	self->resetflag = 0;
	self->loops = 0;
}


/************************************/
/*-- SCI0 iterator implementation --*/
/************************************/

#define SCI0_MIDI_OFFSET 33
#define SCI0_END_OF_SONG 0xfc /* proprietary MIDI command */

#define SCI0_PCM_SAMPLE_RATE_OFFSET 0x0e
#define SCI0_PCM_SIZE_OFFSET 0x20
#define SCI0_PCM_DATA_OFFSET 0x2c

#define CHECK_FOR_END_ABSOLUTE(offset) \
	if (offset > self->size) { \
		fprintf(stderr, SIPFX "Reached end of song without terminator (%x/%x) at %d!\n", offset, self->size, __LINE__); \
		return SI_FINISHED; \
	}

#define CHECK_FOR_END(offset_augment) \
	if ((channel->offset + (offset_augment)) > channel->end) { \
		channel->state = SI_STATE_FINISHED; \
		fprintf(stderr, SIPFX "Reached end of track %d without terminator (%x+%x/%x) at %d!\n", channel->id, channel->offset, offset_augment, channel->end, __LINE__); \
		return SI_FINISHED; \
	}



static inline int
_parse_ticks(byte *data, int *offset_p, int size)
{
	int ticks = 0;
	int tempticks;
	int offset = 0;

	do {
		tempticks = data[offset++];
		ticks += (tempticks == SCI_MIDI_TIME_EXPANSION_PREFIX)?
			SCI_MIDI_TIME_EXPANSION_LENGTH : tempticks;
	} while (tempticks == SCI_MIDI_TIME_EXPANSION_PREFIX
		 && offset < size);

	if (offset_p)
		*offset_p = offset;

	return ticks;
}

#if 1
static void
midi_hexdump(byte *data, int size, int notational_offset)
{ /* Specialised for SCI01 tracks (this affects the way cumulative cues are treted ) */
	int offset = 0;
	int prev = 0;

	while (offset < size) {
		int old_offset = offset;
		int offset_mod;
		int time = _parse_ticks(data + offset, &offset_mod,
					size);
		int cmd;
		int pleft;
		int firstarg;
		int i;
		int blanks = 0;

		offset += offset_mod;
		fprintf(stderr, "  [%04x] %d\t",
			old_offset + notational_offset, time);

		cmd = data[offset];
		if (!(cmd & 0x80)) {
			cmd = prev;
			if (prev < 0x80) {
				fprintf(stderr, "Track broken at %x after"
					" offset mod of %d\n",
					offset + notational_offset, offset_mod);
				sci_hexdump(data, size, notational_offset);
				return;
			}
			fprintf(stderr, "(rs %02x) ", cmd);
			blanks += 8;
		} else {
			++offset;
			fprintf(stderr, "%02x ", cmd);
			blanks += 3;
		}
		prev = cmd;

		pleft = MIDI_cmdlen[cmd >> 4];
		if (SCI_MIDI_CONTROLLER(cmd) && data[offset]
		    == SCI_MIDI_CUMULATIVE_CUE)
			--pleft; /* This is SCI(0)1 specific */

		for (i = 0; i < pleft; i++) {
			if (i == 0)
				firstarg = data[offset];
			fprintf(stderr, "%02x ", data[offset++]);
			blanks += 3;
		}

		while (blanks < 16) {
			blanks += 4;
			fprintf(stderr, "    ");
		}

		while (blanks < 20) {
			++blanks;
			fprintf(stderr, " ");
		}

		if (cmd == SCI_MIDI_EOT)
			fprintf(stderr, ";; EOT");
		else if (cmd == SCI_MIDI_SET_SIGNAL) {
			if (firstarg == SCI_MIDI_SET_SIGNAL_LOOP)
				fprintf(stderr, ";; LOOP point");
			else
				fprintf(stderr, ";; CUE (%d)", firstarg);
		} else if (SCI_MIDI_CONTROLLER(cmd)) {
			if (firstarg == SCI_MIDI_CUMULATIVE_CUE)
				fprintf(stderr, ";; CUE (cumulative)");
			else if (firstarg == SCI_MIDI_RESET_ON_SUSPEND)
				fprintf(stderr, ";; RESET-ON-SUSPEND flag");
		}
		fprintf(stderr, "\n");

		if (old_offset >= offset) {
			fprintf(stderr, "-- Not moving forward anymore,"
				" aborting (%x/%x)\n", offset, old_offset);
			return;
		}
	}
}
#endif



static int
_sci0_read_next_command(sci0_song_iterator_t *self,
			unsigned char *buf, int *result);

#define PARSE_FLAG_LOOPS_UNLIMITED (1 << 0) /* Unlimited # of loops? */
#define PARSE_FLAG_PARAMETRIC_CUE (1 << 1) /* Assume that cues take an additional "cue value" argument */
/* This implements a difference between SCI0 and SCI1 cues. */

static int
_parse_sci_midi_command(base_song_iterator_t *self, unsigned char *buf,	int *result,
			song_iterator_channel_t *channel,
			int flags)
{
	unsigned char cmd;
	int paramsleft;
	int midi_op;
	int midi_channel;

	channel->state = SI_STATE_DELTA_TIME;

	cmd = self->data[channel->offset++];

	if (!(cmd & 0x80)) {
		/* 'Running status' mode */
		channel->offset--;
		cmd = channel->last_cmd;
	}

	midi_op = cmd >> 4;
	midi_channel = cmd & 0xf;
	paramsleft = MIDI_cmdlen[midi_op];
#if 0
if (1) {
	fprintf(stderr, "[IT]: off=%x, cmd=%02x, takes %d args ",
		channel->offset - 1, cmd, paramsleft);
	fprintf(stderr, "[%02x %02x <%02x> %02x %02x %02x]\n",
		self->data[channel->offset-3],
		self->data[channel->offset-2],
		self->data[channel->offset-1],
		self->data[channel->offset],
		self->data[channel->offset+1],
		self->data[channel->offset+2]);
}
#endif

	buf[0] = cmd;


	CHECK_FOR_END(paramsleft);
	memcpy(buf + 1, self->data + channel->offset, paramsleft);
	*result = 1 + paramsleft;

	channel->offset += paramsleft;

	channel->last_cmd = cmd;

	/* Are we supposed to play this channel? */
	if (
	    /* First, exclude "global" properties-- such as cues-- from consideration */
	     (midi_op < 0xf
	      && !(cmd == SCI_MIDI_SET_SIGNAL)
	      && !(SCI_MIDI_CONTROLLER(cmd)
		   && buf[1] == SCI_MIDI_CUMULATIVE_CUE))

	    /* Next, check if the channel is allowed */
	     && (!((1 << midi_channel) & channel->playmask)))
		return  /* Execute next command */
			self->next((song_iterator_t *) self, buf, result);


	if (cmd == SCI_MIDI_EOT) {
		/* End of track? */
		if (self->loops > 1 && channel->notes_played) {

			/* If allowed, decrement the number of loops */
			if (!(flags & PARSE_FLAG_LOOPS_UNLIMITED))
				*result = --self->loops;

#ifdef DEBUG_DECODING
			fprintf(stderr, "%s L%d: (%p):%d Looping ", __FILE__, __LINE__, self, channel->id);
			if (flags & PARSE_FLAG_LOOPS_UNLIMITED)
				fprintf(stderr, "(indef.)");
			else
				fprintf(stderr, "(%d)", self->loops);
			fprintf(stderr, " %x -> %x\n",
				channel->offset, channel->loop_offset);
#endif
			channel->offset = channel->loop_offset;
			channel->notes_played = 0;
			channel->state = SI_STATE_COMMAND;
			channel->total_timepos = channel->loop_timepos;

			return SI_LOOP;
		} else {
			channel->state = SI_STATE_FINISHED;
#ifdef DEBUG_DECODING
			fprintf(stderr, "%s L%d: (%p):%d EOT because"
				" %d notes, %d loops\n",
				__FILE__, __LINE__, self, channel->id,
				channel->notes_played, self->loops);
#endif
			return SI_FINISHED;
		}

	} else if (cmd == SCI_MIDI_SET_SIGNAL) {
		if (buf[1] == SCI_MIDI_SET_SIGNAL_LOOP) {
			channel->loop_offset = channel->offset - 1 - paramsleft;
			channel->loop_timepos = channel->total_timepos;
			return /* Execute next command */
				self->next((song_iterator_t *) self, buf, result);
		} else {
			/* Used to be conditional <= 127 */
			*result = buf[1]; /* Absolute cue */
			return SI_ABSOLUTE_CUE;
		}
	} else if (SCI_MIDI_CONTROLLER(cmd)) {
		switch (buf[1]) {

		case SCI_MIDI_CUMULATIVE_CUE:
			if (flags & PARSE_FLAG_PARAMETRIC_CUE)
				self->ccc += buf[2];
			else { /* No parameter to CC */
				self->ccc++;
				channel->offset--;
			}
			*result = self->ccc;
			return SI_RELATIVE_CUE;

		case SCI_MIDI_RESET_ON_SUSPEND:
			self->resetflag = buf[2];
			break;

		case SCI_MIDI_SET_POLYPHONY:
			self->polyphony[midi_channel] = buf[2];
			break;

		case SCI_MIDI_SET_REVERB:
			break;

		case SCI_MIDI_SET_VELOCITY:
			break;

		case 0x04: /* UNKNOWN NYI (happens in LSL2 gameshow) */
		case 0x46: /* UNKNOWN NYI (happens in LSL3 binoculars) */
		case 0x61: /* UNKNOWN NYI (special for adlib? Iceman) */
		case 0x73: /* UNKNOWN NYI (happens in Hoyle) */
		case 0xd1: /* UNKNOWN NYI (happens in KQ4 when riding the unicorn) */
			return /* Execute next command */
				self->next((song_iterator_t *) self, buf, result);

		case 0x01: /* modulation */
		case 0x07: /* volume */
		case 0x0a: /* panpot */
		case 0x0b: /* expression */
		case 0x40: /* hold */
		case 0x79: /* reset all */
			/* No special treatment neccessary */
			break;

		}
		return 0;

	} else {
		if ((cmd & 0xf0) == 0x90) /* note on? */
			channel->notes_played++;

		/* Process as normal MIDI operation */
		return 0;
	}
}


static int
_sci_midi_process_state(base_song_iterator_t *self, unsigned char *buf, int *result,
			song_iterator_channel_t *channel,
			int flags)
{
	CHECK_FOR_END(0);

	switch (channel->state) {

	case SI_STATE_UNINITIALISED:
		fprintf(stderr, SIPFX "Attempt to read command from uninitialized iterator!\n");
		self->init((song_iterator_t *) self);
		return self->next((song_iterator_t *) self, buf, result);

	case SI_STATE_FINISHED:
		return SI_FINISHED;

	case SI_STATE_DELTA_TIME: {
		int offset;
		int ticks = _parse_ticks(self->data + channel->offset,
					 &offset,
					 self->size - channel->offset);

		channel->offset += offset;
		channel->delay += ticks;
		channel->timepos_increment = ticks;

		CHECK_FOR_END(0);

		channel->state = SI_STATE_COMMAND;

		if (ticks)
			return ticks;
	}

	  /* continute otherwise... */

	case SI_STATE_COMMAND: {
		int retval;
		channel->total_timepos += channel->timepos_increment;
		channel->timepos_increment = 0;

		retval = _parse_sci_midi_command(self, buf, result,
						 channel, flags);

		if (retval == SI_FINISHED) {
			if (self->active_channels)
				--(self->active_channels);
#ifdef DEBUG_DECODING
			fprintf(stderr, "%s L%d: (%p):%d Finished channel, %d channels left\n",
				__FILE__, __LINE__, self, channel->id,
				self->active_channels);
#endif
			/* If we still have channels left... */
			if (self->active_channels) {
				return self->next((song_iterator_t *) self, buf, result);
			}

			/* Otherwise, we have reached the end */
			self->loops = 0;
		}

		return retval;
	}

	default:
		fprintf(stderr, SIPFX "Invalid iterator state %d!\n",
			channel->state);
		BREAKPOINT();
		return SI_FINISHED;
	}
}

static inline int
_sci_midi_process(base_song_iterator_t *self, unsigned char *buf, int *result,
		  song_iterator_channel_t *channel,
		  int flags)
{
	return _sci_midi_process_state(self, buf, result,
				       channel,
				       flags);
}

static int
_sci0_read_next_command(sci0_song_iterator_t *self, unsigned char *buf,
			int *result)
{
	return _sci_midi_process((base_song_iterator_t *) self, buf, result,
				   &(self->channel),
				   PARSE_FLAG_PARAMETRIC_CUE);

}


static inline int
_sci0_header_magic_p(unsigned char *data, int offset, int size)
{
	if (offset + 0x10 > size)
		return 0;
	return
		(data[offset] == 0x1a)
		&& (data[offset + 1] == 0x00)
		&& (data[offset + 2] == 0x01)
		&& (data[offset + 3] == 0x00);
}

static unsigned char *
_sci0_check_pcm(sci0_song_iterator_t *self, int *size,
		sfx_pcm_config_t *format)
{
	unsigned int offset;
	int tries = 2;
	int found_it = 0;
	unsigned char *pcm_data;
	offset = SCI0_MIDI_OFFSET;

	if (self->data[0] != 2)
		return NULL;
	/* No such luck */

	while ((tries--) && (offset < self->size) && (!found_it)) {
		/* Search through the garbage manually */
		unsigned char *fc = memchr(self->data + offset, SCI0_END_OF_SONG,
					self->size - offset);

		if (!fc) {
			fprintf(stderr, SIPFX "Warning: Playing unterminated song!\n");
			return NULL;
		}

		/* add one to move it past the END_OF_SONG marker */
		offset = fc - self->data + 1;


		if (_sci0_header_magic_p(self->data, offset, self->size))
			found_it = 1;
	}

	if (!found_it) {
		fprintf(stderr, SIPFX "Warning: Song indicates presence of PCM, but"
			" none found (finally at offset %04x)\n", offset);

		return NULL;
	}

	pcm_data = self->data + offset;

	*size = getInt16(pcm_data + SCI0_PCM_SIZE_OFFSET);

	/* Two of the format parameters are fixed by design: */
	format->format = SFX_PCM_FORMAT_U8;
	format->stereo = SFX_PCM_MONO;
	format->rate = getInt16(pcm_data + SCI0_PCM_SAMPLE_RATE_OFFSET);

	if (offset + SCI0_PCM_DATA_OFFSET + *size != self->size) {
		int d = offset + SCI0_PCM_DATA_OFFSET + *size - self->size;

		fprintf(stderr, SIPFX "Warning: PCM advertizes %d bytes of data, but %d"
			  " bytes are trailing in the resource!\n",
			  *size, self->size - (offset + SCI0_PCM_DATA_OFFSET));

		if (d > 0)
			*size -= d; /* Fix this */
	}

	return pcm_data + SCI0_PCM_DATA_OFFSET;
}

static song_iterator_t *
_sci0_handle_message(sci0_song_iterator_t *self, song_iterator_message_t msg)
{
	if (msg.recipient == _SIMSG_BASE) {
		switch (msg.type) {

		case _SIMSG_BASEMSG_SET_LOOPS:
			self->loops = msg.args[0];
			break;

		case _SIMSG_BASEMSG_CLONE: {
			int tsize = sizeof(sci0_song_iterator_t);
			base_song_iterator_t *mem = sci_malloc(tsize);
			memcpy(mem, self, tsize);
			mem->data = sci_malloc(mem->size);
			memcpy(mem->data, self->data, self->size);
			return (struct _song_iterator *) mem; /* Assume caller has another copy of this */
		}

		case _SIMSG_BASEMSG_SET_PLAYMASK: {
			int i;
			self->device_id = msg.args[0];

			/* Set all but the rhytm channel mask bits */
			self->channel.playmask &= ~(1 << MIDI_RHYTHM_CHANNEL);

			for (i = 0; i < MIDI_CHANNELS; i++)
				if (self->data[2 + (i << 1)] & self->device_id
				    && i != MIDI_RHYTHM_CHANNEL)
					self->channel.playmask |= (1 << i);
		}
			break;

		case _SIMSG_BASEMSG_SET_RHYTHM:
			self->channel.playmask &= ~(1 << MIDI_RHYTHM_CHANNEL);
			if (msg.args[0])
				self->channel.playmask |= (1 << MIDI_RHYTHM_CHANNEL);
			break;

		default:
			return NULL;
		}

		return (song_iterator_t *)self;
	}
	return NULL;
}

static void
_base_init_channel(song_iterator_channel_t *channel, int id, int offset,
		   int end)
{
	channel->playmask = PLAYMASK_NONE; /* Disable all channels */
	channel->id = id;
	channel->notes_played = 0;
	channel->state = SI_STATE_DELTA_TIME;
	channel->loop_timepos = 0;
	channel->total_timepos = 0;
	channel->timepos_increment = 0;
	channel->delay = 0; /* Only used for more than one channel */

	channel->offset
		= channel->loop_offset
		= channel->initial_offset
		= offset;
	channel->end = end;
}

static void
_sci0_init(sci0_song_iterator_t *self)
{
	_common_init((base_song_iterator_t *) self);

	self->ccc = 0; /* Reset cumulative cue counter */
	self->active_channels = 1;
	_base_init_channel(&(self->channel), 0, SCI0_MIDI_OFFSET, self->size);
	self->delay_remaining = 0;
}

static void
_sci0_cleanup(sci0_song_iterator_t *self)
{
	if (self->data)
		sci_free(self->data);
	self->data = NULL;
}

/***************************/
/*-- SCI1 song iterators --*/
/***************************/

#define SCI01_INVALID_DEVICE 0xff

/* First index determines whether DSP output is supported */
static int sci0_to_sci1_device_map[][2] = {
	{0x06, 0x0c}, /* MT-32 */
	{0xff, 0xff}, /* YM FB-01 */
	{0x00, 0x00}, /* CMS/Game Blaster-- we assume OPL/2 here... */
	{0xff, 0xff}, /* Casio MT540/CT460 */
	{0x13, 0x13}, /* Tandy 3-voice */
	{0x12, 0x12}, /* PC speaker */
	{0xff, 0xff},
	{0xff, 0xff},
}; /* Maps bit number to device ID */

#define SONGDATA(x) self->data[offset + (x)]
#define SCI1_CHANDATA(off) self->data[channel->offset + (off)]

static int
_sci1_sample_init(sci1_song_iterator_t *self, int offset)
{
	sci1_sample_t *sample, **seekerp;
	int rate;
	int length;
	int begin;
	int end;

	CHECK_FOR_END_ABSOLUTE(offset + 10);
	if (self->data[offset + 1] != 0)
		sciprintf("[iterator-1] In sample at offset 0x04x: Byte #1 is %02x instead of zero\n",
			  self->data[offset + 1]);

	rate = getInt16(self->data + offset + 2);
	length = getInt16(self->data + offset + 4);
	begin = getInt16(self->data + offset + 6);
	end = getInt16(self->data + offset + 8);

	CHECK_FOR_END_ABSOLUTE(offset + 10 + length);

	sample = sci_malloc(sizeof(sci1_sample_t));
	sample->delta = begin;
	sample->size = length;
	sample->data = self->data + offset + 10;

	fprintf(stderr, "[SAMPLE] %x/%x/%x/%x l=%x\n",
		offset + 10, begin, end, self->size, length);

	sample->format.format = SFX_PCM_FORMAT_U8;
	sample->format.stereo = SFX_PCM_MONO;
	sample->format.rate = rate;

	sample->announced = 0;

	/* Perform insertion sort */
	seekerp = &(self->next_sample);

	while (*seekerp && (*seekerp)->delta < begin)
		seekerp = &((*seekerp)->next);

	sample->next = *seekerp;
	*seekerp = sample;

	return 0; /* Everything's fine */
}


static int
_sci1_song_init(sci1_song_iterator_t *self)
{
	sci1_sample_t *seeker;
	int last_time;
	int offset = 0;

	self->channels_nr = 0;
	self->next_sample = 0;

	CHECK_FOR_END_ABSOLUTE(0);
	while (SONGDATA(0) != 0xff
	       && SONGDATA(0) != self->device_id) {
		offset++;
		CHECK_FOR_END_ABSOLUTE(offset + 1);
		while (SONGDATA(0) != 0xff) {
			CHECK_FOR_END_ABSOLUTE(offset + 7);
			offset += 6;
		}
		offset++;
	}

	if (SONGDATA(0) == 0xff) {
		sciprintf("[iterator-1] Song does not support"
			  " hardware 0x%02x\n",
			  self->device_id);
		return 1;
	}

	offset++;

	while (SONGDATA(0) != 0xff) { /* End of list? */
		int track_offset;
		int end;

		offset += 2;
		CHECK_FOR_END_ABSOLUTE(offset + 4);

		track_offset = getInt16(self->data + offset);
		end = getInt16(self->data + offset + 2);


		CHECK_FOR_END_ABSOLUTE(track_offset - 1);

		if (self->data[track_offset] == 0xfe) {
			if (_sci1_sample_init(self, track_offset))
				return 1; /* Error */
		} else {
			/* Regular MIDI channel */
			if (self->channels_nr >= MIDI_CHANNELS) {
				sciprintf("[iterator-1] Warning: Song has more than %d channels, cutting them off\n",
					  MIDI_CHANNELS);
				break; /* Scan for remaining samples */
			} else {
				song_iterator_channel_t *channel =
					&(self->channels[self->channels_nr++]);

				_base_init_channel(channel,
						   self->data[track_offset] & 0xf,
						   /* Skip over header bytes: */
						   track_offset + 2,
						   track_offset + end);

				self->polyphony[self->channels_nr - 1]
					= SCI1_CHANDATA(-1);

				channel->playmask = ~0; /* Enable all */

				CHECK_FOR_END_ABSOLUTE(offset + end);
			}
		}
		offset += 4;
		CHECK_FOR_END_ABSOLUTE(offset);
	}

	sciprintf("[iterator-1] (%p) DEBUG: detected %d channels\n",
		  self, self->channels_nr);
	{
		int i;
		sciprintf("[iterator-1] DEBUG: Polyphony = [ ");
		for (i = 0; i < self->channels_nr; i++)
			sciprintf("%d ", self->polyphony[i]);
		sciprintf("]\n");
	}

	/* Now ensure that sapmle deltas are relative to the previous sample */
	seeker = self->next_sample;
	last_time = 0;
	self->active_channels = self->channels_nr;
	self->channels_looped = 0;

	while (seeker) {
		int prev_last_time = last_time;
		sciprintf("[iterator-1] Detected sample: %d Hz, %d bytes at time %d\n",
			  seeker->format.rate, seeker->size, seeker->delta);
		last_time = seeker->delta;
		seeker->delta -= prev_last_time;
		seeker = seeker->next;
	}

	return 0; /* Success */
}

#undef SONGDATA

static inline int
_sci1_get_smallest_delta(sci1_song_iterator_t *self)
{
	int i, d = -1;
	for (i = 0; i < self->channels_nr; i++)
		if (self->channels[i].state == SI_STATE_COMMAND
		    && (d == -1 || self->channels[i].delay < d))
			d = self->channels[i].delay;

	if (self->next_sample && self->next_sample->delta < d)
		return self->next_sample->delta;
	else
		return d;
}

static inline void
_sci1_update_delta(sci1_song_iterator_t *self, int delta)
{
	int i;

	if (self->next_sample)
		self->next_sample->delta -= delta;

	for (i = 0; i < self->channels_nr; i++)
		if (self->channels[i].state == SI_STATE_COMMAND)
			self->channels[i].delay -= delta;
}

static inline int
_sci1_no_delta_time(sci1_song_iterator_t *self)
{ /* Checks that none of the channels is waiting for its delta to be read */
	int i;

	for (i = 0; i < self->channels_nr; i++)
		if (self->channels[i].state == SI_STATE_DELTA_TIME)
		return 0;

	return 1;
}

#define COMMAND_INDEX_PCM -2

static inline int /* Determine the channel # of the next active event, or -1 */
_sci1_command_index(sci1_song_iterator_t *self)
{
	int i;
	int base_delay = 0x7ffffff;
	int best_chan = -1;

	for (i = 0; i < self->channels_nr; i++)
		if ((self->channels[i].state != SI_STATE_PENDING)
		    && (self->channels[i].state != SI_STATE_FINISHED))  {

			if ((self->channels[i].state == SI_STATE_DELTA_TIME)
			    && (self->channels[i].delay == 0))
				return i;
			/* First, read all unknown delta times */

			if (self->channels[i].delay < base_delay) {
				best_chan = i;
				base_delay = self->channels[i].delay;
			}
		}

	if (self->next_sample && base_delay >= self->next_sample->delta)
		return COMMAND_INDEX_PCM;

	return best_chan;
}

static void
_sci1_dump_state(sci1_song_iterator_t *self)
{
	int i;

	sciprintf("-- [%p] ------------------------\n", self);
	for (i = 0; i < self->channels_nr; i++) {
		int j;
		sciprintf("%d(s%02d): d-%d:\t(%x/%x)  ",
			  self->channels[i].id,
			  self->channels[i].state,
			  self->channels[i].delay,
			  self->channels[i].offset,
			  self->channels[i].end);
		for (j = -3; j < 9; j++) {
			if (j == 0)
				sciprintf(">");
			else
				sciprintf(" ");

			sciprintf("%02x", self->data[self->channels[i].offset+j]);

			if (j == 0)
				sciprintf("<");
			else
				sciprintf(" ");
		}
		sciprintf("\n");
	}
	if (self->next_sample) {
		sciprintf("\t[sample %d]\n",
			  self->next_sample->delta);
	}
	sciprintf("------------------------------------------\n");
}


static unsigned char *
_sci1_get_pcm(sci1_song_iterator_t *self,
	      int *size, sfx_pcm_config_t *format)
{
	if (self->next_sample
	    && self->next_sample->delta <= 0) {
		sci1_sample_t *sample = self->next_sample;
		byte *retval = sample->data;

#warning "MEMORY LEAK-- fixme (API change -> return iterator, refcounting!)"
retval = memdup(retval, sample->size);

		self->next_sample = self->next_sample->next;

		if (format)
			*format = sample->format;
		if (size)
			*size = sample->size;
		sci_free(sample);

		return retval;
	} else
		return NULL;
}


static int
_sci1_process_next_command(sci1_song_iterator_t *self,
			unsigned char *buf, int *result)
{
	int retval = -42; /* Shouldn't happen, but gcc doesn't agree */
	int chan;

	if (!self->initialised) {
		sciprintf("[iterator-1] DEBUG: Initialising for %d\n",
			  self->device_id);
		self->initialised = 1;
		if (_sci1_song_init(self))
			return SI_FINISHED;
	}


	if (self->delay_remaining) {
		int delay = self->delay_remaining;
		self->delay_remaining = 0;
		return delay;
	}

	do {
		chan = _sci1_command_index(self);

		if (chan == COMMAND_INDEX_PCM) {

			if (self->next_sample->announced) {
				/* Already announced; let's discard it */
				_sci1_get_pcm(self, NULL, NULL);
			} else {
				int delay = self->next_sample->delta;

				if (delay) {
					_sci1_update_delta(self, delay);
					return delay;
				}
				/* otherwise we're touching a PCM */
				self->next_sample->announced = 1;
				return SI_PCM;
			}
		} else { /* Not a PCM */

		retval = _sci_midi_process((base_song_iterator_t *) self,
					   buf, result,
					   &(self->channels[chan]),
					   PARSE_FLAG_LOOPS_UNLIMITED);

		if (retval == SI_LOOP) {
			self->channels_looped++;
			self->channels[chan].state = SI_STATE_PENDING;

			if (self->channels_looped == self->active_channels) {
				int i;

				/* Everyone's ready: Let's loop */
				for (i = 0; i < self->channels_nr; i++)
					if (self->channels[i].state
					    == SI_STATE_PENDING)
						self->channels[i].state
							= SI_STATE_DELTA_TIME;

				self->channels_looped = 0;
				return SI_LOOP;
			}
		} else

		if (retval > 0) {
			int sd ;
			sd = _sci1_get_smallest_delta(self);

			if (_sci1_no_delta_time(self) && sd) {
				/* No other channel is ready */
				_sci1_update_delta(self, sd);

				/* Only from here do we return delta times */
				return sd;
			}
		}

		} /* Not a PCM */

	} while (retval > 0); /* All delays must be processed separately */

	return retval;
}

static struct _song_iterator *
_sci1_handle_message(sci1_song_iterator_t *self,
		     song_iterator_message_t msg)
{
	if (msg.recipient == _SIMSG_BASE) { /* May extend this in the future */
		switch (msg.type) {

		case _SIMSG_BASEMSG_CLONE: {
			int tsize = sizeof(sci1_song_iterator_t);
			sci1_song_iterator_t *mem = sci_malloc(tsize);
			sci1_sample_t **samplep;
			int delta = msg.args[0]; /* Delay until next step */

			memcpy(mem, self, tsize);
			samplep = &(mem->next_sample);

			mem->data = sci_malloc(mem->size);
			memcpy(mem->data, self->data, self->size);

			mem->delay_remaining += delta;

			/* Clone chain of samples */
			while (*samplep) {
				sci1_sample_t *newsample
					= sci_malloc(sizeof(sci1_sample_t));
				memcpy(newsample, *samplep,
				       sizeof(sci1_sample_t));
				*samplep = newsample;
				samplep = &(newsample->next);
			}

			return (struct _song_iterator *) mem; /* Assume caller has another copy of this */
		}

		case _SIMSG_BASEMSG_SET_PLAYMASK: {
			self->device_id
				= sci0_to_sci1_device_map
				[sci_ffs(msg.args[0] & 0xff) - 1]
				[sfx_pcm_available()]
				;

			if (self->device_id == 0xff) {
				sciprintf("[iterator-1] Warning: Device %d(%d) not supported",
					  msg.args[0] & 0xff, sfx_pcm_available());
			}

			if (self->initialised) {
				int i;
				int toffset = -1;

				for (i = 0; i < self->channels_nr; i++)
					if (self->channels[i].state != SI_STATE_FINISHED
					    && self->channels[i].total_timepos > toffset) {
						toffset = self->channels[i].total_timepos
							+ self->channels[i].timepos_increment
							- self->channels[i].delay;
					}

				/* Find an active channel so that we can
				** get the correct time offset  */

				_sci1_song_init(self);

				toffset -= self->delay_remaining;
				self->delay_remaining = 0;

				if (toffset > 0)
					return new_fast_forward_iterator((song_iterator_t *) self,
									 toffset);
			}

			break;

		}

		case _SIMSG_BASEMSG_SET_LOOPS:
			self->loops = (msg.args[0] > 32767)? 99 : 0;
			/* 99 is arbitrary, but we can't use '1' because of
			** the way we're testing in the decoding section.  */
			break;

		case _SIMSG_BASEMSG_SET_RHYTHM:
			/* Ignore */
			break;

		default:
			fprintf(stderr, SIPFX "Unsupported command %d to"
				" SCI1 iterator", msg.type);
		}
		return (song_iterator_t *) self;
	}
	return NULL;
}


static int
_sci1_read_next_command(sci1_song_iterator_t *self,
			unsigned char *buf, int *result)
{
	return _sci1_process_next_command(self, buf, result);
}


static void
_sci1_init(sci1_song_iterator_t *self)
{
	_common_init((base_song_iterator_t *) self);
	self->ccc = 127;
	self->device_id = 0x12; /* Default to PC speaker for purposes
				** of cue computation  */
	self->next_sample = NULL;
	self->channels_nr = 0;
	self->initialised = 0;
	self->delay_remaining = 0;
	self->loops = 0;
}

static void
_sci1_cleanup(sci1_song_iterator_t *it)
{
	sci1_sample_t *sample_seeker = it->next_sample;
	while (sample_seeker) {
		sci1_sample_t *old_sample = sample_seeker;
		sample_seeker = sample_seeker->next;
		sci_free(old_sample);
	}

	_sci0_cleanup((sci0_song_iterator_t *)it);
}


/**********************************/
/*-- Fast-forward song iterator --*/
/**********************************/

static int
_ff_read_next_command(fast_forward_song_iterator_t *self,
		      byte *buf, int *result)
{
	int rv;

	if (self->delta <= 0)
		return SI_MORPH; /* Did our duty */

	while (1) {
		rv = self->delegate->next(self->delegate, buf, result);

		if (rv > 0) {
			/* Subtract from the delta we want to wait */
			self->delta -= rv;

			/* Done */
			if (self->delta < 0)
				return -self->delta;
		}

		if (rv == SI_FINISHED
		    || (rv == 0 && buf[0] >= 0xa0)) /* Filter out play events */
			return rv;
	}
}

static byte *
_ff_check_pcm(fast_forward_song_iterator_t *self,
	      int *size, sfx_pcm_config_t *format)
{
	return self->delegate->get_pcm(self->delegate,
				       size, format);
}

static song_iterator_t *
_ff_handle_message(fast_forward_song_iterator_t *self,
		   song_iterator_message_t msg)
{
	if (msg.recipient == _SIMSG_PLASTICWRAP)
		switch (msg.type) {

		case _SIMSG_PLASTICWRAP_ACK_MORPH:
			if (self->delta <= 0) {
				song_iterator_t *it = self->delegate;
				sci_free(self);
				return it;
			}
			break;

		default:
			BREAKPOINT();
		}

	return NULL;
}


static void
_ff_init(fast_forward_song_iterator_t *self)
{
	return;
}


song_iterator_t *
new_fast_forward_iterator(song_iterator_t *capsit, int delta)
{
	fast_forward_song_iterator_t *it =
		sci_malloc(sizeof(fast_forward_song_iterator_t));

	fprintf(stderr, "[ff] Asked to forward %d\n", delta);

	it->delegate = capsit;
	it->delta = delta;

	it->next = (int(*)(song_iterator_t *, unsigned char *, int *))
		_ff_read_next_command;
	it->get_pcm = (unsigned char*(*)(song_iterator_t *, int*, int *, int *))
		_ff_check_pcm;
	it->handle_message = (song_iterator_t *(*)(song_iterator_t *, song_iterator_message_t))
		_ff_handle_message;
	it->init = (void(*)(song_iterator_t *))
		_ff_init;
	it->cleanup = NULL;


	return (song_iterator_t *) it;
}


/*************************************/
/*-- General purpose functionality --*/
/*************************************/

int
songit_next(song_iterator_t **it, unsigned char *buf, int *result, int mask)
{
	int retval;

	do {
		retval = (*it)->next(*it, buf, result);
		if (retval == SI_MORPH)
			if (!SIMSG_SEND((*it), SIMSG_ACK_MORPH)) {
				BREAKPOINT();
			}
	} while (! ( /* Until one of the following holds */
		    (retval > 0 && (mask & IT_READER_MASK_DELAY))
		    || (retval == 0 && (mask & IT_READER_MASK_MIDI))
		    || (retval == SI_LOOP && (mask & IT_READER_MASK_LOOP))
		    || (retval == SI_ABSOLUTE_CUE &&
			(mask & IT_READER_MASK_CUE))
		    || (retval == SI_RELATIVE_CUE &&
			(mask & IT_READER_MASK_CUE))
		    || (retval == SI_PCM && (mask & IT_READER_MASK_PCM))
		    || (retval == SI_FINISHED)
		    ));

	return retval;
}



song_iterator_t *
songit_new(unsigned char *data, unsigned int size, int type)
{
	base_song_iterator_t *it;
	int i;

	if (!data || size < 33) {
		fprintf(stderr, SIPFX "Attempt to instantiate song iterator for null"
			  " song data\n");
		return NULL;
	}


	switch (type) {

	    case SCI_SONG_ITERATOR_TYPE_SCI0:
		    /**-- Playing SCI0 sound resources --**/
		    it = sci_malloc(sizeof(sci0_song_iterator_t));

		    for (i = 0; i < MIDI_CHANNELS; i++)
			    it->polyphony[i] = data[1 + (i << 1)];

		    it->next = (int(*)(song_iterator_t *, unsigned char *, int *))
			    _sci0_read_next_command;
		    it->get_pcm = (unsigned char*(*)(song_iterator_t *, int*, int *, int *))
			    _sci0_check_pcm;
		    it->handle_message = (song_iterator_t *(*)(song_iterator_t *, song_iterator_message_t))
			    _sci0_handle_message;
		    it->init = (void(*)(song_iterator_t *))_sci0_init;
		    it->cleanup = (void(*)(song_iterator_t *))_sci0_cleanup;
		    ((sci0_song_iterator_t *)it)->channel.state
			    = SI_STATE_UNINITIALISED;
		    break;

	    case SCI_SONG_ITERATOR_TYPE_SCI1:
		    /**-- SCI01 or later sound resource --**/
		    it = sci_malloc(sizeof(sci1_song_iterator_t));

		    for (i = 0; i < MIDI_CHANNELS; i++)
			    it->polyphony[i] = 0; /* Unknown */

		    it->next = (int(*)(song_iterator_t *, unsigned char *, int *))
			    _sci1_read_next_command;
		    it->get_pcm = (unsigned char*(*)(song_iterator_t *, int*, int *, int *))
			    _sci1_get_pcm;
		    it->handle_message = (song_iterator_t *(*)(song_iterator_t *, song_iterator_message_t))
			    _sci1_handle_message;
		    it->init = (void(*)(song_iterator_t *))_sci1_init;
		    it->cleanup = (void(*)(song_iterator_t *))_sci1_cleanup;
		    break;

	    default:
		    /**-- Invalid/unsupported sound resources --**/
		    fprintf(stderr, SIPFX "Attempt to instantiate invalid/unknown"
			    " song iterator type %d\n", type);
		    return NULL;
	}

	it->delegate = NULL;

	it->data = sci_malloc(size);
	memcpy(it->data, data, size);
	it->size = size;

	it->init((song_iterator_t *) it);

	return (song_iterator_t *) it;
}

void
songit_free(song_iterator_t *it)
{
	if (it) {
		if (it->cleanup)
			it->cleanup(it);
		sci_free(it);
	}
}

song_iterator_message_t
songit_make_message(int recipient, int type, int a1, int a2)
{
	song_iterator_message_t rv;
	rv.recipient = recipient;
	rv.type = type;
	rv.args[0] = a1;
	rv.args[1] = a2;

	return rv;
}


int
songit_handle_message(song_iterator_t **it_reg_p, song_iterator_message_t msg)
{
	song_iterator_t *it = *it_reg_p;
	song_iterator_t *newit;

	if (it->delegate && songit_handle_message(&(it->delegate), msg))
		return 1; /* Handled recursively */

	/* No one else could handle it: */

	newit = it->handle_message(it, msg);

	if (!newit)
		return 0; /* Couldn't handle */

	*it_reg_p = newit; /* Might have self-morphed */
	return 1;
}

song_iterator_t *
songit_clone(song_iterator_t *it, int delta)
{
	SIMSG_SEND(it, SIMSG_CLONE(delta));
	return it;
}

