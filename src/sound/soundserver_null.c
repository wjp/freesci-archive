/***************************************************************************
 sounderver_null.c Copyright (C) 1999 Christoph Reichenbach


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

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/
/* Sound server without sound output- just takes care of the events */


#include <engine.h>
#include <soundserver.h>
#include <sciresource.h>
#include <midi_device.h>

#include <signal.h>
#include <sound.h>
#include <sys/types.h>
#include <dirent.h>
#ifdef HAVE_SYS_UIO_H
#  ifdef HAVE_LIMITS_H
#    include <limits.h>
#  endif
#  include <sys/uio.h>
#endif

#ifdef HAVE_SOCKETPAIR
#include <sys/socket.h>

#ifndef AF_LOCAL
# ifdef AF_UNIX
#  define AF_LOCAL AF_UNIX
# else
#  warn Neither AF_LOCAL nor AF_UNIX are defined!
#  undef HAVE_SOCKETPAIR
# endif
#endif

#endif /* HAVE_SOCKETPAIR */

sfx_driver_t sound_null;

int x_fd_in, x_fd_out = 0;
int x_fd_events, x_fd_debug = 0;

int soundserver_dead = 0;

#ifdef HAVE_FORK
void
_sound_server_oops_handler(int signal)
{
	if (signal == SIGCHLD) {
		fprintf(stderr, "Warning: Sound server died\n");
		soundserver_dead = 1;
	} else if (signal == SIGPIPE) {
		fprintf(stderr, "Warning: Connection to sound server was severed\n");
		soundserver_dead = 1;
	} else
		fprintf(stderr,"Warning: Signal handler cant' handle signal %d\n", signal);
}

int
_make_pipe(int fildes[2])
     /* Opens an IPC channel */
{
#ifdef HAVE_PIPE
  if (pipe(fildes) == 0)
    return 0; /* :-) */
#endif
#ifdef HAVE_SOCKETPAIR
  if (socketpair(AF_LOCAL, SOCK_STREAM, 0, fildes) == 0)
    return 0; /* :-) */
#endif
  return 1; /* :-( */
}

#endif /* HAVE_FORK */

void
_sound_confirm_death(int signal)
{
  printf("Sound server shut down\n");
}

void
_sound_server_sigpipe_handler(int signal)
{
	fprintf(stderr,"Sound server process: Parent process is dead, terminating.\n");
	_exit(0);
}

int
sound_null_init(state_t *s)
{
	int child_pid;
	pid_t ppid;
	int fd_in[2], fd_out[2], fd_events[2], fd_debug[2];

	soundserver = &sound_null;

	if (_make_pipe(fd_in)
	    || _make_pipe(fd_out)
	    || _make_pipe(fd_events)
	    || _make_pipe(fd_debug))
	  {
	    fprintf(stderr, "Could not create IPC connection to server\n");
	    return 1;
	  }

	signal(SIGCHLD, &_sound_server_oops_handler);
	signal(SIGPIPE, &_sound_server_oops_handler);

	ppid = getpid(); /* Get process ID */

	if (init_midi_device(s) < 0)
	  return -1;
	
	child_pid = fork();

	if (child_pid < 0) {
		fprintf(stderr,"NULL Sound server init failed: fork() failed\n");
		/* If you get this message twice, something funny has happened :-> */

		return 1; /* Forking failed */
	}

	if (child_pid) { /* Parent process */

		x_fd_out = fd_in[1];
		x_fd_in = fd_out[0];
		x_fd_events = fd_events[0];
		x_fd_debug = fd_debug[0];

		close(fd_in[0]);
		close(fd_out[1]);
		close(fd_events[1]);
		close(fd_debug[1]); /* Close pipes at the other end */

	} else {/* Sound server */

		x_fd_in = fd_in[0];
		x_fd_out = fd_out[1];
		x_fd_events = fd_events[1];
		x_fd_debug = fd_debug[1];

		close(fd_in[1]);
		close(fd_out[0]);
		close(fd_events[0]);
		close(fd_debug[0]); /* Close pipes at the other end */

		signal(SIGPIPE, &_sound_server_sigpipe_handler); /* Die on sigpipe */
		ds = fdopen(x_fd_debug, "w"); /* We want to output text to it */
		ppid = getppid(); /* Get parent PID */

		sci0_soundserver();
	}

	return 0;
}

int
sound_null_configure(state_t *s, char *option, char *value)
{
	return 1; /* No options apply to this driver */
}

void 
sound_null_queue_event(int handle, int signal, int value)
{
  sound_event_t event;

  event.handle = handle;
  event.signal = signal;
  event.value = value;

  write(x_fd_events, &event, sizeof(sound_event_t));

}

