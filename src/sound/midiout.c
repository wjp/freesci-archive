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
	&midiout_driver_ossopl3,
#endif /* HAVE_SYS_SOUNDCARD_H */
#if !defined(_DOS) && !defined(_WIN32) && !defined(_DREAMCAST) && !defined(__MORPHOS__) && !defined(ARM_WINCE) && !defined(_GP32)
	&midiout_driver_unixraw,
#endif
#ifdef HAVE_ALSA
	&midiout_driver_alsaraw,
#endif
#ifdef HAVE_DMEDIA_MIDI_H
	&midiout_driver_sgimd,
#endif
#ifdef __MORPHOS__
	&midiout_driver_morphos,
#endif
#ifdef _WIN32
	&midiout_driver_win32mci,
#endif
#ifdef _DREAMCAST
	&midiout_driver_dcraw,
#endif
#ifdef HAVE_FLUIDSYNTH_H
	&midiout_driver_fluidsynth,
#endif
	&midiout_driver_null,
	NULL
};

static unsigned char running_status = 0;

int midiout_open(void *other_data)
{
  return midiout_driver->midiout_open(other_data);
}


int midiout_close(void)
{
  return midiout_driver->midiout_close();
}

int midiout_write_event(guint8 *buffer, unsigned int count, guint32 other_data)
{
  if (buffer[0] == running_status)
    return midiout_driver->midiout_write(buffer + 1, count - 1, other_data);
  else {
  running_status = buffer[0];
    return midiout_driver->midiout_write(buffer, count, other_data);
  }

}

int midiout_write_block(guint8 *buffer, unsigned int count, guint32 other_data)
{
  running_status = 0;
  return midiout_driver->midiout_write(buffer, count, other_data);
}

int midiout_flush(guint8 code)
{
	return midiout_driver->midiout_flush(code);
}

/* the midiout_null sound driver */

int midiout_null_open(void *other_data)
{
  printf("Opened null midiout device\n");
  return 0;
}

int midiout_null_close(void)
{
  printf("Closed null sound device\n");
  return 0;
}

int midiout_null_flush(guint8 code)
{
	printf("Flushing null sound device\n");
	return 0;
}

int midiout_null_write(guint8 *buffer, unsigned int count, guint32 other_data)
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
