#include "fake_glib.h"
#include <stdio.h>
#include <sys/asoundlib.h>
#include "midiout_alsaraw.h"

static snd_rawmidi_t *handle;

int midiout_alsaraw_open(int card, int device)
{
  int err;

  if ((err = snd_rawmidi_open(&handle, card, device, SND_RAWMIDI_OPEN_OUTPUT)) < 0) {
    fprintf(stderr, "Open failed (%i): /dev/snd/midiC%iD%i\n", err, card, device);
    return -1;
  }
  return 0;
}

int midiout_alsaraw_close()
{
  if (snd_rawmidi_close(handle) < 0)
    return -1;
  return 0;
}

int midiout_alsaraw_write(guint8 *buffer, unsigned int count)
{
  if (snd_rawmidi_write(handle, buffer, count) != count)
    return -1;
  if (snd_rawmidi_output_flush(handle) < 0)
    return -1;
  return 0;
}
