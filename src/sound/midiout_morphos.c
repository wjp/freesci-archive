/***************************************************************************
 midiout_morphos.c Copyright (C) 2002 Ruediger Hanke


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

#include <midiout.h>

#ifdef __MORPHOS__

#include <exec/devices.h>
#include <exec/io.h>
#include <devices/etude.h>

#include <proto/exec.h>
#include <proto/etude.h>

static int midi_unit = 0;
static struct IOMidiRequest *midi_request = NULL;
static struct MsgPort *midi_port = NULL;
static struct Library *EtudeBase = NULL;

static int
midiout_etude_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	if (!strcasecmp(attribute, "unit")) {
		midi_unit = atoi(value);
	} else
		sciprintf("Unknown Etude option '%s'!\n", attribute);

	return 0;
}

int midiout_etude_open()
{
	midi_port = CreateMsgPort();
	if (midi_port)
	{
		midi_request = (struct IOMidiRequest *) CreateIORequest(midi_port, sizeof (struct IOMidiRequest));
		if (midi_request)
		{
			midi_request->emr_Version = ETUDE_CURRENT_VERSION;
			if (OpenDevice(ETUDENAME, midi_unit, (struct IORequest *) midi_request, ETUDEF_DIRECT))
			{
				DeleteIORequest((struct IORequest *) midi_request);
				DeleteMsgPort(midi_port);
				midi_request = NULL;
				midi_port = NULL;
			}
			else
				EtudeBase = (struct Library *)midi_request->emr_Std.io_Device;
		}
		else
		{
			DeleteMsgPort(midi_port);
			midi_port = NULL;
		}
	}

	if (!midi_request)
	{
		fprintf(stderr, "Could not open Etude - music will not play\n");
		return -1;
	}
	
	return 0;
}

int midiout_etude_close()
{
	if (midi_request)
	{
		CloseDevice((struct IORequest *) midi_request);
		DeleteIORequest((struct IORequest *) midi_request);
		DeleteMsgPort(midi_port);
		midi_request = NULL;
		midi_port = NULL;
	}
	return 0;
}

int midiout_etude_flush(guint8 code)
{
	return 0;
}

int midiout_etude_write(guint8 *buffer, unsigned int count, guint32 other_data)
{
	/*printf("writing %d -- %02x %02x %02x\n", count,
		buffer[0], buffer[1], buffer[2]);*/
	return SendLongMidiMsg(midi_request, buffer);
}

midiout_driver_t midiout_driver_morphos = {
  "etude",
  "v0.2",
  &midiout_etude_set_parameter,
  &midiout_etude_open,
  &midiout_etude_close,
  &midiout_etude_write,
  &midiout_etude_flush
};

#endif
