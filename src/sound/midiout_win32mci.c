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

***************************************************************************/

#include <midiout.h>

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <windows.h>
#include <mmsystem.h>

#ifndef O_SYNC
#  define O_SYNC 0
#endif

/* static int fd; */
LPHMIDIOUT devicename;

static int win32mci_lastwrote = 0;

static int
midiout_win32mci_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	if (!strcasecmp(attribute, "device")) 
	{
		devicename = value;
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
			fprintf(stderr, "Something went wrong querying device %d: %s\n",
				loop, ret);
			exit;
		}

		fprintf(stderr, "MCI Device %d: ",loop);
		fprintf(stderr, "%s,", devicecaps.szPname);
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
	
	ret = midiOutOpen(&devicename, 1, NULL, NULL, NULL);
	if (MMSYSERR_NOERROR != ret)
	{
		fprintf(stderr, "Something went wrong opening device %d: %s\n",
			loop, ret);
		exit;
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
		fprintf(stderr, "Something went wrong closing the MIDI output device!: %s\n", ret);
		return -1;
	}

	return 0;
}

int midiout_win32mci_flush()
{
  /* opened with O_SYNC; already flushed.. */
  usleep (320 * win32mci_lastwrote);  /* delay to make sure all was written */
  return 0;
}

int midiout_win32mci_write(guint8 *buffer, unsigned int count)
{
	unsigned int loop = 0;
	MMRESULT ret;

	for (loop = 0; loop < count; loop++)
	{
		ret = midiOutShortMsg(devicename, buffer[loop]);
		if (ret != MMSYSERR_NOERROR) 
		{
			printf("write error on MIDI device: ");
			switch(ret)
			{
				case MIDIERR_BADOPENMODE: printf("MIDIERR_BADOPENMODE\n");
				case MIDIERR_NOTREADY: printf("MIDIERR_NOTREADY\n");
				case MMSYSERR_INVALHANDLE: printf("MMSYSERR_INVALHANDLE\n");
				default: printf("Unknown error!\n");
			}
			return -1;
		}

		printf("writing %d (%d)-- %02x %02x %02x\n", loop, count, 
			buffer[0], buffer[1], buffer[2]); 
		
	}
  
  win32mci_lastwrote = loop;
  
  return loop;
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
