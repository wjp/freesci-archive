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
#  include <SDL/SDL.h>
#  include <SDL/SDL_thread.h>
#  include <sys/timeb.h>
#else
#  include <SDL.h>
#  include <SDL_thread.h>
#  include <windows.h>
#endif

static SDL_Thread *child;
static SDL_mutex *out_mutex;
static SDL_mutex *in_mutex;
static SDL_cond *in_cond;

static Uint32 master;


static int sound_data_size = 0;

extern sound_server_t sound_server_sdl;

static SDL_mutex *bulk_mutices[2];
static sci_queue_t bulk_queues[2];
static SDL_cond *bulk_conds[2];

static sound_eq_t inqueue; /* The in-event queue */
static sound_eq_t queue; /* The event queue */

int 
sdl_soundserver_init(void *args) 
{
  sci0_soundserver();
  return 0;
}

int
sound_sdl_init(state_t *s)
{
  int i;

  global_sound_server = &sound_server_sdl;

  SDL_Init(SDL_INIT_EVENTTHREAD | SDL_INIT_NOPARACHUTE);
  master = SDL_ThreadID();

  if (init_midi_device(s) < 0)
    return -1;


  /* spawn thread */

  out_mutex = SDL_CreateMutex();
  in_mutex = SDL_CreateMutex();
  in_cond = SDL_CreateCond();

  for (i = 0; i < 2; i++) {
    bulk_conds[i] = SDL_CreateCond();
    sci_init_queue(&(bulk_queues[i]));
    bulk_mutices[i] = SDL_CreateMutex();
  }

  ds = stderr; 

  sound_eq_init(&inqueue);
  sound_eq_init(&queue);

  child = SDL_CreateThread( sdl_soundserver_init, s);

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

  if (!sound_eq_peek_event(&queue)) {
	  SDL_UnlockMutex(out_mutex);
	  return NULL;
  }

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
	sound_event_t *event	= NULL;

	if (!sound_eq_peek_event(&inqueue)) {
#ifdef _MSC_VER
	  Sleep(0);
#else
	  usleep(0);
#endif
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
  int index				= (SDL_ThreadID() == master);
  SDL_mutex *mutex		= bulk_mutices[index];
  SDL_cond *cond		= bulk_conds[index];
  sci_queue_t *queue	= &(bulk_queues[index]);
  void *data			= NULL;

  /* we ignore maxlen */

  SDL_LockMutex(mutex);

  while (!(data = sci_get_from_queue(queue, size))) 
    SDL_CondWait(cond, mutex);

  SDL_UnlockMutex(mutex);

  *data_ptr = (byte *) data;

  return *size;
}

int
sound_sdl_send_data(byte *data_ptr, int maxsend) 
{
  int index = 1 - (SDL_ThreadID() == master);
  SDL_mutex *mutex = bulk_mutices[index];
  SDL_cond *cond = bulk_conds[index];
  sci_queue_t *queue = &(bulk_queues[index]);

  SDL_LockMutex(mutex);

  sci_add_to_queue(queue, sci_memdup(data_ptr, maxsend), maxsend);

  SDL_UnlockMutex(mutex);

  SDL_CondSignal(cond);
  return maxsend;
}

void 
sound_sdl_exit(state_t *s) 
{
  int i;
  sound_command(s, SOUND_COMMAND_SHUTDOWN, 0, 0); /* Kill server */

  /* clean up */
  SDL_WaitThread(child, NULL);
  SDL_DestroyMutex(out_mutex);
  SDL_DestroyMutex(in_mutex);
  SDL_DestroyCond(in_cond);
  
  for (i = 0; i < 2; i++) {
    void *data;
    SDL_DestroyCond(bulk_conds[i]);
    SDL_DestroyMutex(bulk_mutices[i]);

    while (data = sci_get_from_queue(&(bulk_queues[i]), NULL))
      free(data); /* Flush queues */
  }

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
	0,
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
