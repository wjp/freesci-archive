/***************************************************************************
 polled_ss_unix.c Copyright (C) 1999 Christoph Reichenbach


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
/* Sound server using standard UNIX IPC */

#include <engine.h>

#ifdef HAVE_SYSV_IPC
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
#  warning Neither AF_LOCAL nor AF_UNIX are defined!
#  undef HAVE_SOCKETPAIR
# endif
#endif

#endif /* HAVE_SOCKETPAIR */

static int x_fd_in = 0, x_fd_out = 0;
static int x_fd_events = 0, x_fd_debug = 0;
static pid_t ppid, server_pid = 0;


static int
verify_pid(int pid) /* Checks if the specified PID is in use */
{
	return kill(pid, 0);
}

static void
sound_unix_server_verify_ppid()
{
	if (verify_pid(ppid)) {
		fprintf(debug_stream,"FreeSCI Sound server: Parent process is dead, terminating\n");
		_exit(1); /* Die semi-ungracefully */
	}
}

static int
checked_write(int file, byte *buf, size_t size)
{
	if (soundserver_dead)
		return size;

	if (!file) {
		fprintf(debug_stream,"Soundserver UNIX: Attempt to use fd0 for write!\n");
		BREAKPOINT();
	}
	return write(file, buf, size);
}


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
		fprintf(stderr, "Warning: Signal handler cant' handle signal %d\n", signal);
}

/* --- Signal handlers for TSTP and CONT --- */

static void
_sound_server_suspension_handler(int signr)
{
	/* Horror! Romance! Suspension! The ultimate signal handler experience! */
	int other_signal;

	if (signr == SIGCONT)
		other_signal = SIGTSTP;
	else if (signr == SIGTSTP)
		other_signal = SIGCONT;
	else {
		fprintf(stderr, "%s L%d: Signal handler not prepared for signal %d!\n", __FILE__, __LINE__, signr);
		return;
	}

	if (server_pid && getpid() != server_pid) {
		/* Pass the signal on to the sound server, if it's running
		** and we really are the main process */
		kill(server_pid, signr);

		/* Prepare for handling the complementary signal */
		signal(other_signal, &_sound_server_suspension_handler);

	}
	/* Now invoke the default handler to stop/resume the process */
	signal(signr, SIG_DFL);
	kill(getpid(), signr);
}

/* --- -- --- */

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

extern sound_server_t sound_server_unix;

void
_sound_confirm_death(int signal)
{
	printf("Sound server shut down\n");
}

void
_sound_server_sigpipe_handler(int signal)
{
	fprintf(debug_stream,"Sound server process: Parent process is dead, terminating.\n");
	_exit(0);
}

int
sound_unix_init(state_t *s, int flags)
{
	int fd_in[2], fd_out[2], fd_events[2], fd_debug[2];

	global_sound_server = &sound_server_unix;

	global_sound_server->flags |= SOUNDSERVER_FLAG_PIPED;

	if (_make_pipe(fd_in)
	    || _make_pipe(fd_out)
	    || _make_pipe(fd_events)
	    || _make_pipe(fd_debug))
	  {
	    fprintf(debug_stream, "Could not create IPC connection to server\n");
	    return 1;
	  }

	signal(SIGCHLD, &_sound_server_oops_handler);
	signal(SIGPIPE, &_sound_server_oops_handler);

	ppid = getpid(); /* Get process ID */

	if (init_midi_device(s) < 0)
	  return -1;

	server_pid = fork();

	if (server_pid < 0) {
		fprintf(debug_stream,"UNIX Sound server init failed: fork() failed\n");
		/* If you get this message twice, something funny has happened :-> */

		return 1; /* Forking failed */
	}

	if (server_pid) { /* Parent process */

		x_fd_out = fd_in[1];
		x_fd_in = fd_out[0];
		x_fd_events = fd_events[0];
		x_fd_debug = fd_debug[0];

		close(fd_in[0]);
		close(fd_out[1]);
		close(fd_events[1]);
		close(fd_debug[1]); /* Close pipes at the other end */

		/* Handle Terminal SToP and CONTinue
		** (^Z and 'bg'/'fg') in the main process  */
		signal(SIGTSTP, &_sound_server_suspension_handler);

	} else {/* Sound server */

		sound_server_state_t sss;
		memset(&sss, 0, sizeof(sound_server_state_t));

		x_fd_in = fd_in[0];
		x_fd_out = fd_out[1];
		x_fd_events = fd_events[1];
		x_fd_debug = fd_debug[1];

		close(fd_in[1]);
		close(fd_out[0]);
		close(fd_events[0]);
		close(fd_debug[0]); /* Close pipes at the other end */

		signal(SIGPIPE, &_sound_server_sigpipe_handler); /* Die on sigpipe */
		debug_stream = fdopen(x_fd_debug, "w"); /* We want to output text to it */
		ppid = getppid(); /* Get parent PID */

		sci0_polled_ss(flags & SOUNDSERVER_INIT_FLAG_REVERSE_STEREO, &sss);
		_exit(0); /* quit */
	}

	return 0;
}

