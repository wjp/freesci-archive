/***************************************************************************
 midiout.c Copyright (C) 2000 Rickard Lind


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

midiout_driver_t *midiout_driver = NULL;

midiout_driver_t *midiout_drivers[] = {
#ifdef HAVE_SYS_SOUNDCARD_H
    &midiout_driver_ossseq,
#endif /* HAVE_SYS_SOUNDCARD_H */
    &midiout_driver_null,
#if !defined(_DOS) && !defined(WIN32)
    &midiout_driver_unixraw,
#endif
#ifdef HAVE_ALSA
    &midiout_driver_alsaraw,
#endif
#ifdef _WIN32
	&midiout_driver_win32mci,
#endif
    NULL
};

static unsigned char running_status = 0;

int midiout_open()
{
  return midiout_driver->midiout_open();
}


int midiout_close()
{
  return midiout_driver->midiout_close();
}

int midiout_write_event(guint8 *buffer, unsigned int count)
{

  if (buffer[0] == running_status)
    return midiout_driver->midiout_write(buffer + 1, count -1);
  else {
  running_status = buffer[0]; 
    return midiout_driver->midiout_write(buffer, count);
  } 

}

int midiout_write_block(guint8 *buffer, unsigned int count)
{
  running_status = 0;
  return midiout_driver->midiout_write(buffer, count);
}

int midiout_flush()
{
	return midiout_driver->midiout_flush();
}

/* the midiout_null sound driver */

int midiout_null_open()
{
  printf("Opened null sound device\n");
  return 0;
}

int midiout_null_close()
{
  printf("Closed null sound device\n");
  return 0;
}

int midiout_null_flush()
{
	printf("Flushing null sound device\n");
	return 0;
}

int midiout_null_write(guint8 *buffer, unsigned int count)
{

  /*  printf("Got %d bytes to write to null device\n", count); */

  return 0;
}

midiout_driver_t midiout_driver_null = {
  "null",
  "v0.01",
  NULL,
  &midiout_null_open,
  &midiout_null_close,
  &midiout_null_write,
  &midiout_null_flush
};

struct _midiout_driver *midiout_find_driver(char *name)
{
        int retval = 0;

        if (!name) { /* Find default driver */
	  return midiout_drivers[0];
        }

        while (midiout_drivers[retval] && 
	       strcasecmp(name, midiout_drivers[retval]->name))
                retval++;

        return midiout_drivers[retval];
}
