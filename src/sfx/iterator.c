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
#include <sfx_iterator.h>
#include <resource.h>
#include <sci_memory.h>

#define SCI_SONG_ITERATOR_TYPE_SCI0 0
#define SCI_SONG_ITERATOR_TYPE_SCI1 1

#define SIPFX __FILE__" : "


static const int MIDI_cmdlen[16] = {0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 0};

#define PLAYMASK_NONE 0xffff

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
	memset(self->instruments, 0, sizeof(int) * MIDI_CHANNELS);
	memset(self->velocity, 0, sizeof(int) * MIDI_CHANNELS);
	memset(self->pressure, 0, sizeof(int) * MIDI_CHANNELS);
	memset(self->pitch, 0, sizeof(int) * MIDI_CHANNELS);
	memset(self->channel_map, 1, sizeof(int) * MIDI_CHANNELS);
	memset(self->reverb, 1, sizeof(int) * MIDI_CHANNELS);

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
		self->state = SI_STATE_FINISHED; \
		fprintf(stderr, SIPFX "Reached end of song without terminator (%d)!\n", __LINE__); \
		return SI_FINISHED; \
	}

#define CHECK_FOR_END(offset_augment) \
	CHECK_FOR_END_ABSOLUTE(self->offset + (offset_augment))

#define SONGDATA(x) self->data[self->offset + (x)]


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

static int
_sci0_read_next_command(base_song_iterator_t *self,
			unsigned char *buf, int *result);

