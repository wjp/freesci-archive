/***************************************************************************
 midiout_win32mci.c Copyright (C) 2000 Rickard Lind


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

 Current maintainer: Matt Hargett <matt@use.net>

***************************************************************************/


/* NOTE: See http://msdn.microsoft.com for documentation on the midiOut* API
	calls used in this driver. */

#include <midiout.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <windows.h>
#include <mmsystem.h>

HMIDIOUT devicename;				/* global handle to midiOut device */
MIDIHDR midioutput;					/* used by midiOut* */

static int win32mci_lastwrote = 0;

static int
midiout_win32mci_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	if (!strcasecmp(attribute, "device")) 
	{
/*		TODO: user should be able to specify MCI device number */
	} 
	else
		sciprintf("Unknown win32mci option '%s'!\n", attribute);

	return 0;
}

int midiout_win32mci_open()
{
	int numdevs				= 0;	/* total number of MIDIout devices */
	MMRESULT ret;					/* return value of MCI calls */
	MIDIOUTCAPS devicecaps;			/* device capabilities structure */
	int loop				= 0;
	
	numdevs = midiOutGetNumDevs();

	if (numdevs == 0)
	{
		fprintf(stderr, "No win32 MCI MIDI output devices found!\n");
		return -1;
	}

	fprintf(stderr, "MCI MIDI output devices found: %d \n",numdevs);


	for (loop = 0; loop < numdevs; loop++)
	{
		ret = midiOutGetDevCaps(loop, &devicecaps, sizeof(devicecaps));
		if (MMSYSERR_NOERROR != ret)
		{
			fprintf(stderr, "Something went wrong querying device %d: \n",
				loop);
			return -1;
		}

		fprintf(stderr, "MCI Device %d: ",loop);
		fprintf(stderr, "%s,", devicecaps.szPname);
		/* which kind of device would be the best to default to? */
		switch (devicecaps.wTechnology)
		{
			case MOD_FMSYNTH: fprintf(stderr, "FM synth, "); break;
			case MOD_MAPPER: fprintf(stderr, "MIDI mapper, "); break;
			case MOD_WAVETABLE: fprintf(stderr, "Wavetable synth, "); break;
			case MOD_MIDIPORT: fprintf(stderr, "MIDI port, "); break;
			case MOD_SYNTH: fprintf(stderr, "Generic synth, "); break;
			default: fprintf(stderr, "Unknown synth, "); break;
		}

		fprintf(stderr, "%d voices\n", devicecaps.wVoices);

	}

	/* TODO: use user specified device number for parameter 2 */
	ret = midiOutOpen(&devicename, 1, 0, 0, CALLBACK_NULL);
	if (MMSYSERR_NOERROR != ret)
	{
		fprintf(stderr, "Something went wrong opening device %d: \n",
			loop);
		return -1;
	}
		else
		{
			fprintf(stderr, "Successfully opened MCI MIDI device\n");
		}

	return 0;
}

int midiout_win32mci_close()
{
	MMRESULT ret;

	ret = midiOutClose(devicename);
	if (MMSYSERR_NOERROR != ret)
	{
		fprintf(stderr, "Something went wrong closing the MIDI output device!\n");
		return -1;
	}

	return 0;
}

int midiout_win32mci_flush()
{
	/* usleep (win32mci_lastwrote);  /* delay to make sure all was written */

	return 0;
}

int midiout_win32mci_write(guint8 *buffer, unsigned int count)
{
	MMRESULT ret;

	/* first, we populate the fields of the MIDIHDR */
	midioutput.lpData			= buffer;	
	midioutput.dwBufferLength	= sizeof(buffer);
	midioutput.dwBytesRecorded	= count + 1;	/* WTF is count? */
	midioutput.dwFlags			= 0;			/* MSDN sez to init this to zero */

	/* then we pass the MIDIHDR here to have the rest of it initialized */
	ret = midiOutPrepareHeader(devicename, &midioutput, sizeof(midioutput));
	if (ret != MMSYSERR_NOERROR) 
	{
		printf("midiOutPrepareHeader() failed: ");
		switch(ret)
		{
			case MMSYSERR_NOMEM: fprintf(stderr, "MMSYSERR_NOMEM\n"); break;
			case MMSYSERR_INVALHANDLE: fprintf(stderr, "MMSYSERR_INVALHANDLE\n"); break;
			case MMSYSERR_INVALPARAM: fprintf(stderr, "MMSYSERR_INVALPARAM\n"); break;
			default: printf("Unknown error!\n"); break;
		}
		return -1;
	}
	
	/* then we send MIDIHDR to be played */
	ret = midiOutLongMsg(devicename, &midioutput, sizeof(midioutput));
	if (ret != MMSYSERR_NOERROR) 
	{
		printf("write error on MIDI device: ");
		switch(ret)
		{
			case MIDIERR_NOTREADY: fprintf(stderr, "MIDIERR_NOTREADY\n"); break;
			case MIDIERR_UNPREPARED: fprintf(stderr, "MIDIERR_UNPREPARED\n"); break;
			case MMSYSERR_INVALHANDLE: fprintf(stderr, "MMSYSERR_INVALHANDLE\n"); break;
			case MMSYSERR_INVALPARAM: fprintf(stderr, "MMSYSERR_INVALPARAM\n"); break;
			default: printf("Unknown error!\n"); break;
		}
		return -1;
	}

	/* then we free/flush the MIDIHDR */
	/* TODO: this can't happen right after, things get cleared before they're done */
	ret = midiOutUnprepareHeader(devicename, &midioutput, sizeof(midioutput));
	if (ret != MMSYSERR_NOERROR) 
	{
		printf("midiOutUnprepareHeader() failed: ");
		switch(ret)
		{
			case MIDIERR_STILLPLAYING: fprintf(stderr, "MIDIERR_STILLPLAYING\n"); break;
			case MMSYSERR_INVALHANDLE: fprintf(stderr, "MMSYSERR_INVALHANDLE\n"); break;
			case MMSYSERR_INVALPARAM: fprintf(stderr, "MMSYSERR_INVALPARAM\n"); break;
			default: printf("Unknown error!\n"); break;
		}
		return -1;
	}

  win32mci_lastwrote = count;

  return count;
}

midiout_driver_t midiout_driver_win32mci = {
  "win32mci",
  "v0.01",
  &midiout_win32mci_set_parameter,
  &midiout_win32mci_open,
  &midiout_win32mci_close,
  &midiout_win32mci_write,
  &midiout_win32mci_flush
};
