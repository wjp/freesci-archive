/***************************************************************************
 soundserver_sdl.c Copyright (C) 2001 Solomon Peachy

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

/* Please note that this is currently a kludge hack to give win32 sound
   support.  the soundserver code is all being refactored into something 
   cleaner -- abstracting out the communications layer, basically.
*/

#include <engine.h>
#include <soundserver.h>
#include <sound.h>

#ifdef HAVE_SDL

#ifndef _MSC_VER
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#else
#include <SDL.h>
#include <SDL_thread.h>
#endif

SDL_Thread *child;
SDL_mutex *out_mutex;
SDL_mutex *in_mutex;
SDL_cond *in_cond;

byte *sound_data = NULL;
int sound_data_size = 0;
SDL_mutex *data_mutex;
SDL_cond *datain_cond;
SDL_cond *dataout_cond;

sfx_driver_t sound_sdl;

sound_eq_t inqueue; /* The in-event queue */
sound_eq_t queue; /* The event queue */

int* sdl_soundserver_init(void *args) {

  state_t *s = (state_t *) args;
  sci0_soundserver();

}

int
sound_sdl_init(state_t *s)
{
  soundserver = &sound_sdl;
  
  if (init_midi_device(s) < 0)
    return -1;

  /* spawn thread */

  out_mutex = SDL_CreateMutex();
  in_mutex = SDL_CreateMutex();
  in_cond = SDL_CreateCond();
  data_mutex = SDL_CreateMutex();
  datain_cond = SDL_CreateCond();
  dataout_cond = SDL_CreateCond();

  ds = stderr; 
  sound_eq_init(&inqueue);
  sound_eq_init(&queue);

  child = SDL_CreateThread( &sdl_soundserver_init, s);
  SDL_CondSignal(datain_cond);
  return 0;
}

int
sound_sdl_configure(state_t *s, char *option, char *value)
{
	return 1; /* No options apply to this driver */
}

sound_event_t *
sound_sdl_get_event(state_t *s)
{
  sound_event_t *event = NULL;
  if (!sound_eq_peek_event(&queue))
    return NULL;

  SDL_LockMutex(out_mutex);

  event = sound_eq_retreive_event(&queue);

  SDL_UnlockMutex(out_mutex);
  /*
  if (event)
    printf("got %04x %d %d\n", event->handle, event->signal, event->value);
  */
  return event;
}

void 
sound_sdl_queue_event(int handle, int signal, int value)
{
  SDL_LockMutex(out_mutex);

  sound_eq_queue_event(&queue, handle, signal, value);
  /*  printf("set %04x %d %d\n", handle, signal, value); */
  SDL_UnlockMutex(out_mutex);
}

void 
sound_sdl_queue_command(int handle, int signal, int value)
{
  SDL_LockMutex(in_mutex);

  sound_eq_queue_event(&inqueue, handle, signal, value);
  
  SDL_UnlockMutex(in_mutex);
  SDL_CondSignal(in_cond);
}

sound_event_t *
sound_sdl_get_command(GTimeVal *wait_tvp)
{
  sound_event_t *event = NULL;

  if (!sound_eq_peek_event(&inqueue)) {
    usleep(wait_tvp->tv_usec);
    return NULL;
  }
    SDL_LockMutex(in_mutex);

  event = sound_eq_retreive_event(&inqueue);

  SDL_UnlockMutex(in_mutex);

  return event;
}

void
sound_sdl_get_data(void **data_ptr, int *size, int maxlen)
{
  /* we ignore maxlen */
  while (sound_data == NULL) 
    SDL_CondWait(datain_cond, data_mutex);

  *data_ptr = sound_data;
  *size = sound_data_size;
  sound_data = NULL;

  SDL_UnlockMutex(data_mutex);
  SDL_CondSignal(dataout_cond);
}

void
sound_sdl_send_data(void *data_ptr, int maxsend) 
{
  while(sound_data != NULL) 
    SDL_CondWait(dataout_cond, data_mutex);

  sound_data = xalloc(maxsend);
  memcpy(sound_data, data_ptr, maxsend);
  sound_data_size = maxsend;
  printf(" %d bytes ",maxsend);
  SDL_UnlockMutex(data_mutex);
  SDL_CondSignal(datain_cond);
}

void 
sound_sdl_exit(state_t *s) 
{
  sound_command(s, SOUND_COMMAND_SHUTDOWN, 0, 0); /* Kill server */
}


sfx_driver_t sound_sdl = {
	"sdl",
	&sound_sdl_init,
	&sound_sdl_configure,
	&sound_sdl_exit,
	&sound_sdl_get_event,
	&sound_sdl_queue_event,
	&sound_sdl_get_command,
	&sound_sdl_queue_command,
	&sound_sdl_get_data,
	&sound_sdl_send_data,
	&sound_save,
	&sound_restore,
	&sound_command,
	&sound_suspend,
	&sound_resume
};

#endif
