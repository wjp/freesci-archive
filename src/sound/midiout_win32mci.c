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
int devicenum			= -1;		/* device number */


static unsigned int win32mci_lastwrote = 0;


int _win32mci_print_error(int ret)
{
	char *err	= NULL;
	err = sci_malloc(MAXERRORLENGTH);

	if (NULL == err)
	{
		fprintf(stderr, "midiout_win32mci.c: out of memory!\n");
		return -1;
	}

	midiOutGetErrorText(ret, err, MAXERRORLENGTH);
	fprintf(stderr, "%s\n", err);
	free(err);
	return -1;
}


static int
midiout_win32mci_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	if (!strcasecmp(attribute, "device"))
	{
		devicenum = ((int)*value) - 48;
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
			fprintf(stderr, "midiOutGetDevCaps: ");
			return (_win32mci_print_error(ret));
		}

		fprintf(stderr, "MCI Device %d: ",loop);
		fprintf(stderr, "%s, ", devicecaps.szPname);
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

	if (-1 == devicenum)
	{
		devicenum = numdevs - 1;
	}

	ret = midiOutOpen(&devicename, devicenum, 0, 0, CALLBACK_NULL);
	if (MMSYSERR_NOERROR != ret)
	{
		fprintf(stderr, "midiOutOpen of device #%d: ",devicenum);
		return (_win32mci_print_error(ret));
	}
		else
		{
			fprintf(stderr, "Successfully opened MCI MIDI device #%d\n",devicenum);
		}

	return 0;
}

int midiout_win32mci_close()
{
	MMRESULT ret;

	ret = midiOutClose(devicename);
	if (MMSYSERR_NOERROR != ret)
	{
		fprintf(stderr, "midiOutClose: ");
		return(_win32mci_print_error(ret));
	}

	return 0;
}

int midiout_win32mci_flush()
{
	Sleep(win32mci_lastwrote);
	return 0;
}

int midiout_win32mci_write(guint8 *buffer, unsigned int count)
{
	MMRESULT ret;
	unsigned int midioutputsize = 0;

	/* first, we populate the fields of the MIDIHDR */
	midioutput.lpData			= buffer;		/* pointer to a MIDI event stream */
	midioutput.dwBufferLength	= count;			/* size of buffer */
	midioutput.dwBytesRecorded	= 1;		/* actual number of events */
	midioutput.dwFlags			= 0;			/* MSDN sez to init this to zero */

	midioutputsize = sizeof(midioutput);

	/* then we pass the MIDIHDR here to have the rest of it initialized */
	ret = midiOutPrepareHeader(devicename, &midioutput, midioutputsize);
	if (ret != MMSYSERR_NOERROR)
	{
		fprintf(stderr, "midiOutPrepareHeader: ");
		return(_win32mci_print_error(ret));
	}

	/* then we send MIDIHDR to be played */
	ret = midiOutLongMsg(devicename, &midioutput, midioutputsize);
	if (ret != MMSYSERR_NOERROR)
	{
		fprintf(stderr, "midiOutLongMsg: ");
		return(_win32mci_print_error(ret));
	}

	/* things can't get freed before they're done playing */
	ret = midiOutUnprepareHeader(devicename, &midioutput, midioutputsize);
	if (ret != MMSYSERR_NOERROR)
	{
		/* if it fails, it's probably because it's still playing */
		Sleep(count);
		ret = midiOutUnprepareHeader(devicename, &midioutput, midioutputsize);
		/* if it still fails, it's probably something else */
		if (ret != MMSYSERR_NOERROR)
		{
			printf("midiOutUnprepareHeader() failed: ");
			return(_win32mci_print_error(ret));
		}
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
