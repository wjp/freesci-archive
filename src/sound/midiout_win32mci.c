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

static int fd;
static char *devicename = "/dev/midi00";

static int win32mci_lastwrote = 0;

static void
Win32_usleep (long usec)
{
	LARGE_INTEGER lFrequency;
	LARGE_INTEGER lEndTime;
	LARGE_INTEGER lCurTime;

	QueryPerformanceFrequency (&lFrequency);
	if (lFrequency.QuadPart)
	{
		QueryPerformanceCounter (&lEndTime);
		lEndTime.QuadPart += (LONGLONG) usec * lFrequency.QuadPart / 1000000;
		do
		{
			QueryPerformanceCounter (&lCurTime);
		} while (lCurTime.QuadPart < lEndTime.QuadPart);
	}
}

static int
midiout_win32mci_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	if (!strcasecmp(attribute, "device")) {
		devicename = value;
	} else
		sciprintf("Unknown win32mci option '%s'!\n", attribute);

	return 0;
}

int midiout_win32mci_open()
{
	int numdevs		= 0;
	
	numdevs = midiOutGetNumDevs();

	if (numdevs == 0)
	{
		fprintf(stderr, "No win32 MCI MIDI output devices found!");
		return -1;
	}

	fprintf(stderr, "%d MCI MIDI output devices found");
	
	if ((fd = open(devicename, O_WRONLY|O_SYNC)) < 0) {
	    fprintf(stderr, "Open failed (%d): %s\n", fd, devicename);
	    return -1;
	}
	return 0;
}

int midiout_win32mci_close()
{
  if (close(fd) < 0)
    return -1;
  return 0;
}

int midiout_win32mci_flush()
{
  /* opened with O_SYNC; already flushed.. */
  Win32_usleep (320 * win32mci_lastwrote);  /* delay to make sure all was written */
  return 0;
}

int midiout_win32mci_write(guint8 *buffer, unsigned int count)
{
  int rval = 0;
  /*  printf("writing %d (%d)-- %02x %02x %02x\n", count, fd, 
      buffer[0], buffer[1], buffer[2]); */
  rval = write(fd, buffer, count);
  if (rval != count) {
    printf("write error on fd %d: %d -- %d\n", fd, rval, errno);
    rval = -1;
  }
  win32mci_lastwrote = rval;
  return rval;
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