static int
_parse_sci_midi(base_song_iterator_t *self, unsigned char *buf,
		int *result, int *offsetp, int *loopsp,
		byte *lastcmd, int playmask_check)
{
	unsigned char cmd;
	int paramsleft;
	int midi_op;
	int midi_channel;

	self->state = SI_STATE_DELTA_TIME;

	cmd = self->data[(*offsetp)++];

	if (!(cmd & 0x80)) {
		/* 'Running status' mode */
		(*offsetp)--;
		cmd = *lastcmd;
	}

	midi_op = cmd >> 4;
	midi_channel = cmd & 0xf;
	paramsleft = MIDI_cmdlen[midi_op];
#if 0
if (self->playmask == 0) {
	fprintf(stderr, "[IT]: off=%x, cmd=%02x, takes %d args ",
		(*offsetp) - 1, cmd, paramsleft);
	fprintf(stderr, "[%02x %02x <%02x> %02x %02x %02x]\n",
		self->data[(*offsetp)-3],
		self->data[(*offsetp)-2],
		self->data[(*offsetp)-1],
		self->data[(*offsetp)],
		self->data[(*offsetp)+1],
		self->data[(*offsetp)+2]);
}
#endif

	buf[0] = cmd;


	CHECK_FOR_END(paramsleft);
	memcpy(buf + 1, self->data + (*offsetp), paramsleft);
	*result = 1 + paramsleft;

	(*offsetp) += paramsleft;

	*lastcmd = cmd;

	/* Are we supposed to play this channel? */
	if (playmask_check &&
	    (
	    /* First, exclude "global" properties-- such as cues-- from consideration */
	     (midi_op < 0xf
	      && !(cmd == SCI_MIDI_SET_SIGNAL)
	      && !(SCI_MIDI_CONTROLLER(cmd)
		   && buf[1] == SCI_MIDI_CUMULATIVE_CUE))

	    /* Next, check if the channel is allowed */
	     && (self->playmask == PLAYMASK_NONE /* Any channels at all? */

		/* Rhythm channel? Abort if it's explicitly disabled */
		 || ((midi_channel == MIDI_RHYTHM_CHANNEL) && !self->play_rhythm)
		 /* Not rhythm channel? Abort if it's filtered out by the playmask */
		 || ((midi_channel != MIDI_RHYTHM_CHANNEL)
		     && !(self->data[2 + (midi_channel << 1)] & self->playmask)))
	     )
	    )
		return /* Execute next command */
			self->next((song_iterator_t *) self, buf, result);

	if (cmd == SCI_MIDI_EOT) {
		/* End of track? */
		if ((*loopsp) > 1) {
			*result = --(*loopsp);
			(*offsetp) = self->loop_offset;
			*lastcmd = 0;
			self->state = SI_STATE_COMMAND;
			return SI_LOOP;
		} else {
			(*loopsp) = 0;
			self->state = SI_STATE_FINISHED;
			return SI_FINISHED;
		}

	} else if (cmd == SCI_MIDI_SET_SIGNAL) {
		if (buf[1] == SCI_MIDI_SET_SIGNAL_LOOP) {
			self->loop_offset = (*offsetp) - 1 - paramsleft;
			return /* Execute next command */
				self->next((song_iterator_t *) self, buf, result);
		} else {
			/* Used to be conditional <= 127 */
			*result = buf[1]; /* Absolute cue */
			return SI_CUE;
		}
	} else if (SCI_MIDI_CONTROLLER(cmd)) {
		switch (buf[1]) {

		case SCI_MIDI_CUMULATIVE_CUE:
			self->ccc += buf[2];
			*result = self->ccc;
			return SI_CUE;

		case SCI_MIDI_RESET_ON_SUSPEND:
			self->resetflag = buf[2];
			break;

		case SCI_MIDI_SET_POLYPHONY:
			self->polyphony[midi_channel] = buf[2];
			break;

		case SCI_MIDI_SET_REVERB:
			self->reverb[midi_channel] = buf[2];
			break;

		case SCI_MIDI_SET_VELOCITY:
			self->velocity[midi_channel] = buf[2];
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

	} else
		/* Process as normal MIDI operation */
		return 0;
}

static int
_sci0_read_next_command(base_song_iterator_t *self, unsigned char *buf, int *result)
{
	CHECK_FOR_END(0);

	switch (self->state) {

	case SI_STATE_UNINITIALISED:
		fprintf(stderr, SIPFX "Attempt to read command from uninitialized iterator!\n");
		self->init((song_iterator_t *) self);
		return self->next((song_iterator_t *) self, buf, result);

	case SI_STATE_FINISHED:
		return SI_FINISHED;

	case SI_STATE_DELTA_TIME: {
		int offset;
		int ticks = _parse_ticks(self->data + self->offset,
					 &offset,
					 self->size - self->offset);

		self->offset += offset;
		CHECK_FOR_END(0);

		self->state = SI_STATE_COMMAND;

		if (ticks)
			return ticks;
	}

	  /* continute otherwise... */

	case SI_STATE_COMMAND:
		return _parse_sci_midi(self, buf, result,
				       &(self->offset), &(self->loops),
				       &(self->last_cmd), 1);

	default:
		fprintf(stderr, SIPFX "Invalid iterator state %d!\n", self->state);
		return SI_FINISHED;
	}
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
_sci0_check_pcm(base_song_iterator_t *self, int *size, sfx_pcm_config_t *format)
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
_base_handle_message(base_song_iterator_t *self, song_iterator_message_t msg)
{
	if (msg.recipient == _SIMSG_BASE) {
		switch (msg.type) {

		case _SIMSG_BASEMSG_SET_LOOPS:
			self->loops = msg.args[0];
			break;

		case _SIMSG_BASEMSG_CLONE: {
			int tsize = sizeof(base_song_iterator_t);
			base_song_iterator_t *mem = sci_malloc(sizeof(base_song_iterator_t));
			memcpy(mem, self, sizeof(base_song_iterator_t));
			mem->data = sci_malloc(mem->size);
			memcpy(mem->data, self->data, self->size);
			return (struct _song_iterator *) mem; /* Assume caller has another copy of this */
		}

		case _SIMSG_BASEMSG_SET_PLAYMASK:
			self->playmask = msg.args[0];
			break;

		case _SIMSG_BASEMSG_SET_RHYTHM:
			self->play_rhythm = msg.args[0];
			break;

		default:
			return NULL;
		}

		return (song_iterator_t *)self;
	}
	return NULL;
}

static void
_sci0_init(base_song_iterator_t *self)
{
	_common_init(self);

	self->offset = self->loop_offset = SCI0_MIDI_OFFSET;
	self->state = SI_STATE_DELTA_TIME;
	self->ccc = 127; /* Reset cumulative cue counter */
	self->playmask = PLAYMASK_NONE; /* All channels */
	self->play_rhythm = 0; /* Disable rhythm channel */
}

static void
_sci0_cleanup(base_song_iterator_t *self)
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

typedef struct _sci1_sample {
	int delta; /* Time left-- initially, this is 'Sample point 1'.
		   ** After initialisation, it is 'sample point 1 minus the sample point of the previous sample'  */
	int size;
	sfx_pcm_config_t format;
	byte *data;
	struct _sci1_sample *next;
} sci1_sample_t;

typedef struct  {
	BASE_SONG_ITERATOR_BODY;
	int channel_delay[MIDI_CHANNELS];	/* Number of ticks before the specified channel is next used */
	int channel_offset[MIDI_CHANNELS];      /* Offset into the data chunk */
	int channel_end[MIDI_CHANNELS];		/* Last allowed byte in track */
	int channel_id[MIDI_CHANNELS];		/* Actual channel number */
	byte channel_last_cmd[MIDI_CHANNELS];	/* Actual channel number */
	unsigned int channel_not_loop_waiting;  /* Bits for each channel are
						** set iff they're not waiting
						** for other channels to finish
						** looping  */

	/* Invariant: Whenever channel_delay[i] == 0, channel_offset[i] points
	** to a delta time object. */

	int initialised; /* Whether the MIDI channel setup has been initialised */
	int channels_nr; /* Number of channels actually used */
	sci1_sample_t *next_sample;
} sci1_song_iterator_t;

#define SCI1_CHECK_FOR_END(chan, offset)  \
	if (self->channel_offset[chan] + (offset) > self->channel_end[chan]) { \
		self->state = SI_STATE_FINISHED; \
		fprintf(stderr, SIPFX "Reached end of song without terminator!\n"); \
		return SI_FINISHED; \
	}

#define SCI1_CHANDATA(chan, offset) self->data[self->channel_offset[chan] + (offset)]
#define SCI1_NOT_LOOP_WAITING(chan) (self->channel_not_loop_waiting & (1 << (chan)))

static inline void
_sci1_set_channel_delay(sci1_song_iterator_t *self, int chan);

static inline void
_sci1_resume_all_channels_from_waiting_to_loop(sci1_song_iterator_t *self)
{
	int i;

	self->channel_not_loop_waiting = 0;
	for (i = 0; i < self->channels_nr; i++)
		self->channel_not_loop_waiting |= (1 << i);
}

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

	sample->format.format = SFX_PCM_FORMAT_U8;
	sample->format.stereo = SFX_PCM_MONO;
	sample->format.rate = rate;

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

	self->channels_nr = 0;
	self->offset = 0;
	self->next_sample = 0;

	CHECK_FOR_END(0);
	while (SONGDATA(0) != 0xff
	       && SONGDATA(0) != self->playmask) {
		self->offset++;
		CHECK_FOR_END(1);
		while (SONGDATA(0) != 0xff) {
			CHECK_FOR_END(7);
			self->offset += 6;
		}
		self->offset++;
	}

	if (SONGDATA(0) == 0xff) {
		sciprintf("[iterator-1] Song does not support"
			  " hardware 0x%02x\n",
			  self->playmask);
		return 1;
	}

	self->offset++;

	while (SONGDATA(0) != 0xff) { /* End of list? */
		int offset;
		int end;

		self->offset += 2;
		CHECK_FOR_END(4);

		offset = getInt16(self->data + self->offset);
		end = getInt16(self->data + self->offset + 2);

		CHECK_FOR_END_ABSOLUTE(offset - 1);

		if (self->data[offset] == 0xfe) {
			if (_sci1_sample_init(self, offset))
				return 1; /* Error */
		} else {
			/* Regular MIDI channel */
			if (self->channels_nr >= MIDI_CHANNELS) {
				sciprintf("[iterator-1] Warning: Song has more than %d channels, cutting them off\n",
					  MIDI_CHANNELS);
				break; /* Scan for remaining samples */
			} else {
				self->channel_offset[self->channels_nr]
					= offset + 2; /* Skip over header
						      ** bytes */
				self->channel_end[self->channels_nr]
					= offset + end;
				self->channel_delay[self->channels_nr]
					= 0; /* Start with delta time */
				self->polyphony[self->channels_nr]
					= SCI1_CHANDATA(self->channels_nr, -1);
				self->channel_id[self->channels_nr]
					= SCI1_CHANDATA(self->channels_nr, -2) & 0xf;
				_sci1_set_channel_delay(self,
							self->channels_nr);
				self->channel_last_cmd[self->channels_nr]
					= 0;


				self->channels_nr++;

				CHECK_FOR_END_ABSOLUTE(offset + end);
			}
		}
		self->offset += 4;
		CHECK_FOR_END(0);
	}

	sciprintf("[iterator-1] DEBUG: detected %d channels\n", self->channels_nr);
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

	while (seeker) {
		int prev_last_time = last_time;
		sciprintf("[iterator-1] Detected sample: %d Hz, %d bytes at time %d\n",
			  seeker->format.rate, seeker->size, seeker->delta);
		last_time = seeker->delta;
		seeker->delta -= prev_last_time;
		seeker = seeker->next;
	}

	_sci1_resume_all_channels_from_waiting_to_loop(self);

	return 0; /* Success */
}

static inline int
_sci1_get_smallest_delta(sci1_song_iterator_t *self)
{
	int i, d = -1;
	for (i = 0; i < self->channels_nr; i++)
		if (SCI1_NOT_LOOP_WAITING(i) &&
		    (d == -1 || self->channel_delay[i] < d))
			d = self->channel_delay[i];

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
		if (SCI1_NOT_LOOP_WAITING(i))
			if (0 == (self->channel_delay[i] -= delta)) {
				int offset;
				_parse_ticks(self->data + self->channel_offset[i],
					     &offset,
					     self->channel_end[i]
					     - self->channel_offset[i]);

				/* Advance over delta time */
				self->channel_offset[i] += offset;
			}
}

static inline int /* Determine the channel # of the next active event, or -1 */
_sci1_command_index(sci1_song_iterator_t *self)
{
	int i;
	for (i = 0; i < self->channels_nr; i++)
		if (SCI1_NOT_LOOP_WAITING(i) &&
		    !self->channel_delay[i])
			return i;
	return -1;
}

static inline void
_sci1_retire_channel(sci1_song_iterator_t *self, int chan)
{
	if (chan != --self->channels_nr) {
		self->channel_offset[chan]
			= self->channel_offset[self->channels_nr];
		self->channel_end[chan]
			= self->channel_end[self->channels_nr];
		self->channel_delay[chan]
			= self->channel_delay[self->channels_nr];
		self->channel_id[chan]
			= self->channel_id[self->channels_nr];
	}
}

static inline void
_sci1_set_channel_delay(sci1_song_iterator_t *self, int chan)
{
	self->channel_delay[chan] =
		_parse_ticks(self->data + self->channel_offset[chan],
			     NULL,
			     self->channel_end[chan]
			     - self->channel_offset[chan]);

	/* If we don't have to wait, skip over time
	** delta in order to ensure that the invariant
	** regarding channel_delay[i] == 0 holds  */
	if (self->channel_delay[chan] == 0)
		++self->channel_offset[chan];
}

static void
_sci1_dump_state(sci1_song_iterator_t *self)
{
	int i;
	return;
	sciprintf("-- [%p: %d] ------------------------\n", self, self->state);
	for (i = 0; i < self->channels_nr; i++) {
		int j;
		sciprintf("d-%d:\t(%d/%d)  ",
			  self->channel_delay[i],
			  self->channel_offset[i],
			  self->channel_end[i]);
		for (j = -3; j < 10; j++) {
			if (j == 0)
				sciprintf(">");
			else
				sciprintf(" ");

			sciprintf("%02x", self->data[self->channel_offset[i]+j]);

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
	    && self->next_sample->delta == 0
	    && self->state == SI_STATE_PCMWAIT) {
		sci1_sample_t *sample = self->next_sample;
		byte *retval = sample->data;
		self->next_sample = self->next_sample->next;

		if (format)
			*format = sample->format;
		if (size)
			*size = sample->size;
		sci_free(sample);

		self->state = SI_STATE_DELTA_TIME;
		return retval;
	} else
		return NULL;
}


static int
_sci1_read_next_command(sci1_song_iterator_t *self,
			unsigned char *buf, int *result)
{
/* 	sciprintf("[iterator-1] Called with (%d/%d)\n", */
/* 		  self->initialised, self->state); */
	if (!self->initialised) {
		sciprintf("[iterator-1] DEBUG: Initialising for %d\n",
			  self->playmask);
		self->initialised = 1;
		if (_sci1_song_init(self))
			return SI_FINISHED;
	}


	if (self->channels_nr == 0)
		self->state = SI_STATE_FINISHED; /* Nothing to play... */

	switch (self->state) {

	case SI_STATE_FINISHED:
		return SI_FINISHED;

	case SI_STATE_UNINITIALISED:
		fprintf(stderr, "[iterator-1] Fatal: Uninitialised use\n");
		exit(1);

	case SI_STATE_PCMWAIT:
		/* So someone doesn't like our PCM sample, eh? */
		sciprintf("[iterator-1] DEBUG: Skipped sample\n");
		_sci1_get_pcm(self, NULL, NULL); /* Discard the sample */

		/* Fall through */

	case SI_STATE_DELTA_TIME: {
		int delta = _sci1_get_smallest_delta(self);
		_sci1_dump_state(self);

		if (delta) {
			_sci1_update_delta(self, delta);
			self->state = SI_STATE_COMMAND;
			return delta;
		}
	}
		/* Otherwise, fall through */

	case SI_STATE_COMMAND: {
		int chan = _sci1_command_index(self);
		int run_result;
		int looping = self->loops? 3 : 0;

		if (self->next_sample && self->next_sample->delta == 0) {
			self->state = SI_STATE_PCMWAIT;
			return SI_PCM; /* Need to pick up that PCM before we go on */
		}

		if (chan == -1) {
			fprintf(stderr, "[iterator-1] Surprise: COMMAND"
				" state w/o an empty channel\n");
			self->state = SI_FINISHED;
			return SI_FINISHED;
		}

		run_result = _parse_sci_midi((base_song_iterator_t *) self,
					     buf, result,
					     &(self->channel_offset[chan]),
					     &looping,
					     /* Pass in a temporary variable--
					     ** SCI1 only specifies WHETHER
					     ** we loop at all  */
					     &(self->channel_last_cmd[chan]),
					     0);


		self->state = SI_STATE_DELTA_TIME;

		switch (run_result) {

		case SI_FINISHED:
			_sci1_retire_channel(self, chan);
			if (self->channels_nr == 0)
				return SI_FINISHED;
			else
				return _sci1_read_next_command(self,
							       buf, result);

		case SI_LOOP: {
			if (self->channel_not_loop_waiting)
				/* There are still channels
				** unprepared for the loop  */
				self->channel_not_loop_waiting &= ~(1 << chan);
			else
				_sci1_resume_all_channels_from_waiting_to_loop(self);
			fprintf(stderr, "LOOP for %d/%d -> %04x\n", chan,
				self->channels_nr, self->channel_not_loop_waiting);

			return SI_LOOP;
		}

		case SI_CUE: /* Let's hope that we only get these once */
			sciprintf("[iterator-1] DEBUG: Cue in %x\n", chan);
			_sci1_set_channel_delay(self, chan);
			return SI_CUE;

		case 0: /* Regular event */
			_sci1_set_channel_delay(self, chan);
			return 0;

		default:
			sciprintf("[iterator-1] Unexpected parse return %d\n",
				  run_result);
			break;

		}
		}
		/* Won't get here-- endless loop */

	default:
		fprintf(stderr, "[iterator-1] Fatal: Weird state\n");
		exit(1);
	}
}

static struct _song_iterator *
_sci1_handle_message(sci1_song_iterator_t *self,
		     song_iterator_message_t msg)
{
	if (msg.recipient == _SIMSG_BASE) { /* May extend this in the future */
		switch (msg.type) {

		case _SIMSG_BASEMSG_CLONE: {
			int tsize = sizeof(sci1_song_iterator_t);
			sci1_song_iterator_t *mem = sci_malloc(sizeof(sci1_song_iterator_t));
			sci1_sample_t **samplep;
/* 			fprintf(stderr, "Cloning %p -> %p (%d,n*%d)\n", self, mem, sizeof(sci1_song_iterator_t), sizeof(sci1_sample_t)); */

			memcpy(mem, self, sizeof(sci1_song_iterator_t));
			samplep = &(mem->next_sample);

			mem->data = sci_malloc(mem->size);
			memcpy(mem->data, self->data, self->size);

/* 			{ */
/* 				sci1_sample_t *seeker = self->next_sample; */
/* 				while (seeker) { */
/* 					fprintf(stderr, "next = (%p)%p\n", self, seeker); */
/* 					seeker = seeker->next; */
/* 				} */
/* 			} */

			/* Clone chain of samples */
			while (*samplep) {
				sci1_sample_t *newsample
					= sci_malloc(sizeof(sci1_sample_t));
/* 				fprintf(stderr, "copying (%p)%p -> (%p)%p\n", self, *samplep, mem, newsample); */
				memcpy(newsample, *samplep,
				       sizeof(sci1_sample_t));
				*samplep = newsample;
				samplep = &(newsample->next);
			}

			return (struct _song_iterator *) mem; /* Assume caller has another copy of this */
		}

		case _SIMSG_BASEMSG_SET_PLAYMASK:
			self->playmask
				= sci0_to_sci1_device_map
				[sci_ffs(msg.args[0] & 0xff) - 1]
				[sfx_pcm_available()]
				;

			if (self->playmask == 0xff) {
				sciprintf("[iterator-1] Warning: Device %d(%d) not supported",
					  msg.args[0] & 0xff, sfx_pcm_available());
			}

			break;

		default:
			_base_handle_message((base_song_iterator_t *)self,
					     msg);
		}
		return (song_iterator_t *) self;
	}
	return NULL;
}

static void
_sci1_init(sci1_song_iterator_t *self)
{
	_common_init((base_song_iterator_t *) self);
	self->ccc = 127;
	self->playmask = 0x12; /* Default to PC speaker for purposes
			       ** of cue computation  */
	self->play_rhythm = 0;
	self->next_sample = NULL;
	self->channels_nr = 0;
	self->initialised = 0;
	self->state = SI_STATE_DELTA_TIME;
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

	_sci0_cleanup((base_song_iterator_t *)it);
}



/*************************************/
/*-- General purpose functionality --*/
/*************************************/

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
		    it = sci_malloc(sizeof(base_song_iterator_t));

		    for (i = 0; i < MIDI_CHANNELS; i++) {
			    it->polyphony[i] = data[1 + (i << 1)];
			    it->flags[i] = data[2 + (i << 1)];
		    }
		    it->next = (int(*)(song_iterator_t *, unsigned char *, int *))
			    _sci0_read_next_command;
		    it->get_pcm = (unsigned char*(*)(song_iterator_t *, int*, int *, int *))
			    _sci0_check_pcm;
		    it->handle_message = (song_iterator_t *(*)(song_iterator_t *, song_iterator_message_t))
			    _base_handle_message;
		    it->init = (void(*)(song_iterator_t *))_sci0_init;
		    it->cleanup = (void(*)(song_iterator_t *))_sci0_cleanup;
		    break;

	    case SCI_SONG_ITERATOR_TYPE_SCI1:
		    /**-- SCI01 or later sound resource --**/
		    it = sci_malloc(sizeof(sci1_song_iterator_t));

		    for (i = 0; i < MIDI_CHANNELS; i++) {
			    it->polyphony[i] = data[1 + (i << 1)];
			    it->flags[i] = data[2 + (i << 1)];
		    }
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

	it->state = SI_STATE_UNINITIALISED;
	it->init((song_iterator_t *) it);

	return (song_iterator_t *) it;
}

void
songit_free(song_iterator_t *it)
{
	if (it) {
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
songit_clone(song_iterator_t *it)
{
	SIMSG_SEND(it, SIMSG_CLONE);
	return it;
}

