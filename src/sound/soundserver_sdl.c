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

byte *sound_data;
int sound_data_size;
SDL_mutex *data_mutex;
SDL_cond *data_cond;

sfx_driver_t sound_sdl;

void
sound_null_server(int fd_in, int fd_out, int fd_events);

int* sdl_soundserver_init(void *args) {
  state_t *s = (state_t *) args;

  sound_null_server(s->sound_pipe_in[0], s->sound_pipe_out[1],
		    s->sound_pipe_events[1]);

}

int
sound_sdl_init(state_t *s)
{
  int fd_in[2], fd_out[2], fd_events[2], fd_debug[2];

  soundserver = &sound_sdl;
  
  sound_init_pipes(s);
  
  memcpy(&fd_in, &(s->sound_pipe_in), sizeof(int)*2);
  memcpy(&fd_out, &(s->sound_pipe_out), sizeof(int)*2);
  memcpy(&fd_events, &(s->sound_pipe_events), sizeof(int)*2);
  memcpy(&fd_debug, &(s->sound_pipe_debug), sizeof(int)*2);
  
  if (init_midi_device(s) < 0)
    return -1;

  /* spawn thread */

  out_mutex = SDL_CreateMutex();
  in_mutex = SDL_CreateMutex();
  in_cond = SDL_CreateCond();
  data_mutex = SDL_CreateMutex();
  data_cond = SDL_CreateCond();

  ds = stderr; 

  child = SDL_CreateThread( &sdl_soundserver_init, s);

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

  SDL_LockMutex(out_mutex);

  event = sound_eq_retreive_event(&queue);

  SDL_UnlockMutex(out_mutex);

  return event;
}

void 
sound_sdl_queue_event(int handle, int signal, int value)
{
  SDL_LockMutex(out_mutex);

  sound_eq_queue_event(&queue, handle, signal, value);

  SDL_UnlockMutex(out_mutex);
}

void 
sound_sdl_queue_command(state_t *s, int handle, int signal, int value)
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

  /*  if (wait_tvp == NULL)
    SDL_CondWait(in_cond, in_mutex);
  else */
    SDL_LockMutex(in_mutex);

  event = sound_eq_retreive_event(&inqueue);

  SDL_UnlockMutex(in_mutex);

  /*  if (wait_tvp != NULL)
    usleep(wait_tvp->tv_usec);
  */
  return event;
}

void
sound_sdl_get_data(byte **data_ptr, int *size, int maxlen)
{

  if (sound_data == NULL)
    SDL_CondWait(data_cond, data_mutex);
  else
    SDL_LockMutex(data_mutex);

  *data_ptr = sound_data;
  *size = sound_data_size;
  
  SDL_UnlockMutex(data_mutex);
}

void
sound_sdl_send_data(byte *data_ptr, int maxsend) 
{
  SDL_LockMutex(data_mutex);

  sound_data = data_ptr;
  sound_data_size = maxsend;

  SDL_UnlockMutex(data_mutex);
  SDL_CondSignal(data_cond);
}


sfx_driver_t sound_sdl = {
	"sdl",
	&sound_sdl_init,
	&sound_sdl_configure,
	&sound_exit,
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