static int get_event_error_counter = 0;

sound_event_t *
sound_null_get_event(state_t *s)
{
  fd_set inpfds;
  int inplen;
  int success;
  GTimeVal waittime = {0, 0};
  char debug_buf[65];
  sound_event_t *event = xalloc(sizeof(sound_event_t));

  
  FD_ZERO(&inpfds);
  FD_SET(x_fd_debug, &inpfds);
  while ((select(x_fd_debug + 1, &inpfds, NULL, NULL, (struct timeval *)&waittime)
	  && (inplen = read(x_fd_debug, debug_buf, 64)) > 0)) {
    
    debug_buf[inplen] = 0; /* Terminate string */
    sciprintf(debug_buf); /* Transfer debug output */
    waittime.tv_sec = 0;
    waittime.tv_usec = 0;
    
    FD_ZERO(&inpfds);
    FD_SET(x_fd_debug, &inpfds);
    
  }
  
  waittime.tv_sec = 0;
  waittime.tv_usec = 0;

  FD_ZERO(&inpfds);
  FD_SET(x_fd_events, &inpfds);
  
  success = select(x_fd_events + 1, &inpfds, NULL, NULL, (struct timeval *)&waittime);
  
  if (success && read(x_fd_events, event, sizeof(sound_event_t)) == sizeof(sound_event_t)) {
    
    return event;
    
  } else {
    free(event);
    return NULL;
  }
}

void 
sound_null_queue_command(int handle, int signal, int value)
{
  sound_event_t event;

  event.handle = handle;
  event.signal = signal;
  event.value = value;
  /*
  printf("writing %04x %d %d\n", event.handle, event.signal, event.value);
  */
  write(x_fd_out, &event, sizeof(sound_event_t));

}

sound_event_t *
sound_null_get_command(GTimeVal *wait_tvp)
{
  fd_set input_fds;
  sound_event_t *event = NULL;

  FD_ZERO(&input_fds);
  FD_SET(x_fd_in, &input_fds);
  
  if(select(x_fd_in + 1, &input_fds, NULL, NULL, (struct timeval *)wait_tvp)) {
    event = xalloc(sizeof(sound_event_t));
    if (read(x_fd_in, event, sizeof(sound_event_t)) != sizeof(sound_event_t)) {
      free(event);
      event = NULL;
    }
  }
  /*  if (event)
      printf("reading %04x %d %d\n", event->handle, event->signal, event->value); */
  return event;
}

void
sound_null_get_data(byte **data_ptr, int *size, int maxlen)
{
  int len = 0;
  fd_set fds;
  int fd = x_fd_in;

  FD_ZERO(&fds);
  FD_SET(fd, &fds); 
  /*GTimeVal timeout = {0, SOUND_SERVER_TIMEOUT}; */

  select(fd +1, &fds, NULL, NULL, NULL);

  if ((len = read(fd, size, sizeof(int))) < 0)
    perror("...");

  printf("(%d), %d, %d to read\n", fd, len, *size);
  fflush(stdout);
  *data_ptr = xalloc(*size);

  FD_ZERO(&fds);
  FD_SET(fd, &fds); 
  select(fd +1, &fds, NULL, NULL, NULL);

  /* XXXX need to add a read loop to wait */
 
  len = read(fd, *data_ptr, *size);
  printf("%d read\n", len);
  fflush(stdout);
}

void
sound_null_send_data(byte *data_ptr, int maxsend) 
{
  int len;
  int fd = x_fd_out;

  len = write(fd, &maxsend, sizeof(int));
  printf("%d wrote\n", len);  
  fflush(stdout);
  len = write(fd, data_ptr, maxsend);
  printf("%d wrote of %d\n", len, maxsend);  
  fflush(stdout);
}
void

sound_null_exit(state_t *s)
{
  signal(SIGPIPE, SIG_IGN); /* Ignore SIGPIPEs */
  signal(SIGCHLD, &_sound_confirm_death);

  sound_command(s, SOUND_COMMAND_SHUTDOWN, 0, 0); /* Kill server */

  close(x_fd_in);
  close(x_fd_out);
  close(x_fd_debug);
  close(x_fd_events);  /* Close all pipe file descriptors */
}

sfx_driver_t sound_null = {
	"null",
	&sound_null_init,
	&sound_null_configure,
	&sound_null_exit,
	&sound_null_get_event,
	&sound_null_queue_event,
	&sound_null_get_command,
	&sound_null_queue_command,
	&sound_null_get_data,
	&sound_null_send_data,
	&sound_save,
	&sound_restore,
	&sound_command,
	&sound_suspend,
	&sound_resume
};

/************************/
/***** SOUND SERVER *****/
/************************/




