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

midi_device_t *midi_devices[] = {
	&midi_device_mt32gm,
	&midi_device_mt32,
#ifdef HAVE_SYS_SOUNDCARD_H
	&midi_device_adlib,
#endif
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

int midi_noteoff(guint8 channel, guint8 note, guint8 velocity) {
	return midi_device->event(0x80 & channel, note, velocity);
}

int midi_noteon(guint8 channel, guint8 note, guint8 velocity) {
	return midi_device->event(0x90 & channel, note, velocity);
}

int midi_event(guint8 command, guint8 param, guint8 param2) {
	if (mididebug)
		printf("MIDI: Event	%02x %02x %02x\n", command, param, param2);
	return midi_device->event(command, param, param2);
}

int midi_event2(guint8 command, guint8 param) {
	if (mididebug)
		printf("MIDI: Event	%02x %02x\n", command, param);
	return midi_device->event2(command, param);
}

int midi_volume(guint8 volume) {
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

