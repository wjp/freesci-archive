#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "midi_mt32.h"

int main()
{
  int fd;
  unsigned char *patch;
  unsigned int length;

  patch = (unsigned char *)malloc(65536);

  fd = open("patch.001", O_RDONLY);
  length = read(fd, patch, 65536);
  close(fd);

  if (patch[0] == 0x89 && patch[1] == 0x00)
    midi_mt32_open(patch + 2, length - 2);
  else
    midi_mt32_open(patch, length);

  midi_mt32_close();

  free(patch);
  return 0;
}
