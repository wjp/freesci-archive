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

static gint16 *buffer;

/* SDL wants its buffer to be filled completely, so we generate samples
   to fill the whole buffer.
*/
static void fill_audio (void *udata, guint8 *stream, int len)
{
  int remain = 0;
  int shift = (pcmout_stereo) ? 2: 1;

  while (len > 0) {
    remain = mix_sound(len >> shift) << shift;
    memcpy(stream, buffer, remain);
    stream += remain;
    len -= remain;
  }
}

static int pcmout_sdl_open(gint16 *b, guint16 size, guint16 rate, guint8 stereo) 
{
  SDL_AudioSpec a;
  
  buffer = b;

  if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_NOPARACHUTE) != 0) {
    fprintf (stderr, "SDL: %s\n", SDL_GetError());
    return -1;
  }
  
  a.freq = rate;
#ifdef WORDS_BIGENDIAN
  a.format = AUDIO_S16MSB;
#else
  a.format = AUDIO_S16LSB;
#endif
  a.channels = (stereo) ? 2 : 1;
  a.samples = size * 2;
  a.callback = fill_audio;
  a.userdata = NULL;
  
  if (SDL_OpenAudio (&a, NULL) < 0) {
    fprintf (stderr, "SDL: %s\n", SDL_GetError());
    return -1;
  }
  
  SDL_PauseAudio (0);
  return 0;
}

static int pcmout_sdl_close() {
  SDL_PauseAudio (1);
  SDL_CloseAudio();
  return 0;
}

pcmout_driver_t pcmout_driver_sdl = {
  "sdl",
  "v0.01",
  NULL,
  &pcmout_sdl_open,
  &pcmout_sdl_close
};

#endif
