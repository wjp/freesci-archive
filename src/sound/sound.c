/***************************************************************************
 sound.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

***************************************************************************/

#include <stdarg.h>
#include <sound.h>
#ifdef HAVE_GSI
#include <gsi/gsi_interface.c>
#endif

int sci_sound_interface = 0;
/* the current sound interface */

int sci_sound_uninitializer_registered = 0;
/* Ha! Now _that_ is an evil variable name! ;-) */
/* Stores whether atexit(&sciSoundCleanup) has been run yet */

void _SCIsdebug(const char *format, ...)
{
#ifdef SCI_SOUND_DEBUG
  va_list va;

  va_start(va, format);
  vfprintf(stderr, format, va);
  va_end (va);
#endif
}

void
sciSoundCleanup(void)
{
  switch (sci_sound_interface) {
  case SCI_SOUND_INTERFACE_GSI:
#ifdef HAVE_GSI

    gsi_release_synth();
    if (sci_version > SCI_VERSION_0)
      gsi_release_pcm();
    gsi_stop_song();
    gsi_flush_all();
    gsi_close();

#endif /* HAVE_GSI */
    break;
  }
}

int
initSound(int mode)
{
  if (sci_sound_uninitializer_registered == 0) {
    atexit(&sciSoundCleanup);
    sci_sound_uninitializer_registered = 1;
  } else sciSoundCleanup;

  sci_sound_interface = SCI_SOUND_INTERFACE_NONE;
  /* Cleanup/Preparation done, preparing to initialize... */

  if (mode == SCI_SOUND_INTERFACE_AUTODETECT) {
    int i = SCI_SOUND_INTERFACE_LAST;
    while ((i > 0) && (initSound(i) == SCI_SOUND_INTERFACE_NONE)) i--;
    return i;
  } else switch (mode) {

  case SCI_SOUND_INTERFACE_NONE:
    sci_sound_interface = SCI_SOUND_INTERFACE_NONE;
    break;

  case SCI_SOUND_INTERFACE_GSI: {
#ifdef HAVE_GSI
    int i;

    if (i= gsi_init(NULL)) {
      SCIswarn("GSI initialization failed (%d)\n", i);
      return SCI_SOUND_INTERFACE_NONE;
    }
    gsi_grab_synth();
    gsi_init_synth(0);
    if (sci_version > SCI_VERSION_0)
      gsi_grab_pcm(); /* Grab dsp software mixer if neccessary */
    sci_sound_interface = SCI_SOUND_INTERFACE_GSI;

#else /* !HAVE_GSI */
    SCIswarn("GSI %s available\n", "not");
    sci_sound_interface = 0;
#endif /* !HAVE_GSI */
  }
  break;
  }

  return sci_sound_interface;
}


int
playSound(guint8 *data, int loop)
{
  if (sci_version > SCI_VERSION_0) return 1; /* Only SCI0 ATM */
  switch (sci_sound_interface) {
  case SCI_SOUND_INTERFACE_NONE:
    return 1;
    break;
#ifdef HAVE_GSI
  case SCI_SOUND_INTERFACE_GSI:
    switch (sci_version) {
    case SCI_VERSION_0: { /* SCI0 has only support for MIDI music */
      int songlength;
      guint8 *tempmusic = makeMIDI0(data, &songlength);

      if (strcmp(_GSI_SERVER_VERSION, "0.8.1") <= 0)
	  tempmusic[13] = 60; /* Hack to circumvent GSI speed bug */

      gsi_sync();
      gsi_send_song(songlength, tempmusic);
      gsi_play_song(loop);
      gsi_flush_all();
      free(tempmusic);
      break;
    }
    }
    break;
#endif /* HAVE_GSI */
  default:      /* [DJ] shut up compiler warning */
    return 1;
    break;
  }
}


int
setSoundVolume(int volume)
{
  switch (sci_sound_interface) {
  case SCI_SOUND_INTERFACE_NONE:
    return 1;
#ifdef HAVE_GSI
  case SCI_SOUND_INTERFACE_GSI:
    gsi_set_volume(GSI_SYNTH, volume);
    if (sci_version > SCI_VERSION_0)
      gsi_set_volume(GSI_PCM, volume);
      gsi_flush_all();
    break;
#endif /* HAVE_GSI */
  }
  return 0;
}


void
stopSound(void)
{
  switch (sci_sound_interface) {
  case SCI_SOUND_INTERFACE_GSI:
#ifdef HAVE_GSI

    gsi_stop_song();
    gsi_flush_all();

#endif /* HAVE_GSI */
    break;
  }
}

