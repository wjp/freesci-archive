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

#ifdef _WIN32

#include <windows.h>
#include <mmsystem.h>
#include <sci_memory.h>

HMIDIOUT devicename;				/* global handle to midiOut device */
MIDIHDR midioutput;					/* used to send midi data */
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
	sci_free(err);
	return -1;
}


static int
midiout_win32mci_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	if (NULL == value)
	{
		sciprintf("midiout_win32mci_set_parameter(): NULL passed for value.\n");
		return -1;
	}

	if (!strcasecmp(attribute, "device"))
	{
		devicenum = ((int)*value) - 48;
	}
	else
		sciprintf("Unknown win32mci option '%s'!\n", attribute);

	return 0;
}

int midiout_win32mci_open(void)
{
	int numdevs				= 0;	/* total number of MIDIout devices */
	MMRESULT ret;					/* return value of MCI calls */
	MIDIOUTCAPS devicecaps;			/* device capabilities structure */
	int loop				= 0;
	int device_score = 0;

	numdevs = midiOutGetNumDevs();

	if (numdevs == 0)
	{
		fprintf(stderr, "No win32 MCI MIDI output devices found!\n");
		return -1;
	}

	fprintf(stderr, "MCI MIDI output devices found: %d \n",numdevs);

	if (devicenum == -1)
		for (loop = 0; loop < numdevs; loop++)
		{
			ret = midiOutGetDevCaps(loop, &devicecaps, sizeof(devicecaps));
			if (MMSYSERR_NOERROR != ret)
			{
				fprintf(stderr, "midiOutGetDevCaps: ");
				return (_win32mci_print_error(ret));
			}

			fprintf(stderr, "MIDI Out %02d: ", loop);
			fprintf(stderr, "[%s]\n             ", devicecaps.szPname);

			/* which kind of device would be the best to default to? */
			/*   ignore hardware ports because they may not be connected
			**   midi mapper is what is selected in control panel and should be first preference
			**   software synths will take a lot of cpu time so they should be lower
			**   1. MOD_MAPPER
			**   2. MOD_WAVETABLE
			**   3. MOD_SQSYNTH
			**   4. MOD_SYNTH
			**   5. MOD_FMSYNTH
			**   6. MOD_SWSYNTH
			**   7. MOD_MIDIPORT
			*/
			switch (devicecaps.wTechnology)
			{
				case MOD_MAPPER:
					fprintf(stderr, "MIDI mapper, ");
					devicenum = loop;
					device_score = 7;
					break;

#ifdef MOD_WAVETABLE
				case MOD_WAVETABLE:
					fprintf(stderr, "Hardware wavetable synth, ");
					if (device_score < 6)
					{
						devicenum = loop;
						device_score = 6;
					}
					break;
#endif

				case MOD_SQSYNTH:
					fprintf(stderr, "Square wave synth, ");
					if (device_score < 5)
					{
						devicenum = loop;
						device_score = 5;
					}
					break;

				case MOD_SYNTH:
					fprintf(stderr, "Generic synth, ");
					if (device_score < 4)
					{
						devicenum = loop;
						device_score = 4;
					}
					break;

				case MOD_FMSYNTH:
					fprintf(stderr, "FM synth, ");
					if (device_score < 3)
					{
						devicenum = loop;
						device_score = 3;
					}
					break;

#ifdef MOD_SWSYNTH
				case MOD_SWSYNTH:
					fprintf(stderr, "Software synth, ");
					if (device_score < 2)
					{
						devicenum = loop;
						device_score = 2;
					}
					break;
#endif

				case MOD_MIDIPORT:
					fprintf(stderr, "MIDI hardware port, ");
					if (device_score < 1)
					{
						devicenum = loop;
						device_score = 1;
					}
					break;

				default:
					fprintf(stderr, "Unknown synth, ");
					if (devicenum == -1)
					{
						devicenum = loop;
						device_score = 0;
					}
					break;
			}

		fprintf(stderr, "%d voices\n", devicecaps.wVoices);

	}

	ret = midiOutOpen(&devicename, devicenum, 0, 0, CALLBACK_NULL);
	if (MMSYSERR_NOERROR != ret)
	{
		fprintf(stderr, "midiOutOpen of device #%02d: ",devicenum);
		return (_win32mci_print_error(ret));
	}
	else
	{
		fprintf(stderr, "Successfully opened MCI MIDI device #%02d\n",devicenum);
	}

	/* set up midihdr struct */
	memset(&midioutput, 0, sizeof(midioutput));
	midioutput.dwBytesRecorded	= 1;	/* number of events that will be sent */

	return 0;
}

int midiout_win32mci_close(void)
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

int midiout_win32mci_flush(void)
{
	Sleep(win32mci_lastwrote);
	return 0;
}

int midiout_win32mci_write(guint8 *buffer, unsigned int count)
{
	MMRESULT ret;
	unsigned int midioutputsize;

	/* first, we populate the fields of the MIDIHDR */
	midioutput.lpData			= buffer;		/* pointer to a MIDI event stream */
	midioutput.dwBufferLength	= count;		/* size of buffer */

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
  "0.1",
  &midiout_win32mci_set_parameter,
  &midiout_win32mci_open,
  &midiout_win32mci_close,
  &midiout_win32mci_write,
  &midiout_win32mci_flush
};
#endif
