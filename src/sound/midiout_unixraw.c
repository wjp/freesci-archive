/***************************************************************************
 midiout_unixraw.c Copyright (C) 2000 Rickard Lind

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
#include <unistd.h>

#ifndef O_SYNC
#  define O_SYNC 0
#endif

static int fd;
static char *devicename = "/dev/midi";

static int unixraw_lastwrote = 0;

static int
midiout_unixraw_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	if (!strcasecmp(attribute, "device")) {
		devicename = value;
	} else
		sciprintf("Unknown unixraw option '%s'!\n", attribute);

	return 0;
}

int midiout_unixraw_open()
{
  if (!IS_VALID_FD(fd = open(devicename, O_WRONLY|O_SYNC))) {
    fprintf(stderr, "Open failed (%d): %s\n", fd, devicename);
    return -1;
  }
  return 0;
}

int midiout_unixraw_close()
{
  if (close(fd) < 0)
    return -1;
  return 0;
}

int midiout_unixraw_flush(guint8 code)
{
  /* opened with O_SYNC; already flushed.. */
  usleep (320 * unixraw_lastwrote);  /* delay to make sure all was written */
  return 0;
}

int midiout_unixraw_write(guint8 *buffer, unsigned int count, guint32 delta)
{
  int rval = 0;
  /*  printf("writing %d (%d)-- %02x %02x %02x\n", count, fd,
      buffer[0], buffer[1], buffer[2]); */
  rval = write(fd, buffer, count);
  if (rval != count) {
    printf("write error on fd %d: %d -- %d\n", fd, rval, errno);
    rval = -1;
  }
  unixraw_lastwrote = rval;
  return rval;
}

midiout_driver_t midiout_driver_unixraw = {
  "unixraw",
  "v0.01",
  &midiout_unixraw_set_parameter,
  &midiout_unixraw_open,
  &midiout_unixraw_close,
  &midiout_unixraw_write,
  &midiout_unixraw_flush
};
