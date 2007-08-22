/***************************************************************************
 midi_device.c Copyright (C) 2001 Solomon Peachy

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

***************************************************************************/

#include <midi_device.h>

midi_device_t *midi_device;

midi_device_t midi_device_null;

midi_device_t *midi_devices[] = {
	&midi_device_adlibemu,
	&midi_device_mt32gm,
	&midi_device_mt32,
#ifdef HAVE_SYS_SOUNDCARD_H
	&midi_device_adlib,
#endif
	&midi_device_null,
	NULL
};

int mididebug = 0;

int midi_open (guint8 *data_ptr, unsigned int data_length) {
	if (mididebug)
		printf("MIDI: Open\n");
	return midi_device->open(data_ptr, data_length);
}

int midi_close (void) {
	if (mididebug)
		printf("MIDI: Close\n");
	return midi_device->close();
}

/*
int midi_noteoff(guint8 channel, guint8 note, guint8 velocity) {
	return midi_device->event(0x80 & channel, note, velocity);
}

int midi_noteon(guint8 channel, guint8 note, guint8 velocity) {
	return midi_device->event(0x90 & channel, note, velocity);
}
*/

int midi_event(guint8 command, guint8 param, guint8 param2, guint32 other_data) {
	if (mididebug)
		printf("MIDI: Event	%02x %02x %02x\n", command, param, param2);
	return midi_device->event(command, param, param2, other_data);
}

int midi_event2(guint8 command, guint8 param, guint32 other_data) {
	if (mididebug)
		printf("MIDI: Event	%02x %02x\n", command, param);
	return midi_device->event2(command, param, other_data);
}

int midi_volume(guint8 volume) {
        if (! midi_device->volume)
                return -1;
	if (mididebug)
		printf("MIDI: Set volume to: %d\n", volume);
	return midi_device->volume(volume);
}

int midi_allstop(void) {
	if (mididebug)
		printf("MIDI: All notes off\n");
	return midi_device->allstop();
}

int midi_reverb(short param) {
	return midi_device->reverb(param);
}

/* now for the setup commands */
struct _midi_device *midi_find_device(char *name)
{
	int retval = 0;

	if (!name) { /* Find default device */
		return midi_devices[0];
	}

	while (midi_devices[retval] &&
	 strcasecmp(name, midi_devices[retval]->name))
		retval++;

	return midi_devices[retval];
}

/* Common stuff for the adlib code. */

adlib_instr adlib_sbi[96];

void make_sbi(adlib_def *one, guint8 *buffer)
{
  memset(buffer, 0, sizeof(adlib_instr));

#if 0
 printf ("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ", one->keyscale1, one->freqmod1, one->feedback1, one->attackrate1, one->sustainvol1, one->envelope1, one->decayrate1, one->releaserate1, one->volume1, one->ampmod1, one->vibrato1, one->keybdscale1, one->algorithm1, one->waveform1);

  printf (" %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ", one->keyscale2, one->freqmod2, one->feedback2, one->attackrate2, one->sustainvol2, one->envelope2, one->decayrate2, one->releaserate2, one->volume2, one->ampmod2, one->vibrato2, one->keybdscale2, one->algorithm2, one->waveform2);

  printf("\n");
#endif

  buffer[0] |= ((one->ampmod1 & 0x1) << 7);
  buffer[0] |= ((one->vibrato1 & 0x1) << 6);
  buffer[0] |= ((one->envelope1 & 0x1) << 5);
  buffer[0] |= ((one->keybdscale1 & 0x1) << 4);
  buffer[0] |= (one->freqmod1 & 0xf);
  buffer[1] |= ((one->ampmod2 & 0x1) << 7);
  buffer[1] |= ((one->vibrato2 & 0x1) << 6);
  buffer[1] |= ((one->envelope2 & 0x1) << 5);
  buffer[1] |= ((one->keybdscale2 & 0x1) << 4);
  buffer[1] |= (one->freqmod2 & 0xf);
  buffer[2] |= ((one->keyscale1 & 0x3) << 6);
  buffer[2] |= (one->volume1 & 0x3f);
  buffer[3] |= ((one->keyscale2 & 0x3) << 6);
  buffer[3] |= (one->volume2 & 0x3f);
  buffer[4] |= ((one->attackrate1 & 0xf) << 4);
  buffer[4] |= (one->decayrate1 & 0xf);
  buffer[5] |= ((one->attackrate2 & 0xf) << 4);
  buffer[5] |= (one->decayrate2 & 0xf);
  buffer[6] |= ((one->sustainvol1 & 0xf) << 4);
  buffer[6] |= (one->releaserate1 & 0xf);
  buffer[7] |= ((one->sustainvol2 & 0xf) << 4);
  buffer[7] |= (one->releaserate2 & 0xf);
  buffer[8] |= (one->waveform1 & 0x3);
  buffer[9] |= (one->waveform2 & 0x3);

  buffer[10] |= ((one->feedback1 & 0x7) << 1);
  buffer[10] |= (1-(one->algorithm1 & 0x1));

  return;
}


/** NULL MIDI device **/

_midi_device_null_open(guint8 *_0, unsigned int _1) { return 0; }
_midi_device_null_close() { return 0; }
_midi_device_null_event(guint8 _0, guint8 _1, guint8 _2, guint32 _3) { return 0; }
_midi_device_null_event2(guint8 _0, guint8 _1, guint32 _2) { return 0; }
_midi_device_null_allstop() { return 0; }
_midi_device_null_reverb(short _) { return 0; }

midi_device_t midi_device_null = {
	"null",
	"0.1",
	_midi_device_null_open,
	_midi_device_null_close,
	_midi_device_null_event,
	_midi_device_null_event2,
	_midi_device_null_allstop,
	NULL,
	_midi_device_null_reverb,
	1,
	0,
	0,
	1
};
