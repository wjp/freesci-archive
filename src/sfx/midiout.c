#include "fake_glib.h"
#include "midiout.h"
#include "midiout_alsaraw.h"
#include "midiout_unixraw.h"

static int (*midiout_ptr_close)();
static int (*midiout_ptr_write)(guint8 *, unsigned int);

static unsigned char running_status = 0;

int midiout_open()
{
  midiout_ptr_close = midiout_alsaraw_close;
  midiout_ptr_write = midiout_alsaraw_write;
  return midiout_alsaraw_open(0, 0);

  /*
  midiout_ptr_close = midiout_unixraw_close;
  midiout_ptr_write = midiout_unixraw_write;
  return midiout_unixraw_open("/dev/midi00");
  */
}

int midiout_close()
{
  return midiout_ptr_close();
}

int midiout_write_event(guint8 *buffer, unsigned int count)
{
  if (buffer[0] == running_status)
    return midiout_ptr_write(buffer + 1, count - 1);
  else {
    running_status = buffer[0];
    return midiout_ptr_write(buffer, count);
  }
}

int midiout_write_block(guint8 *buffer, unsigned int count)
{
  running_status = 0;
  return midiout_ptr_write(buffer, count);
}
