/***************************************************************************
 pcmout_sdl.c Copyright (C) 2002 Solomon Peachy (And Claudio Matsuoka)

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

#include <pcmout.h>

#ifdef HAVE_SDL

#ifndef _MSC_VER
#       include <sys/time.h>
#       include <SDL/SDL.h>
#else
#       include <SDL.h>
#endif

static guint16 *buffer;

/* SDL wants its buffer to be filled completely and we generate sound
 * in smaller chunks. So we fill SDL's buffer and keep the remaining
 * sound in the mixer buffer to be used in the next call.
 */
static void fill_audio (void *udata, guint8 *stream, int len)
{
  Uint32 p;
  static Uint32 n = 0, s = 0;
  
  memcpy (stream, (guint8 *) buffer + s, p = n);
  for (n = 0, len -= p; n < len; p += n, len -= n) {
      n = mix_sound () << 1;
      memcpy (stream + p, buffer, n);
  }
  n = mix_sound () << 1;
  memcpy (stream + p, buffer, s = len);
  n -= s;
}

static int pcmout_sdl_open(guint16 *b) {
  SDL_AudioSpec a;
  
  buffer = b;

  if (SDL_Init(SDL_INIT_AUDIO) != 0) {
    fprintf (stderr, "SDL: %s\n", SDL_GetError());
    return -1;
  }
  
  a.freq = 44100;
  a.format = (AUDIO_S16);
  a.channels = 1;
  a.samples = 2048;
  a.callback = fill_audio;
  a.userdata = NULL;
  
  if (SDL_OpenAudio (&a, NULL) < 0) {
    fprintf (stderr, "SDL: %s\n", SDL_GetError());
    return -1;
  }
        
  SDL_PauseAudio (0);
  return 0;
}

static void pcmout_sdl_close() {
  SDL_CloseAudio();
}

pcmout_driver_t pcmout_driver_sdl = {
  "sdl",
  "v0.01",
  NULL,
  &pcmout_sdl_open,
  &pcmout_sdl_close
};

#endif
