#include "fake_glib.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "midiout_unixraw.h"

static int fd;

int midiout_unixraw_open(char *devicename)
{
  if ((fd = open(devicename, O_WRONLY|O_SYNC)) < 0) {
    fprintf(stderr, "Open failed (%i): %s\n", fd, devicename);
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

int midiout_unixraw_write(guint8 *buffer, unsigned int count)
{
  if (write(fd, buffer, count) != count)
    return -1;
  return 0;
}