int
sound_unix_configure(state_t *s, char *option, char *value)
{
	return 1; /* No options apply to this driver */
}

void
sound_unix_queue_event(unsigned int handle, unsigned int signal, long value)
{
	sound_event_t event;

	event.handle = handle;
	event.signal = signal;
	event.value = value;

	checked_write(x_fd_events, (byte *)&event, sizeof(sound_event_t));

}

static int get_event_error_counter = 0;

sound_event_t *
sound_unix_get_event(state_t *s)
{
	fd_set inpfds;
	int inplen;
	int success;
	GTimeVal waittime = {0, 0};
	char debug_buf[65];
	sound_event_t *event = sci_malloc(sizeof(sound_event_t));

	sound_unix_server_verify_ppid();


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
sound_unix_queue_command(unsigned int handle, unsigned int signal, long value)
{
	sound_event_t event;

	event.handle = handle;
	event.signal = signal;
	event.value = value;
	/*
	fprintf(debug_stream, "SIG: writing %04x %d %d\n",
		event.handle, event.signal, event.value);
	*/
	checked_write(x_fd_out, (byte *)&event, sizeof(sound_event_t));

}

sound_event_t *
sound_unix_get_command(GTimeVal *wait_tvp)
{
	fd_set input_fds;
	sound_event_t *event = NULL;

	sound_unix_server_verify_ppid();

	FD_ZERO(&input_fds);
	FD_SET(x_fd_in, &input_fds);

	if(select(x_fd_in + 1, &input_fds, NULL, NULL,
		  (struct timeval *)wait_tvp)) {

		event = sci_malloc(sizeof(sound_event_t));
		if (read(x_fd_in, event, sizeof(sound_event_t)) != sizeof(sound_event_t)) {
			free(event);
			event = NULL;
		}
	}
	/*
	if (event)
		fprintf(debug_stream, "SIG: reading %04x %d %d\n",
		       event->handle, event->signal, event->value);
	*/
	return event;
}

int
sound_unix_get_data(byte **data_ptr, int *size)
{
	int len = 0;
	fd_set fds;
	int fd = x_fd_in;
	int remaining_size;
	byte *data_ptr_pos;
	GTimeVal timeout = {0, SOUND_SERVER_TIMEOUT};

	sound_unix_server_verify_ppid();

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	select(fd +1, &fds, NULL, NULL, (struct timeval *) &timeout);

	if ((len = read(fd, size, sizeof(int))) < 0) {
		perror("...");
		return 1;
	}

	/* printf("(%d), %d, %d to read\n", fd, len, *size); */
	fflush(stdout);

	remaining_size = *size;
	data_ptr_pos = *data_ptr = sci_malloc(*size);

	while (remaining_size) {
		GTimeVal timeout = {0, SOUND_SERVER_TIMEOUT};

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		select(fd +1, &fds, NULL, NULL, (struct timeval *) &timeout);

		len = read(fd, data_ptr_pos, remaining_size);

		if (len < 0) {
			fprintf(debug_stream," Sound server UNIX: File transfer failed! Aborting...");
			return 1;
		}
		/*
		printf("%d read at pos %p(%d), %d to go\n",
		       len, data_ptr_pos, (data_ptr_pos - *data_ptr), remaining_size);
		fflush(stdout);
		*/

		remaining_size -= len;
		data_ptr_pos += len;
	}

	return 0;
}

int
sound_unix_send_data(byte *data_ptr, int maxsend)
{
	int len;
	int fd = x_fd_out;
	int to_go = maxsend;

	len = checked_write(fd, (byte *)&maxsend, sizeof(int));

	while (to_go) {
		len = checked_write(fd, data_ptr, to_go);
		if (len < 0) {
			fprintf(debug_stream," Writing to the sound server failed!\n");
			return 1;
		}
		to_go -= len;
	}
	return maxsend;
}

void
sound_unix_exit(state_t *s)
{
	int i;

	signal(SIGPIPE, SIG_IGN); /* Ignore SIGPIPEs */
	signal(SIGTSTP, SIG_DFL);
	signal(SIGCONT, SIG_DFL);
	signal(SIGCHLD, &_sound_confirm_death);

	global_sound_server->queue_command(0, SOUND_COMMAND_SHUTDOWN, 0); /* Kill server */

	close(x_fd_in);
	close(x_fd_out);
	close(x_fd_debug);
	close(x_fd_events);  /* Close all pipe file descriptors */
}


void
sound_unix_poll()
{
}

sound_server_t sound_server_unix = {
	"unix",
	"0.1",
	SOUNDSERVER_FLAG_SEPARATE_CWD,
	&sound_unix_init,
	&sound_unix_configure,
	&sound_unix_exit,
	&sound_unix_get_event,
	&sound_unix_queue_event,
	&sound_unix_get_command,
	&sound_unix_queue_command,
	&sound_unix_get_data,
	&sound_unix_send_data,
	&sound_save_default,
	&sound_restore_default,
	&sound_command_default,
	&sound_suspend_default,
	&sound_resume_default,
	&sound_unix_poll
};

/************************/
/***** SOUND SERVER *****/
/************************/

#endif /* HAVE_SYSV_IPC */
