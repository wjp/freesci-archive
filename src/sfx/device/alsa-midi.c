/***************************************************************************
 alsa-midi.c  Copyright (C) 2002 Christoph Reichenbach


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

#include <sfx_engine.h>
#include <sfx_device.h>
#ifdef HAVE_ALSA

#include <alsa/asoundlib.h>

static snd_midi_event_t *parser = NULL;
static snd_seq_t *seq = NULL;
static int queue = -1;
static int delta = 0;
static int port_out = -1;

static char *seq_name = "default";

static void
_set_tempo()
{
	int tempo = 1000000 / 60;
	snd_seq_queue_tempo_t *queue_tempo;

	snd_seq_queue_tempo_malloc(&queue_tempo);
	snd_seq_queue_tempo_set_tempo(queue_tempo, tempo);
	snd_seq_queue_tempo_set_ppq(queue_tempo, 1);
	snd_seq_set_queue_tempo(seq, queue, queue_tempo);
	snd_seq_queue_tempo_free(queue_tempo);
}

static int
aminit(midi_writer_t *self)
{
	snd_midi_event_new(4096, &parser);
	snd_midi_event_init(parser);

	if (snd_seq_open(&seq, seq_name, SND_SEQ_OPEN_OUTPUT, SND_SEQ_NONBLOCK)) {
		fprintf(stderr, "[SFX] Failed to open ALSA MIDI sequencer '%s' for output\n",
			seq_name);
		return SFX_ERROR;
	}

	if ((port_out = snd_seq_create_simple_port(seq, "FreeSCI",
						      SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
						      SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
		fprintf(stderr, "[SFX] Could not create ALSA sequencer port\n");
		return SFX_ERROR;
	}

	queue = snd_seq_alloc_queue(seq);
	_set_tempo();
	snd_seq_start_queue(seq, queue, NULL);

	return SFX_OK;
}

static int
amsetopt(midi_writer_t *self, char *name, char *value)
{
	return SFX_ERROR;
}


static int
amwrite(midi_writer_t *self, char *buf, int len)
{
	snd_seq_event_t evt;

	snd_midi_event_encode(parser, buf, len, &evt);
	snd_seq_ev_schedule_tick(&evt, queue,  1, delta);
	snd_seq_ev_set_source(&evt, port_out);
	snd_seq_event_output(seq, &evt);

	return SFX_OK;
}

static void
amdelay(midi_writer_t *self, int ticks)
{
	delta += ticks;
}

static void
amflush(midi_writer_t *self)
{
	snd_seq_drain_output(seq);
}

static void
amclose(midi_writer_t *self)
{
	snd_midi_event_free(parser);
	parser = NULL;
}


midi_writer_t sfx_device_midi_alsa = {
	"alsa",
	aminit,
	amsetopt,
	amwrite,
	amdelay,
	amflush,
	amclose,
};

#endif
