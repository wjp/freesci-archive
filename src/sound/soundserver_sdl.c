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

Uint32 master;


byte *sound_data_serv = NULL;
byte *sound_data_game = NULL;
int sound_data_size = 0;
SDL_mutex *data_mutex;
SDL_cond *datain_cond;
SDL_cond *dataout_cond;

extern sound_server_t sound_server_sdl;

sound_eq_t inqueue; /* The in-event queue */
sound_eq_t queue; /* The event queue */

int 
sdl_soundserver_init(void *args) 
{
  sci0_soundserver();
  return 0;
}

int
sound_sdl_init(state_t *s)
{
  global_sound_server = &sound_server_sdl;

  master = SDL_ThreadID();

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

	/* SDL_LockMutex(in_mutex); */
	if (!sound_eq_peek_event(&inqueue)) {
	  if (wait_tvp)
	    usleep(wait_tvp->tv_usec >> 4);
	  /*	  if(SDL_CondWaitTimeout(in_cond, in_mutex, 10))  */
	    return NULL;
	}
	SDL_LockMutex(in_mutex);

	event = sound_eq_retreive_event(&inqueue);

	SDL_UnlockMutex(in_mutex);

	return event;
}

int
sound_sdl_get_data(byte **data_ptr, int *size, int maxlen)
{
  SDL_cond *cond;
  byte **sound_data;

  cond = (SDL_ThreadID() == master) ? datain_cond : dataout_cond ;
  sound_data = (SDL_ThreadID() == master) ? &sound_data_serv : &sound_data_game;

  /* we ignore maxlen */

  SDL_LockMutex(data_mutex);
  while (*sound_data == NULL) 
    SDL_CondWait(cond, data_mutex);

  *data_ptr = *sound_data;
  *size = sound_data_size;
  *sound_data = NULL;

  /*  printf("%d got %d bytes ",SDL_ThreadID(), sound_data_size); */
  fflush(stdout);

  SDL_UnlockMutex(data_mutex);

  cond = (SDL_ThreadID() == master) ? dataout_cond : datain_cond ;


  SDL_CondSignal(cond);
  return *size;
}

int
sound_sdl_send_data(byte *data_ptr, int maxsend) 
{
  SDL_cond *cond;
  byte *data;
  byte **sound_data;

  cond = (SDL_ThreadID() == master) ? datain_cond : dataout_cond ;
  sound_data = (SDL_ThreadID() == master) ? &sound_data_game : &sound_data_serv;

  SDL_LockMutex(data_mutex);
  while(*sound_data != NULL) 
    SDL_CondWait(cond, data_mutex);

  data = xalloc(maxsend);
  memcpy(data, data_ptr, maxsend);
  sound_data_size = maxsend;
  /* printf("%d wrote %d bytes ",SDL_ThreadID(), maxsend); */
  fflush(stdout);

  *sound_data = data;

  SDL_UnlockMutex(data_mutex);

  cond = (SDL_ThreadID() == master) ? dataout_cond : datain_cond ;

  SDL_CondSignal(cond);
  return maxsend;
}

void 
sound_sdl_exit(state_t *s) 
{
  sound_command(s, SOUND_COMMAND_SHUTDOWN, 0, 0); /* Kill server */

  /* clean up */
  SDL_WaitThread(child, NULL);
  SDL_DestroyMutex(out_mutex);
  SDL_DestroyMutex(in_mutex);
  SDL_DestroyMutex(data_mutex);
  SDL_DestroyCond(in_cond);
  SDL_DestroyCond(datain_cond);
  SDL_DestroyCond(dataout_cond);
  
}


int
sound_sdl_save(state_t *s, char *dir)
{
  int *success;
  int retval;
  int size;
  /* we ignore the dir */

  sound_command(s, SOUND_COMMAND_SAVE_STATE, 0, 2);
  sound_send_data((byte *) ".", 2);

  sound_get_data((byte **) &success, &size, sizeof(int));
  retval = *success;
  free(success);
  return retval;
}

sound_server_t sound_server_sdl = {
	"sdl",
	"0.1",
	&sound_sdl_init,
	&sound_sdl_configure,
	&sound_sdl_exit,
	&sound_sdl_get_event,
	&sound_sdl_queue_event,
	&sound_sdl_get_command,
	&sound_sdl_queue_command,
	&sound_sdl_get_data,
	&sound_sdl_send_data,
	&sound_sdl_save,
	&sound_restore,
	&sound_command,
	&sound_suspend,
	&sound_resume
};

#endif
