/***************************************************************************
 midiout_fsynth.c Copyright (C) 2003 Walter van Niftrik

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

   Walter van Niftrik <w.f.b.w.v.niftrik@stud.tue.nl>

***************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_FLUIDSYNTH_H

#include <midiout.h>
#include <pcmout.h>
#include <fluidsynth.h>

static fluid_settings_t* settings;
static fluid_synth_t* synth;
static guint8 status;
static char *soundfont = "/etc/midi/8MBGMSFX.SF2";

static int
synth_mixer(gint16 *buffer, int count)
{
	if (synth) {
		fluid_synth_write_s16(synth, count, buffer, 0, 2, buffer+1, 0, 2);
		return count;
	}
	else return 0;
}

static int
midiout_fluidsynth_open()
{
	int sfont_id;
	double min, max;

	if (pcmout_stereo == 0) {
		sciprintf("FluidSynth ERROR: Mono sound output not supported.\n");
		return -1;
	}

	settings = new_fluid_settings();

	fluid_settings_getnum_range(settings, "synth.sample-rate", &min, &max);
	if (pcmout_sample_rate < min || pcmout_sample_rate > max) {
		sciprintf("FluidSynth ERROR: Sample rate '%i' not supported. Valid "
			"range is (%i-%i).\n", pcmout_sample_rate, (int) min, (int) max);
		delete_fluid_settings(settings);
		return -1;
	}

	fluid_settings_setnum(settings, "synth.sample-rate", pcmout_sample_rate);
	fluid_settings_setnum(settings, "synth.gain", 2.0f);

	synth = new_fluid_synth(settings);

	if ((sfont_id = fluid_synth_sfload(synth, soundfont, 1)) < 0) {
		delete_fluid_synth(synth);
		delete_fluid_settings(settings);
		return -1;
	}

	pcmout_set_mixer(synth_mixer);

	return 0;
}

static int
midiout_fluidsynth_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	char *testptr;

	if (!strcasecmp(attribute, "soundfont"))
		soundfont = value;
	else
		sciprintf("FluidSynth WARNING: Option '%s' does not exist.\n", attribute);

	return 0;
}

static int
midiout_fluidsynth_close()
{
	delete_fluid_synth(synth);
	delete_fluid_settings(settings);
	return 0;
}

static int
midiout_fluidsynth_flush(guint8 code)
{
	return 0;
}

static int
midiout_fluidsynth_write(guint8 *buffer, unsigned int count, guint32 delta)
{
	guint8 command, channel, param1, param2;

	if (count < 1) return -1;

	if (buffer[0] >= 0x80) {
		status = buffer[0];
		buffer++;
		count--;
	}

	command = status & 0xf0;
	channel = status & 0x0f;

	if (count < 1) return -1;
	param1 = buffer[0];
	if (count > 1) param2 = buffer[1];

	switch(command) {
	case 0x80:
		fluid_synth_noteoff(synth, channel, param1);
		break;
	case 0x90:
		fluid_synth_noteon(synth, channel, param1, param2);
		break;
	case 0xb0:
		fluid_synth_cc(synth, channel, param1, param2);
		break;
	case 0xc0:
		fluid_synth_program_change(synth, channel, param1);
		break;
	case 0xe0:
		fluid_synth_pitch_bend(synth, channel, (param2 << 7) | param1);
		break;
	default:
		printf("FluidSynth WARNING: Unsupported midi command 0x%02x\n", command);
	}
	return count;
}

midiout_driver_t
midiout_driver_fluidsynth = {
	"fluidsynth",
	"v0.01",
	&midiout_fluidsynth_set_parameter,
	&midiout_fluidsynth_open,
	&midiout_fluidsynth_close,
	&midiout_fluidsynth_write,
	&midiout_fluidsynth_flush
};

#endif /* HAVE_FLUIDSYNTH_H */

